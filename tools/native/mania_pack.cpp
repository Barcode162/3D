// Builds 4K Mania's Training pack (J117.9-13): every map from its recipe
// (tools/mania/maps.json: stage, skill, song) and its song's measured beat
// grid, drums and loudness (tools/mania/songs/<song>.json, written by
// tools/mania/song_features.py). For each map it places rows on the beat grid
// where the music hits, gives them lanes by the skill's patterns (Speed:
// streams, jumpstream, handstream; Chords: jumps to chordjacks; Tech: trills,
// split jumptrills, rolls, runningmen, holds against taps, inverse), repairs
// anything the stage's rules refuse (bar/projects/mania/pack.cpp), and tunes
// one density dial until osu!mania's star rating lands on the stage's target.
// Writes assets/music/rhythm/pack/charts/<id>.json and pack.json.
//
// Usage (from the repo root):
//   g++ -std=c++20 -O2 -I. tools/native/mania_pack.cpp bar/projects/mania/pack.cpp
//       bar/projects/mania/stars.cpp looped/json.cpp -o build/native/mania_pack
//   build/native/mania_pack build [id…]   build every map (or the named ones)
//   build/native/mania_pack check         check the saved charts against their stage
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "bar/projects/mania/pack.h"
#include "bar/projects/mania/stars.h"
#include "looped/json.h"

using app::ChartNote;
using app::PackMap;
using app::StageRules;

namespace {

std::string readFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

engine::Json readJson(const std::string& path) {
  engine::JsonLimits limits;
  limits.maxString = 1 << 20;
  std::string err;
  engine::Json j = engine::Json::parse(readFile(path), &err, limits);
  if (!err.empty()) std::fprintf(stderr, "%s: %s\n", path.c_str(), err.c_str());
  return j;
}

// A song as the mapper sees it: the beat grid and, per 1/8 beat from the
// first beat, how strong the low (kick), mid (snare) and high (hats) onsets
// are (0-9); per beat, how loud it is and how much it rings on (0-9).
struct Song {
  std::string id, title, artist, licence, file;
  double bpm = 0, offsetMs = 0, length = 0;
  std::vector<int> low, mid, high;  // per 1/8 beat
  std::vector<int> rms, sus;        // per beat
  int slots() const { return static_cast<int>(low.size()); }
  double slotTime(int s) const { return offsetMs / 1000.0 + s * (60.0 / bpm) / 8.0; }
  int w(int s) const { return s >= 0 && s < slots() ? std::max({low[s], mid[s], high[s]}) : 0; }
  int accent(int s) const { return s >= 0 && s < slots() ? std::max(low[s], mid[s]) : 0; }
  int loud(int beat) const { return beat >= 0 && beat < static_cast<int>(rms.size()) ? rms[beat] : 0; }
  int ring(int beat) const { return beat >= 0 && beat < static_cast<int>(sus.size()) ? sus[beat] : 0; }
};

std::vector<int> digits(const std::string& s) {
  std::vector<int> v;
  v.reserve(s.size());
  for (char c : s) v.push_back(c >= '0' && c <= '9' ? c - '0' : 0);
  return v;
}

bool loadSong(const std::string& id, Song& s) {
  const engine::Json j = readJson("tools/mania/songs/" + id + ".json");
  if (!j.isObject()) return false;
  s.id = id;
  s.title = j["title"].string();
  s.artist = j["artist"].string();
  s.licence = j["licence"].string();
  s.file = j["file"].string();
  s.bpm = j["bpm"].number();
  s.offsetMs = j["offset_ms"].number();
  s.length = j["length"].number();
  s.low = digits(j["low"].string());
  s.mid = digits(j["mid"].string());
  s.high = digits(j["high"].string());
  s.rms = digits(j["rms"].string());
  s.sus = digits(j["sus"].string());
  return s.bpm > 0 && !s.low.empty();
}

struct Recipe {
  std::string id, skill, song, about;
  int stage = 1;
  int fromBeat = -1, toBeat = -1;  // the part of the song mapped (beats from the first)
  unsigned seed = 1;
};

// A row being built: a slot (1/8 beat) and the columns pressed there; a
// column held from here to holdEnd[col] (a slot) when > slot.
struct Row {
  int slot;
  int mask = 0;
  std::array<int, 4> holdEnd{-1, -1, -1, -1};
  int want = 1;      // how many keys this row should get
  bool accent = false;
};

int bits(int m) { return __builtin_popcount(static_cast<unsigned>(m)); }

class Builder {
 public:
  Builder(const Song& song, const Recipe& r) : s_(song), r_(r), rules_(app::stageRules(r.stage)) {}

  // Builds the map with the density dial at `x` (0 = easiest, 1 = hardest).
  PackMap build(double x) {
    rng_.seed(r_.seed * 7919u + static_cast<unsigned>(x * 1000));
    rows_.clear();
    rhythm(x);
    chords(x);
    lanes(x);
    if (r_.skill == "tech" || r_.stage <= 2) holds(x);
    repair();
    return toMap();
  }

