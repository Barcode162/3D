#pragma once
// Loops (docs/LOOPED.md D2): the world plays itself. Every living thing is a
// loop, a small recipe (a JSON file a game ships, e.g. assets/loops/bar.json)
// that says what moves and how, and is worked out from one shared clock: the
// room's time, so every phone shows the same moth in the same spot without
// the server sending anything. When music plays, a loop can count in beats
// instead of seconds and lock to the song.
//
// Kinds:
//   swarm   — a few small glowing things (moths, fireflies, dust) drifting
//             around a point on paths that never quite repeat; shy ones dart
//             away when you come close, then drift back.
//   pulse   — a value that breathes (seconds) or thumps on the beat (music).
//   flicker — mostly steady, with seeded little dips and buzzes (neon, flame).
//
// Pure logic, no GPU: tools/native/loops_test.cpp checks it. The game maps
// each loop's `lights` / `models` / swarm onto its renderer.
#include <cstdint>
#include <string>
#include <vector>

#include "looped/math.h"

namespace engine::loops {

// The time a loop sees. `seconds` = the shared clock (the room's, the same
// for everyone; the phone's own when offline). `beats` = beats into the
// song playing now, `music` = whether that's known (else loops on the beat
// fall back to seconds).
struct Clock {
  double seconds = 0;
  double beats = 0;
  bool music = false;
};

enum class Kind { kSwarm, kPulse, kFlicker };

struct Recipe {
  std::string id;
  std::string about;      // plain words: what it is (the hub shows it)
  Kind kind = Kind::kPulse;
  // Timing: one cycle lasts `period` seconds, or `period` beats when
  // `onBeat` and music plays.
  float period = 4;
  bool onBeat = false;
  float amount = 0.1f;    // pulse / flicker depth (0..1)
  uint32_t seed = 1;
  float spread = 0;       // each target (light, model, member) this much of a cycle later
  // Swarm.
  int count = 3;
  Vec3 center;
  float radius = 0.3f, height = 0.1f;
  float size = 0.012f;    // metres
  Vec3 color{1.0f, 0.9f, 0.7f};
  float glow = 1;
  float shy = 0;          // how far (m) they dart from you when you're within 1.2 m
  // What it drives (the game resolves them): light indices, models by file
  // name (every placement of it), lights near a point.
  std::vector<int> lights;
  std::vector<std::string> models;
  bool hasNear = false;
  Vec3 near;
  float within = 0;
  bool enabled = true;
  bool musicOnly = false;  // a pulse that rests (stays at 1) when no music plays
  std::string when;        // a game condition it waits for (the bar: "fire")
};

// Reads a recipe file: {"loops": [{"id": .., "kind": "swarm"|"pulse"|"flicker", ...}]}.
// Unknown kinds are skipped (logged in `error`, not fatal).
std::vector<Recipe> parse(const std::string& json, std::string* error = nullptr);

// A pulse's multiplier for target `index`: 1 ± amount.
float pulse(const Recipe& r, const Clock& c, int index);
// A flicker's multiplier for target `index`: about 1, dipping to 1 - amount.
float flicker(const Recipe& r, const Clock& c, int index);
// Where swarm member `member` is now (before any shy reaction).
Vec3 swarmPosition(const Recipe& r, const Clock& c, int member);
// How bright member `member` glows now (0..1, a slow twinkle).
float swarmTwinkle(const Recipe& r, const Clock& c, int member);

// The beat clock: beats into a song from its position (seconds into it as
// heard), its tempo and where its first beat falls.
inline double beatsAt(double songSeconds, double bpm, double firstBeat) {
  return (songSeconds - firstBeat) * bpm / 60.0;
}

}  // namespace engine::loops
