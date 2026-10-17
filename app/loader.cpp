#include "app/loader.h"

#include <emscripten/emscripten.h>

#include <string>

#include "engine/log.h"
#include "engine/profiler.h"

namespace app {

void Loader::enter(Stage s, const char* text) {
  stage_ = s;
  static const char* kNames[] = {"models", "finishing", "shaders", "warm-up", "steady", "ready", "failed"};
  engine::profiler().stage(std::string("load:") + kNames[static_cast<int>(s)]);
  EM_ASM({ if (window['barLoading'] && window['barLoading']['stage']) window['barLoading']['stage'](UTF8ToString($0)); },
         text);
}

bool Loader::update(const LoaderInputs& in, double nowMs) {
  switch (stage_) {
    case Stage::kModels:
      if (in.progress != shownProgress_) {
        shownProgress_ = in.progress;
        EM_ASM({ if (window['barLoading']) window['barLoading']['set']($0, $1, $2); }, static_cast<int>(in.filesDone),
               static_cast<int>(in.files), in.progress);
      }
      if (in.filesDone < in.files) return false;
      LOG_INFO("Everything loaded (%zu model files).", in.files);
      engine::profiler().stage("loaded");
      enter(Stage::kFinishing, "Finishing textures…");
      [[fallthrough]];
    case Stage::kFinishing:
      if (in.jobsPending > 0) return false;
      enter(Stage::kShaders, "Preparing shaders…");
      [[fallthrough]];
    case Stage::kShaders:
      if (in.pipelinesPending > 0) return false;
      enter(Stage::kWarmUp, "Warming up…");
      return false;  // the warm-up frame is drawn by the frame loop
    case Stage::kWarmUp:
      if (!in.warmUpDone) return false;
      stage_ = Stage::kSteady;
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
        EM_ASM({ if (window['barLoading'] && window['barLoading']['fail'])
                   window['barLoading']['fail'](UTF8ToString($0), $1); },
               text.c_str(), in.criticalFailed ? 1 : 0);
        if (in.criticalFailed) {
          stage_ = Stage::kFailed;
          engine::profiler().stage("load:failed");
          return false;
        }
      }
      stage_ = Stage::kReady;
      engine::profiler().stage("ready");
      EM_ASM({ if (window['barLoading']) window['barLoading']['ready'](); });
      return true;
    }
    case Stage::kReady:
    case Stage::kFailed:
      return false;
  }
  return false;
}

}  // namespace app