  const StageRules& rules() const { return rules_; }

 private:
  const Song& s_;
  Recipe r_;
  StageRules rules_;
  std::vector<Row> rows_;
  std::mt19937 rng_;

  double rnd() { return std::uniform_real_distribution<double>(0, 1)(rng_); }
  int beats() const { return s_.slots() / 8; }
  int firstBeat() const {
    if (r_.fromBeat >= 0) return r_.fromBeat;
    for (int b = 0; b < beats(); ++b) {
      if (s_.loud(b) >= 2) return b - b % 4;
    }
    return 0;
  }
  int lastBeat() const {
    if (r_.toBeat >= 0) return std::min(r_.toBeat, beats());
    int last = beats();
    while (last > 0 && s_.loud(last - 1) < 2) --last;
    const double maxBeats = s_.length / (60.0 / s_.bpm) - 1;
    return std::min(last, static_cast<int>(maxBeats));
  }
  // Loudness of a bar (4 beats) against the song: 0 quiet … 3 the loudest fifth.
  std::vector<int> barLevels() const {
    const int b0 = firstBeat(), b1 = lastBeat();
    std::vector<double> bars;
    for (int b = b0; b + 4 <= b1; b += 4) {
      double sum = 0;
      for (int k = 0; k < 4; ++k) sum += s_.loud(b + k);
      bars.push_back(sum / 4);
    }
    std::vector<double> sorted = bars;
    std::sort(sorted.begin(), sorted.end());
    const auto q = [&](double f) { return sorted.empty() ? 0 : sorted[static_cast<size_t>(f * (sorted.size() - 1))]; };
    const double q15 = q(0.15), q45 = q(0.45), q80 = q(0.8);
    std::vector<int> levels;
    for (double v : bars) {
      // Relative to the song (its quiet parts and its peaks), lifted where it's
      // loud in absolute terms (a song loud all the way through stays busy).
      const int rel = v <= q15 ? 0 : v <= q45 ? 1 : v < q80 ? 2 : 3;
      const int abs = v >= 8 ? 3 : v >= 7 ? 2 : v >= 5 ? 1 : 0;
      levels.push_back(std::min(3, std::max(rel, abs - 1)));
    }
    return levels;
  }

  // The rhythm: which slots get a row. Each bar has a steady layer (beats,
  // halves or quarters, by how loud the bar is, the stage and the dial) and
  // finer notes where the music clearly hits.
  void rhythm(double x) {
    const std::vector<int> levels = barLevels();
    const int b0 = firstBeat();
    const int st = r_.stage;
    const bool speed = r_.skill == "speed", chordsSkill = r_.skill == "chords";
    // Each bar's loudness rank (0 = the loudest), so a dial can switch bars on
    // one by one from the loudest down instead of all at once.
    std::vector<double> loudness;
    for (size_t bar = 0; bar < levels.size(); ++bar) {
      double v = 0;
      for (int k = 0; k < 4; ++k) v += s_.loud(b0 + static_cast<int>(bar) * 4 + k);
      loudness.push_back(v + 0.001 * static_cast<double>(bar % 7));
    }
    std::vector<double> sortedLoud = loudness;
    std::sort(sortedLoud.begin(), sortedLoud.end(), std::greater<double>());
    const auto rankOf = [&](size_t bar) {
      return static_cast<double>(std::lower_bound(sortedLoud.begin(), sortedLoud.end(), loudness[bar], std::greater<double>()) -
                                 sortedLoud.begin()) / std::max<double>(1, static_cast<double>(sortedLoud.size()));
    };
    for (size_t bar = 0; bar < levels.size(); ++bar) {
      const int level = levels[bar];
      // The steady layer, in slots (8 = a beat, 4 = a half, 2 = a quarter).
      int steady = 8;
      if (speed) {
        if (level == 3) steady = st >= 3 ? 2 : (x > 0.55 ? 2 : 4);
        else if (level == 2) steady = st >= 4 ? 2 : (st >= 3 && x > 0.4) || (st == 2 && x > 0.75) ? 2 : 4;
        else if (level == 1) steady = st >= 6 ? 2 : 4;
        else steady = 8;
      } else if (chordsSkill) {
        // Quarter-beat chordjack is the top of the scale: from stage 7 (stage 5-6 only at a high dial).
        // Quarter-beat chordjack, bar by bar from the loudest as the dial rises.
        const double q0 = st >= 10 ? -0.3 : st == 9 ? -0.1 : st == 8 ? 0.0 : st == 7 ? 0.25 : st == 6 ? 0.5 : st == 5 ? 0.65 : 2.0;
        const double share = std::clamp((x - q0) / (1.0 - q0), 0.0, 1.0);
        if (level >= 2) steady = rankOf(bar) < share ? 2 : 4;
        else if (level == 1) steady = st >= 9 ? 2 : 4;
        else steady = 8;
      } else {  // tech: as dense as speed a notch down the dial (the patterns make it hard)
        if (level == 3) steady = st >= 3 ? 2 : (x > 0.45 ? 2 : 4);
        else if (level == 2) steady = st >= 4 ? 2 : (st >= 2 && x > 0.5) ? 2 : 4;
        else if (level == 1) steady = st >= 6 ? 2 : 4;
        else steady = 8;
      }
      // A high dial fills mid-loud bars too (from stage 4).
      if (x > (speed ? 0.7 : 0.55) && level == 1 && st >= 4 && !chordsSkill) steady = 2;
      // Quieter parts thin out further with a low dial.
      if (level <= 1 && x < 0.3 && steady < 8) steady *= 2;
      const int finest = st >= 7 ? 1 : 2;  // 1/8 bursts from stage 7
      const int fineThr = static_cast<int>(std::lround(8.5 - 3.5 * x));
      for (int k = 0; k < 32; ++k) {
        const int slot = (b0 + static_cast<int>(bar) * 4) * 8 + k;
        if (slot >= s_.slots()) break;
        const int w = s_.w(slot);
        bool on = false;
        if (k % steady == 0) on = level >= 2 || w >= 1 || k % 8 == 0;  // steady: always in loud bars
        else if (k % finest == 0 && w >= fineThr && (k % 2 == 0 || st >= 7)) on = true;
        if (level == 0 && k % 8 != 0 && w < 6) on = false;  // quiet bars: beats only
        if (on) rows_.push_back({slot});
      }
    }
    // Stage limits on the fastest step: 1/8 notes come only in short bursts.
    std::vector<Row> kept;
    for (const Row& r : rows_) {
      if (!kept.empty()) {
        const int gap = r.slot - kept.back().slot;
        const double gapBeats = gap / 8.0;
        if (gapBeats < rules_.minGapBeats - 1e-6) continue;
      }
      kept.push_back(r);
    }
    rows_ = kept;
  }

