#include "bar/gpu_check.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "bar/app_state.h"
#include "looped/log.h"
#include "looped/profiler.h"
#include "looped/ui.h"

namespace barapp {
void applyQuality(const char* why);  // bar/game.cpp: puts every feature back as the level and switches say
}  // namespace barapp
using barapp::applyQuality;
using barapp::g_app;

namespace app {
namespace {

struct Step {
  const char* name;  // what's off
  void (*apply)();   // turns it off (on top of the level's state)
};

void nothing() {}
void noShade() { g_app.renderer.setSoftShade(false); }
void noBeams() { g_app.renderer.setLightBeams(false); }
void noShadows() { g_app.renderer.setLampShadows(false); }
void noBounce() { g_app.renderer.setGiEnabled(false); }
void fewerLamps() { g_app.renderer.setLightsPerPixel(6); }
void fewerPixels() { g_app.gpu.resolutionCap = 1.0; }
void oneQueued() { g_app.gpu.maxFramesInFlight = 1; }  // a touch shows a frame sooner

const Step kSteps[] = {
    {"everything", nothing},          {"no soft shade", noShade},     {"no light beams", noBeams},
    {"no lamp shadows", noShadows},   {"no bounced light", noBounce}, {"6 lamps a pixel", fewerLamps},
    {"1x pixels", fewerPixels},       {"1 frame queued", oneQueued},
};
constexpr int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);
constexpr double kSettleMs = 900, kMeasureMs = 2000;

// Per step: the average frame (what the screen shows: 16.7 ms is 60 fps),
// the late frames (over 20 ms: what you feel as stutter), the slowest 5 %,
// and the wait from a frame's hand-over to the phone saying it's done
// ("gpu": it includes frames queued ahead, so it's not the chip's own time;
// only its differences between steps tell).
struct Result {
  float interval = 0, gpu = 0, p95 = 0;
  int frames = 0, late = 0, drawn = 0;  // drawn: frames with a new picture (the rest: GPU still busy)
};
constexpr float kLateMs = 20.0f;

int g_step = -1;
double g_stepStart = 0;
Result g_results[kStepCount];
float g_sumInterval = 0, g_sumGpu = 0;
int g_frames = 0, g_late = 0, g_drawn = 0;
std::vector<float> g_intervals;

void begin(int step, double nowMs) {
  g_step = step;
  g_stepStart = nowMs;
  g_sumInterval = g_sumGpu = 0;
  g_frames = g_late = g_drawn = 0;
  g_app.gpu.maxFramesInFlight = 2;
  g_intervals.clear();
  applyQuality(nullptr);
  g_app.renderer.setGiEnabled(g_app.hooks.feature("bounce"));
  kSteps[step].apply();
}

void finish() {
  std::string j = "{\"gpuCheck\":[";
  char b[240];
  for (int i = 0; i < kStepCount; ++i) {
    const Result& r = g_results[i];
    std::snprintf(b, sizeof(b), "%s{\"what\":\"%s\",\"frameMs\":%.2f,\"late\":%d,\"p95\":%.1f,\"waitMs\":%.2f,\"frames\":%d,\"drawn\":%d}",
                  i ? "," : "", kSteps[i].name, r.interval, r.late, r.p95, r.gpu, r.frames, r.drawn);
    j += b;
  }
  j += "]}";
  engine::profiler().extra = j;
  for (int i = 0; i < kStepCount; ++i)
    LOG_INFO("GPU check: %s: %.1f ms a frame, %d of %d drawn, %d late, slowest 5%% %.0f ms (wait %.0f ms).", kSteps[i].name,
             g_results[i].interval, g_results[i].drawn, g_results[i].frames, g_results[i].late, g_results[i].p95, g_results[i].gpu);
  g_step = -1;
  g_app.gpu.maxFramesInFlight = 2;
  applyQuality(nullptr);
  g_app.renderer.setGiEnabled(g_app.hooks.feature("bounce"));
  engine::ui::toast("GPU check done: send a report (Settings → Send report).");
}

}  // namespace

bool gpuCheckRunning() { return g_step >= 0; }

void startGpuCheck(double nowMs) {
  if (g_step >= 0) return;
  engine::ui::toast("Checking the GPU for 25 seconds: stand where it stutters most, don't touch the screen.");
  LOG_INFO("GPU check: started.");
  begin(0, nowMs);
}

void updateGpuCheck(double nowMs) {
  if (g_step < 0) return;
  const double t = nowMs - g_stepStart;
  if (t >= kSettleMs) {
    const engine::Profiler& p = engine::profiler();
    if (p.frameCount() > 0) {
      const engine::Profiler::Frame& f = p.frame(0);
      g_sumInterval += f.intervalMs;
      g_sumGpu += f.gpuMs;
      ++g_frames;
      if (f.intervalMs > kLateMs) ++g_late;
      if (f.drawn) ++g_drawn;
      g_intervals.push_back(f.intervalMs);
    }
  }
  if (t < kSettleMs + kMeasureMs) return;
  Result& r = g_results[g_step];
  r.frames = g_frames;
  r.late = g_late;
  r.drawn = g_drawn;
  if (!g_intervals.empty()) {
    const size_t k = std::min(g_intervals.size() - 1, g_intervals.size() * 95 / 100);
    std::nth_element(g_intervals.begin(), g_intervals.begin() + static_cast<long>(k), g_intervals.end());
    r.p95 = g_intervals[k];
  }
  r.interval = g_frames ? g_sumInterval / static_cast<float>(g_frames) : 0;
  r.gpu = g_frames ? g_sumGpu / static_cast<float>(g_frames) : 0;
  if (g_step + 1 < kStepCount) begin(g_step + 1, nowMs);
  else finish();
}

}  // namespace app
