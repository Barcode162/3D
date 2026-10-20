// The bar's physics for the room (cloud/room.js): the same C++ the phones
// run (looped/physics + the bar's layout from bar/bar_scene + bar/props),
// compiled to a small WebAssembly module the Cloudflare Worker loads
// (cloud/sim/build.sh → cloud/sim/room_sim.wasm). The room simulates every
// glass: held ones follow their holder's hand, thrown and knocked ones fly,
// bounce and settle exactly once, for everyone.
//
// Calls (all numbers go through one float buffer, sim_buffer()):
//   sim_init() → number of bodies (glasses), at their layout spots
//   sim_step(dt)            advance dt seconds (fixed 60 Hz steps inside)
//   sim_get(i) → 1          buffer = pos x,y,z, quat x,y,z,w, vel x,y,z, asleep, held
//   sim_hold(i)             buffer[0..2] = hand position: pulled there, no gravity
//   sim_release(i)          buffer = pos x,y,z, quat x,y,z,w, vel x,y,z: let go there
//   sim_place(i)            buffer = pos x,y,z, quat x,y,z,w: put down at rest
//   sim_awake() → count of bodies moving or held
#include <emscripten/emscripten.h>

#include <vector>

#include "bar/bar_scene.h"
#include "bar/props.h"
#include "looped/physics.h"

namespace {
engine::PhysicsWorld* world = nullptr;
float buffer[16];

engine::Quat quatAt(const float* b) {
  engine::Quat q{b[0], b[1], b[2], b[3]};
  const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (!(n > 0.5f)) return {};
  return {q.x / n, q.y / n, q.z / n, q.w / n};
}

bool valid(int i) { return world && i >= 0 && static_cast<size_t>(i) < world->bodyCount(); }
}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE float* sim_buffer() { return buffer; }

EMSCRIPTEN_KEEPALIVE int sim_init() {
  if (!world) {
    world = new engine::PhysicsWorld();
    app::BarScene scene = app::buildBarScene();
    std::vector<int> bodyOfModel;
    app::buildPhysics(scene, *world, bodyOfModel);
    // Let the glasses settle on their spots once (they start a hair above).
    for (int k = 0; k < 180; ++k) world->step(engine::PhysicsWorld::kStep);
  }
  return static_cast<int>(world->bodyCount());
}

EMSCRIPTEN_KEEPALIVE void sim_step(float dt) {
  if (world) world->step(dt);
}

EMSCRIPTEN_KEEPALIVE int sim_get(int i) {
  if (!valid(i)) return 0;
  const engine::Body& b = world->body(static_cast<size_t>(i));
  const float v[12] = {b.position.x, b.position.y, b.position.z, b.rotation.x, b.rotation.y, b.rotation.z,
                       b.rotation.w, b.velocity.x, b.velocity.y, b.velocity.z, b.asleep ? 1.0f : 0.0f,
                       b.held ? 1.0f : 0.0f};
  for (int k = 0; k < 12; ++k) buffer[k] = v[k];
  return 1;
}

EMSCRIPTEN_KEEPALIVE void sim_hold(int i) {
  if (!valid(i)) return;
  engine::Body& b = world->body(static_cast<size_t>(i));
  b.held = true;
  b.kinematic = false;
  b.asleep = false;
  b.sleepTimer = 0;
  b.holdTarget = {buffer[0], buffer[1], buffer[2]};
}

EMSCRIPTEN_KEEPALIVE void sim_release(int i) {
  if (!valid(i)) return;
  engine::Body& b = world->body(static_cast<size_t>(i));
  b.held = false;
  b.asleep = false;
  b.sleepTimer = 0;
  world->teleport(static_cast<size_t>(i), {buffer[0], buffer[1], buffer[2]}, quatAt(buffer + 3));
  b.velocity = {buffer[7], buffer[8], buffer[9]};
  b.angularVelocity = {};
}

EMSCRIPTEN_KEEPALIVE void sim_place(int i) {
  if (!valid(i)) return;
  engine::Body& b = world->body(static_cast<size_t>(i));
  b.held = false;
  world->teleport(static_cast<size_t>(i), {buffer[0], buffer[1], buffer[2]}, quatAt(buffer + 3));
  b.velocity = {};
  b.angularVelocity = {};
  b.asleep = true;
}

EMSCRIPTEN_KEEPALIVE int sim_awake() {
  if (!world) return 0;
  int n = 0;
  for (size_t i = 0; i < world->bodyCount(); ++i) {
    const engine::Body& b = world->body(i);
    if (!b.asleep || b.held) ++n;
  }
  return n;
}

}  // extern "C"