  // How many keys each row gets: chords on the heavy hits (kick, snare),
  // more of them as the dial and the stage go up.
  void chords(double x) {
    const int maxC = rules_.maxChord;
    const std::vector<int> levels = barLevels();
    const int b0 = firstBeat();
    for (Row& r : rows_) {
      const int k = r.slot % 32 - (b0 * 8) % 32;
      (void)k;
      const int bar = (r.slot / 8 - b0) / 4;
      const int level = bar >= 0 && bar < static_cast<int>(levels.size()) ? levels[static_cast<size_t>(bar)] : 1;
      const int a = s_.accent(r.slot);
      const bool onBeat = r.slot % 8 == 0, onHalf = r.slot % 4 == 0;
      r.accent = a >= 6;
      int want = 1;
      // Each harder pattern spreads evenly over a growing share of the rows
      // as the dial rises (u: a fixed random number per row), so the stars
      // climb smoothly instead of jumping when a pattern switches on.
      const double u = std::fmod(std::fabs(std::sin(r.slot * 12.9898 + r_.seed * 78.233) * 43758.5453), 1.0);
      const auto ramp = [&](double from, double to) { return std::clamp((x - from) / (to - from), 0.0, 1.0); };
      if (r_.skill == "speed") {
        // Jumpstream → handstream: jumps on beats, then halves; hands on beats, then halves.
        if (onBeat && level >= 2 && (u < ramp(0.0, 0.35) || (a >= 6 && x > 0.1))) want = 2;
        if (onBeat && level == 1 && u < ramp(0.3, 0.6)) want = 2;
        if (r_.stage >= 5 && onHalf && !onBeat && level >= 2 && u < ramp(0.35, 0.7)) want = 2;
        if (maxC >= 3 && onBeat && level >= 2 && u < ramp(0.55, 0.85)) want = 3;
        if (r_.stage >= 6 && maxC >= 3 && onHalf && !onBeat && level == 3 && u < ramp(0.75, 0.95)) want = 3;
        if (r_.stage >= 8 && r.slot % 4 == 2 && level == 3 && u < ramp(0.85, 1.0)) want = std::max(want, 2);
      } else if (r_.skill == "chords") {
        want = level >= 1 ? 2 : 1;
        if (maxC >= 3 && (a >= 7 - static_cast<int>(4 * x)) && level >= 2) want = 3;
        if (maxC >= 4 && a >= 8 - static_cast<int>(3 * x) && level == 3 && onBeat) want = 4;
        if (level == 0 && !r.accent) want = 1;
        // Between the half-beats (quarter-beat chordjack bars): single notes
        // first (a chordstream), chords only further up the dial, so the
        // difficulty climbs smoothly.
        if (r.slot % 4 == 2) {
          const int st = r_.stage;
          const double q0 = st >= 10 ? -0.3 : st == 9 ? -0.1 : st == 8 ? 0.0 : st == 7 ? 0.25 : st == 6 ? 0.5 : st == 5 ? 0.65 : 2.0;
          const double share = std::clamp((x - q0) / (1.0 - q0), 0.0, 1.0);
          want = share > 0.55 ? std::min(want, 2) : 1;
          if (share > 0.85 && a >= 6) want = std::min(3, maxC);
        }
      } else {  // tech: mostly single notes, jumps on the beat as the dial rises
        if (onBeat && a >= 7 - static_cast<int>(2 * x)) want = 2;
        if (onBeat && level >= 1 && u < ramp(0.2, 0.6)) want = 2;
        if (r_.stage >= 5 && onHalf && !onBeat && level == 3 && u < ramp(0.6, 0.9)) want = 2;
        if (maxC >= 3 && onBeat && level == 3 && a >= 4 && u < ramp(0.7, 1.0)) want = 3;
        // Split jumptrill phrases (stage 5 up): two-key chords alternating hands' pairs.
        const std::vector<std::string> techs = techList();
        const std::string tech = techs.empty() ? "" : techs[static_cast<size_t>(r.slot / 64) % techs.size()];
        if (tech == "splittrill" && r_.stage >= 5 && level >= 1 && (r.slot % 4 == 0 || u < ramp(0.35, 0.95))) want = 2;
      }
      r.want = std::min(want, maxC);
    }
    if (r_.stage <= 4) {
      int lastChord = -1000;
      for (size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        if (r.want < 2) continue;
        const bool before = i > 0 && rows_[i].slot - rows_[i - 1].slot <= 2;
        const bool after = i + 1 < rows_.size() && rows_[i + 1].slot - rows_[i].slot <= 2;
        if (!before && !after) continue;
        if ((r_.stage <= 2 && before && after) || (r_.stage >= 3 && r.slot - lastChord < 8)) r.want = 1;
        else lastChord = r.slot;
      }
    }
  }

