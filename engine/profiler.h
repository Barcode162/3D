#pragma once
// Frame profiler: how long each part of a frame takes, how many things were
// drawn, hitches, and how long start-up took. Shown by the performance
// overlay (Settings → Performance overlay, or ?perf=1) and readable as
// window.bar3dPerf (gathered only when read; tests print it, reports will
// send it).
//
// Per frame:  beginFrame()  →  PROFILE(Phase::kX) scopes ...  →  endFrame()
// Draw calls: prof::draw(indexCount) next to every Draw/DrawIndexed, with
// prof::setPass() choosing which pass they count for.
//
// Note: Safari rounds performance.now() to about 1 ms (no cross-origin
// isolation), so a single phase's time is coarse; averages over many frames
// are still right.
#include <cstdint>
#include <string>
#include <vector>

namespace engine {

class TextRenderer;

enum class Phase : uint8_t {
  kJobs,         // background loading work (runJobs)
  kInput,        // menu commands, settings, camera + walking
  kInteraction,  // what you look at / tap, seats, pickups
  kPhysics,      // glasses and bottles
  kNet,          // shared world, events, other players, voice, menu info
  kGames,        // piano, bartender
  kAcquire,      // getting the screen's next image
  kScene,        // boxes, avatars posed, uploads before drawing
  kText,         // HUD and labels queued
  kRender,       // drawing commands encoded and submitted
  kRace,         // the full-screen race (its sim + drawing)
  kProfiler,     // this profiler and its overlay
  kCount
};
const char* phaseName(Phase phase);

namespace prof {
enum Pass : uint8_t { kMain, kMirror, kRacePass, kPassCount };
extern uint8_t gPass;
extern uint32_t gDraws[kPassCount];
extern uint64_t gTris[kPassCount];
inline void setPass(Pass p) { gPass = p; }
inline void draw(uint32_t indexOrVertexCount) {
  ++gDraws[gPass];
  gTris[gPass] += indexOrVertexCount / 3;
}
}  // namespace prof

class Profiler {
 public:
  static constexpr int kFrames = 300;
  static constexpr float kHitchMs = 20.0f;

  struct Frame {
    float phaseMs[static_cast<int>(Phase::kCount)] = {};
    float cpuMs = 0;       // time inside frame()
    float intervalMs = 0;  // time since the previous frame started (what you feel)
    uint32_t draws[prof::kPassCount] = {};
    uint32_t tris[prof::kPassCount] = {};
    uint16_t jobsRun = 0, jobsPending = 0;
    float maxJobMs = 0;
    uint16_t heapMB = 0;
  };
  struct Stats {
    int frames = 0;
    float avg = 0, p95 = 0, worst = 0;  // frame interval, ms
    float cpuAvg = 0, cpuWorst = 0;
    float phaseAvg[static_cast<int>(Phase::kCount)] = {};
    int hitches = 0;
  };
  struct Hitch {
    double atMs;
    float intervalMs, cpuMs;
    Phase phase;  // the phase that took longest in that frame
    float phaseMs;
  };

  void beginFrame(double nowMs);
  void endFrame(double nowMs);
  void add(Phase phase, double ms) { current_.phaseMs[static_cast<int>(phase)] += static_cast<float>(ms); }
  void jobs(int ran, size_t pending, double maxJobMs);

  // A start-up milestone ("gpu", "scene", "model:piano"...), in ms since the
  // page started loading. Logged too.
  void stage(const std::string& name, bool log = true);
  void entered(double nowMs);

  // Last `count` complete frames.
  Stats stats(int count) const;
  const Frame& frame(int back) const;  // 0 = the newest complete frame
  int frameCount() const { return count_; }
  // Everything window.bar3dPerf shows, as JSON.
  std::string json() const;

  // Overlay on/off (settings "perf" or ?perf=1).
  bool overlay = false;
  // Queues the overlay into the text renderer (W, H in device pixels; px =
  // device pixels per CSS pixel). Call before text.upload().
  void drawOverlay(TextRenderer& text, float W, float H, float px);

 private:
  void publish();

  Frame frames_[kFrames];
  int head_ = 0, count_ = 0;
  Frame current_;
  double frameStartMs_ = 0, lastStartMs_ = 0;
  bool published_ = false;
  std::vector<std::pair<std::string, double>> stages_;
  std::vector<Hitch> hitches_;  // newest last, at most 32
  int hitchesTotal_ = 0, hitchesAfterEnter_ = 0;
  double enteredMs_ = 0;
  float worstAfterEnter_ = 0;  // worst frame in the first 10 s after entering
};

Profiler& profiler();

// Adds the time from construction to destruction to a phase.
class ProfileScope {
 public:
  explicit ProfileScope(Phase phase);
  ~ProfileScope();
  ProfileScope(const ProfileScope&) = delete;
  ProfileScope& operator=(const ProfileScope&) = delete;

 private:
  Phase phase_;
  double start_;
};

}  // namespace engine

#define ENGINE_PROFILE_CAT2(a, b) a##b
#define ENGINE_PROFILE_CAT(a, b) ENGINE_PROFILE_CAT2(a, b)
#define PROFILE(phase) engine::ProfileScope ENGINE_PROFILE_CAT(profileScope_, __LINE__)(phase)
