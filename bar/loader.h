#pragma once
// The loading screen's stages, from start-up to "Tap to enter" (see
// docs/RESTRUCTURE.md §2.7). Each stage is named on the loading screen and
// timed by the profiler. "Ready" = every file is in (or failed, if not the
// room itself), background jobs are done, every pipeline has compiled, the
// warm-up frame has finished on the GPU, and frames are steady (30 in a row
// under 20 ms, or 2 s).
#include <cstddef>

namespace app {

struct LoaderInputs {
  size_t files = 0, filesDone = 0, filesFailed = 0;
  bool criticalFailed = false;  // the room itself is missing
  float progress = 0;           // 0..1 over all files
  size_t jobsPending = 0;
  int pipelinesPending = 0;
  bool warmUpDone = false;      // the warm-up frame finished on the GPU
};

class Loader {
 public:
  enum class Stage { kModels, kFinishing, kShaders, kWarmUp, kSteady, kReady, kFailed };

  // Call once per frame. Returns true once, when the bar becomes ready.
  bool update(const LoaderInputs& in, double nowMs);
  Stage stage() const { return stage_; }
  bool filesDone() const { return stage_ != Stage::kModels; }
  // True while the one full warm-up frame should be drawn.
  bool wantsWarmUpFrame() const { return stage_ == Stage::kWarmUp && !warmUpDrawn_; }
  void warmUpFrameDrawn() { warmUpDrawn_ = true; }

 private:
  void enter(Stage s, const char* text);

  Stage stage_ = Stage::kModels;
  bool warmUpDrawn_ = false;
  float shownProgress_ = -1;
  double steadySinceMs_ = 0, prevFrameMs_ = 0;
  int steadyFrames_ = 0;
};

}  // namespace app