  // Lanes. Each row picks columns by scoring every choice: no jack faster
  // than the stage allows (Chords: jacks are the point, within the stage's
  // chain limit), hands alternating in streams, patterns that fit the skill.
  void lanes(double x) {
    const double beatSec = 60.0 / s_.bpm;
    std::array<int, 4> lastSlot{-1000, -1000, -1000, -1000};
    std::array<int, 4> chain{0, 0, 0, 0};
    int prevMask = 0, prev2Mask = 0;
    int lastHand = 1;  // 0 left (columns 1-2), 1 right (3-4)
    // Tech: each 2-bar phrase uses one technique.
    const std::vector<std::string> techs = techList();
    int dir = 1, rollPos = static_cast<int>(rnd() * 4);
    int trillLen = 0;  // single notes alternating between two columns, so far
    for (size_t i = 0; i < rows_.size(); ++i) {
      Row& r = rows_[i];
      const int phrase = r.slot / 64;
      const std::string tech = techs.empty() ? "" : techs[static_cast<size_t>(phrase) % techs.size()];
      double best = 1e18;
      int bestMask = 0;
      for (int mask = 1; mask < 16; ++mask) {
        if (bits(mask) != r.want) continue;
        double cost = rnd() * 0.6;
        for (int c = 0; c < 4; ++c) {
          if (!(mask & (1 << c))) continue;
          const double gapBeats = (r.slot - lastSlot[c]) / 8.0;
          if (gapBeats < 0.49) {
            // A jack.
            if (gapBeats < rules_.minJackBeats - 1e-6) cost += 1000;
            else if (chain[c] + 1 > rules_.maxFastJack) cost += 1000;
            else cost += r_.skill == "chords" ? (x > 0.4 ? -0.6 : 0.4) : 6;
          } else if (gapBeats < 0.99 && r_.skill != "chords") {
            cost += 0.8;  // a slow repeat: fine, a little less flowing
          }
        }
        const int hand = (mask & 3) && !(mask & 12) ? 0 : (mask & 12) && !(mask & 3) ? 1 : 2;
        // Streams: alternate hands; avoid three on one hand in a row.
        if (r.want == 1) {
          if (hand == lastHand) cost += r_.skill == "speed" ? 1.2 : 0.6;
          // A gentle roll now and then (speed, tech rolls).
          const int c = __builtin_ctz(static_cast<unsigned>(mask));
          if ((r_.skill == "speed" && rollPos >= 0) || tech == "roll") {
            if (c == (rollPos + dir + 4) % 4) cost -= tech == "roll" ? 3.0 : 0.7;
          }
        }
        // Chords: share one column with the last chord (gluts), at higher dials.
        if (r_.skill == "chords" && r.want >= 2) {
          const int shared = bits(mask & prevMask);
          if (x > 0.5 && shared == 1) cost -= 0.8;
          if (shared >= 2 && x < 0.7) cost += 1.5;
        }
        // Speed jumps: one on each hand reads best ([13], [24], [14], [23]).
        if (r_.skill == "speed" && r.want == 2 && hand != 2) cost += 1.0;
        // Tech patterns.
        if (tech == "trill" && r.want == 1) {
          // Two columns, one per hand, alternating.
          const int a = phrase % 2 == 0 ? 0 : 1, b = phrase % 2 == 0 ? 2 : 3;
          const int want = (prevMask == (1 << a)) ? (1 << b) : (1 << a);
          if (mask == want) cost -= 4;
        }
        if (tech == "splittrill") {
          const int p1 = phrase % 2 == 0 ? 0b0101 : 0b1001, p2 = phrase % 2 == 0 ? 0b1010 : 0b0110;
          const int want = (prevMask & p1) ? p2 : p1;
          if (r.want == 1 && (mask & want)) cost -= 3;
          if (r.want == 2 && mask == want) cost -= 6;  // [13][24] or [14][23], alternating
        }
        if (tech == "runningman" && r.want == 1) {
          const int anchor = phrase % 2 == 0 ? 0 : 3;
          const bool anchorNow = !(prevMask & (1 << anchor));
          if (anchorNow && mask == (1 << anchor)) cost -= 4;
          if (!anchorNow && !(mask & (1 << anchor))) cost -= 1;
        }
        if (mask == prevMask && r.want == 1 && r_.skill != "chords") cost += 2;
        // Never a trill longer than the stage allows.
        if (r.want == 1 && mask == prev2Mask && mask != prevMask && bits(prevMask) == 1 && trillLen + 1 >= rules_.maxTrill) cost += 500;
        if (mask == prev2Mask && r.want == 1 && tech != "trill" && tech != "runningman") cost += 0.3;
        if (cost < best) {
          best = cost;
          bestMask = mask;
        }
      }
      if (best >= 1000) {  // no legal choice with this many keys: one fewer
        r.want = std::max(1, r.want - 1);
        --i;
        if (r.want == 1 && best >= 1000) {
          // As a last resort, the row is dropped.
          bool any = false;
          for (int c = 0; c < 4; ++c) any |= (r.slot - lastSlot[c]) / 8.0 >= rules_.minJackBeats - 1e-6;
          if (!any) {
            rows_.erase(rows_.begin() + static_cast<long>(i + 1));
          }
        }
        continue;
      }
      r.mask = bestMask;
      for (int c = 0; c < 4; ++c) {
        if (!(bestMask & (1 << c))) continue;
        chain[c] = (r.slot - lastSlot[c]) / 8.0 < 0.49 ? chain[c] + 1 : 1;
        lastSlot[c] = r.slot;
      }
      if (r.want == 1) {
        const int c = __builtin_ctz(static_cast<unsigned>(bestMask));
        if (c == (rollPos + dir + 4) % 4) rollPos = c;
        else {
          rollPos = c;
          if (rnd() < 0.3) dir = -dir;
        }
        lastHand = c < 2 ? 0 : 1;
      } else {
        lastHand = 2;
      }
      const bool alternates = r.want == 1 && bestMask == prev2Mask && bestMask != prevMask && bits(prevMask) == 1;
      trillLen = alternates && i > 0 && r.slot - rows_[i - 1].slot <= 2 ? trillLen + 1 : (r.want == 1 ? 2 : 0);
      prev2Mask = prevMask;
      prevMask = bestMask;
      (void)beatSec;
    }
  }

