// The bar as things (bar/bar_place.*, docs/ENGINE.md E2): the real layout
// and the real kinds file (assets/world/kinds/bar.json) make a world where
// every thing has a kind and every kind a live loop, and the bar's events
// (music, the fire) move the right things.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "bar/bar_scene.h"
#include "looped/world/world.h"

using namespace engine;
using namespace engine::world;

static int failed = 0;
static void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++failed;
}

static std::string readFile(const char* path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

int main() {
  const app::BarScene scene = app::buildBarScene();
  check(scene.problems.empty(), "the bar's place and kinds files read cleanly" + (scene.problems.empty() ? "" : ": " + scene.problems));
  check(scene.world != nullptr, "the bar is a world of things");
  World w = *scene.world;
  std::printf("     %zu things: %zu models, %zu lights, and small living things\n", w.size(), scene.models.size(),
              scene.lights.size());
  check(scene.modelThing.size() == scene.models.size() && scene.lightThing.size() == scene.lights.size(),
        "every model and light is a thing");

  const auto problems = w.check();
  check(problems.empty(), "every thing in the bar plays a live loop");
  for (const auto& p : problems) std::printf("     %s\n", p.c_str());
  const auto still = w.stillKinds();
  std::printf("     standing still, with a reason:\n");
  for (const auto& s : still) std::printf("       %s\n", s.c_str());
  check(still.size() <= 11, "only room shells, the mirror and invisible spots stand still");

  // The lights with loops of their own.
  auto lightKind = [&](int l) { return w.thing(scene.lightThing[static_cast<size_t>(l)]).kind->id; };
  check(lightKind(scene.fireLight) == "light-fire", "the fire's light flickers like fire");
  int grooves = 0, waves = 0, signs = 0, jukes = 0;
  for (size_t l = 0; l < scene.lights.size(); ++l) {
    const std::string k = lightKind(static_cast<int>(l));
    const engine::Vec3 p = scene.lights[l].position;
    if (k == "light-groove") grooves += p.y > 3;
    if (k == "light-neon-wave") waves += p.z < -9;
    if (k == "light-sign") signs += engine::distance(p, scene.arcadeDoor) < 2.5f;
    if (k == "light-jukebox") jukes += engine::distance(p, scene.jukeboxSpeaker) < 1.5f;
  }
  check(grooves == 3 && waves == 4 && signs == 1 && jukes == 1,
        "three ceiling lights groove, four arcade lights wave, one buzzes by the sign, one is the jukebox's glow");

  // Moths sit on their lamp and sway with it.
  int moths = 0, onLamp = 0;
  for (size_t i = 0; i < w.size(); ++i) {
    const Thing& t = w.thing(static_cast<int>(i));
    if (t.kind->id != "moth") continue;
    ++moths;
    if (t.parent >= 0 && w.thing(t.parent).kind->id == "pendant_lamp") ++onLamp;
  }
  check(moths == 5 && onLamp == 5, "five moths, each on its lamp");

  // The bar's events.
  Clock c;
  c.seconds = 1.7e9;
  w.update(c);
  auto stateOfKind = [&](const char* kind) {
    for (size_t i = 0; i < w.size(); ++i)
      if (w.thing(static_cast<int>(i)).kind->id == kind) return w.stateOf(static_cast<int>(i)).name;
    return std::string("?");
  };
  check(stateOfKind("jukebox") == "idle" && stateOfKind("fire_flames") == "lit", "quiet, the fire burning");
  w.sendAll("music", c.seconds);
  w.sendAll("fire-off", c.seconds);
  c.seconds += 2;
  c.music = true;
  c.beats = 41;
  w.update(c);
  check(stateOfKind("jukebox") == "playing" && stateOfKind("light-jukebox") == "playing" &&
            stateOfKind("light-groove") == "groove" && stateOfKind("rhythm") == "playing",
        "a song: the jukebox, its glow, the ceiling and the 4K Mania cabinet move to it");
  check(stateOfKind("fire_flames") == "out" && stateOfKind("ember") == "gone" && stateOfKind("light-fire") == "out",
        "the fire put out: flames sink, sparks stop, its glow goes dark");
  // Every thing moved by a loop is somewhere sensible.
  bool finite = true;
  for (size_t i = 0; i < w.size(); ++i)
    for (float v : w.thing(static_cast<int>(i)).world.m) finite = finite && std::isfinite(v);
  check(finite, "every thing's place is a real number");

  std::printf(failed ? "%d FAILED\n" : "BAR WORLD TESTS PASSED\n", failed);
  return failed ? 1 : 0;
}
