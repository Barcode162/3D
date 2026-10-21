// The bar as things (bar/bar_place.*, docs/ENGINE.md E2): the real layout
// and the real kinds file (assets/world/kinds/bar.json) make a world where
// every thing has a kind and every kind a live loop, and the bar's events
// (music, the fire) move the right things.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "bar/bar_place.h"
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
  Library kinds;
  kinds.add(readFile("assets/world/kinds/bar.json"), "assets/world/kinds/bar.json");
  kinds.resolve();
  check(kinds.errors().empty(), "the bar's kinds file reads cleanly" + (kinds.errors().empty() ? "" : ": " + kinds.errors()));

  for (const app::ModelPlacement& m : scene.models)
    if (app::kindForModel(kinds, m.file).empty()) check(false, "a kind for " + m.file);
  check(app::kindForModel(kinds, "chess_king_w.glb") == "chess", "a chess piece's kind is chess (the nearest shorter name)");

  const Place place = app::barPlace(scene, kinds);
  World w;
  std::string err;
  check(w.build(kinds, place, &err), "the bar builds as things" + (err.empty() ? "" : ": " + err));
  std::printf("     %zu things: %zu models, %zu lights, and small living things\n", w.size(), scene.models.size(),
              scene.lights.size());

  const auto problems = w.check();
  check(problems.empty(), "every thing in the bar plays a live loop");
  for (const auto& p : problems) std::printf("     %s\n", p.c_str());
  const auto still = w.stillKinds();
  std::printf("     standing still, with a reason:\n");
  for (const auto& s : still) std::printf("       %s\n", s.c_str());
  check(still.size() <= 8, "only room shells and the mirror stand still");

  // The lights with loops of their own are where their names say.
  float phase;
  auto lightKind = [&](int i) { return app::kindForLight(scene, static_cast<size_t>(i), &phase); };
  check(lightKind(scene.fireLight) == "light-fire", "the fire's light flickers like fire");
  for (int i : {7, 8, 9}) check(scene.lights[static_cast<size_t>(i)].position.y > 3 && lightKind(i) == "light-groove",
                                "ceiling light " + std::to_string(i) + " grooves with the music");
  for (int i : {22, 23, 24, 25})
    check(scene.lights[static_cast<size_t>(i)].position.z < -9 && lightKind(i) == "light-neon-wave",
          "arcade light " + std::to_string(i) + " is in the arcade's neon wave");
  check(engine::distance(scene.lights[28].position, scene.arcadeDoor) < 2.5f && lightKind(28) == "light-sign",
        "light 28 is by the ARCADE sign");
  int jukeLights = 0;
  for (size_t i = 0; i < scene.lights.size(); ++i) jukeLights += lightKind(static_cast<int>(i)) == "light-jukebox";
  check(jukeLights == 1, "exactly one light is the jukebox's glow");

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