  std::vector<std::string> techList() const {
    if (r_.skill != "tech") return {};
    std::vector<std::string> t = {"roll", "trill"};
    if (r_.stage >= 3) t.push_back("runningman");
    if (r_.stage >= 4) t.push_back("splittrill");
    return t;
  }

  // Hold notes: on beats where the music rings on, one column holds while
  // the other hand keeps tapping; at stage 6 and up, Tech maps get an inverse
  // phrase (every gap in the loudest ringing phrase filled with holds).
  void holds(double x) {
    const double minHold = rules_.minHoldBeats, gapAfter = rules_.holdGapBeats;
    const int minHoldSlots = static_cast<int>(std::ceil(std::max(minHold, 0.5) * 8));
    const int gapSlots = static_cast<int>(std::ceil(gapAfter * 8));
    // The next row (index) that uses each column, for a hold's room.
    const auto nextInColumn = [&](size_t i, int c) {
      for (size_t j = i + 1; j < rows_.size(); ++j) {
        if (rows_[j].mask & (1 << c)) return rows_[j].slot;
      }
      return s_.slots();
    };
    const double share = r_.skill == "tech" ? 0.18 + 0.2 * x : 0.08;
    int inverseFrom = -1;
    if (r_.skill == "tech" && r_.stage >= 6) {
      // The 2-bar phrase that rings most.
      int bestScore = -1;
      for (int ph = 0; ph * 8 + 8 <= static_cast<int>(s_.sus.size()); ++ph) {
        int sc = 0;
        for (int b = ph * 8; b < ph * 8 + 8; ++b) sc += s_.ring(b) + s_.loud(b);
        if (sc > bestScore && ph * 8 > firstBeat() + 16) {
          bestScore = sc;
          inverseFrom = ph * 64;
        }
      }
    }
    for (size_t i = 0; i < rows_.size(); ++i) {
      Row& r = rows_[i];
      const int beat = r.slot / 8;
      const bool inverse = inverseFrom >= 0 && r.slot >= inverseFrom && r.slot < inverseFrom + 64;
      if (!inverse && (s_.ring(beat) < 6 || r.slot % 8 != 0 || rnd() > share * 2.5)) continue;
      for (int c = 0; c < 4; ++c) {
        if (!(r.mask & (1 << c))) continue;
        const int next = nextInColumn(i, c);
        int end = inverse ? next - gapSlots : std::min(next - gapSlots, r.slot + (rnd() < 0.5 ? 8 : 16));
        if (end - r.slot < minHoldSlots) continue;
        // Keys held at once stay within the stage's chord limit.
        int held = bits(r.mask);
        for (size_t j = i + 1; j < rows_.size() && rows_[j].slot <= end; ++j) {
          int during = bits(rows_[j].mask) + 1;
          for (size_t k = 0; k < i; ++k) {
            for (int cc = 0; cc < 4; ++cc) {
              if (rows_[k].holdEnd[cc] > rows_[j].slot && cc != c) ++during;
            }
          }
          held = std::max(held, during);
        }
        if (held > rules_.maxChord) continue;
        r.holdEnd[c] = end;
        if (!inverse) break;  // one hold at a time outside inverse
      }
    }
  }

