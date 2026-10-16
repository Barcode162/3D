// Native test of a whole race (no browser): 8 bots, 3 laps. Shows lap
// times, finishing order, how often bots got stuck, and the track's numbers.
//   g++ -std=c++20 -O2 -I. tools/native/race_test.cpp app/race/world.cpp app/race/track.cpp app/race/mesh.cpp
//       engine/vehicle.cpp -o build/race_test && build/race_test
#include <cmath>
#include <cstdio>

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

  w.start(3, 7, 3, 0);
  w.racers()[w.you()].bot = true;  // everyone is a bot here
  int resets = 0;
  double now = 0;
  std::vector<float> lastStuck(w.racers().size(), 0);
  for (int frame = 0; frame < 60 * 400 && !w.allFinished(); ++frame) {
    now += 1000.0 / 60;
    w.update(now, 1.0f / 60);
    for (size_t i = 0; i < w.racers().size(); ++i) {
      const float st = w.racers()[i].stuckTime;
      if (lastStuck[i] > 3.0f && st == 0) ++resets;
      lastStuck[i] = st;
    }
    if (frame % (60 * 20) == 0) {
      const Racer& r = w.racers()[0];
      std::printf("  t=%3.0f s  leader-ish: %s lap %d s=%.0f %.0f km/h gear %d place %d\n", (now - w.goMs()) / 1000,
                  r.name.c_str(), r.lap, r.s, r.car.speed() * 3.6f, r.car.gear, r.place);
    }
  }
  for (const Racer& r : w.racers()) {
    std::printf("%-7s place %d laps %d best %.2f s finish %.1f s\n", r.name.c_str(), r.place, r.lap,
                r.bestLapMs / 1000, r.finishMs / 1000);
  }
  std::printf("stuck resets: %d, race time %.0f s\n", resets, (now - w.goMs()) / 1000);
  return 0;
}
