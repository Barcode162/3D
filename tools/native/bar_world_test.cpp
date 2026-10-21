// The bar as things (assets/world/places/bar.json, docs/ENGINE.md E2): the real layout
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
  check(still.size() <= 12, "only room shells, the mirror and invisible spots stand still");

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
  auto thingOfKind = [&](const char* kind) {
    for (size_t i = 0; i < w.size(); ++i)
      if (w.thing(static_cast<int>(i)).kind->id == kind) return static_cast<int>(i);
    return -1;
  };
  w.takeChanges();
  check(w.reacts(thingOfKind("fire_flames"), "tap") && w.send(thingOfKind("fire_flames"), "tap", c.seconds),
        "tapping the fire puts it out");
  check(w.takeChanges().size() == 8, "eight changes to share: the flames, their glow and six sparks");
  c.seconds += 2;
  c.music = true;
  c.beats = 41;
  w.update(c);
  check(stateOfKind("jukebox") == "playing" && stateOfKind("light-jukebox") == "playing" &&
            stateOfKind("light-groove") == "groove" && stateOfKind("rhythm") == "playing",
        "a song: the jukebox, its glow, the ceiling and the 4K Mania cabinet move to it");
  check(stateOfKind("fire_flames") == "out" && stateOfKind("ember") == "gone" && stateOfKind("light-fire") == "out",
        "the fire put out: flames sink, sparks stop, its glow goes dark");
  // Things that react to a touch, and things that don't.
  for (const char* k : {"pendant_lamp", "round_table", "candle", "dartboard", "chalkboard", "back_bar_tall", "light-switch"})
    check(w.reacts(thingOfKind(k), "tap"), std::string(k) + " reacts to a tap");
  for (const char* k : {"hall", "moth", "light-bulb", "pint_glass"})
    check(!w.reacts(thingOfKind(k), "tap"), std::string(k) + " doesn't");
  {
    const int lamp = thingOfKind("pendant_lamp");
    w.send(lamp, "tap", c.seconds);
    Clock t = c;
    t.seconds += 0.4;
    w.update(t);
    check(std::fabs(w.thing(lamp).motion.ch[kTurnZ]) > 2, "a touched lamp swings (more than 2 degrees)");
    t.seconds += 4;
    w.update(t);
    check(w.stateOf(lamp).name == "lit", "and settles back to its sway after one swing");
  }

  // E3.4: a new song makes the jukebox flourish, then it grooves on.
  {
    const int juke = thingOfKind("jukebox");
    w.sendAll("song", c.seconds);
    check(w.stateOf(juke).name == "flourish", "a new song: the jukebox flares");
    Clock t = c;
    t.seconds += 1.5;
    w.update(t);
    check(w.stateOf(juke).name == "playing", "then it grooves on the beat again");
  }

  // E3.3: a stool someone sits on sinks and holds still; free again, it drifts.
  {
    check(scene.seatThing.size() == scene.seats.size(), "every seat belongs to a thing");
    const int stool = scene.seatThing[0];
    check(w.thing(stool).kind->id == "bar_stool", "seat 0 is a bar stool");
    w.send(stool, "sit", c.seconds);
    check(w.stateOf(stool).name == "taken", "someone sits: the stool is taken");
    Clock t = c;
    t.seconds += 2;
    w.update(t);
    check(w.thing(stool).motion.ch[kScaleY] < -0.005f, "and sinks a little");
    w.send(stool, "stand", t.seconds);
    check(w.stateOf(stool).name == "idle", "they stand up: it drifts again");
  }

  // Every thing moved by a loop is somewhere sensible.
  bool finite = true;
  for (size_t i = 0; i < w.size(); ++i)
    for (float v : w.thing(static_cast<int>(i)).world.m) finite = finite && std::isfinite(v);
  check(finite, "every thing's place is a real number");

  std::printf(failed ? "%d FAILED\n" : "BAR WORLD TESTS PASSED\n", failed);
  return failed ? 1 : 0;
}
