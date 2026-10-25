#include "looped/world/timeline.h"

#include <algorithm>
#include <cmath>

#include "looped/json.h"

namespace engine::world {

namespace {

void note(std::string* error, const std::string& where, const std::string& what) {
  if (error) *error += where + ": " + what + "\n";
}

// 0..1, eased at both ends.
double smooth(double x) {
  x = std::clamp(x, 0.0, 1.0);
  return x * x * (3 - 2 * x);
}

}  // namespace

Timeline parseTimeline(const Json& j, std::string* error) {
  Timeline tl;
  if (!j.isObject()) return tl;
  tl.length = j["length"].number(tl.length);
  if (!(tl.length > 0)) {
    note(error, "timeline", "length must be above 0");
    tl.length = 16;
  }
  tl.beat = j["beat"].boolean(true);
  int n = 0;
  for (const Json& tj : j["tracks"].items()) {
    const std::string where = "timeline track " + std::to_string(++n);
    Track t;
    t.name = tj["name"].string();
    if (tj["on"].isString()) t.on.push_back(tj["on"].string());
    for (const Json& o : tj["on"].items())
      if (o.isString()) t.on.push_back(o.string());
    if (t.on.empty()) note(error, where, "\"on\" names no thing (\"#id\" or \"kind:id\")");
    t.spread = static_cast<float>(tj["spread"].number(0));
    t.mute = tj["mute"].boolean(false);
    int k = 0;
    for (const Json& cj : tj["clips"].items()) {
      const std::string cw = where + " clip " + std::to_string(++k);
      Clip c;
      const Json& lj = cj["loop"];
      if (lj.isString()) {
        const std::string& name = lj.string();
        if (name.size() < 2 || name[0] != '@') {
          note(error, cw, "\"loop\" must be \"@<pattern>\" or a loop");
          continue;
        }
        c.pattern = name.substr(1);
      } else if (lj.isObject()) {
        c.loop = parseLoop(lj, cw, error);
      } else {
        note(error, cw, "needs a \"loop\"");
        continue;
      }
      c.at = cj["at"].number(0);
      c.len = cj["len"].number(4);
      if (!(c.len > 0)) {
        note(error, cw, "len must be above 0");
        continue;
      }
      c.mix = static_cast<float>(cj["mix"].number(1));
      c.fade = std::max(0.0, cj["fade"].number(0));
      c.speed = static_cast<float>(cj["speed"].number(1));
      t.clips.push_back(std::move(c));
    }
    tl.tracks.push_back(std::move(t));
  }
  return tl;
}

double timelineTime(const Timeline& tl, const Clock& clock) {
  const double units = tl.beat ? (clock.music ? clock.beats : clock.seconds * 2.0) : clock.seconds;
  const double t = std::fmod(units, tl.length);
  return t < 0 ? t + tl.length : t;
}

float clipWeight(const Clip& c, double t, double length, double* into) {
  double d = std::fmod(t - c.at, length);
  if (d < 0) d += length;
  // A clip as long as the timeline (or longer) never ends: it plays on, its
  // own loop running from where the round began.
  if (c.len >= length) {
    *into = d;
    return c.mix;
  }
  if (d >= c.len) return 0;
  *into = d;
  double w = c.mix;
  if (c.fade > 0) w *= smooth(d / c.fade) * smooth((c.len - d) / c.fade);
  return static_cast<float>(w);
}

double clipCycles(const Clip& c, double into, bool beatUnits, double bpm) {
  const double beatsPerSecond = (bpm > 0 ? bpm : 120.0) / 60.0;
  const double beats = beatUnits ? into : into * beatsPerSecond;
  const double seconds = beatUnits ? into / beatsPerSecond : into;
  const double period = std::max(0.01, static_cast<double>(c.loop.period));
  return (c.loop.beat ? beats / period : seconds / period) * c.speed;
}

}  // namespace engine::world
