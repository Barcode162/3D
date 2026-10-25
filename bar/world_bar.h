#pragma once
// The bar's world, live (docs/archive/ENGINE.md): the things of the bar's place file
// (assets/world/places/bar.json, kinds in kinds/bar.json), played every frame on the
// shared clock and drawn: models move, turn, glow and tint; lights breathe,
// flicker and thump; moths and sparks are small glowing shapes. The clock is
// the room's (the same for everyone), the beat the jukebox's song as heard
// (tempos in assets/world/songs.json). The bar's events go to every thing:
// "music" / "quiet". Touching a thing (a lamp, a table, the fire, the light
// switch) follows its kind's "tap" rule at once here; online, the room runs
// the same rule and shares the result (world key t<net>, docs/archive/ENGINE.md E3).
#include <string>
#include <unordered_map>
#include <vector>

#include "bar/bar_scene.h"
#include "bar/renderer.h"
#include "looped/place_player.h"
#include "looped/world/world.h"

namespace app {

class BarWorld {
 public:
  // Takes the scene's world (made from the place file) and fetches the tempos.
  void load(const BarScene& scene);
  bool ready() const { return built_; }
  size_t count() const { return world_.size(); }
  const engine::world::World& world() const { return world_; }
  // The Studio moves things (bar/studio_mode.cpp); nothing else changes the world this way.
  engine::world::World& editWorld() { return world_; }
  double now() const { return now_; }  // the world's clock (seconds), for events the Studio sends
  // The Loop Rack (S8.2): stop the clock (seconds >= 0: hold it there) or let it run (-1).
  void holdClock(double seconds) { held_ = seconds; }

  // Whether thing `i` would react to a touch now (only those are tappable).
  bool reacts(int i) const { return built_ && world_.reacts(i, "tap"); }
  // A player touches thing `i`. False if it doesn't react.
  bool touch(int i);
  // A thing's state from the room (world key t<net> = state, start).
  void applyShared(uint16_t net, int state, double start);
  // Sounds things make by themselves this frame (E6.1: a "sound" part), to play.
  struct Cue {
    int effect = 0;  // engine::synth::Effect
    engine::Vec3 at;
    float gain = 1;
  };
  std::vector<Cue> takeSounds() { return std::move(sounds_); }
  // The sound a thing makes when touched (its kind's "touchSound"), or false.
  bool touchSound(int i, int* effect, float* gain) const;
  // Where a thing really is (its pivot in the world: the fire, not the origin).
  engine::Vec3 where(int i) const;
  // The state of the first thing of `kind` ("" if none): the fire, the switch.
  std::string stateOf(const char* kind) const;

  struct Inputs {
    double nowMs = 0;
    engine::Vec3 eye;
    double songSeconds = -1;  // the jukebox's song as heard (-1 = silent)
    std::vector<bool> seated;  // per seat (BarScene::seats): someone sits there
    int dartHits = 0;          // your darts that hit the board so far
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
  const BarScene* scene_ = nullptr;
  engine::world::World world_;
  std::vector<Tempo> tempos_;
  engine::PlacePlayer player_;  // the things on screen
  struct Voice {  // a thing that makes its own sound now and then
    int thing = -1, effect = 0;
    float minS = 1, maxS = 3, gain = 0.3f, near = 6;
    std::vector<std::string> states;  // only in these (empty = always)
    double next = 0;
    uint32_t count = 0;
  };
  std::vector<Voice> voices_;
  std::vector<Cue> sounds_;
  struct Pending {  // a touch the room hasn't answered yet
    int request = 0;
    std::vector<engine::world::Change> changes;
  };
  void undo(const engine::world::Change& c);
  std::vector<Pending> pending_;
  std::unordered_map<uint16_t, engine::world::Change> shared_;  // the room's word per thing
  std::vector<bool> dims_;   // per light: the switch dims it
  bool requested_ = false, built_ = false;
  bool music_ = false;
  std::string song_;
  int dartHits_ = 0;  // the song playing (a new one: "song")
  std::vector<bool> seated_;  // last frame's (seats taken → "sit", freed → "stand")
  double lastMs_ = 0, now_ = 0;
  double held_ = -1;  // the clock held by the Studio (-1: running)
};

}  // namespace app
