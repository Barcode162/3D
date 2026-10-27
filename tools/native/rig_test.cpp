// Looped Script's light rig (J116.2, looped/words/rig.h): the bar's rig
// (assets/world/rigs/bar.looped) reads cleanly, names every lamp once, and
// moves every lamp exactly as the world's loops do today (music, the fire,
// state blends included); broken copies of it say what's wrong, where, and
// how to fix it; fades, moods and the step budget work.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "bar/bar_scene.h"
#include "bar/light_rig.h"
#include "looped/light_units.h"
#include "looped/words/rig.h"
#include "looped/world/world.h"

using namespace engine;
using namespace engine::words;

static int failed = 0;
static void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++failed;
}

static std::string readFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

static std::string all(const Problems& p) {
  std::string s;
  for (const Problem& x : p) s += "\n       " + x.text();
  return s;
}

// Replaces the first `from` in `text` (which must be there).
static std::string broken(const std::string& text, const std::string& from, const std::string& to) {
  const size_t at = text.find(from);
  if (at == std::string::npos) {
    check(false, "the test's broken copy: \"" + from + "\" is in the rig");
    return text;
  }
  return text.substr(0, at) + to + text.substr(at + from.size());
}

static int lineOf(const std::string& text, const std::string& what) {
  const size_t at = text.find(what);
  int line = 1;
  for (size_t i = 0; i < at && i < text.size(); ++i) line += text[i] == '\n';
  return line;
}

