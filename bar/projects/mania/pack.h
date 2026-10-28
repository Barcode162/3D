#pragma once
// The Training pack's charts (J117.7): hand-built 4K maps saved as files
// (assets/music/rhythm/pack/), each with its song, its exact timing, the stage
// it belongs to and the skill it trains; and the map checker: every stage's
// rules (from osu!mania's ranking criteria, scaled to the stage) that a map
// must keep, used by the tool that builds the maps (tools/native/mania_pack.cpp).
//
// A chart file: {"v":1, "id":"s3-tech", "stage":3, "skill":"tech",
//   "title":"…", "artist":"…", "licence":"CC BY 4.0", "song":"assets/…m4a",
//   "bpm":150, "offset":312, "length":171.2, "stars":3.02, "about":"…",
//   "notes":[[ms, lane], [ms, lane, endMs], …]}  (offset: the first beat, ms)
#include <string>
#include <vector>

#include "bar/projects/mania/rhythm_chart.h"

namespace app {

struct PackMap {
  std::string id, skill, title, artist, licence, song, about;
  int stage = 0;
  double bpm = 0, offsetMs = 0, length = 0, stars = 0;
  Chart chart;
};

// Reads a chart file; false (and `error`) if it isn't one.
bool parsePackMap(const std::string& text, PackMap& out, std::string* error = nullptr);
// Writes one (notes in time order).
std::string writePackMap(const PackMap& m);

// What a stage allows. Snaps and gaps in beats.
struct StageRules {
  int stage = 1;
  double starsMin = 0, starsMax = 99;
  int maxChord = 4;             // keys at once
  double minGapBeats = 0.25;    // the fastest step between two rows
  int maxRun = 1 << 20;         // most rows in a row at a 1/4 beat or faster
  double minJackBeats = 0.5;    // the fastest repeat in one column…
  int maxFastJack = 1 << 20;    // …and how many notes such repeats may chain (2 = a minijack)
  double minHoldBeats = 0.5;    // shortest hold note
  double holdGapBeats = 0.5;    // from a hold's end to the next note in its column
  int maxTrill = 1 << 20;       // notes alternating between two columns at 1/4 or faster
  double maxSpike = 99;         // hardest 400 ms against the map's typical hard parts
};
StageRules stageRules(int stage);

// Every rule the map breaks, in words with the time (empty = it passes).
std::vector<std::string> checkPackMap(const PackMap& m, const StageRules& rules);

}  // namespace app
