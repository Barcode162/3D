// The bar's physics for the room (cloud/room.js): the same C++ the phones
// run (looped/physics + the bar's layout from bar/bar_scene + bar/props),
// compiled to a small WebAssembly module the Cloudflare Worker loads
// (cloud/sim/build.sh → cloud/sim/room_sim.wasm). The room simulates every
// glass: held ones follow their holder's hand, thrown and knocked ones fly,
// bounce and settle exactly once, for everyone.
//
// Calls (all numbers go through one float buffer, sim_buffer()):
//   sim_text(n) → a place for n bytes: "path\0text" of a world file
//   sim_file()              takes that file (the bar's place and kinds,
//                           assets/world/: sim.js hands both over first)
//   sim_init() → number of bodies (glasses), at their layout spots
//   sim_step(dt)            advance dt seconds (fixed 60 Hz steps inside)
//   sim_get(i) → 1          buffer = pos x,y,z, quat x,y,z,w, vel x,y,z, asleep, held
//   sim_hold(i)             buffer[0..2] = hand position: pulled there, no gravity
//   sim_release(i)          buffer = pos x,y,z, quat x,y,z,w, vel x,y,z: let go there
//   sim_place(i)            buffer = pos x,y,z, quat x,y,z,w: put down at rest
//   sim_awake() → count of bodies moving or held
//
// Things (docs/ENGINE.md E3): the bar's world, the same rules the phones run.
//   sim_dbuffer() → 96 doubles for results that need them (times)
//   sim_thing_event(net, nowS) the event named in sim_text (e.g. "tap") for
//                           thing `net` → number of changes; dbuffer = net,
//                           state, start for each (max 32)
//   sim_thing_apply(net, state, start)   a stored change (waking up)
//   sim_thing_at(net, nowS) → 1   buffer = where it is now (x, y, z)
//   sim_thing_net() → net   of the thing whose id is in sim_text (0 = none)
#include <emscripten/emscripten.h>

#include <string>
#include <vector>

#include "bar/bar_scene.h"
#include "bar/world_files.h"
#include "bar/props.h"
#include "looped/physics.h"
#include "looped/world/world.h"

namespace {
engine::PhysicsWorld* world = nullptr;
engine::world::World* things = nullptr;  // the bar's things (the scene's world, our own copy)
double dbuffer[96];
float buffer[16];
std::string text;  // a world file on its way in (sim_text → sim_file)

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

EMSCRIPTEN_KEEPALIVE char* sim_text(int n) {
  text.assign(static_cast<size_t>(n > 0 ? n : 0), '\0');
  return text.data();
}

EMSCRIPTEN_KEEPALIVE void sim_file() {
  const size_t cut = text.find('\0');
  if (cut != std::string::npos) app::setWorldFile(text.substr(0, cut), text.substr(cut + 1));
}

EMSCRIPTEN_KEEPALIVE double* sim_dbuffer() { return dbuffer; }

EMSCRIPTEN_KEEPALIVE int sim_thing_event(int net, double now) {
  if (!things) return 0;
  const int i = things->findNet(static_cast<uint16_t>(net));
  if (i < 0) return 0;
  engine::world::Clock clock;
  clock.seconds = now;
  things->update(clock);  // timed states up to now
  things->takeChanges();
  if (!things->send(i, text, now)) return 0;
  const std::vector<engine::world::Change> changes = things->takeChanges();
  int n = 0;
  for (const engine::world::Change& c : changes) {
    if (n >= 32) break;
    dbuffer[n * 3] = c.net, dbuffer[n * 3 + 1] = c.state, dbuffer[n * 3 + 2] = c.start;
    ++n;
  }
  return n;
}

EMSCRIPTEN_KEEPALIVE void sim_thing_apply(int net, int state, double start) {
  if (things && state >= 0 && state < 256) things->apply({static_cast<uint16_t>(net), static_cast<uint8_t>(state), start});
}

EMSCRIPTEN_KEEPALIVE int sim_thing_at(int net, double now) {
  if (!things) return 0;
  const int i = things->findNet(static_cast<uint16_t>(net));
  if (i < 0) return 0;
  engine::world::Clock clock;
  clock.seconds = now;
  things->update(clock);  // places worked out (the world starts unplaced)
  // Where it really is: models made in place sit at the world's origin (the
  // fire, the mirror), so use its pivot, else the middle of its tap spots,
  // else of its solid shapes, else its origin.
  const engine::world::Thing& t = things->thing(i);
  engine::Vec3 local = t.kind->pivot;
  auto middle = [&](const char* part, const char* key) {
    const engine::Json& list = things->part(i, part);
    if (list.size() == 0) return false;
    engine::Vec3 sum;
    for (const engine::Json& b : list.items()) sum = sum + engine::world::readVec3(b[key], {});
    local = sum * (1.0f / static_cast<float>(list.size()));
    return true;
  };
  if (local.x == 0 && local.y == 0 && local.z == 0 && !middle("uses", "tag")) middle("solids", "at");
  const engine::Vec4 p = engine::transformPoint(t.world, local);
  buffer[0] = p.x, buffer[1] = p.y, buffer[2] = p.z;
  return 1;
}

EMSCRIPTEN_KEEPALIVE int sim_thing_net() {
  if (!things) return 0;
  const int i = things->find(text);
  return i < 0 ? 0 : things->thing(i).net;
}

EMSCRIPTEN_KEEPALIVE int sim_init() {
  if (!world) {
    world = new engine::PhysicsWorld();
    // The place this room is (a world file "place" from sim.js: "bar", "home").
    const std::string place = app::worldFile("place");
    app::BarScene scene = app::buildBarScene(place.empty() ? "bar" : place);
    std::vector<int> bodyOfModel;
    app::buildPhysics(scene, *world, bodyOfModel);
    if (scene.world) things = new engine::world::World(*scene.world);
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