int main() {
  const app::BarScene scene = app::buildBarScene();
  check(scene.problems.empty() && scene.world, "the bar reads cleanly");
  const std::vector<LampInfo> lamps = app::rigLamps(scene);
  const std::string text = readFile(app::rigPath("bar"));
  check(!text.empty(), "the bar's rig file is there (" + app::rigPath("bar") + ")");

  // ---- The bar's rig ----
  Rig rig;
  Problems problems;
  const bool ok = rig.load(text, lamps, app::rigEvents(), problems);
  check(ok && problems.empty(), "the bar's rig reads with no problems" + all(problems));
  size_t named = 0;
  for (size_t i = 0; i < lamps.size(); ++i) named += rig.channelOf(static_cast<int>(i)) >= 0;
  check(named == lamps.size(), "every one of the " + std::to_string(lamps.size()) + " lamps is in a light (" + std::to_string(named) + ")");
  std::printf("     %zu channels:\n", rig.channels().size());
  for (const Rig::Channel& c : rig.channels())
    std::printf("       %-12s %2zu lamps%s (line %d)\n", c.name.c_str(), c.lamps.size(), c.each ? ", each its own flicker" : "", c.line);
  check(rig.channels().size() >= 8 && rig.channels().size() <= 14, "the room's lights come to a handful of channels");

  // ---- The same as the world's loops ----
  // The world (looped/world) moves each lamp's thing; the bar scales the
  // lamp by max(0, 1 + its light channel). The rig must give the same, at
  // every moment, through music, the fire going out and back, and blends.
  {
    world::World w = *scene.world;
    Rig r;
    Problems p;
    r.load(text, lamps, app::rigEvents(), p);
    double worst = 0;
    std::string worstAt;
    int samples = 0;
    world::Clock c;
    for (double t = 0.05; t < 80; t += 0.173) {
      c.seconds = t;
      auto both = [&](const char* event) {
        w.sendAll(event, t);
        r.send(event, t);
      };
      // Music from 20 s to 40 s (and 60 s to 70 s), the fire out from 30 s to 50 s.
      if (std::fabs(t - 20.0) < 0.087 || std::fabs(t - 60.0) < 0.087) both("music"), c.music = true;
      if (std::fabs(t - 40.0) < 0.087 || std::fabs(t - 70.0) < 0.087) both("quiet"), c.music = false;
      if (std::fabs(t - 30.0) < 0.087) both("fire-off");
      if (std::fabs(t - 50.0) < 0.087) both("fire-on");
      c.beats = c.music ? (t - 20.0) * 2.0 : 0;  // 120 bpm
      w.update(c);
      r.update(c);
      for (size_t i = 0; i < lamps.size(); ++i) {
        const int thing = scene.lightThing[i];
        const double want = std::max(0.0, 1.0 + w.thing(thing).motion.ch[world::kLight]);
        const double got = r.lamp(static_cast<int>(i)).x;
        const double d = std::fabs(want - got);
        ++samples;
        if (d > worst) {
          worst = d;
          char buf[160];
          std::snprintf(buf, sizeof buf, "%s at %.2f s: world %.4f, rig %.4f", lamps[i].id.c_str(), t, want, got);
          worstAt = buf;
        }
      }
    }
    check(worst < 1e-4, "every lamp moves as the world's loops move it (" + std::to_string(samples) +
                            " moments; the biggest difference " + std::to_string(worst) + (worstAt.empty() ? "" : ", " + worstAt) + ")");
    check(r.stateOf("strings") == "calm" && r.stateOf("fire") == "lit", "after the music and the fire, the states are back to calm and lit");
  }

  // ---- Fades (the light switch) ----
  {
    Rig r;
    Problems p;
    r.load(text, lamps, app::rigEvents(), p);
    world::Clock c;
    c.seconds = 10;
    r.update(c);
    int bulb = -1, neon = -1;
    for (size_t i = 0; i < lamps.size(); ++i) {
      if (lamps[i].id == "light-bulb-1") bulb = static_cast<int>(i);
      if (lamps[i].id == "light-neon-1") neon = static_cast<int>(i);
    }
    const double bulbBefore = r.lamp(bulb).x, neonBefore = r.lamp(neon).x;
    r.send("lights-down", 10);
    c.seconds = 10.15;
    r.update(c);
    const double halfway = r.lamp(bulb).x;
    c.seconds = 11;
    r.update(c);
    const double bulbAfter = r.lamp(bulb).x, neonAfter = r.lamp(neon).x;
    check(bulbAfter < bulbBefore * 0.3 && bulbAfter > bulbBefore * 0.2 && halfway < bulbBefore && halfway > bulbAfter,
          "lights down: the lamps over the bar fade to a quarter over 0.3 s (" + std::to_string(bulbBefore) + " → " +
              std::to_string(halfway) + " → " + std::to_string(bulbAfter) + ")");
    check(std::fabs(neonAfter / neonBefore) > 0.8, "lights down leaves the neon alone");
    r.send("lights-up", 11);
    c.seconds = 12;
    r.update(c);
    check(r.lamp(bulb).x > bulbBefore * 0.85, "lights up brings them back");
  }

  // ---- Moods, colours, the channel's average ----
  {
    const std::string moody = text + "\nmood party\n  strings brightness 130%, colour 6500 K\n  over the bar brightness 50%\n";
    Rig r;
    Problems p;
    const bool moodLoaded = r.load(moody, lamps, app::rigEvents(), p);
    check(moodLoaded, "a rig with a mood reads" + all(p));
    world::Clock c;
    c.seconds = 5;
    r.update(c);
    int strings = -1, bar = -1;
    for (size_t k = 0; k < r.channels().size(); ++k) {
      if (r.channels()[k].name == "strings") strings = static_cast<int>(k);
      if (r.channels()[k].name == "over the bar") bar = static_cast<int>(k);
    }
    const Vec3 s0 = r.channel(strings), b0 = r.channel(bar);
    check(r.setMood("party", 5, 1) && !r.setMood("disco", 5), "moods are picked by name (and an unknown one is refused)");
    c.seconds = 7;
    r.update(c);
    const Vec3 s1 = r.channel(strings), b1 = r.channel(bar);
    const Vec3 shift = light::kelvinColor(6500) * (1.0f / 1.0f);
    const Vec3 base = light::kelvinColor(5000);
    check(std::fabs(light::luminance(s1) / light::luminance(s0) - 1.3f) < 0.12f, "party: the strings at 130%");
    check(s1.z / s1.x > s0.z / s0.x * 1.05f && std::fabs((s1.z / s1.x) / (s0.z / s0.x) - (shift.z / shift.x) / (base.z / base.x)) < 0.02f,
          "party: the strings shift from 5000 K to 6500 K (bluer)");
    check(std::fabs(light::luminance(b1) / light::luminance(b0) - 0.5f) < 0.06f, "party: over the bar at 50%");
    r.setMood("", 7, 0);
    c.seconds = 7.01;
    r.update(c);
    check(std::fabs(light::luminance(r.channel(bar)) / light::luminance(b0) - 1) < 0.06f, "no mood: back as they were");
  }

  // ---- Broken copies: the line, the words, the fix ----
  struct Case {
    const char* from;
    const char* to;
    const char* expect;  // part of the first problem's text
  };
  const Case cases[] = {
      {"noise(period 3 s, speed 0.5)", "nois(period 3 s, speed 0.5)", "`nois` isn't a shape; did you mean `noise`?"},
      {"brightness each buzz(speed 0.8)\n\nlight arcade", "brightness each bzz(speed 0.8)\n\nlight arcade", "`bzz` isn't a shape; did you mean `buzz`?"},
      {"brightness each buzz(by 60%, every 1.2 s)", "brightness each buzz(by 60%, evry 1.2 s)", "`evry` isn't a setting of `buzz`; did you mean `every`?"},
      {"brightness each buzz(by 60%, every 1.2 s)", "brightness each buzz(by 60%, every 1.2)", "a period is a time"},
      {"  1 + by * flicker(period every, speed speed)", "  1 + by * buzz(every every)", "`buzz` is made of itself (buzz → buzz)"},
      {"shape buzz (by 12%", "shape wave (by 12%", "`wave` is already a word the engine has"},
      {"brightness each breathe\n  when lights down", "brightness each breathe(every time)\n  when lights down", "`period` needs a number that doesn't change"},
      {"noise(period every, speed speed)", "noise(period every, sped speed)", "`sped` isn't a setting of `noise`; did you mean `speed`?"},
      {"wave(period 5 s)", "wave(period 5)", "a period is a time; write it in seconds or beats"},
      {"wave(period 5 s)", "wave()", "`wave` needs a period"},
      {"when music: go to dancing", "when musik: go to dancing", "`musik` isn't something that happens here; did you mean `music`?"},
      {"when music: go to dancing", "when music: go to dancin", "`dancin` isn't a state of `strings`; did you mean `dancing`?"},
      {"when music: go to dancing", "when music: go dancing", "`go` needs `to` after it"},
      {"when lights down: fade to 25% over 0.3 s", "when lights down: fade to 25%", "`fade` needs how long it takes"},
      {"when lights down: fade to 25% over 0.3 s", "when lights down: fade to 25% over 0.3", "`over` needs a time in seconds"},
      {"light spots:", "lihgt spots:", "`lihgt` isn't something a light rig has; did you mean `light`?"},
      {"  brightness each buzz(speed 0.8)\n\nlight arcade", "  brightnes each buzz(speed 0.8)\n\nlight arcade", "did you mean `brightness`?"},
      {"light spots: stage-spot-*, dart-spot-*", "light spots: stage-spot-*, dart-spot-*, light-bulb-5", "`light-bulb-5` is in two lights"},
      {"light spots: stage-spot-*, dart-spot-*", "light spots: stage-spots-*", "no lamp here is called `stage-spots-*`"},
      {"light spots: stage-spot-*, dart-spot-*", "light spots: light-bulb-1 to light-bulb-12", "`light-bulb-9` (in light-bulb-1 to light-bulb-12) isn't a lamp here"},
      {"light spots: stage-spot-*, dart-spot-*", "light spots: light-neon", "`light-neon` is a kind: write `every light-neon`"},
      {"light spots: stage-spot-*, dart-spot-*", "light spots stage-spot-*, dart-spot-*", "a light needs `:` and then its lamps"},
      {"light lamps: light-bulb-5 to light-bulb-8, light-wall-sconce-*\n", "light lamps: light-bulb-5 to light-bulb-8, light-wall-sconce-*\n  colour 3000 K\n", "lamps aren't all one colour temperature"},
      {"light sign: light-sign\n", "light sign: light-sign\n  colour 3000 K\n", "coloured lights without a temperature; use `tint`"},
      {"light sign: light-sign\n", "light sign: light-sign\n  colour 3000\n", "a colour is a temperature in Kelvin"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + 18% * wave(period 5 s", "the brackets aren't closed"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + 18% wave(period 5 s)", "`wave` doesn't fit after what's before it; join them with + - * or /"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + 18% * waves", "`waves` isn't a name here; did you mean `wave`?"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + light-sign", "to subtract, put spaces around the minus"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + 18% * wave", "`wave` needs its settings in brackets"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 2 s", "a time can't be a brightness"},
      {"  state lit\n", "\tstate lit\n", "a tab before the words; indent with spaces"},
      {"light jukebox: light-jukebox\n  state waiting\n", "light jukebox: light-jukebox\n  state waiting\n   ", "doesn't line up"},
      {"brightness 1 + 18% * wave(period 5 s)", "brightness 1 + 18% * wave(period 5 s) …", "use plain letters and signs"},
  };
  int caught = 0;
  for (const Case& k : cases) {
    const std::string bad = broken(text, k.from, k.to);
    Rig r;
    Problems p;
    const bool loaded = r.load(bad, lamps, app::rigEvents(), p);
    const std::string first = p.empty() ? std::string("(no problem)") : p[0].text();
    bool found = false;
    for (const Problem& x : p) found = found || x.text().find(k.expect) != std::string::npos;
    // The line points at the change (or the line just after it, for an indent).
    const int line = lineOf(bad, k.to);
    bool lineOk = false;
    for (const Problem& x : p)
      if (x.text().find(k.expect) != std::string::npos) lineOk = x.line >= line && x.line <= line + 3;
    const bool good = !loaded && found && lineOk && !r.loaded();
    caught += good;
    check(good, std::string("broken: ") + k.to + "\n       → " + first + (found && !lineOk ? " (wrong line)" : ""));
  }
  std::printf("     %d of %zu broken copies caught with their line and a hint\n", caught, sizeof(cases) / sizeof(cases[0]));

  // ---- Safe: a runaway can't freeze the bar ----
  {
    std::string deep = "light sign: light-sign\n  brightness ";
    for (int i = 0; i < 40; ++i) deep += "(";
    deep += "1";
    for (int i = 0; i < 40; ++i) deep += ")";
    Rig r;
    Problems p;
    const bool deepLoaded = r.load(deep + "\n", lamps, app::rigEvents(), p);
    check(!deepLoaded && !p.empty(), "40 brackets deep is refused, not a crash:" + all(p));
    std::string longOne = "light sign: light-sign\n  brightness 1";
    for (int i = 0; i < 2000; ++i) longOne += " + 1";
    Problems p2;
    const bool longLoaded = r.load(longOne + "\n", lamps, app::rigEvents(), p2);
    check(!longLoaded && !p2.empty(), "a 2000-term brightness is refused:" + all(p2));
    // Many lights each a little long: the per-update budget.
    std::string many;
    for (size_t i = 0; i < lamps.size(); ++i) {
      many += "light l" + std::to_string(i) + ": " + lamps[i].id + "\n  brightness each 1";
      for (int k = 0; k < 40; ++k) many += " + 1% * noise(period 2 s)";
      many += "\n";
    }
    Rig big;
    Problems p3;
    const bool bigLoaded = big.load(many, lamps, app::rigEvents(), p3);
    check(bigLoaded, "63 lights of 40 shapes each read" + all(p3));
    world::Clock c;
    c.seconds = 3;
    big.update(c);
    check(big.overBudget() && big.steps() > 8000, "a rig that needs too much work in one update stops at its budget (" + std::to_string(big.steps()) + " steps)");
    Rig fine;
    Problems p4;
    fine.load(text, lamps, app::rigEvents(), p4);
    fine.update(c);
    check(!fine.overBudget() && fine.steps() < 2000, "the bar's rig takes " + std::to_string(fine.steps()) + " steps an update (budget 8000)");
  }

  std::printf(failed ? "RIG TEST FAILED (%d)\n" : "rig_test: all passed\n", failed);
  return failed ? 1 : 0;
}
