// Picture quality on Auto and Ultra (looped/quality.h): steps down when the
// GPU keeps frames slow or late, not when the CPU does; back up when steady;
// no ping-pong. The GPU wait is ignored (J110.3: on phones it counts queued
// frames), so the runs below pass a misleading 50 ms where it shouldn't matter.
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

// Frames at 60 Hz with one late frame (`lateMs`) every `every` frames.
static double stutter(engine::QualityController& q, double t, double seconds, int every, float lateMs,
                      int* changes = nullptr) {
  int i = 0;
  for (double end = t + seconds * 1000; t < end; ++i) {
    const float interval = i % every == 0 ? lateMs : 16.7f;
    t += interval;
    if (q.frame(t, interval, 1.3f, 50, true) && changes) ++*changes;
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
    t = run(q, t, 30, 16.7f, 2, 50);
    check(q.tier() == 0, "steady frames step back up, whatever the GPU wait says");
  }
  {
    Q q;
    // The user's phone on Ultra (report SC7QKJ): ~57 fps, about 1 frame in 11 late.
    double t = stutter(q, 0, 10, 11, 25);
    check(q.tier() >= 1, "Auto: frequent late frames (stutter) step down though the average is near 60");
    Q u;
    u.setMode(Q::kUltra, 0);
    t = stutter(u, 0, 5, 11, 25);
    check(u.level() == Q::kLevelUltra && u.pixelRatioCap() == 2.5, "Ultra eases to 2.5x and keeps every feature");
    t = stutter(u, t, 6, 11, 25);
    check(u.pixelRatioCap() == 2.0, "then 2x while still late (no lower)");
    t = stutter(u, t, 20, 11, 25);
    check(u.pixelRatioCap() == 2.0 && u.level() == Q::kLevelUltra, "Ultra never goes under 2x");
    t = run(u, t, 30, 16.7f, 1.3f, 50);
    check(u.pixelRatioCap() > 2.0, "steady: Ultra sharpens again");
    t = run(u, t, 30, 16.7f, 1.3f, 50);
    check(u.pixelRatioCap() == 3.0, "back to 3x");
    Q v;
    stutter(v, 0, 20, 100, 25);
    check(v.tier() == 0, "a rare late frame (1 in 100) doesn't step down");
  }
  {
    Q q;
    double t = run(q, 0, 3, 22, 2, 18);    // down
    t = run(q, t, 12, 16.7f, 2, 5);         // up
    const int ups = q.tier();
    t = run(q, t, 6, 22, 2, 18);            // straight back down: a bounce (one or two steps)
    const int down = q.tier();
    int changes = 0;
    t = run(q, t, 40, 16.7f, 2, 5, &changes);
    check(ups == 0 && down >= 1 && q.tier() == down && changes == 0, "after a bounce it stays down for a minute");
    t = run(q, t, 40, 16.7f, 2, 5);
    check(q.tier() == 0, "then tries again");
  }
  {
    Q q;
    q.setMode(Q::kLow, 0);
    run(q, 0, 30, 16.7f, 2, 3);
    check(q.tier() == 2 && q.pixelRatioCap() == 1.3, "Low stays Low (no auto changes)");
    run(q, 0, 30, 25, 2, 50);
    check(q.tier() == 2 && q.pixelRatioCap() == 1.3, "even when slow");
    q.setMode(Q::kHigh, 1);
    check(q.tier() == 0 && q.pixelRatioCap() == 2.0, "High is the old picture (2x)");
    q.setMode(Q::kUltra, 2);
    check(q.level() == Q::kLevelUltra && q.pixelRatioCap() == 3.0, "Ultra draws at up to 3x (level Ultra)");
    q.setMode(Q::kMedium, 3);
    check(q.level() == Q::kLevelMedium && q.pixelRatioCap() == 1.6, "Medium is level Medium");
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