  // Fixes what the stage's rules refuse, note by note.
  void repair() {
    for (int pass = 0; pass < 6; ++pass) {
      PackMap m = toMap();
      std::vector<std::string> v = app::checkPackMap(m, rules_);
      bool changed = false;
      // Runs too long: drop the note in the middle of each long run.
      int run = 1;
      for (size_t i = 1; i < rows_.size(); ++i) {
        const double gap = (rows_[i].slot - rows_[i - 1].slot) / 8.0;
        run = gap <= 0.26 ? run + 1 : 1;
        if (run > rules_.maxRun) {
          rows_.erase(rows_.begin() + static_cast<long>(i));
          run = 1;
          changed = true;
        }
      }
      // Trills too long: change a note's column to break it.
      int trill = 1;
      for (size_t i = 2; i < rows_.size(); ++i) {
        const bool single = bits(rows_[i].mask) == 1 && bits(rows_[i - 1].mask) == 1 && bits(rows_[i - 2].mask) == 1;
        const double gap = (rows_[i].slot - rows_[i - 1].slot) / 8.0;
        const bool alt = single && rows_[i].mask == rows_[i - 2].mask && rows_[i].mask != rows_[i - 1].mask && gap <= 0.26;
        trill = alt ? trill + 1 : 2;
        if (trill > rules_.maxTrill) {
          const int used = rows_[i].mask | rows_[i - 1].mask;
          for (int c = 0; c < 4; ++c) {
            if (!(used & (1 << c)) && !(i + 1 < rows_.size() && (rows_[i + 1].mask & (1 << c)))) {
              rows_[i].mask = 1 << c;
              rows_[i].holdEnd = {-1, -1, -1, -1};
              break;
            }
          }
          trill = 2;
          changed = true;
        }
      }
      // Holds that crowd the next note or are too short: shortened or dropped.
      for (size_t i = 0; i < rows_.size(); ++i) {
        for (int c = 0; c < 4; ++c) {
          if (rows_[i].holdEnd[c] < 0) continue;
          for (size_t j = i + 1; j < rows_.size(); ++j) {
            if (!(rows_[j].mask & (1 << c))) continue;
            const int limit = rows_[j].slot - static_cast<int>(std::ceil(rules_.holdGapBeats * 8));
            if (rows_[i].holdEnd[c] > limit) {
              rows_[i].holdEnd[c] = limit;
              changed = true;
            }
            break;
          }
          if ((rows_[i].holdEnd[c] - rows_[i].slot) / 8.0 < std::max(rules_.minHoldBeats, 0.25) - 1e-6) {
            rows_[i].holdEnd[c] = -1;
            changed = true;
          }
        }
      }
      // Column and hand balance: if one column is starved, move single notes to it.
      std::array<int, 4> per{};
      int total = 0;
      for (const Row& r : rows_) {
        for (int c = 0; c < 4; ++c) per[c] += (r.mask >> c) & 1;
        total += bits(r.mask);
      }
      for (int c = 0; c < 4; ++c) {
        if (per[c] >= total * 0.17) continue;
        const int most = static_cast<int>(std::max_element(per.begin(), per.end()) - per.begin());
        for (size_t i = 1; i + 1 < rows_.size() && per[c] < total * 0.2; ++i) {
          Row& r = rows_[i];
          if (r.mask != (1 << most) || r.holdEnd[most] >= 0) continue;
          const bool near = (rows_[i - 1].mask & (1 << c)) || (rows_[i + 1].mask & (1 << c));
          if (near) continue;
          r.mask = 1 << c;
          --per[most];
          ++per[c];
          changed = true;
        }
      }
      // Jacks faster or longer than the stage allows: the note moves to a
      // free column, or goes.
      for (int c = 0; c < 4; ++c) {
        int lastSlot = -1000, chain = 0;
        for (size_t i = 0; i < rows_.size(); ++i) {
          Row& r = rows_[i];
          if (!(r.mask & (1 << c))) continue;
          const double gap = (r.slot - lastSlot) / 8.0;
          const int nextChain = gap < 0.49 ? chain + 1 : 1;
          const bool bad = gap < rules_.minJackBeats - 1e-6 || nextChain > rules_.maxFastJack;
          if (!bad) {
            chain = nextChain;
            lastSlot = r.slot;
            continue;
          }
          int moved = -1;
          for (int d = 0; d < 4 && moved < 0; ++d) {
            if (r.mask & (1 << d)) continue;
            bool ok = true;
            for (size_t j = 0; j < rows_.size() && ok; ++j) {
              if (j == i || !(rows_[j].mask & (1 << d))) continue;
              const double g = std::fabs(rows_[j].slot - r.slot) / 8.0;
              if (g < 0.49 || (rows_[j].holdEnd[d] >= r.slot && rows_[j].slot < r.slot)) ok = false;
            }
            if (ok) moved = d;
          }
          r.mask &= ~(1 << c);
          r.holdEnd[c] = -1;
          if (moved >= 0) r.mask |= 1 << moved;
          changed = true;
        }
      }
      rows_.erase(std::remove_if(rows_.begin(), rows_.end(), [](const Row& r) { return r.mask == 0; }), rows_.end());
      if (!changed) return;
    }
  }

