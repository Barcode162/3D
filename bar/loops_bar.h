#pragma once
// The bar's loops: assets/loops/bar.json read by Looped's loop system
// (looped/loops.h) and played on the bar each frame: moths and sparks as
// small glowing shapes, lights that flicker, breathe or thump on the beat.
// The clock is the room's (the same for everyone); the beat is the
// jukebox's song as you hear it.
#include <string>
#include <vector>

#include "bar/renderer.h"
#include "looped/loops.h"

namespace app {

class BarLoops {
 public:
  // Fetches the recipes (once).
  void load();
  size_t count() const { return loops_.size(); }
  const std::vector<engine::loops::Recipe>& recipes() const { return loops_; }

  // The bar's own setting for a light (the switch dims, the fire goes out):
  // loops multiply it.
  void setLightBase(int light, float scale);

  struct Inputs {
    double nowMs = 0;
    engine::Vec3 eye;
    bool fireOn = true;
    double songSeconds = -1;  // the jukebox's song as heard (-1 = silent)
    std::string songUrl;
  };
  // Adds the swarms to `boxes` and sets the lights the loops drive.
  void update(const Inputs& in, std::vector<SceneRenderer::Box>& boxes, SceneRenderer& renderer);

 private:
  struct Tempo {
    std::string file;
    double bpm = 0, firstBeat = 0;
  };
  std::vector<engine::loops::Recipe> loops_;
  std::vector<Tempo> tempos_;
  std::vector<float> base_;                 // per light, the bar's own scale
  std::vector<int> driven_;                 // lights the loops set last frame
  std::vector<std::vector<engine::Vec3>> away_;  // per swarm, per member: how far it darted from you
  double lastMs_ = 0;
  bool requested_ = false;
};

}  // namespace app
