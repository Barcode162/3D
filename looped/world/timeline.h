#pragma once
// The timeline (J85, docs/plan/jobs/J85.md): a place's arrangement, the way a
// music app lays patterns out on tracks or a video editor lays clips. Each
// track plays over some things; each clip plays a loop (a pattern from the
// library, "@swell", or one written in place) from a beat, for a length,
// eased in and out, layered on top of what each thing already does. The
// whole timeline repeats every `length` beats (on the music's beat when a
// song plays, 120 bpm when not) or seconds, worked out from the shared clock
// alone, so every phone and the room agree without talking.
//
//   "timeline": {"length": 16, "beat": true, "tracks": [
//     {"name": "Lamps", "on": ["#lamp-1", "#lamp-2"], "spread": 0.25,
//      "clips": [{"loop": "@swell", "at": 0, "len": 8, "fade": 1, "mix": 1}]},
//     {"name": "Stools", "on": "kind:bar_stool", "clips": [{"loop": "@spin", "at": 12, "len": 4}]}]}
//
// Pure data and maths, no GPU: tools/native/world_test.cpp.
#include <string>
#include <string_view>
#include <vector>

#include "looped/world/loop.h"

namespace engine {
class Json;
}

namespace engine::world {

struct Clip {
  std::string pattern;  // a library pattern's name ("swell"), or "" with `loop` written in place
  Loop loop;            // the loop it plays (the pattern's, once the world is built)
  double at = 0;        // where it starts, in the timeline's units (beats or seconds)
  double len = 4;       // how long it lasts
  float mix = 1;        // its weight on top of the thing's own loop
  double fade = 0;      // eased in over this long at its start, and out at its end
  float speed = 1;      // the loop runs this many times as fast
};

struct Track {
  std::string name;
  std::vector<std::string> on;  // "#<thing id>" or "kind:<kind id>", one or several
  float spread = 0;             // each next thing on it plays this much of a cycle later (a wave along a row)
  bool mute = false;
  std::vector<Clip> clips;
};

struct Timeline {
  double length = 16;  // one round, in beats (beat) or seconds
  bool beat = true;    // counts in the music's beats
  std::vector<Track> tracks;
  bool empty() const { return tracks.empty(); }
};

// Reads {"length", "beat", "tracks": [{"name", "on", "spread", "mute",
// "clips": [{"loop", "at", "len", "mix", "fade", "speed"}]}]}. A clip's
// "loop" is "@name" (a library pattern) or a loop itself. Problems are
// appended to `error` ("timeline track 2 clip 1: what").
Timeline parseTimeline(const Json& j, std::string* error);

// Where the timeline is at `clock`: 0 <= t < length. Beats follow the
// music's when it plays (Clock::beats), else 120 bpm from the clock.
double timelineTime(const Timeline& tl, const Clock& clock);

// A clip's weight at timeline time `t` (0 outside it; a clip may wrap past
// the end), and how far into it that is (`into`, the timeline's units).
float clipWeight(const Clip& c, double t, double length, double* into);

// How many cycles of the clip's loop have played `into` its start (the
// timeline's units), at the music's tempo `bpm` when the units are beats.
double clipCycles(const Clip& c, double into, bool beatUnits, double bpm);

}  // namespace engine::world
