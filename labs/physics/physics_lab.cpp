// The physics lab (J116.5; request 2026-10-27 #9, "settle and repair"):
// the light lab's two stools are bodies. Knock one over: while it moves it
// is lit live (its bounce and how much of each lamp it sees, worked out a
// little every frame from the room itself); once it comes to rest, only the
// light around where it was and where it is now is worked out again, a few
// milliseconds a frame (nothing stutters), and its own corners join the
// baked room. Everything else keeps the light baked once.
//   the page: labs/web/physics/index.html (?gl=1 forces WebGL2)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "labs/draw/room_view.h"
#include "labs/draw/scene_shader.h"
#include "looped/bake/light_paths.h"
#include "looped/draw/draw.h"
#include "looped/draw/half.h"
#include "looped/json.h"
#include "looped/light_units.h"
#include "looped/material.h"
#include "looped/physics.h"
#include "looped/scene_geometry.h"
#include "looped/world/loop.h"

using namespace engine;

namespace {

constexpr float kRepairRadius = 1.0f;  // metres around where a thing was and is: the light worked out again
constexpr double kBudgetMs = 4.0;      // light work a frame
constexpr float kEmissiveNits = 8;     // as the baker (bake::Room::emissiveNits)

enum class State { kBaked, kMoving, kRepairing, kSettled };
const char* stateName(State s) {
  switch (s) {
    case State::kBaked: return "baked";
    case State::kMoving: return "moving: lit live";
    case State::kRepairing: return "resting: working out the light around it again";
    case State::kSettled: return "settled: part of the baked room again";
  }
  return "";
}

struct Mover {
  std::string id;
  size_t body = 0;
  uint32_t firstTri = 0, triCount = 0;
  Vec3 p0;
  Quat r0;
  Vec3 center0;            // its middle, as baked
  Quat restTurn;           // the pose the room's light was last made for
  Vec3 restShift;
  State state = State::kBaked;
  std::vector<uint8_t> original;  // its vertices as baked
  // The live light: per way (+x -x +y -y +z -z), per channel, the bounce (lux at brightness 1); per lamp, how much it sees.
  std::vector<Vec3> cube;
  std::vector<float> seen;
  int samples = 0;
  // The repair.
  std::vector<uint32_t> dirty;  // texel numbers (in the repair list)
  size_t done = 0;
  size_t cornersDone = 0;
};

struct Lab {
  std::unique_ptr<draw::Device> dev;
  draw::Handle pipe = 0, vb = 0, ib = 0, atlas = 0;
  lab::BakedRoom room;
  std::vector<uint16_t> atlasBaked;  // the atlas as baked (put back)
  std::vector<world::Loop> loops;
  bool alive = true;
  // The light, for working it out again.
  SceneGeometry geo;
  bake::Room light;
  struct Texel {
    uint32_t at;  // y * width + x
    Vec3 pos, nrm;
  };
  std::vector<Texel> texels;
  PhysicsWorld physics;
  Material leather;
  std::vector<Mover> movers;
  bake::Rng rng{2026, 11};
  float yaw = 0, pitch = -12, exposure = 0.7f;
  Vec3 eye{0, 1.65f, 2.9f};
  bool dragging = false;
  double lastX = 0, lastY = 0, lastT = 0, fpsT0 = 0, worstMs = 0;
  int frames = 0;
  bool atlasDirty = false;
  bool ready = false;
};
Lab g;

EM_JS(void, js_status, (const char* backend, const char* problem), {
  if (window['physicsLabStatus']) window['physicsLabStatus'](UTF8ToString(backend), UTF8ToString(problem));
});
EM_JS(void, js_fps, (double fps, double worst), { if (window['physicsLabFps']) window['physicsLabFps'](fps, worst); });
EM_JS(void, js_stool, (int i, const char* state, double progress), {
  if (window['physicsLabStool']) window['physicsLabStool'](i, UTF8ToString(state), progress);
});
EM_JS(int, js_forceGl, (), { return /[?&]gl=1(&|$)/.test(location.search) ? 1 : 0; });

Vec3 vertexPos(const std::vector<uint8_t>& v, size_t i) {
  Vec3 p;
  std::memcpy(&p, &v[i * lab::kVertexBytes], 12);
  return p;
}

// ---- Loading ----

bool loadRepair(const uint8_t* data, size_t size) {
  if (size < 20 || std::memcmp(data, "LREP", 4) != 0) return false;
  const uint8_t* p = data + 4;
  const uint32_t tris = lab::take<uint32_t>(p), models = lab::take<uint32_t>(p), texels = lab::take<uint32_t>(p),
                 lamps = lab::take<uint32_t>(p);
  if (tris * 3 != g.room.vertexCount || lamps != g.room.lamps.size()) return false;
  // The room's surfaces for light: positions, a material per colour and glow.
  g.geo.positions.resize(g.room.vertexCount);
  g.geo.indices = g.room.indices;
  std::map<std::vector<uint8_t>, uint32_t> mats;
  g.geo.triMaterial.resize(tris);
  for (uint32_t t = 0; t < tris; ++t) {
    for (int k = 0; k < 3; ++k) g.geo.positions[3 * t + k] = vertexPos(g.room.vertices, 3 * t + k);
    const uint8_t* v = &g.room.vertices[3 * t * lab::kVertexBytes];
    std::vector<uint8_t> key(v + 32, v + 48);  // colour and glow
    key[3] = 0;
    auto it = mats.find(key);
    if (it == mats.end()) {
      GeoMaterial m;
      m.albedo = {v[32] / 255.0f, v[33] / 255.0f, v[34] / 255.0f};
      Vec3 glow;
      std::memcpy(&glow, v + 36, 12);
      m.emission = glow * (1 / kEmissiveNits);
      it = mats.emplace(key, static_cast<uint32_t>(g.geo.materials.size())).first;
      g.geo.materials.push_back(m);
    }
    g.geo.triMaterial[t] = it->second;
  }
  g.geo.triModel.resize(tris);
  for (uint32_t& m : g.geo.triModel) m = lab::take<uint32_t>(p);
  std::vector<bake::Lamp> lampList(lamps);
  for (uint32_t l = 0; l < lamps; ++l) {
    const lab::LampInfo& li = g.room.lamps[l];
    bake::Lamp& b = lampList[l];
    b.position = li.pos, b.color = li.color, b.cd = li.cd, b.radius = li.radius, b.shape = static_cast<int>(li.shape);
    b.axis = li.axis, b.axis2 = li.axis2, b.oneSided = li.oneSided > 0.5f, b.channel = li.channel;
    b.model = static_cast<int>(lab::take<uint32_t>(p));
  }
  std::vector<std::string> thingOf(models);
  for (uint32_t m = 0; m < models; ++m) {
    const uint16_t n = lab::take<uint16_t>(p);
    thingOf[m].assign(reinterpret_cast<const char*>(p), n);
    p += n;
  }
  g.texels.resize(texels);
  for (Lab::Texel& t : g.texels) {
    const uint16_t x = lab::take<uint16_t>(p), y = lab::take<uint16_t>(p);
    t.at = static_cast<uint32_t>(y) * static_cast<uint32_t>(g.room.atlasW) + x;
    for (int k = 0; k < 3; ++k) (&t.pos.x)[k] = lab::take<float>(p);
    for (int k = 0; k < 3; ++k) (&t.nrm.x)[k] = lab::take<float>(p);
  }
  // The stools: their triangles (one placement each: a run of triangles).
  std::vector<std::vector<uint32_t>> moverTris;
  for (const char* id : {"stool-1", "stool-2"}) {
    std::vector<uint32_t> tl;
    for (uint32_t t = 0; t < tris; ++t)
      if (thingOf[g.geo.triModel[t]] == id) tl.push_back(t);
    if (tl.empty()) continue;
    Mover m;
    m.id = id;
    m.firstTri = tl.front();
    m.triCount = static_cast<uint32_t>(tl.size());
    if (tl.back() - tl.front() + 1 != tl.size()) return false;  // not one run
    Vec3 lo{1e9f, 1e9f, 1e9f}, hi{-1e9f, -1e9f, -1e9f};
    for (uint32_t t : tl)
      for (int k = 0; k < 3; ++k) {
        const Vec3 q = g.geo.positions[3 * t + k];
        lo = minComponents(lo, q), hi = maxComponents(hi, q);
      }
    m.center0 = (lo + hi) * 0.5f;
    m.p0 = {m.center0.x, 0, m.center0.z};
    m.original.assign(g.room.vertices.begin() + static_cast<long>(m.firstTri) * 3 * lab::kVertexBytes,
                      g.room.vertices.begin() + static_cast<long>(m.firstTri + m.triCount) * 3 * lab::kVertexBytes);
    m.cube.assign(6 * static_cast<size_t>(g.room.channels), Vec3{});
    m.seen.assign(lamps, 0.0f);
    g.movers.push_back(std::move(m));
    moverTris.push_back(tl);
  }
  g.light.build(g.geo, lampList, moverTris);
  g.light.emissiveNits = kEmissiveNits;
  return true;
}

void buildPhysics() {
  g.leather.name = "leather";
  g.leather.density = 900;
  g.leather.friction = 0.7f;
  g.leather.restitution = 0.2f;
  static Material oak, steel;
  oak.density = 700, oak.friction = 0.5f, oak.restitution = 0.3f;
  steel.density = 7800, steel.friction = 0.4f, steel.restitution = 0.3f;
  auto box = [&](Vec3 c, Vec3 h, const Material* m) { g.physics.addStaticBox({c, h, Quat{}, m}); };
  box({0, -0.05f, 0}, {3.2f, 0.05f, 3.2f}, &oak);
  box({0, 3.05f, 0}, {3.2f, 0.05f, 3.2f}, &oak);
  box({0, 1.5f, -3.1f}, {3.2f, 1.6f, 0.1f}, &oak);
  box({0, 1.5f, 3.1f}, {3.2f, 1.6f, 0.1f}, &oak);
  box({-3.1f, 1.5f, 0}, {0.1f, 1.6f, 3.2f}, &oak);
  box({3.1f, 1.5f, 0}, {0.1f, 1.6f, 3.2f}, &oak);
  box({0, 0.7625f, 0}, {0.45f, 0.0225f, 0.45f}, &oak);  // the table's top, leg and foot
  box({0, 0.37f, 0}, {0.05f, 0.37f, 0.05f}, &steel);
  box({0, 0.02f, 0}, {0.3f, 0.02f, 0.3f}, &steel);
  g.physics.setBounds({{-3.2f, -0.5f, -3.2f}, {3.2f, 3.2f, 3.2f}});
  // A stool: a ring of spheres for the seat, four feet, four knees.
  std::vector<BodySphere> s;
  for (int k = 0; k < 6; ++k) {
    const float a = k * 1.0471976f;
    s.push_back({{0.13f * std::cos(a), 0.6f, 0.13f * std::sin(a)}, 0.05f});
  }
  for (int k = 0; k < 4; ++k) {
    const float x = (k & 1) ? 0.14f : -0.14f, z = (k & 2) ? 0.14f : -0.14f;
    s.push_back({{x, 0.025f, z}, 0.025f});
    s.push_back({{x * 0.8f, 0.3f, z * 0.8f}, 0.025f});
  }
  for (Mover& m : g.movers) {
    m.body = g.physics.addBody(m.p0, m.r0, s, 0.003f, &g.leather);
    m.restShift = {};
    m.restTurn = {};
  }
}

// ---- The light around things that move ----

// Where a mover's baked shape is now: now = turn · baked + shift.
void poseOf(const Mover& m, Quat* turn, Vec3* shift) {
  const Body& b = g.physics.body(m.body);
  *turn = normalized(b.shownRotation * conjugate(m.r0));
  *shift = b.shownPosition - turn->rotate(m.p0);
}

// A little more of the live light: the bounce from six ways and each lamp's visibility, averaged over time.
void sampleLive(Mover& m, size_t index) {
  const size_t C = static_cast<size_t>(g.room.channels);
  bake::Room::Mover& lm = g.light.movers[index];
  Quat turn;
  Vec3 shift;
  poseOf(m, &turn, &shift);
  const Vec3 c = turn.rotate(m.center0) + shift;
  static const Vec3 kWays[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  lm.present = false;  // it doesn't light itself
  const float a = m.samples < 30 ? 1.0f / static_cast<float>(m.samples + 1) : 0.08f;
  std::vector<Vec3> acc(C);
  for (int w = 0; w < 6; ++w) {
    std::fill(acc.begin(), acc.end(), Vec3{});
    const int paths = 3;
    for (int k = 0; k < paths; ++k) bake::incomingByChannel(g.light, c, kWays[w], g.rng, 3, 0, acc.data());
    for (size_t ch = 0; ch < C; ++ch) m.cube[w * C + ch] = m.cube[w * C + ch] * (1 - a) + acc[ch] * (a / paths);
  }
  for (size_t l = 0; l < g.light.lamps.size(); ++l) {
    const bake::Lamp& lamp = g.light.lamps[l];
    int clear = 0;
    const int rays = 3;
    for (int k = 0; k < rays; ++k) {
      Vec3 q = lamp.position + lamp.axis * (2 * g.rng.uniform() - 1) + lamp.axis2 * (2 * g.rng.uniform() - 1);
      clear += g.light.clear(c, q, lamp) ? 1 : 0;
    }
    m.seen[l] = m.seen[l] * (1 - a) + a * static_cast<float>(clear) / rays;
  }
  lm.present = true;
  ++m.samples;
}

// The lamps' own light on a texel, channel by channel (lux at brightness 1), with the same random points each call.
void directAt(const Lab::Texel& t, uint64_t seed, std::vector<Vec3>& out) {
  std::fill(out.begin(), out.end(), Vec3{});
  bake::Rng rng(seed, 3);
  const Vec3 p = t.pos + t.nrm * 0.002f;
  for (const bake::Lamp& l : g.light.lamps) {
    Vec3 e;
    const int rays = 12;
    for (int k = 0; k < rays; ++k) e = e + bake::lampLux(g.light, l, p, t.nrm, rng, true);
    out[static_cast<size_t>(l.channel)] = out[static_cast<size_t>(l.channel)] + e * (1.0f / rays);
  }
}

void startRepair(Mover& m, size_t index) {
  Quat turn;
  Vec3 shift;
  poseOf(m, &turn, &shift);
  const Vec3 was = m.restTurn.rotate(m.center0) + m.restShift, is = turn.rotate(m.center0) + shift;
  m.dirty.clear();
  for (uint32_t i = 0; i < g.texels.size(); ++i) {
    const Vec3 p = g.texels[i].pos;
    if (length(p - was) < kRepairRadius || length(p - is) < kRepairRadius) m.dirty.push_back(i);
  }
  m.done = 0;
  m.cornersDone = 0;
  m.state = State::kRepairing;
  g.light.movers[index].turn = turn;
  g.light.movers[index].shift = shift;
}

// Some of the repair, within the frame's budget: the room's texels near the
// thing (the lamps' light with the thing where it was and where it is now;
// the difference goes into the baked numbers), then its own corners.
bool repairSome(Mover& m, size_t index, double until) {
  const size_t C = static_cast<size_t>(g.room.channels), dims = 3 * C;
  const int K = std::min(g.room.layers, 4);
  bake::Room::Mover& lm = g.light.movers[index];
  const Quat nowTurn = lm.turn;
  const Vec3 nowShift = lm.shift;
  std::vector<Vec3> before(C), after(C);
  while (m.done < m.dirty.size()) {
    if (emscripten_get_now() > until) return false;
    const Lab::Texel& t = g.texels[m.dirty[m.done]];
    const uint64_t seed = 0x9E37ull * (m.dirty[m.done] + 1);
    lm.turn = m.restTurn, lm.shift = m.restShift;
    directAt(t, seed, before);
    lm.turn = nowTurn, lm.shift = nowShift;
    directAt(t, seed, after);
    for (int k = 0; k < K; ++k) {
      double du = 0;
      for (size_t c = 0; c < C; ++c) {
        const Vec3 d = after[c] - before[c];
        const float* b = &g.room.basis[static_cast<size_t>(k) * dims + 3 * c];
        du += b[0] * d.x + b[1] * d.y + b[2] * d.z;
      }
      uint16_t& h = g.room.atlas[static_cast<size_t>(t.at) * 4 + static_cast<size_t>(k)];
      h = draw::toHalf(draw::fromHalf(h) + static_cast<float>(du));
    }
    ++m.done;
    g.atlasDirty = true;
  }
  // Its own corners: where they are now, the bounce from its live light, its four strongest lamps and how much of each they see.
  const size_t nl = g.light.lamps.size();
  std::vector<uint8_t>& vtx = g.room.vertices;
  while (m.cornersDone < static_cast<size_t>(m.triCount)) {
    if (emscripten_get_now() > until) return false;
    const uint32_t t = m.firstTri + static_cast<uint32_t>(m.cornersDone);
    const uint8_t* orig = &m.original[m.cornersDone * 3 * lab::kVertexBytes];
    Vec3 n0;
    std::memcpy(&n0, orig + 12, 12);
    const Vec3 n = normalize(nowTurn.rotate(n0));
    int way[3] = {n.x >= 0 ? 0 : 1, n.y >= 0 ? 2 : 3, n.z >= 0 ? 4 : 5};
    const float ww[3] = {n.x * n.x, n.y * n.y, n.z * n.z};
    for (int c3 = 0; c3 < 3; ++c3) {
      uint8_t* v = &vtx[(3 * static_cast<size_t>(t) + c3) * lab::kVertexBytes];
      std::memcpy(v, orig + c3 * lab::kVertexBytes, lab::kVertexBytes);
      Vec3 p0;
      std::memcpy(&p0, v, 12);
      const Vec3 p = nowTurn.rotate(p0) + nowShift;
      std::memcpy(v, &p, 12);
      std::memcpy(v + 12, &n, 12);
      v[35] = 0;  // part of the room again (alpha 0: lit at its corners)
      // Bounce, in the shared layers.
      float layer[4] = {0, 0, 0, 0};
      for (int k = 0; k < K; ++k) {
        double u = 0;
        for (size_t c = 0; c < C; ++c) {
          Vec3 e;
          for (int j = 0; j < 3; ++j) e = e + m.cube[static_cast<size_t>(way[j]) * C + c] * ww[j];
          const float* b = &g.room.basis[static_cast<size_t>(k) * dims + 3 * c];
          u += b[0] * e.x + b[1] * e.y + b[2] * e.z;
        }
        layer[k] = static_cast<float>(u);
      }
      std::memcpy(v + 48, layer, 16);
      // Four lamps: by how much light they could give here; how much of each this corner sees.
      bake::Rng rng(0xC0DEull * (3 * t + c3 + 1), 5);
      const Vec3 at = p + n * 0.002f;
      std::vector<std::pair<float, size_t>> give;
      for (size_t l = 0; l < nl; ++l) {
        Vec3 e;
        for (int k = 0; k < 2; ++k) e = e + bake::lampLux(g.light, g.light.lamps[l], at, n, rng, false);
        give.push_back({light::luminance(e), l});
      }
      std::sort(give.rbegin(), give.rend());
      for (int k = 0; k < 4; ++k) {
        const bool has = static_cast<size_t>(k) < give.size() && give[static_cast<size_t>(k)].first > 0;
        const size_t l = has ? give[static_cast<size_t>(k)].second : 0;
        int clear = 0, facing = 0;
        if (has)
          for (int r = 0; r < 12; ++r) {
            facing += light::luminance(bake::lampLux(g.light, g.light.lamps[l], at, n, rng, false)) > 0;
            clear += light::luminance(bake::lampLux(g.light, g.light.lamps[l], at, n, rng, true)) > 0;
          }
        v[64 + k] = static_cast<uint8_t>(l);
        v[68 + k] = has && facing ? static_cast<uint8_t>(std::lround(255.0 * clear / facing)) : 0;
      }
    }
    ++m.cornersDone;
  }
  g.dev->updateVertices(g.vb, static_cast<size_t>(m.firstTri) * 3 * lab::kVertexBytes, &vtx[static_cast<size_t>(m.firstTri) * 3 * lab::kVertexBytes],
                        static_cast<size_t>(m.triCount) * 3 * lab::kVertexBytes);
  m.restTurn = nowTurn;
  m.restShift = nowShift;
  m.state = State::kSettled;
  return true;
}

// Knocking it over: its vertices go back to the baked shape, numbered so the shader moves them.
void knock(size_t i, Vec3 push) {
  if (i >= g.movers.size()) return;
  Mover& m = g.movers[i];
  if (m.state == State::kRepairing) return;  // finish settling first
  std::vector<uint8_t> v = m.original;
  for (size_t k = 0; k < v.size(); k += lab::kVertexBytes) v[k + 35] = static_cast<uint8_t>(i + 1);
  g.dev->updateVertices(g.vb, static_cast<size_t>(m.firstTri) * 3 * lab::kVertexBytes, v.data(), v.size());
  Body& b = g.physics.body(m.body);
  b.asleep = false;
  b.sleepTimer = 0;
  b.velocity = b.velocity + push;
  b.angularVelocity = b.angularVelocity + Vec3{push.z * 4, 0.5f, -push.x * 4};
  m.state = State::kMoving;
  m.samples = 0;
}

void putBack() {
  for (size_t i = 0; i < g.movers.size(); ++i) {
    Mover& m = g.movers[i];
    g.physics.teleport(m.body, m.p0, m.r0);
    g.physics.body(m.body).velocity = {};
    g.physics.body(m.body).angularVelocity = {};
    std::memcpy(&g.room.vertices[static_cast<size_t>(m.firstTri) * 3 * lab::kVertexBytes], m.original.data(), m.original.size());
    g.dev->updateVertices(g.vb, static_cast<size_t>(m.firstTri) * 3 * lab::kVertexBytes, m.original.data(), m.original.size());
    m.state = State::kBaked;
    m.restTurn = {};
    m.restShift = {};
    g.light.movers[i].turn = {};
    g.light.movers[i].shift = {};
  }
  g.room.atlas = g.atlasBaked;
  g.atlasDirty = true;
}

// ---- The frame ----

void frame() {
  if (!g.ready) return;
  const double t0 = emscripten_get_now();
  uint32_t w = 1, h = 1;
  g.dev->resize(&w, &h);
  const double now = t0 / 1000.0;
  const float dt = g.lastT > 0 ? static_cast<float>(std::min(0.1, now - g.lastT)) : 1.0f / 60;
  g.lastT = now;
  g.physics.step(dt);
  // The lights' brightness now.
  std::vector<float> s(static_cast<size_t>(g.room.channels), 1.0f);
  world::Clock clock;
  clock.seconds = now;
  for (int c = 0; c < g.room.channels; ++c) {
    const world::Loop& L = g.loops[static_cast<size_t>(c)];
    float v = 1;
    if (g.alive && L.live()) {
      world::Motion mo;
      const uint32_t seed = world::hash32(g.room.ids[static_cast<size_t>(c)]);
      L.add(L.cycles(clock, 0, seed), seed, 1, mo);
      v += mo.ch[world::kLight];
    }
    s[static_cast<size_t>(c)] = std::max(0.0f, v);
  }
  // The movers: live light while moving; when one rests, the repair.
  const double until = t0 + kBudgetMs;
  for (size_t i = 0; i < g.movers.size(); ++i) {
    Mover& m = g.movers[i];
    if (m.state == State::kBaked || m.state == State::kSettled) {  // bumped by the other one: moving too
      Quat turn;
      Vec3 shift;
      poseOf(m, &turn, &shift);
      const Vec3 was = m.restTurn.rotate(m.center0) + m.restShift, is = turn.rotate(m.center0) + shift;
      if (!g.physics.body(m.body).asleep && (length(is - was) > 0.02f || std::fabs(dot(turn, m.restTurn)) < 0.9997f)) knock(i, {});
    }
    if (m.state == State::kMoving) {
      Quat turn;
      Vec3 shift;
      poseOf(m, &turn, &shift);
      g.light.movers[i].turn = turn;
      g.light.movers[i].shift = shift;
      sampleLive(m, i);
      if (g.physics.body(m.body).asleep && m.samples > 20) startRepair(m, i);
    } else if (m.state == State::kRepairing) {
      if (m.samples < 200) sampleLive(m, i);
      repairSome(m, i, until);
    }
    const double progress = m.state == State::kRepairing
                                ? static_cast<double>(m.done + m.cornersDone) / static_cast<double>(std::max<size_t>(1, m.dirty.size() + m.triCount))
                                : (m.state == State::kSettled ? 1.0 : 0.0);
    js_stool(static_cast<int>(i), stateName(m.state), progress);
  }
  if (g.atlasDirty) {
    g.dev->updateTexture(g.atlas, g.room.atlasW, g.room.atlasH, draw::Format::kRgba16f, g.room.atlas.data());
    g.atlasDirty = false;
  }
  lab::SceneUniforms u{};
  lab::camera(u, g.eye, g.yaw, g.pitch, static_cast<float>(w) / static_cast<float>(h), g.dev->backend() == draw::Backend::kWebGpu, g.exposure);
  lab::lights(u, g.room, s);
  const size_t C = static_cast<size_t>(g.room.channels);
  for (size_t i = 0; i < g.movers.size() && i < 2; ++i) {
    const Mover& m = g.movers[i];
    if (m.state != State::kMoving && m.state != State::kRepairing) continue;
    Quat turn;
    Vec3 shift;
    poseOf(m, &turn, &shift);
    u.objTurn[i][0] = turn.x, u.objTurn[i][1] = turn.y, u.objTurn[i][2] = turn.z, u.objTurn[i][3] = turn.w;
    u.objShift[i][0] = shift.x, u.objShift[i][1] = shift.y, u.objShift[i][2] = shift.z, u.objShift[i][3] = 1;
    for (int wy = 0; wy < 6; ++wy) {
      Vec3 e;
      for (size_t c = 0; c < C; ++c) e = e + m.cube[static_cast<size_t>(wy) * C + c] * s[c];
      u.objCube[i * 6 + static_cast<size_t>(wy)][0] = e.x, u.objCube[i * 6 + static_cast<size_t>(wy)][1] = e.y, u.objCube[i * 6 + static_cast<size_t>(wy)][2] = e.z;
    }
    for (size_t l = 0; l < m.seen.size() && l < 16; ++l) u.objSeen[i * 4 + l / 4][l % 4] = m.seen[l];
  }
  const float clear[4] = {0.02f, 0.02f, 0.025f, 1};
  if (g.dev->begin(clear)) {
    g.dev->drawIndexed(g.pipe, g.vb, g.ib, static_cast<uint32_t>(g.room.indices.size()), &u, sizeof u, &g.atlas, 1);
    g.dev->end();
  }
  const double took = emscripten_get_now() - t0;
  g.worstMs = std::max(g.worstMs, took);
  ++g.frames;
  if (now - g.fpsT0 > 1) {
    js_fps(g.frames / (now - g.fpsT0), g.worstMs);
    g.frames = 0;
    g.fpsT0 = now;
    g.worstMs = 0;
  }
}

void loadPlace(const char* text, int size) {
  std::string err;
  const Json j = Json::parse(std::string_view(text, static_cast<size_t>(size)), &err);
  for (const Json& t : j["things"].items())
    for (int c = 0; c < g.room.channels; ++c)
      if (g.room.ids[static_cast<size_t>(c)] == t["id"].string())
        for (const auto& [name, lj] : t["loops"].members()) g.loops[static_cast<size_t>(c)] = world::parseLoop(lj, name, &err);
}

void fail(const char* why) { js_status(draw::backendName(g.dev->backend()), why); }

void start(std::unique_ptr<draw::Device> dev) {
  if (!dev) {
    js_status("none", "This browser can draw with neither WebGPU nor WebGL2.");
    return;
  }
  g.dev = std::move(dev);
  draw::PipelineDesc pd;
  pd.label = "scene";
  pd.wgsl = lab::kSceneWgsl;
  pd.glsl = lab::kSceneGlsl;
  pd.textures = 1;
  pd.layout.stride = 72;
  pd.layout.items = {{draw::Attr::kFloat3, 0},    {draw::Attr::kFloat3, 12}, {draw::Attr::kFloat2, 24}, {draw::Attr::kUnorm8x4, 32},
                     {draw::Attr::kFloat3, 36},   {draw::Attr::kFloat4, 48}, {draw::Attr::kUint8x4, 64}, {draw::Attr::kUnorm8x4, 68}};
  g.pipe = g.dev->pipeline(pd);
  js_status(draw::backendName(g.dev->backend()), g.dev->problem.c_str());
  emscripten_async_wget_data(
      "../draw/data/scene.bin", nullptr,
      [](void*, void* buf, int size) {
        if (!lab::parseBakedRoom(static_cast<const uint8_t*>(buf), static_cast<size_t>(size), g.room)) return fail("The baked room didn't load.");
        g.vb = g.dev->vertexBuffer(g.room.vertices.data(), g.room.vertices.size());
        g.ib = g.dev->indexBuffer(g.room.indices.data(), g.room.indices.size());
        g.atlas = g.dev->texture(g.room.atlasW, g.room.atlasH, draw::Format::kRgba16f, g.room.atlas.data());
        g.atlasBaked = g.room.atlas;
        g.loops.resize(static_cast<size_t>(g.room.channels));
        emscripten_async_wget_data(
            "../draw/data/repair.bin", nullptr,
            [](void*, void* b2, int s2) {
              if (!loadRepair(static_cast<const uint8_t*>(b2), static_cast<size_t>(s2))) return fail("The room's repair data didn't load.");
              buildPhysics();
              emscripten_async_wget_data(
                  "../../assets/world/places/lab.json", nullptr,
                  [](void*, void* b3, int s3) {
                    loadPlace(static_cast<const char*>(b3), s3);
                    g.ready = true;
                  },
                  [](void*) { g.ready = true; });
            },
            [](void*) { fail("The room's repair data didn't load."); });
      },
      [](void*) { fail("The baked room didn't load."); });
}

EM_BOOL onDown(int, const EmscriptenMouseEvent* e, void*) {
  g.dragging = true, g.lastX = e->targetX, g.lastY = e->targetY;
  return EM_TRUE;
}
EM_BOOL onMove(int, const EmscriptenMouseEvent* e, void*) {
  if (!g.dragging) return EM_FALSE;
  g.yaw += static_cast<float>(e->targetX - g.lastX) * 0.25f;
  g.pitch = std::clamp(g.pitch - static_cast<float>(e->targetY - g.lastY) * 0.25f, -80.0f, 80.0f);
  g.lastX = e->targetX, g.lastY = e->targetY;
  return EM_TRUE;
}
EM_BOOL onUp(int, const EmscriptenMouseEvent*, void*) {
  g.dragging = false;
  return EM_TRUE;
}
EM_BOOL onTouch(int type, const EmscriptenTouchEvent* e, void*) {
  if (e->numTouches < 1) return EM_FALSE;
  const auto& t = e->touches[0];
  if (type == EMSCRIPTEN_EVENT_TOUCHSTART) g.dragging = true, g.lastX = t.targetX, g.lastY = t.targetY;
  else if (type == EMSCRIPTEN_EVENT_TOUCHMOVE && g.dragging) {
    g.yaw += static_cast<float>(t.targetX - g.lastX) * 0.3f;
    g.pitch = std::clamp(g.pitch - static_cast<float>(t.targetY - g.lastY) * 0.3f, -80.0f, 80.0f);
    g.lastX = t.targetX, g.lastY = t.targetY;
  } else if (type == EMSCRIPTEN_EVENT_TOUCHEND) g.dragging = false;
  return EM_TRUE;
}

}  // namespace

extern "C" {
// Knocks stool i (0 or 1) over, away from where you stand.
EMSCRIPTEN_KEEPALIVE void pl_knock(int i) {
  if (!g.ready || i < 0 || static_cast<size_t>(i) >= g.movers.size()) return;
  const Vec3 c = g.physics.body(g.movers[static_cast<size_t>(i)].body).shownPosition;
  Vec3 away = c - g.eye;
  away.y = 0;
  away = length(away) > 1e-3f ? normalize(away) : Vec3{0, 0, -1};
  knock(static_cast<size_t>(i), away * 1.6f + Vec3{0, 0.9f, 0});
}
EMSCRIPTEN_KEEPALIVE void pl_putBack() {
  if (g.ready) putBack();
}
EMSCRIPTEN_KEEPALIVE void pl_alive(int on) { g.alive = on != 0; }
EMSCRIPTEN_KEEPALIVE void pl_spot(float x, float y, float z, float yaw, float pitch) {
  g.eye = {x, y, z}, g.yaw = yaw, g.pitch = pitch;
}
// For the test: 0 baked, 1 moving, 2 repairing, 3 settled.
EMSCRIPTEN_KEEPALIVE int pl_state(int i) {
  return i >= 0 && static_cast<size_t>(i) < g.movers.size() ? static_cast<int>(g.movers[static_cast<size_t>(i)].state) : -1;
}
EMSCRIPTEN_KEEPALIVE int pl_ready() { return g.ready ? 1 : 0; }
}

int main() {
  emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, onDown);
  emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onMove);
  emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onUp);
  emscripten_set_touchstart_callback("#canvas", nullptr, EM_TRUE, onTouch);
  emscripten_set_touchmove_callback("#canvas", nullptr, EM_TRUE, onTouch);
  emscripten_set_touchend_callback("#canvas", nullptr, EM_TRUE, onTouch);
  draw::create("#canvas", js_forceGl() != 0, start);
  emscripten_set_main_loop(frame, 0, false);
  return 0;
}
