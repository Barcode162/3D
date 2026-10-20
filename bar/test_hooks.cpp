#include "bar/test_hooks.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "looped/ui.h"

namespace app {
namespace {

// The value of `name` in the query string ("" if present without a value),
// or false if it isn't there.
bool param(const std::string& query, const char* name, std::string& value) {
  const size_t len = std::strlen(name);
  size_t at = 0;
  while (at < query.size()) {
    size_t end = query.find('&', at);
    if (end == std::string::npos) end = query.size();
    const std::string part = query.substr(at, end - at);
    if (part.compare(0, len, name) == 0 && (part.size() == len || part[len] == '=')) {
      value = part.size() > len ? part.substr(len + 1) : "";
      return true;
    }
    at = end + 1;
  }
  return false;
}

}  // namespace

TestHooks TestHooks::parse() {
  // name=value pairs, each value decoded (so "&" inside one can't split it).
  const std::string query = engine::ui::pageQuery();
  TestHooks h;
  std::string v;
  h.still = param(query, "still", v);
  h.perf = param(query, "perf", v) && v == "1";
  h.autoplay = param(query, "autoplay", v);
  h.shift = param(query, "shift", v);
  h.darts = param(query, "darts", v);
  h.garage = param(query, "garage", v);
  h.chess = param(query, "chess", v);
  h.pad = param(query, "pad", v);
  h.telemetry = param(query, "telemetry", v) && v == "1";
  h.walkcheck = param(query, "walkcheck", v);
  if (param(query, "walkpose", v) && !v.empty()) h.walkPose = static_cast<float>(std::atof(v.c_str()));
  if (param(query, "previewturn", v) && !v.empty()) h.previewTurn = static_cast<float>(std::atof(v.c_str()));
  if (param(query, "walkdir", v) && !v.empty()) h.walkDir = static_cast<float>(std::atof(v.c_str()));
  if (param(query, "sit", v) && !v.empty()) h.sit = std::atoi(v.c_str());
  if (param(query, "emote", v) && !v.empty()) h.emote = std::atoi(v.c_str());
  if (param(query, "pianosong", v) && !v.empty()) h.pianoSong = std::atoi(v.c_str());
  if (param(query, "mirrorq", v) && !v.empty()) h.mirrorQ = std::atoi(v.c_str());
  if (param(query, "quality", v)) {
    const char* names[] = {"auto", "high", "medium", "low"};
    for (int i = 0; i < 4; ++i) if (v == names[i]) h.quality = i;
  }
  if (param(query, "rhythm", v)) {
    if (v == "menu") h.rhythmSong = -2;  // just the song select
    else std::sscanf(v.c_str(), "%d,%d", &h.rhythmSong, &h.rhythmDifficulty);
  }
  if (param(query, "cam", v)) {
    h.hasCam = std::sscanf(v.c_str(), "%f,%f,%f,%f,%f", &h.cam[0], &h.cam[1], &h.cam[2], &h.cam[3], &h.cam[4]) == 5;
  }
  return h;
}

}  // namespace app