 public:
  PackMap toMap() const {
    PackMap m;
    m.id = r_.id;
    m.stage = r_.stage;
    m.skill = r_.skill;
    m.title = s_.title;
    m.artist = s_.artist;
    m.licence = s_.licence;
    m.song = s_.file;
    m.about = r_.about;
    m.bpm = s_.bpm;
    m.offsetMs = s_.offsetMs;
    m.length = s_.length;
    for (const Row& r : rows_) {
      for (int c = 0; c < 4; ++c) {
        if (!(r.mask & (1 << c))) continue;
        ChartNote n{s_.slotTime(r.slot), c};
        if (r.holdEnd[c] > r.slot) n.end = s_.slotTime(r.holdEnd[c]);
        m.chart.notes.push_back(n);
      }
    }
    m.chart.bpm = m.bpm;
    m.chart.length = m.length;
    m.stars = app::starRating(m.chart.notes);
    return m;
  }
};

std::vector<Recipe> loadRecipes() {
  std::vector<Recipe> out;
  const engine::Json j = readJson("tools/mania/maps.json");
  unsigned seed = 1;
  for (const engine::Json& m : j["maps"].items()) {
    Recipe r;
    r.id = m["id"].string();
    r.skill = m["skill"].string();
    r.song = m["song"].string();
    r.about = m["about"].string();
    r.stage = m["stage"].integer(1);
    r.fromBeat = m["from"].integer(-1);
    r.toBeat = m["to"].integer(-1);
    r.seed = static_cast<unsigned>(m["seed"].integer(static_cast<int>(seed++)));
    out.push_back(r);
  }
  return out;
}

// Finds the density dial that puts the map on its stage's star target and
// passes every rule; reports the closest it got otherwise.
bool buildOne(const Recipe& r, PackMap& out, std::vector<std::string>& problems) {
  Song s;
  if (!loadSong(r.song, s)) {
    problems = {"no song features for " + r.song};
    return false;
  }
  Builder b(s, r);
  const StageRules& rules = b.rules();
  const double target = r.stage == 10 ? 6.75 : 1.5 + 0.5 * r.stage;
  PackMap best;
  double bestErr = 1e9, bestX = 0.5;
  std::vector<std::string> bestProblems;
  const auto tryX = [&](double x) {
    PackMap m = b.build(x);
    std::vector<std::string> v = app::checkPackMap(m, rules);
    const double err = std::fabs(m.stars - target) + (v.empty() ? 0 : 0.5 + 0.01 * static_cast<double>(v.size()));
    if (std::getenv("MANIA_DEBUG")) {
      std::printf("   x %.3f  %.2f★  %zu notes  %zu problems%s%s\n", x, m.stars, m.chart.notes.size(), v.size(),
                  v.empty() ? "" : ": ", v.empty() ? "" : v.front().c_str());
    }
    if (err < bestErr) {
      bestErr = err;
      best = m;
      bestProblems = v;
      bestX = x;
    }
  };
  // The dial isn't smooth everywhere (a pattern switches on at a point), so
  // a scan first, then finer steps around the best.
  for (int k = 0; k <= 20; ++k) tryX(k / 20.0);
  for (double step = 0.025; step > 0.002 && bestErr > 0.04; step /= 2) {
    const double c = bestX;
    tryX(std::max(0.0, c - step));
    tryX(std::min(1.0, c + step));
  }
  out = best;
  problems = bestProblems;
  return problems.empty();
}

std::string skillName(const std::string& skill) {
  return skill == "speed" ? "Speed" : skill == "chords" ? "Chords & jacks" : "Tech";
}

}  // namespace

