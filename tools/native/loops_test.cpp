// The loop system (looped/loops.*): reading a recipe file, and that every
// loop is a pure function of the shared clock (so every phone agrees),
// stays where it should and doesn't visibly repeat.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "looped/loops.h"

using engine::Vec3;
using namespace engine::loops;

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

static const Recipe* find(const std::vector<Recipe>& v, const std::string& id) {
  for (const Recipe& r : v)
    if (r.id == id) return &r;
  return nullptr;
}

int main() {
  const std::string text = readFile("assets/loops/bar.json");
  check(!text.empty(), "assets/loops/bar.json is on disk");
  std::string err;
  const std::vector<Recipe> bar = parse(text, &err);
  check(bar.size() == 8 && err.empty(), "the bar's file has 8 loops, no errors");
  const Recipe* moths = find(bar, "moths-left");
  const Recipe* moths2 = find(bar, "moths-right");
  const Recipe* embers = find(bar, "embers");
  const Recipe* fire = find(bar, "fire-flicker");
  const Recipe* buzz = find(bar, "arcade-sign-buzz");
  const Recipe* wave = find(bar, "arcade-neon-wave");
  const Recipe* thump = find(bar, "jukebox-thump");
  const Recipe* groove = find(bar, "room-groove");
  check(moths && moths2 && embers && fire && buzz && wave && thump && groove, "every loop is there by id");
  if (failed) {
    std::printf("LOOPS TEST FAILED (%d)\n", failed);
    return 1;
  }
  check(moths->kind == Kind::kSwarm && moths2->kind == Kind::kSwarm && embers->kind == Kind::kSwarm,
        "moths and embers are swarms");
  check(fire->kind == Kind::kFlicker && buzz->kind == Kind::kFlicker, "fire and sign buzz are flickers");
  check(wave->kind == Kind::kPulse && thump->kind == Kind::kPulse && groove->kind == Kind::kPulse, "the rest are pulses");
  check(moths->count == 3 && moths->seed == 11 && std::fabs(moths->shy - 0.7f) < 1e-6f &&
            std::fabs(moths->center.x + 4.3f) < 1e-5f && std::fabs(moths->center.y - 2.4f) < 1e-5f,
        "moths-left: 3 moths, seed 11, shy 0.7, centered at (-4.3, 2.4, ..)");
  check(embers->when == "fire" && embers->count == 6, "embers wait for the fire, 6 of them");
  check(fire->lights.size() == 1 && fire->lights[0] == 11 && std::fabs(fire->amount - 0.4f) < 1e-6f,
        "fire-flicker drives light 11 by 0.4");
  check(wave->lights.size() == 4 && wave->lights[0] == 22 && std::fabs(wave->spread - 0.25f) < 1e-6f,
        "the neon wave drives 4 lights, a quarter cycle apart");
  check(thump->onBeat && !thump->musicOnly && groove->onBeat && groove->musicOnly, "beat flags read");

  {
    std::string e;
    const auto v = parse(R"({"loops":[{"id":"a","kind":"pulse"},{"id":"b","kind":"tornado"},{"id":"c","kind":"flicker"}]})", &e);
    check(v.size() == 2 && v[0].id == "a" && v[1].id == "c", "an unknown kind is skipped, the rest kept");
    check(e.find("tornado") != std::string::npos && e.find("\"b\"") != std::string::npos,
          "...and reported in error (its id and kind)");
  }
  {
    std::string e;
    const auto v = parse("{\"loops\": [ {\"id\": ", &e);
    check(v.empty() && !e.empty(), "bad JSON: nothing, and an error");
    std::string e2;
    check(parse("[1, 2]", &e2).empty() && !e2.empty(), "not an object: nothing, and an error");
  }

  // Determinism: the same clock gives the same values (every phone agrees).
  {
    bool same = true;
    for (int i = 0; i < 200; ++i) {
      const Clock c{i * 3.71 + 0.123, i * 1.3, (i & 1) != 0};
      const Clock d = c;
      for (int m = 0; m < 3; ++m) {
        const Vec3 a = swarmPosition(*moths, c, m), b = swarmPosition(*moths, d, m);
        same = same && a.x == b.x && a.y == b.y && a.z == b.z;
        same = same && swarmTwinkle(*moths, c, m) == swarmTwinkle(*moths, d, m);
      }
      for (int l = 0; l < 4; ++l) same = same && pulse(*wave, c, l) == pulse(*wave, d, l);
      same = same && pulse(*thump, c, 0) == pulse(*thump, d, 0);
      same = same && flicker(*fire, c, 0) == flicker(*fire, d, 0);
    }
    check(same, "the same clock gives the same swarm, pulse and flicker values");
    const auto again = parse(text);
    const Clock c{1234.5, 0, false};
    const Vec3 a = swarmPosition(*moths, c, 1), b = swarmPosition(*find(again, "moths-left"), c, 1);
    check(a.x == b.x && a.y == b.y && a.z == b.z, "...also from a second read of the file");
    check(distance(swarmPosition(*moths, c, 0), swarmPosition(*moths, c, 1)) > 0.01f,
          "two moths of a swarm are in different places");
  }

  // Swarms stay near their center and don't visibly repeat.
  for (const Recipe* r : {moths, moths2, embers}) {
    const float limit = r->radius * 1.1f + r->height + 0.05f;
    float worst = 0;
    int differ = 0, samples = 0;
    for (int i = 0; i < 10000; ++i) {
      const double t = i * 0.737 + 0.013 * i * i / 10000.0;
      for (int m = 0; m < r->count; ++m) {
        const Vec3 p = swarmPosition(*r, Clock{t, 0, false}, m);
        worst = std::fmax(worst, distance(p, r->center));
        const Vec3 q = swarmPosition(*r, Clock{t + r->period, 0, false}, m);
        ++samples;
        if (distance(p, q) > 0.01f) ++differ;
      }
    }
    std::printf("     %s: farthest %.3f m (limit %.3f), %d/%d differ a period later\n", r->id.c_str(), worst, limit,
                differ, samples);
    check(worst <= limit, r->id + ": stays within radius*1.1 + height + 5 cm of its center");
    check(differ > samples * 9 / 10, r->id + ": a period later, most members are elsewhere (> 1 cm)");
  }
  {
    bool inRange = true;
    for (int i = 0; i < 5000; ++i) {
      const float w = swarmTwinkle(*moths, Clock{i * 0.31, 0, false}, i % 3);
      inRange = inRange && w >= 0 && w <= 1;
    }
    check(inRange, "twinkle stays in 0..1");
  }

  // Pulse.
  {
    bool inRange = true;
    for (int i = 0; i < 10000; ++i) {
      const Clock c{i * 0.0137, i * 0.05, false};
      for (int l = 0; l < 4; ++l) {
        const float v = pulse(*wave, c, l);
        inRange = inRange && v >= 1 - wave->amount - 1e-5f && v <= 1 + wave->amount + 1e-5f;
      }
      const float j = pulse(*thump, c, 0);
      inRange = inRange && j >= 1 - thump->amount - 1e-5f && j <= 1 + thump->amount + 1e-5f;
    }
    check(inRange, "no music: pulses stay in [1 - amount, 1 + amount] (on-beat ones breathe in seconds)");
    check(std::fabs(pulse(*wave, Clock{0, 0, false}, 1) - pulse(*wave, Clock{1.25, 0, false}, 0)) < 1e-5f,
          "spread: light 1 is a quarter cycle ahead of light 0");
    bool peaks = true;
    for (int b = 0; b < 32; ++b) {
      const float start = pulse(*thump, Clock{999, b + 0.02, true}, 0);
      const float mid = pulse(*thump, Clock{999, b + 0.5, true}, 0);
      const float late = pulse(*thump, Clock{999, b + 0.95, true}, 0);
      peaks = peaks && start > 1 + thump->amount && mid < start - 0.2f * thump->amount && late < start;
    }
    check(peaks, "on the beat with music: a peak right after each beat, lower mid-beat");
    bool rests = true;
    for (int i = 0; i < 1000; ++i) rests = rests && pulse(*groove, Clock{i * 0.07, i * 0.1, false}, 0) == 1.0f;
    check(rests, "musicOnly: exactly 1 when no music plays");
    bool moves = false;
    for (int i = 0; i < 100; ++i) moves = moves || pulse(*groove, Clock{0, i * 0.1, true}, 0) != 1.0f;
    check(moves, "musicOnly: moves when music plays");
  }

  // Flicker.
  for (const Recipe* r : {fire, buzz}) {
    bool inRange = true;
    double sum = 0;
    int n = 0, dips = 0;
    for (int i = 0; i < 600 * 60; ++i) {
      const float v = flicker(*r, Clock{i / 60.0, 0, false}, 0);
      inRange = inRange && v >= 0 && v <= 1.05f;
      sum += v;
      ++n;
      if (v < 1 - r->amount / 2) ++dips;
    }
    std::printf("     %s: mean %.3f, %d frames below 1 - amount/2\n", r->id.c_str(), sum / n, dips);
    check(inRange, r->id + ": always in [0, 1.05]");
    check(sum / n > 0.85 && sum / n <= 1.0, r->id + ": mean close to 1");
    check(dips > 0, r->id + ": dips below 1 - amount/2 sometimes (600 s at 60 fps)");
  }

  check(std::fabs(beatsAt(10.0, 120.0, 0.5) - 19.0) < 1e-9, "beatsAt: 9.5 s at 120 bpm = 19 beats");
  check(std::fabs(beatsAt(0.213, 74.875, 0.213)) < 1e-12, "beatsAt: 0 at the first beat");
  check(beatsAt(0.0, 97.5, 0.558) < 0, "beatsAt: negative before the first beat");

  std::printf(failed ? "LOOPS TEST FAILED (%d)\n" : "LOOPS TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
