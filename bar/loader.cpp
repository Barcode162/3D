#include "bar/loader.h"

#include <string>

#include "looped/log.h"
#include "looped/profiler.h"
#include "looped/ui.h"

namespace app {

void Loader::enter(Stage s, const char* text) {
  stage_ = s;
  static const char* kNames[] = {"models", "finishing", "shaders", "warm-up", "steady", "ready", "failed"};
  // Where the loading bar stands when each stage starts (the downloads fill
  // the bar up to 85%, see index.html barLoading.set).
  static const float kBar[] = {0.05f, 0.85f, 0.9f, 0.94f, 0.97f, 1.0f, 1.0f};
  engine::profiler().stage(std::string("load:") + kNames[static_cast<int>(s)]);
  engine::ui::loadingStage(text, kBar[static_cast<int>(s)]);
}

bool Loader::update(const LoaderInputs& in, double nowMs) {
  switch (stage_) {
    case Stage::kModels:
      if (in.progress != shownProgress_) {
        shownProgress_ = in.progress;
        engine::ui::loadingProgress(static_cast<int>(in.filesDone), static_cast<int>(in.files), in.progress);
      }
      if (in.filesDone < in.files) return false;
      LOG_INFO("Everything loaded (%zu model files).", in.files);
      engine::profiler().stage("loaded");
      enter(Stage::kFinishing, "Unpacking the textures");
      [[fallthrough]];
    case Stage::kFinishing:
      if (in.jobsPending > 0) return false;
      enter(Stage::kShaders, "Preparing the lights and materials");
      [[fallthrough]];
    case Stage::kShaders:
      if (in.pipelinesPending > 0) return false;
      enter(Stage::kWarmUp, "Turning the lights on");
      return false;  // the warm-up frame is drawn by the frame loop
    case Stage::kWarmUp:
      if (!in.warmUpDone) return false;
      stage_ = Stage::kSteady;
      engine::ui::loadingStage("", 0.97f);
      steadySinceMs_ = prevFrameMs_ = nowMs;
      return false;
    case Stage::kSteady: {
      steadyFrames_ = nowMs - prevFrameMs_ < 20 ? steadyFrames_ + 1 : 0;
      prevFrameMs_ = nowMs;
      if (steadyFrames_ < 30 && nowMs - steadySinceMs_ < 2000) return false;
      LOG_INFO("Warm-up done (%s).", steadyFrames_ >= 30 ? "30 steady frames" : "2 s");
      if (in.filesFailed > 0) {
        // What's missing, with a Retry; you can still enter unless the room
        // itself is missing.
        LOG_ERROR("Loading: %zu part%s of the bar failed to download.", in.filesFailed, in.filesFailed == 1 ? "" : "s");
        const std::string text =
            std::to_string(in.filesFailed) + (in.filesFailed == 1 ? " part failed" : " parts failed");
        engine::ui::loadingFailed(text, in.criticalFailed);
        if (in.criticalFailed) {
          stage_ = Stage::kFailed;
          engine::profiler().stage("load:failed");
          return false;
        }
      }
      stage_ = Stage::kReady;
      engine::profiler().stage("ready");
      engine::ui::loadingReady();
      return true;
    }
    case Stage::kReady:
    case Stage::kFailed:
      return false;
  }
  return false;
}

}  // namespace app
