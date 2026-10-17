// Native test of a whole race (no browser): 8 bots, 3 laps. Shows lap
// times, finishing order, how often bots got stuck, and the track's numbers.
// Then runs the same race at 30, 57.3 and 120 fps: the results must be
// identical ("frame-rate independent: OK"), since the race runs on ticks.
//   g++ -std=c++20 -O2 -I. tools/native/race_test.cpp app/race/world.cpp app/race/track.cpp app/race/mesh.cpp
//       engine/vehicle.cpp -o build/race_test && build/race_test
#include <cmath>
#include <cstdio>
#include <string>

#include "app/race/world.h"

using namespace app::race;

int main() {
  World w;
  w.init();
  const Track& t = w.track();
  float minR = 1e9f;
  for (const auto& s : t.samples()) minR = std::min(minR, 1.0f / std::max(std::fabs(s.curvature), 1e-5f));
  float minV = 1e9f, maxV = 0;
  for (float v : w.lineSpeed()) {
    minV = std::min(minV, v);
    maxV = std::max(maxV, v);
  }
  std::printf("track %.0f m, %zu samples, tightest radius %.0f m, line speeds %.0f..%.0f km/h\n", t.length(),
              t.samples().size(), minR, minV * 3.6f, maxV * 3.6f);
  auto chunks = t.buildMeshes();
  size_t tris = 0;
  for (auto& c : chunks) tris += c.mesh.indices.size() / 3;
  std::printf("meshes: %zu chunks, %zu triangles\n", chunks.size(), tris);

  // One whole race with frames of 1/fps seconds; returns the results.
  const auto race = [&](double fps, bool print) {
    w.start(3, 7, 3, 0);
    w.racers()[w.you()].bot = true;  // everyone is a bot here
    int resets = 0;
    std::vector<float> lastStuck(w.racers().size(), 0);
    for (int frame = 0; frame < static_cast<int>(fps * 400) && !w.allFinished(); ++frame) {
      w.update(static_cast<float>(1.0 / fps));
      for (size_t i = 0; i < w.racers().size(); ++i) {
        const float st = w.racers()[i].stuckTime;
        if (lastStuck[i] > 3.0f && st == 0) ++resets;
        lastStuck[i] = st;
      }
      if (print && frame % static_cast<int>(fps * 20) == 0) {
        const Racer& r = w.racers()[0];
        std::printf("  t=%3.0f s  leader-ish: %s lap %d s=%.0f %.0f km/h gear %d place %d\n",
                    (w.clockMs() - w.goMs()) / 1000, r.name.c_str(), r.lap, r.s, r.car.speed() * 3.6f, r.car.gear,
                    r.place);
      }
    }
    std::string out;
    char line[160];
    for (const Racer& r : w.racers()) {
      std::snprintf(line, sizeof(line), "%-7s place %d laps %d best %.3f s finish %.3f s\n", r.name.c_str(), r.place,
                    r.lap, r.bestLapMs / 1000, r.finishMs / 1000);
      out += line;
    }
    if (print) {
      std::printf("%s", out.c_str());
      std::printf("stuck resets: %d, race time %.0f s\n", resets, (w.clockMs() - w.goMs()) / 1000);
    }
    return out;
  };
  const std::string at60 = race(60, true);
  // Basic checks so CI fails on a broken race: everyone finishes 3 laps in
  // a sane time with sane lap times.
  int failures = 0;
  for (const Racer& r : w.racers()) {
    if (r.lap < 3 || r.finishMs <= 0 || !(r.bestLapMs > 50000 && r.bestLapMs < 120000)) failures++;
  }
  if (failures) std::printf("RACE TEST FAILED (%d racers)\n", failures);
  // The same race at other frame rates must come out exactly the same.
  bool same = true;
  for (double fps : {30.0, 57.3, 120.0}) {
    const std::string r = race(fps, false);
    if (r != at60) {
      same = false;
      std::printf("at %.1f fps the results differ:\n%s", fps, r.c_str());
    }
  }
  std::printf("frame-rate independent: %s\n", same ? "OK" : "FAILED");
  return failures || !same ? 1 : 0;
}
