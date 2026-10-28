#include "bar/projects/mania/pack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>

#include "bar/projects/mania/stars.h"
#include "looped/json.h"

namespace app {

bool parsePackMap(const std::string& text, PackMap& m, std::string* error) {
  engine::JsonLimits limits;
  limits.maxString = 1 << 16;
  const engine::Json j = engine::Json::parse(text, error, limits);
  if (!j.isObject() || !j["notes"].isArray()) {
    if (error && error->empty()) *error = "not a chart file";
    return false;
  }
  m.id = j["id"].string();
  m.skill = j["skill"].string();
  m.title = j["title"].string();
  m.artist = j["artist"].string();
  m.licence = j["licence"].string();
  m.song = j["song"].string();
  m.about = j["about"].string();
  m.stage = j["stage"].integer(0);
  m.bpm = j["bpm"].number();
  m.offsetMs = j["offset"].number();
  m.length = j["length"].number();
  m.stars = j["stars"].number();
  m.chart = {};
  m.chart.bpm = m.bpm;
  m.chart.length = m.length;
  for (const engine::Json& n : j["notes"].items()) {
    if (!n.isArray() || n.size() < 2) continue;
    ChartNote c{n[0].number() / 1000.0, std::clamp(n[1].integer(0), 0, 3)};
    if (n.size() > 2) c.end = n[2].number() / 1000.0;
    m.chart.notes.push_back(c);
  }
  std::stable_sort(m.chart.notes.begin(), m.chart.notes.end(),
                   [](const ChartNote& a, const ChartNote& b) { return a.time < b.time; });
  return true;
}

std::string writePackMap(const PackMap& m) {
  std::vector<ChartNote> notes = m.chart.notes;
  std::stable_sort(notes.begin(), notes.end(), [](const ChartNote& a, const ChartNote& b) {
    return a.time != b.time ? a.time < b.time : a.lane < b.lane;
  });
  char b[160];
  std::string s = "{\"v\":1,\"id\":" + engine::jsonQuote(m.id) + ",\"stage\":" + std::to_string(m.stage) +
                  ",\"skill\":" + engine::jsonQuote(m.skill) + ",\n\"title\":" + engine::jsonQuote(m.title) +
                  ",\"artist\":" + engine::jsonQuote(m.artist) + ",\"licence\":" + engine::jsonQuote(m.licence) +
                  ",\n\"song\":" + engine::jsonQuote(m.song) + ",\"about\":" + engine::jsonQuote(m.about) + ",\n";
  std::snprintf(b, sizeof(b), "\"bpm\":%.3f,\"offset\":%.1f,\"length\":%.2f,\"stars\":%.2f,\n\"notes\":[", m.bpm, m.offsetMs,
                m.length, m.stars);
  s += b;
  for (size_t i = 0; i < notes.size(); ++i) {
    const ChartNote& n = notes[i];
    if (n.end > n.time) std::snprintf(b, sizeof(b), "%s[%.0f,%d,%.0f]", i ? "," : "", n.time * 1000, n.lane, n.end * 1000);
    else std::snprintf(b, sizeof(b), "%s[%.0f,%d]", i ? "," : "", n.time * 1000, n.lane);
    s += b;
    if (i % 12 == 11) s += "\n";
  }
  return s + "]}\n";
}

StageRules stageRules(int stage) {
  StageRules r;
  r.stage = std::clamp(stage, 1, 10);
  // Stars: about +0.5 a stage from 2.0; the last stage 6.5-7.2.
  const double target = 1.5 + 0.5 * r.stage;
  r.starsMin = target - 0.2;
  r.starsMax = r.stage == 10 ? 7.2 : target + 0.2;
  switch (r.stage) {
    case 1:  // osu!mania's Normal: chords of 2, at most 5 consecutive 1/4, no 1/4 jacks, holds ≥ 1/2 beat
      r.maxChord = 2, r.minGapBeats = 0.25, r.maxRun = 5, r.minJackBeats = 0.5, r.maxFastJack = 1;
      r.minHoldBeats = 0.5, r.holdGapBeats = 0.5, r.maxTrill = 5, r.maxSpike = 1.6;
      break;
    case 2:
      r.maxChord = 2, r.minGapBeats = 0.25, r.maxRun = 9, r.minJackBeats = 0.5, r.maxFastJack = 1;
      r.minHoldBeats = 0.5, r.holdGapBeats = 0.5, r.maxTrill = 9, r.maxSpike = 1.6;
      break;
    case 3:  // Hard: chords of 3, 1/4 streams with pauses, trills ≤ 9, holds ≥ 1/4 beat
    case 4:
      r.maxChord = 3, r.minGapBeats = 0.25, r.maxRun = r.stage == 3 ? 16 : 32, r.minJackBeats = 0.25;
      r.maxFastJack = r.stage == 3 ? 1 : 2, r.minHoldBeats = 0.25, r.holdGapBeats = 0.5, r.maxTrill = 9, r.maxSpike = 1.55;
      break;
    case 5:  // Insane: hands, triplets and 1/6 bursts, minijacks
    case 6:
      r.maxChord = 3, r.minGapBeats = 1.0 / 6.0, r.maxRun = 1 << 20, r.minJackBeats = 0.25, r.maxFastJack = 3;
      r.minHoldBeats = 0.25, r.holdGapBeats = 0.25, r.maxTrill = 24, r.maxSpike = 1.5;
      break;
    default:  // Expert and up: quads, chordjacks, 1/8 bursts
      r.maxChord = 4, r.minGapBeats = 0.125, r.maxRun = 1 << 20, r.minJackBeats = 0.25, r.maxFastJack = 1 << 20;
      r.minHoldBeats = 0.25, r.holdGapBeats = 0.25, r.maxTrill = 1 << 20, r.maxSpike = 1.45;
      break;
  }
  return r;
}

std::vector<std::string> checkPackMap(const PackMap& m, const StageRules& r) {
  std::vector<std::string> out;
  char b[200];
  const auto at = [](double t) {
    char s[24];
    std::snprintf(s, sizeof(s), "%d:%05.2f", static_cast<int>(t / 60), std::fmod(t, 60.0));
    return std::string(s);
  };
  const std::vector<ChartNote>& n = m.chart.notes;
  if (n.empty() || m.bpm <= 0) return {"no notes or no tempo"};
  const double beat = 60.0 / m.bpm, eps = 0.003;
  // Rows: notes starting together.
  struct Row {
    double t;
    int mask = 0, count = 0;
  };
  std::vector<Row> rows;
  for (const ChartNote& c : n) {
    if (rows.empty() || c.time - rows.back().t > eps) rows.push_back({c.time});
    if (rows.back().mask & (1 << c.lane)) {
      std::snprintf(b, sizeof(b), "two notes in column %d at %s", c.lane + 1, at(c.time).c_str());
      out.push_back(b);
    }
    rows.back().mask |= 1 << c.lane;
    ++rows.back().count;
  }
  // Keys at once (counting holds still held).
  for (const Row& row : rows) {
    int held = 0;
    for (const ChartNote& c : n) {
      if (c.end > c.time && c.time < row.t - eps && c.end > row.t + eps && !(row.mask & (1 << c.lane))) ++held;
    }
    if (row.count + held > r.maxChord) {
      std::snprintf(b, sizeof(b), "%d keys at once at %s (stage %d allows %d)", row.count + held, at(row.t).c_str(), r.stage,
                    r.maxChord);
      out.push_back(b);
    }
  }
  // Chords inside fast runs (osu!mania's Normal: none; Hard: at least a beat apart).
  if (r.stage <= 4) {
    double lastChord = -1e9;
    for (size_t i = 0; i < rows.size(); ++i) {
      if (rows[i].count < 2) continue;
      const bool inRun = (i > 0 && (rows[i].t - rows[i - 1].t) / beat <= 0.26) ||
                         (i + 1 < rows.size() && (rows[i + 1].t - rows[i].t) / beat <= 0.26);
      if (!inRun) continue;
      if (r.stage <= 2 && i > 0 && i + 1 < rows.size() && (rows[i].t - rows[i - 1].t) / beat <= 0.26 &&
          (rows[i + 1].t - rows[i].t) / beat <= 0.26) {
        std::snprintf(b, sizeof(b), "a chord in the middle of a fast run at %s (stage %d: none)", at(rows[i].t).c_str(), r.stage);
        out.push_back(b);
      } else if (r.stage >= 3 && (rows[i].t - lastChord) / beat < 0.98) {
        std::snprintf(b, sizeof(b), "chords less than a beat apart in a fast run at %s (stage %d)", at(rows[i].t).c_str(), r.stage);
        out.push_back(b);
      }
      lastChord = rows[i].t;
    }
  }
  // Fastest step, runs at a 1/4 beat or faster, trills.
  int run = 1, trill = 1;
  for (size_t i = 1; i < rows.size(); ++i) {
    const double gap = (rows[i].t - rows[i - 1].t) / beat;
    if (gap < r.minGapBeats - 0.02) {
      std::snprintf(b, sizeof(b), "a 1/%d step at %s (stage %d: 1/%d at the fastest)", static_cast<int>(std::lround(1 / gap)),
                    at(rows[i].t).c_str(), r.stage, static_cast<int>(std::lround(1 / r.minGapBeats)));
      out.push_back(b);
    }
    run = gap <= 0.26 ? run + 1 : 1;
    if (run == r.maxRun + 1) {
      std::snprintf(b, sizeof(b), "a run of more than %d fast notes ending after %s", r.maxRun, at(rows[i].t).c_str());
      out.push_back(b);
    }
    const bool single = rows[i].count == 1 && rows[i - 1].count == 1;
    const bool alternates = single && i >= 2 && rows[i - 2].count == 1 && rows[i].mask == rows[i - 2].mask &&
                            rows[i].mask != rows[i - 1].mask;
    trill = gap <= 0.26 && alternates ? trill + 1 : (single && gap <= 0.26 ? 2 : 1);
    if (trill == r.maxTrill + 1) {
      std::snprintf(b, sizeof(b), "a trill longer than %d notes at %s", r.maxTrill, at(rows[i].t).c_str());
      out.push_back(b);
    }
  }
  // Columns: jacks, holds, what follows a hold.
  std::array<int, 4> perColumn{};
  for (int col = 0; col < 4; ++col) {
    const ChartNote* prev = nullptr;
    int chain = 1;
    for (const ChartNote& c : n) {
      if (c.lane != col) continue;
      ++perColumn[static_cast<size_t>(col)];
      if (c.end > c.time && (c.end - c.time) / beat < r.minHoldBeats - 0.02) {
        std::snprintf(b, sizeof(b), "a hold shorter than %.2f beat at %s", r.minHoldBeats, at(c.time).c_str());
        out.push_back(b);
      }
      if (prev) {
        const double from = prev->end > prev->time ? prev->end : prev->time;
        const double gap = (c.time - from) / beat;
        if (prev->end > prev->time && gap < r.holdGapBeats - 0.02) {
          std::snprintf(b, sizeof(b), "a note %.2f beat after a hold's end in column %d at %s", gap, col + 1, at(c.time).c_str());
          out.push_back(b);
        }
        if (prev->end <= prev->time && gap < r.minJackBeats - 0.02) {
          std::snprintf(b, sizeof(b), "a jack faster than %.2f beat in column %d at %s", r.minJackBeats, col + 1,
                        at(c.time).c_str());
          out.push_back(b);
        }
        chain = prev->end <= prev->time && gap < 0.49 ? chain + 1 : 1;
        if (chain == r.maxFastJack + 1) {
          std::snprintf(b, sizeof(b), "a jack of more than %d notes in column %d at %s", r.maxFastJack, col + 1,
                        at(c.time).c_str());
          out.push_back(b);
        }
      }
      prev = &c;
    }
  }
  // Every column used, the hands about even.
  const double total = static_cast<double>(n.size());
  for (int col = 0; col < 4; ++col) {
    if (perColumn[static_cast<size_t>(col)] < total * 0.15) {
      std::snprintf(b, sizeof(b), "column %d has only %d of %d notes", col + 1, perColumn[static_cast<size_t>(col)],
                    static_cast<int>(total));
      out.push_back(b);
    }
  }
  const double left = (perColumn[0] + perColumn[1]) / total;
  if (left < 0.38 || left > 0.62) {
    std::snprintf(b, sizeof(b), "the hands are uneven: %.0f %% left", left * 100);
    out.push_back(b);
  }
  // Stars in the stage's range; no single moment far harder than the map's hard parts.
  const double stars = starRating(n);
  if (stars < r.starsMin || stars > r.starsMax) {
    std::snprintf(b, sizeof(b), "%.2f stars (stage %d: %.1f-%.1f)", stars, r.stage, r.starsMin, r.starsMax);
    out.push_back(b);
  }
  std::vector<double> sections = strainSections(n);
  if (sections.size() > 20) {
    std::vector<double> sorted = sections;
    std::sort(sorted.begin(), sorted.end());
    const double typical = sorted[static_cast<size_t>(static_cast<double>(sorted.size()) * 0.9)];
    const double top = sorted.back();
    if (typical > 0 && top / typical > r.maxSpike) {
      const size_t i = static_cast<size_t>(std::max_element(sections.begin(), sections.end()) - sections.begin());
      std::snprintf(b, sizeof(b), "a spike near %s: %.2f× the map's hard parts (stage %d allows %.2f×)",
                    at(n.front().time + static_cast<double>(i) * 0.4).c_str(), top / typical, r.stage, r.maxSpike);
      out.push_back(b);
    }
  }
  return out;
}

}  // namespace app
