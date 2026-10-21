#pragma once
// The bar's world, live (docs/ENGINE.md): the bar as things (bar/bar_place.h)
// with the kinds in assets/world/kinds/bar.json, played every frame on the
// shared clock and drawn: models move, turn, glow and tint; lights breathe,
// flicker and thump; moths and sparks are small glowing shapes. The clock is
// the room's (the same for everyone), the beat the jukebox's song as heard
// (tempos in assets/world/songs.json). The bar's events go to every thing:
// "music" / "quiet", "fire-on" / "fire-off".
#include <string>
#include <vector>

#include "bar/bar_scene.h"
#include "bar/renderer.h"
#include "looped/world/world.h"

namespace app {

class BarWorld {
 public:
  // Fetches the kinds and tempos (once), then builds the world.
  void load(const BarScene& scene);
  bool ready() const { return built_; }
  size_t count() const { return world_.size(); }
  const engine::world::World& world() const { return world_; }

  // The bar's own setting for a light (the switch dims it): loops multiply it.
  void setLightBase(int light, float scale);

  struct Inputs {
    double nowMs = 0;
    engine::Vec3 eye;
    bool fireOn = true;
    double songSeconds = -1;  // the jukebox's song as heard (-1 = silent)
    std::string songUrl;
  };
  // Plays every thing's loop and hands the result to the renderer; the
  // glowing shapes go into `boxes`.
  void update(const Inputs& in, std::vector<SceneRenderer::Box>& boxes, SceneRenderer& renderer);

 private:
  void build();
  struct Tempo {
    std::string file;
    double bpm = 0, firstBeat = 0;
  };
  struct Drawn {  // what a thing drives
    int thing = -1;
    int model = -1, light = -1;
    bool glowbox = false;
    engine::Vec3 away;  // a shy thing: how far it has darted from you
  };
  const BarScene* scene_ = nullptr;
  std::string kindsText_;
  engine::world::World world_;
  std::vector<Tempo> tempos_;
  std::vector<Drawn> drawn_;
  std::vector<float> base_;  // per light, the bar's own scale
  int files_ = 0;
  bool requested_ = false, built_ = false;
  bool music_ = false, fire_ = true;
  double lastMs_ = 0;
};

}  // namespace app
