#include "bar/gpu_check.h"

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

const Step kSteps[] = {
    {"everything", nothing},          {"no soft shade", noShade},     {"no light beams", noBeams},
    {"no lamp shadows", noShadows},   {"no bounced light", noBounce}, {"6 lamps a pixel", fewerLamps},
    {"1x pixels", fewerPixels},
};
constexpr int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);
constexpr double kSettleMs = 900, kMeasureMs = 2000;

struct Result {
  float interval = 0, gpu = 0;
  int frames = 0;
};

int g_step = -1;
double g_stepStart = 0;
Result g_results[kStepCount];
float g_sumInterval = 0, g_sumGpu = 0;
int g_frames = 0;

void begin(int step, double nowMs) {
  g_step = step;
  g_stepStart = nowMs;
  g_sumInterval = g_sumGpu = 0;
  g_frames = 0;
  applyQuality(nullptr);
  g_app.renderer.setGiEnabled(g_app.hooks.feature("bounce"));
  kSteps[step].apply();
}

void finish() {
  std::string j = "{\"gpuCheck\":[";
  char b[160];
  for (int i = 0; i < kStepCount; ++i) {
    std::snprintf(b, sizeof(b), "%s{\"what\":\"%s\",\"frameMs\":%.2f,\"gpuMs\":%.2f,\"frames\":%d}", i ? "," : "",
                  kSteps[i].name, g_results[i].interval, g_results[i].gpu, g_results[i].frames);
    j += b;
  }
  j += "]}";
  engine::profiler().extra = j;
  for (int i = 0; i < kStepCount; ++i)
    LOG_INFO("GPU check: %s: %.1f ms a frame, GPU %.1f ms.", kSteps[i].name, g_results[i].interval, g_results[i].gpu);
  g_step = -1;
  applyQuality(nullptr);
  g_app.renderer.setGiEnabled(g_app.hooks.feature("bounce"));
  engine::ui::toast("GPU check done: send a report (Settings → Send report).");
}

}  // namespace

bool gpuCheckRunning() { return g_step >= 0; }

void startGpuCheck(double nowMs) {
  if (g_step >= 0) return;
  engine::ui::toast("Checking the GPU for 20 seconds: stand still, don't touch the screen.");
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
    }
  }
  if (t < kSettleMs + kMeasureMs) return;
  Result& r = g_results[g_step];
  r.frames = g_frames;
  r.interval = g_frames ? g_sumInterval / static_cast<float>(g_frames) : 0;
  r.gpu = g_frames ? g_sumGpu / static_cast<float>(g_frames) : 0;
  if (g_step + 1 < kStepCount) begin(g_step + 1, nowMs);
  else finish();
}

}  // namespace app
