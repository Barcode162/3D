// 4K Mania's star counter (bar/projects/mania/stars.cpp, J117.7) against the
// numbers of the prototype the Training pack was priced with (osu!mania's
// formula, re-made twice: in Python for the plan, in C++ for the game).
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>

#include "bar/projects/mania/pack.h"
#include "bar/projects/mania/stars.h"
#include "looped/json.h"

using app::ChartNote;

namespace {
int failures = 0;
void check(const char* name, double got, double want) {
  const bool ok = std::fabs(got - want) < 0.005;
  if (!ok) ++failures;
  std::printf("%s %-34s %.4f (want %.4f)\n", ok ? "ok  " : "FAIL", name, got, want);
}
// `rows` repeat on a 1/`snap` grid for 128 beats; a row is four 0/1 marks.
std::vector<ChartNote> make(const std::vector<std::string>& rows, double bpm, int snap) {
  std::vector<ChartNote> out;
  const double step = 60.0 / bpm / snap;
  for (int k = 0; k < 128 * snap; ++k) {
    const std::string& r = rows[static_cast<size_t>(k) % rows.size()];
    for (int c = 0; c < 4; ++c) {
      if (r[static_cast<size_t>(c)] == '1') out.push_back({k * step, c});
    }
  }
  return out;
}
}  // namespace

int main() {
  check("jumptrill 150 1/4", app::starRating(make({"1100", "0011"}, 150, 4)), 4.2318);
  check("jumptrill 160 1/4", app::starRating(make({"1100", "0011"}, 160, 4)), 4.4878);
  check("chordjack 150 1/2", app::starRating(make({"1100", "1101", "0101", "1011"}, 150, 2)), 3.0837);
  check("chordjack 150 1/4", app::starRating(make({"1100", "1101", "0101", "1011"}, 150, 4)), 5.5907);
  check("quad jack 150 1/2", app::starRating(make({"1111"}, 150, 2)), 4.4226);
  check("roll 150 1/4", app::starRating(make({"1000", "0100", "0010", "0001"}, 150, 4)), 2.2243);
  std::vector<ChartNote> holds;
  for (int k = 0; k < 64; ++k) {
    const double t = k * 0.4;
    holds.push_back({t, k % 4, t + 0.3});
    holds.push_back({t + 0.1, (k + 1) % 4});
    holds.push_back({t + 0.2, (k + 2) % 4});
  }
  check("holds with taps", app::starRating(holds), 2.1554);
  check("empty chart", app::starRating({}), 0);
  // The map checker: a three-key chord is too much for stage 1, and a chart
  // file reads back as it was written.
  app::PackMap m;
  m.id = "t", m.stage = 1, m.bpm = 120;
  m.chart.notes = {{1.0, 0}, {1.0, 1}, {1.0, 2}, {1.5, 3}, {2.0, 0, 3.0}};
  bool chord = false;
  for (const std::string& v : app::checkPackMap(m, app::stageRules(1))) chord |= v.find("3 keys at once") != std::string::npos;
  check("checker: 3 keys at stage 1", chord ? 1 : 0, 1);
  app::PackMap back;
  check("chart file reads back", app::parsePackMap(app::writePackMap(m), back) ? static_cast<double>(back.chart.notes.size()) : -1, 5);
  check("hold end reads back", back.chart.notes.size() == 5 ? back.chart.notes[4].end : -1, 3.0);
  // Every Training map in the pack keeps its stage's rules and stars.
  {
    const auto read = [](const std::string& path) {
      std::ifstream f(path);
      std::stringstream ss;
      ss << f.rdbuf();
      return ss.str();
    };
    const engine::Json index = engine::Json::parse(read("assets/music/rhythm/pack/pack.json"));
    int maps = 0, bad = 0;
    for (const engine::Json& e : index["maps"].items()) {
      app::PackMap pm;
      ++maps;
      if (!app::parsePackMap(read(e["chart"].string()), pm) ||
          !app::checkPackMap(pm, app::stageRules(pm.stage)).empty()) {
        std::printf("FAIL the Training map %s\n", e["id"].string().c_str());
        ++bad;
      }
    }
    check("Training maps keep their stage's rules", bad, 0);
    check("the Training pack has maps", maps > 0 ? 1 : 0, 1);
  }
  std::printf(failures ? "FAILED: %d\n" : "all star and chart checks passed\n", failures);
  return failures ? 1 : 0;
}
