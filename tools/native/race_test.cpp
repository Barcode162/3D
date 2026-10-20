// Native test of a whole race (no browser): 8 bots, 3 laps. Shows lap
// times, finishing order, how often bots got stuck, and the track's numbers.
// Then runs the same race at 30, 57.3 and 120 fps: the results must be
// identical ("frame-rate independent: OK"), since the race runs on ticks.
//   g++ -std=c++20 -O2 -I. tools/native/race_test.cpp bar/projects/racing/world.cpp bar/projects/racing/track.cpp bar/projects/racing/mesh.cpp
//       looped/vehicle.cpp -o build/race_test && build/race_test
#include <cmath>
#include <cstdio>
#include <string>

#include "bar/projects/racing/cars.h"
#include "bar/projects/racing/world.h"

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

  // The Ridgeback GS on the new physics (looped/drive), driven round the
  // Ring by the bots' driver: it finishes, in a sane time, the same at any
  // frame rate.
  const auto gsRace = [&](double fps) {
    RacerSetup me;
    me.model = kRidgeback;
    me.grid = 0;
    w.start({me}, 0, 2, 0);
    Racer& you = w.racers()[w.you()];
    if (!you.drive) std::printf("GS: no new physics attached!\n");
    you.bot = true;  // the bots' driver drives it (on the new physics)
    for (int frame = 0; frame < static_cast<int>(fps * 300) && !w.allFinished(); ++frame) w.update(static_cast<float>(1.0 / fps));
    char line[160];
    std::snprintf(line, sizeof(line), "laps %d best %.3f s finish %.3f s", you.lap, you.bestLapMs / 1000,
                  you.finishMs / 1000);
    return std::string(line);
  };
  const std::string gs60 = gsRace(60);
  std::printf("GS on the new physics, 2 laps: %s\n", gs60.c_str());
  {
    const Racer& you = w.racers()[w.you()];
    if (you.lap < 2 || !(you.bestLapMs > 50000 && you.bestLapMs < 120000)) {
      std::printf("GS RACE FAILED\n");
      ++failures;
    }
  }
  for (double fps : {30.0, 144.0}) {
    const std::string r = gsRace(fps);
    if (r != gs60) {
      same = false;
      std::printf("GS at %.0f fps differs: %s\n", fps, r.c_str());
    }
  }
  // The skid pad: circles for 30 s, stays on the pad, nothing blows up.
  {
    RacerSetup me;
    me.model = kRidgeback;
    w.startPractice(me, true);
    Racer& you = w.racers()[0];
    float maxV = 0;
    for (int frame = 0; frame < 60 * 30; ++frame) {
      you.input.steer = you.wheel = -0.35f;
      you.input.throttle = 0.35f;
      w.update(1.0f / 60);
      maxV = std::max(maxV, you.car.speed());
    }
    const bool ok = w.track().onPad(you.car.position) && std::isfinite(you.car.position.x) && maxV > 8;
    std::printf("skid pad: %.0f km/h top, on the pad: %s\n", maxV * 3.6f, ok ? "yes" : "NO");
    if (!ok) ++failures;
    // Old physics, the same car (the comparison switch).
    engine::drive::Setup st;
    st.oldPhysics = true;
    w.setSetup(st);
    if (you.drive) ++failures;
    for (int frame = 0; frame < 60 * 5; ++frame) w.update(1.0f / 60);
    st.oldPhysics = false;
    w.setSetup(st);
    if (!you.drive) ++failures;
    std::printf("old physics switch: %s\n", you.drive ? "ok" : "FAILED");
  }
  std::printf("GS frame-rate independent: %s\n", same ? "OK" : "FAILED");
  return failures || !same ? 1 : 0;
}
