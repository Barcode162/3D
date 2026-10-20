// Picture quality on Auto (looped/quality.h): steps down when the GPU keeps
// frames slow, not when the CPU does; back up when there's room; no ping-pong.
#include <cstdio>
#include <cstdlib>

#include "looped/quality.h"

static int failures = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failures;
}

// Runs `seconds` of frames at 60 Hz rAF with the given costs; returns the time.
static double run(engine::QualityController& q, double t, double seconds, float interval, float cpu, float gpu,
                  int* changes = nullptr, bool drawing = true) {
  for (double end = t + seconds * 1000; t < end; t += interval) {
    if (q.frame(t, interval, cpu, gpu, drawing) && changes) ++*changes;
  }
  return t;
}

int main() {
  using Q = engine::QualityController;
  {
    Q q;
    double t = run(q, 0, 10, 16.7f, 2, 9);
    check(q.tier() == 0, "smooth frames stay sharp");
    t = run(q, t, 10, 22, 2, 18);
    check(q.tier() >= 1, "GPU-bound slow frames step down");
    const int down = q.tier();
    t = run(q, t, 10, 22, 2, 18);
    check(q.tier() == 2 && down <= 2, "and further while still slow (stops at Low)");
    t = run(q, t, 30, 16.7f, 2, 5);
    check(q.tier() < 2, "plenty of room: steps back up");
  }
  {
    Q q;
    run(q, 0, 20, 30, 20, 10);
    check(q.tier() == 0, "CPU-bound slow frames don't drop the picture (fewer pixels wouldn't help)");
  }
  {
    Q q;
    double t = run(q, 0, 3, 22, 2, 18);
    check(q.tier() == 1, "one step down after a slow stretch");
    // At Medium the GPU takes 9 ms: High would need 9 * 1.51 ≈ 13.6 ms: stays.
    t = run(q, t, 30, 16.7f, 2, 9);
    check(q.tier() == 1, "no step up when the sharper level wouldn't fit");
  }
  {
    Q q;
    double t = run(q, 0, 3, 22, 2, 18);    // down
    t = run(q, t, 12, 16.7f, 2, 5);         // up
    const int ups = q.tier();
    t = run(q, t, 6, 22, 2, 18);            // straight back down: a bounce
    int changes = 0;
    t = run(q, t, 40, 16.7f, 2, 5, &changes);
    check(ups == 0 && q.tier() == 1 && changes == 0, "after a bounce it stays down for a minute");
    t = run(q, t, 40, 16.7f, 2, 5);
    check(q.tier() == 0, "then tries again");
  }
  {
    Q q;
    q.setMode(Q::kLow, 0);
    run(q, 0, 30, 16.7f, 2, 3);
    check(q.tier() == 2 && q.pixelRatioCap() == 1.3, "Low stays Low (no auto changes)");
    q.setMode(Q::kHigh, 1);
    check(q.tier() == 0 && q.pixelRatioCap() == 2.0, "High is the old picture (2x)");
  }
  {
    Q q;
    run(q, 0, 20, 22, 2, 18, nullptr, false);
    check(q.tier() == 0, "nothing changes while not drawing (loading, Meridian)");
    run(q, 0, 20, 250, 2, 18);
    check(q.tier() == 0, "hitches (a frame over 100 ms) aren't a GPU signal");
  }
  if (failures) { std::printf("QUALITY TEST FAILED (%d)\n", failures); return EXIT_FAILURE; }
  std::printf("QUALITY TEST PASSED\n");
  return 0;
}