int main(int argc, char** argv) {
  const std::string mode = argc > 1 ? argv[1] : "build";
  const std::string dir = "assets/music/rhythm/pack/charts/";
  if (mode == "check") {
    int bad = 0;
    const engine::Json index = readJson("assets/music/rhythm/pack/pack.json");
    for (const engine::Json& e : index["maps"].items()) {
      PackMap m;
      std::string err;
      if (!app::parsePackMap(readFile(e["chart"].string()), m, &err)) {
        std::printf("FAIL %s: %s\n", e["chart"].string().c_str(), err.c_str());
        ++bad;
        continue;
      }
      const std::vector<std::string> v = app::checkPackMap(m, app::stageRules(m.stage));
      std::printf("%s %-14s stage %2d %-7s %.2f★ %4zu notes  %s\n", v.empty() ? "ok  " : "FAIL", m.id.c_str(), m.stage,
                  m.skill.c_str(), app::starRating(m.chart.notes), m.chart.notes.size(), m.title.c_str());
      for (const std::string& p : v) std::printf("       %s\n", p.c_str());
      bad += v.empty() ? 0 : 1;
    }
    std::printf(bad ? "%d maps break their stage's rules\n" : "every map keeps its stage's rules\n", bad);
    return bad ? 1 : 0;
  }
  std::set<std::string> only;
  for (int i = 2; i < argc; ++i) only.insert(argv[i]);
  const std::vector<Recipe> recipes = loadRecipes();
  int bad = 0;
  std::string index = "{\"v\":1,\"about\":\"4K Mania's Training pack: ten stages from about 2 to 7 stars (osu!mania's star "
                      "rating), three maps a stage. Built by tools/native/mania_pack.cpp from tools/mania/maps.json.\",\n\"maps\":[\n";
  bool first = true;
  for (const Recipe& r : recipes) {
    PackMap m;
    std::vector<std::string> problems;
    const std::string path = dir + r.id + ".json";
    if (only.empty() || only.count(r.id)) {
      const bool ok = buildOne(r, m, problems);
      std::ofstream(path) << app::writePackMap(m);
      std::printf("%s %-14s stage %2d %-7s %.2f★ %4zu notes  %s\n", ok ? "ok  " : "FAIL", r.id.c_str(), r.stage,
                  r.skill.c_str(), m.stars, m.chart.notes.size(), m.title.c_str());
      for (const std::string& p : problems) std::printf("       %s\n", p.c_str());
      bad += ok ? 0 : 1;
    } else if (!app::parsePackMap(readFile(path), m)) {
      continue;
    }
    char line[512];
    std::snprintf(line, sizeof(line),
                  "%s{\"id\":%s,\"stage\":%d,\"skill\":%s,\"skillName\":%s,\"title\":%s,\"artist\":%s,\"licence\":%s,"
                  "\"song\":%s,\"chart\":%s,\"stars\":%.2f,\"length\":%.1f,\"about\":%s}",
                  first ? "" : ",\n", engine::jsonQuote(m.id).c_str(), m.stage, engine::jsonQuote(m.skill).c_str(),
                  engine::jsonQuote(skillName(m.skill)).c_str(), engine::jsonQuote(m.title).c_str(),
                  engine::jsonQuote(m.artist).c_str(), engine::jsonQuote(m.licence).c_str(),
                  engine::jsonQuote(m.song).c_str(), engine::jsonQuote(path).c_str(), m.stars, m.length,
                  engine::jsonQuote(m.about).c_str());
    index += line;
    first = false;
  }
  index += "\n]}\n";
  std::ofstream("assets/music/rhythm/pack/pack.json") << index;
  std::printf(bad ? "%d maps need another look\n" : "all maps built\n", bad);
  return bad ? 1 : 0;
}
