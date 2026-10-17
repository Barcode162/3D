// Physics test on the real bar (app::buildBarScene + app::buildPhysics):
// - after loading, every glass settles and sleeps within 2 s and stays put
//   (no slow jiggle, e.g. the pint at the left end of the bar);
// - 10,000 random knocks never make a NaN or send a glass out of the room;
// - how long a step takes (worst and average).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "app/bar_scene.h"
#include "app/props.h"
#include "engine/physics.h"

using engine::Vec3;

static double nowMs() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main() {
  int failed = 0;
  app::BarScene scene = app::buildBarScene();
  engine::PhysicsWorld world;
  std::vector<int> bodyOfModel;
  app::buildPhysics(scene, world, bodyOfModel);
  const size_t n = world.bodyCount();
  std::printf("bodies: %zu\n", n);

  // 1. Settling: 10 s at 60 fps. A glass "settles" when it falls asleep and
  // stays asleep for the rest of the run.
  std::vector<double> sleptAt(n, -1);
  std::vector<Vec3> start(n);
  for (size_t i = 0; i < n; ++i) start[i] = world.body(i).position;
  double worst = 0, total = 0;
  int steps = 0;
  const float dt = 1.0f / 60.0f;
  for (int f = 0; f < 600; ++f) {
    const double t0 = nowMs();
    world.step(dt);
    const double ms = nowMs() - t0;
    worst = std::max(worst, ms);
    total += ms;
    ++steps;
    for (size_t i = 0; i < n; ++i) {
      const bool asleep = world.body(i).asleep;
      if (asleep && sleptAt[i] < 0) sleptAt[i] = (f + 1) * dt;
      if (!asleep) sleptAt[i] = -1;
    }
  }
  double latest = 0;
  for (size_t i = 0; i < n; ++i) {
    const engine::Body& b = world.body(i);
    const float moved = engine::length(b.position - start[i]);
    if (sleptAt[i] < 0 || sleptAt[i] > 2.0 || moved > 0.02f) {
      std::printf("FAIL glass %zu at %.2f,%.2f,%.2f: asleep since %.2f s, moved %.3f m, speed %.4f m/s\n", i,
                  start[i].x, start[i].y, start[i].z, sleptAt[i], moved, engine::length(b.velocity));
      ++failed;
    }
    latest = std::max(latest, sleptAt[i]);
  }
  if (!failed) std::printf("all %zu glasses asleep by t <= %.2f s\n", n, latest);
  std::printf("step: worst %.3f ms, average %.3f ms (native)\n", worst, total / steps);

  // 2. Random knocks: impulses on random glasses every frame, 10,000 in all.
  std::mt19937 rng(7);
  std::uniform_real_distribution<float> u(-1, 1);
  int bad = 0;
  for (int k = 0; k < 10000; ++k) {
    engine::Body& b = world.body(rng() % n);
    b.asleep = false;
    b.velocity += Vec3{u(rng), std::fabs(u(rng)), u(rng)} * (k % 97 == 0 ? 200.0f : 3.0f);
    b.angularVelocity += Vec3{u(rng), u(rng), u(rng)} * (k % 89 == 0 ? 1e4f : 20.0f);
    if (k % 5 == 0) world.step(dt);
  }
  for (int f = 0; f < 600; ++f) world.step(dt);
  for (size_t i = 0; i < n; ++i) {
    const engine::Body& b = world.body(i);
    const bool finite = engine::isFinite(b.position) && engine::isFinite(b.rotation) && engine::isFinite(b.velocity);
    const bool inRoom = std::fabs(b.position.x) < 30 && b.position.y > -0.1f && b.position.y < 10 && std::fabs(b.position.z) < 30;
    if (!finite || !inRoom) {
      std::printf("FAIL glass %zu after knocks: %s at %.2f,%.2f,%.2f\n", i, finite ? "outside the room" : "NaN",
                  b.position.x, b.position.y, b.position.z);
      ++bad;
    }
  }
  if (!bad) std::printf("no NaN after 10k random impulses, all glasses in the room (%d put back, %llu steps dropped)\n", world.resets(), static_cast<unsigned long long>(world.droppedSteps()));
  failed += bad;
  std::printf("physics_test: %s\n", failed ? "FAILED" : "ok");
  return failed ? 1 : 0;
}
