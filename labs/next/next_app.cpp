// The bar on the new engine (J116.6): the baked bar (tools/native/bake_tool.cpp
// writePlace) drawn by the new skeleton's drawing piece (looped/draw/) on
// WebGPU or WebGL2, each part with the detail its distance needs (its
// clusters, looped/draw/clusters.h), lit by the one formula, and walked
// through with the thumbs. Today's bar is untouched; this is its preview.
// The lights are alive (J118.2): the bar's rig (assets/world/rigs/bar.looped,
// looped/words/rig.h) runs each frame and sets each light channel's
// brightness and colour; the page sends it what happens (the lights going
// down, the music).
//   the page: labs/web/next/index.html (?gl=1 forces WebGL2)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "labs/draw/room_view.h"
#include "labs/next/next_shader.h"
#include "looped/draw/clusters.h"
#include "looped/draw/draw.h"
#include "looped/draw/half.h"
#include "looped/json.h"
#include "looped/math.h"
#include "looped/words/rig.h"
#include "looped/world/loop.h"

using namespace engine;

namespace {

constexpr size_t kVertex = 44;
constexpr float kEye = 1.6f, kRadius = 0.25f;

struct Uniforms {
  float viewProj[16];
  float eye[4];
  float w[4][4];
  float group[4];
  float picAvg[4];
  float glow[4];
};
static_assert(sizeof(Uniforms) == 192, "std140");

struct Group {
  int picture = -1;
  Vec3 glow;
  uint32_t vertexCount = 0;
  draw::ClusterTree tree;
  draw::Handle vb = 0, ib = 0;
  uint32_t drawn = 0;  // indices in the buffer now
};

struct Lamp {
  Vec3 pos, color, axis, axis2;
  float cd = 0, radius = 0, shape = 0, oneSided = 0;
  int channel = 0;
};

struct Blocker {
  float minX, minZ, maxX, maxZ;
};
struct Floor {
  float minX, minZ, maxX, maxZ, height;
};

struct App {
  std::unique_ptr<draw::Device> dev;
  draw::Handle pipe = 0, atlas = 0, lampTex = 0, white = 0;
  std::vector<Group> groups;
  std::vector<draw::Handle> pictures;
  std::vector<Vec3> pictureAvg;
  int layers = 0, channels = 0;
  std::vector<float> basis;
  std::vector<Lamp> lamps;
  std::vector<std::string> channelNames;
  std::vector<Blocker> blockers;
  std::vector<Floor> floors;
  Vec3 eye{0, kEye, 0};
  float yaw = 0, pitch = -5, exposure = 0.6f;
  float moveX = 0, moveZ = 0;  // the left thumb (-1..1)
  Vec3 pickedAt{1e9f, 0, 0};
  float pickedYaw = 1e9f;
  float limitPixels = 1;
  double lastT = 0, fpsT0 = 0;
  int frames = 0;
  uint64_t trianglesDrawn = 0, trianglesAll = 0;
  bool ready = false;
  std::vector<uint32_t> scratch;
  std::vector<uint32_t> picked;
  // The rig (J118.2).
  words::Rig rig;
  std::vector<int> rigChannel;  // the place's channel k → the rig's (-1: steady)
  bool rigMapped = false;
  world::Clock clock;
  float bpm = 120;
  std::vector<Vec3> level;  // each place channel's colour × brightness this frame
};
App g;

EM_JS(void, js_status, (const char* backend, const char* problem), {
  if (window['nextStatus']) window['nextStatus'](UTF8ToString(backend), UTF8ToString(problem));
});
EM_JS(void, js_stats, (double fps, double drawn, double all), { if (window['nextStats']) window['nextStats'](fps, drawn, all); });
EM_JS(void, js_loadPictures, (int count), { if (window['nextLoadPictures']) window['nextLoadPictures'](count); });
EM_JS(void, js_fetchPlace, (), { if (window['nextFetchPlace']) window['nextFetchPlace'](); });
EM_JS(int, js_forceGl, (), { return /[?&]gl=1(&|$)/.test(location.search) ? 1 : 0; });

template <class T>
T take(const uint8_t*& p) {
  T v;
  std::memcpy(&v, p, sizeof v);
  p += sizeof v;
  return v;
}

bool loadPlace(const uint8_t* data, size_t size) {
  if (size < 36 || std::memcmp(data, "LNX1", 4) != 0) return false;
  const uint8_t* p = data + 4;
  take<uint32_t>(p);
  const uint32_t ng = take<uint32_t>(p), nl = take<uint32_t>(p);
  g.layers = static_cast<int>(take<uint32_t>(p));
  g.channels = static_cast<int>(take<uint32_t>(p));
  const uint32_t aw = take<uint32_t>(p), ah = take<uint32_t>(p), npic = take<uint32_t>(p);
  for (uint32_t k = 0; k < ng; ++k) {
    Group gr;
    gr.picture = static_cast<int>(take<uint32_t>(p));
    gr.glow = {take<float>(p), take<float>(p), take<float>(p)};
    gr.vertexCount = take<uint32_t>(p);
    const uint32_t ni = take<uint32_t>(p), nc = take<uint32_t>(p);
    gr.vb = g.dev->vertexBuffer(p, static_cast<size_t>(gr.vertexCount) * kVertex);
    p += static_cast<size_t>(gr.vertexCount) * kVertex;
    gr.tree.indices.resize(ni);
    std::memcpy(gr.tree.indices.data(), p, ni * 4ull);
    p += ni * 4ull;
    gr.tree.clusters.resize(nc);
    for (draw::Cluster& c : gr.tree.clusters) {
      c.first = take<uint32_t>(p);
      c.count = take<uint32_t>(p);
      c.center = {take<float>(p), take<float>(p), take<float>(p)};
      c.radius = take<float>(p);
      c.error = take<float>(p);
      c.parentCenter = {take<float>(p), take<float>(p), take<float>(p)};
      c.parentRadius = take<float>(p);
      c.parentError = take<float>(p);
      if (c.error <= 0) g.trianglesAll += c.count;
    }
    gr.ib = g.dev->indexBuffer(gr.tree.indices.data(), gr.tree.indices.size());  // sized for the most; refilled with the picked
    g.groups.push_back(std::move(gr));
  }
  g.atlas = g.dev->texture(static_cast<int>(aw), static_cast<int>(ah), draw::Format::kRgba16f, p);
  p += static_cast<size_t>(aw) * ah * 8;
  g.basis.resize(static_cast<size_t>(g.layers) * 3 * g.channels);
  for (float& b : g.basis) b = take<float>(p);
  g.lamps.resize(nl);
  for (Lamp& l : g.lamps) {
    float f[18];
    for (float& x : f) x = take<float>(p);
    l.pos = {f[0], f[1], f[2]}, l.color = {f[3], f[4], f[5]}, l.cd = f[6], l.radius = f[7], l.shape = f[8];
    l.axis = {f[9], f[10], f[11]}, l.axis2 = {f[12], f[13], f[14]}, l.oneSided = f[15], l.channel = static_cast<int>(f[16]);
  }
  for (int c = 0; c < g.channels; ++c) {
    const uint16_t n = take<uint16_t>(p);
    g.channelNames.emplace_back(reinterpret_cast<const char*>(p), n);
    p += n;
  }
  const float sx = take<float>(p);
  take<float>(p);  // (the spawn's height: the eye is at kEye)
  const float sz = take<float>(p);
  g.eye = {sx, kEye, sz};
  const float spawnYaw = take<float>(p);
  g.yaw = spawnYaw * 57.29578f;
  const uint32_t nb = take<uint32_t>(p);
  for (uint32_t k = 0; k < nb; ++k) g.blockers.push_back({take<float>(p), take<float>(p), take<float>(p), take<float>(p)});
  const uint32_t nf = take<uint32_t>(p);
  for (uint32_t k = 0; k < nf; ++k) g.floors.push_back({take<float>(p), take<float>(p), take<float>(p), take<float>(p), take<float>(p)});
  g.pictures.assign(npic, 0);
  g.pictureAvg.assign(npic, Vec3{0.5f, 0.5f, 0.5f});
  const uint8_t w[4] = {255, 255, 255, 255};
  g.white = g.dev->texture(1, 1, draw::Format::kRgba8, w);
  std::vector<uint16_t> lt(4 * 4 * 64, 0);
  g.lampTex = g.dev->texture(4, 64, draw::Format::kRgba16f, lt.data());
  js_loadPictures(static_cast<int>(npic));
  return static_cast<size_t>(p - data) <= size;
}

// The walker pushed out of anything in the way (as bar/bar_scene.cpp resolveCollisions).
Vec3 resolve(Vec3 p) {
  for (int pass = 0; pass < 2; ++pass)
    for (const Blocker& b : g.blockers) {
      const float cx = std::clamp(p.x, b.minX, b.maxX), cz = std::clamp(p.z, b.minZ, b.maxZ);
      const float dx = p.x - cx, dz = p.z - cz, d2 = dx * dx + dz * dz;
      if (d2 >= kRadius * kRadius) continue;
      if (d2 > 1e-8f) {
        const float d = std::sqrt(d2);
        p.x = cx + dx / d * kRadius, p.z = cz + dz / d * kRadius;
      } else {
        const float pushes[4] = {p.x - b.minX, b.maxX - p.x, p.z - b.minZ, b.maxZ - p.z};
        const int side = static_cast<int>(std::min_element(pushes, pushes + 4) - pushes);
        if (side == 0) p.x = b.minX - kRadius;
        if (side == 1) p.x = b.maxX + kRadius;
        if (side == 2) p.z = b.minZ - kRadius;
        if (side == 3) p.z = b.maxZ + kRadius;
      }
    }
  return p;
}

float floorAt(float x, float z) {
  float h = 0;
  for (const Floor& f : g.floors)
    if (x >= f.minX && x <= f.maxX && z >= f.minZ && z <= f.maxZ) h = std::max(h, f.height);
  return h;
}

void pick() {
  draw::DetailView v;
  v.eye = g.eye;
  v.pixelsPerMetre = 600;
  v.limitPixels = g.limitPixels;
  g.trianglesDrawn = 0;
  for (Group& gr : g.groups) {
    draw::pickClusters(gr.tree, v, g.picked);
    g.trianglesDrawn += draw::pickedIndices(gr.tree, g.picked, g.scratch);
    gr.drawn = static_cast<uint32_t>(g.scratch.size());
    if (gr.drawn) g.dev->updateIndices(gr.ib, g.scratch.data(), g.scratch.size());
  }
  g.pickedAt = g.eye;
  g.pickedYaw = g.yaw;
}

// The rig's channels by name, as the bake grouped the lamps (the same rig file).
void mapRig() {
  g.rigChannel.assign(static_cast<size_t>(g.channels), -1);
  const auto& rc = g.rig.channels();
  for (int k = 0; k < g.channels; ++k)
    for (size_t r = 0; r < rc.size(); ++r)
      if (rc[r].name == g.channelNames[static_cast<size_t>(k)]) g.rigChannel[static_cast<size_t>(k)] = static_cast<int>(r);
  g.rigMapped = true;
}

void frame() {
  if (!g.ready) return;
  uint32_t w = 1, h = 1;
  g.dev->resize(&w, &h);
  const double now = emscripten_get_now() / 1000.0;
  const float dt = g.lastT > 0 ? static_cast<float>(std::min(0.1, now - g.lastT)) : 1.0f / 60;
  g.lastT = now;
  // Walking: the left thumb moves you where you look.
  const float yr = g.yaw * 0.017453292f;
  const Vec3 fwd{std::sin(yr), 0, -std::cos(yr)}, right{std::cos(yr), 0, std::sin(yr)};
  const float speed = 1.6f;
  Vec3 next = g.eye + (fwd * (-g.moveZ) + right * g.moveX) * (speed * dt);
  next = resolve(next);
  next.y = floorAt(next.x, next.z) + kEye;
  g.eye = next;
  if (length(g.eye - g.pickedAt) > 0.15f) pick();
  // The light: the rig sets each channel (steady at 1 without one).
  if (g.clock.music) g.clock.beats += dt * g.bpm / 60;
  g.clock.seconds = now;
  if (g.rig.loaded()) {
    if (!g.rigMapped) mapRig();
    g.rig.update(g.clock);
  }
  std::vector<Vec3>& s = g.level;
  s.assign(static_cast<size_t>(g.channels), Vec3{1, 1, 1});
  for (int k = 0; k < g.channels && g.rigMapped; ++k)
    if (const int r = g.rigChannel[static_cast<size_t>(k)]; r >= 0) s[static_cast<size_t>(k)] = g.rig.channel(r);
  Uniforms u{};
  lab::SceneUniforms cam{};
  lab::camera(cam, g.eye, g.yaw, g.pitch, static_cast<float>(w) / static_cast<float>(h), g.dev->backend() == draw::Backend::kWebGpu, g.exposure);
  std::memcpy(u.viewProj, cam.viewProj, sizeof u.viewProj);
  std::memcpy(u.eye, cam.eye, sizeof u.eye);
  for (int k = 0; k < g.layers && k < 4; ++k)
    for (int c = 0; c < g.channels; ++c)
      for (int e = 0; e < 3; ++e) {
        const Vec3& v = s[static_cast<size_t>(c)];
        u.w[k][e] += g.basis[static_cast<size_t>(k * 3 * g.channels + 3 * c + e)] * (e == 0 ? v.x : e == 1 ? v.y : v.z);
      }
  // The lamps' texture.
  std::vector<uint16_t> lt(4 * 4 * 64, 0);
  for (size_t i = 0; i < g.lamps.size() && i < 64; ++i) {
    const Lamp& l = g.lamps[i];
    // Each lamp its own flicker (the rig's `each`); the baked room light carries the channel's average.
    const Vec3 v = g.rig.loaded() ? g.rig.lamp(static_cast<int>(i)) : Vec3{1, 1, 1};
    const float b = l.cd;
    const float row[16] = {l.pos.x, l.pos.y, l.pos.z, l.radius, l.color.x * b * v.x, l.color.y * b * v.y, l.color.z * b * v.z, l.shape,
                           l.axis.x, l.axis.y, l.axis.z, l.oneSided, l.axis2.x, l.axis2.y, l.axis2.z, 0};
    for (int k = 0; k < 16; ++k) lt[i * 16 + static_cast<size_t>(k)] = draw::toHalf(row[k]);
  }
  g.dev->updateTexture(g.lampTex, 4, 64, draw::Format::kRgba16f, lt.data());
  const float clear[4] = {0.01f, 0.01f, 0.012f, 1};
  if (g.dev->begin(clear)) {
    for (const Group& gr : g.groups) {
      if (!gr.drawn) continue;
      const bool pic = gr.picture >= 0 && static_cast<size_t>(gr.picture) < g.pictures.size() && g.pictures[static_cast<size_t>(gr.picture)];
      u.group[0] = pic ? 1.0f : 0.0f;
      const Vec3 avg = pic ? g.pictureAvg[static_cast<size_t>(gr.picture)] : Vec3{1, 1, 1};
      u.picAvg[0] = avg.x, u.picAvg[1] = avg.y, u.picAvg[2] = avg.z;
      u.glow[0] = gr.glow.x, u.glow[1] = gr.glow.y, u.glow[2] = gr.glow.z;
      const draw::Handle tex[3] = {g.atlas, pic ? g.pictures[static_cast<size_t>(gr.picture)] : g.white, g.lampTex};
      g.dev->drawIndexed(g.pipe, gr.vb, gr.ib, gr.drawn, &u, sizeof u, tex, 3);
    }
    g.dev->end();
  }
  ++g.frames;
  if (now - g.fpsT0 > 1) {
    js_stats(g.frames / (now - g.fpsT0), static_cast<double>(g.trianglesDrawn), static_cast<double>(g.trianglesAll));
    g.frames = 0;
    g.fpsT0 = now;
  }
}

void fail(const char* why) { js_status(draw::backendName(g.dev->backend()), why); }

void start(std::unique_ptr<draw::Device> dev) {
  if (!dev) {
    js_status("none", "This browser can draw with neither WebGPU nor WebGL2.");
    return;
  }
  g.dev = std::move(dev);
  draw::PipelineDesc pd;
  pd.label = "bar";
  pd.wgsl = nx::kWgsl;
  pd.glsl = nx::kGlsl;
  pd.textures = 3;
  pd.cullBack = false;
  pd.layout.stride = kVertex;
  pd.layout.items = {{draw::Attr::kFloat3, 0},  {draw::Attr::kFloat2, 12},  {draw::Attr::kUnorm16x2, 20}, {draw::Attr::kUnorm8x4, 24},
                     {draw::Attr::kHalf4, 28},  {draw::Attr::kUint8x4, 36}, {draw::Attr::kUnorm8x4, 40}};
  g.pipe = g.dev->pipeline(pd);
  js_status(draw::backendName(g.dev->backend()), g.dev->problem.c_str());
  js_fetchPlace();  // the page fetches data/place.bin.gz, unpacks it and hands it to nx_place
}

}  // namespace

extern "C" {
// The baked bar arrived (unpacked by the page).
EMSCRIPTEN_KEEPALIVE void nx_place(const uint8_t* data, int size) {
  if (!g.dev || g.ready) return;
  if (!loadPlace(data, static_cast<size_t>(size))) return fail("The baked bar didn't load.");
  pick();
  g.ready = true;
}
// A picture arrived (decoded by the page): RGBA8, w x h.
EMSCRIPTEN_KEEPALIVE void nx_picture(int i, int w, int h, const uint8_t* rgba) {
  if (!g.dev || i < 0 || static_cast<size_t>(i) >= g.pictures.size()) return;
  g.pictures[static_cast<size_t>(i)] = g.dev->texture(w, h, draw::Format::kRgba8, rgba, true);
  double sum[3] = {0, 0, 0};
  const size_t n = static_cast<size_t>(w) * h;
  for (size_t k = 0; k < n; ++k)
    for (int c = 0; c < 3; ++c) {
      sum[c] += rgba[k * 4 + c] / 255.0;  // as the shader samples it (the picture only adds detail around its average)
    }
  g.pictureAvg[static_cast<size_t>(i)] = {static_cast<float>(sum[0] / n), static_cast<float>(sum[1] / n), static_cast<float>(sum[2] / n)};
}
EMSCRIPTEN_KEEPALIVE void nx_move(float x, float z) { g.moveX = x, g.moveZ = z; }
EMSCRIPTEN_KEEPALIVE void nx_look(float dYaw, float dPitch) {
  g.yaw += dYaw;
  g.pitch = std::clamp(g.pitch + dPitch, -80.0f, 80.0f);
}
EMSCRIPTEN_KEEPALIVE void nx_detail(float limitPixels) {
  g.limitPixels = limitPixels;
  if (g.ready) pick();
}
EMSCRIPTEN_KEEPALIVE void nx_exposure(float e) { g.exposure = e; }
// The rig (J118.2): its text and the lamps as it names them (data/lamps.json,
// in the bake's lamp order). Returns 1 when it plays.
EMSCRIPTEN_KEEPALIVE int nx_rig(const char* text, const char* lampsJson) {
  std::vector<words::LampInfo> lamps;
  const Json list = Json::parse(lampsJson);  // kept: a loop over a temporary's items would read freed memory
  for (const Json& j : list.items()) {
    words::LampInfo l;
    l.id = j["id"].string();
    l.kind = j["kind"].string();
    l.seed = static_cast<uint32_t>(j["seed"].number(1));
    l.lumens = static_cast<float>(j["lumens"].number());
    l.kelvin = static_cast<float>(j["kelvin"].number());
    lamps.push_back(std::move(l));
  }
  static const std::vector<std::string> events = {"music", "quiet", "fire-on", "fire-off", "lights-down", "lights-up"};
  words::Problems p;
  const bool ok = g.rig.load(text, lamps, events, p);
  g.rigMapped = false;
  if (!ok && g.dev) {
    const std::string why = "The lights' rig didn't load: " + (p.empty() ? std::string("?") : p[0].text());
    js_status(draw::backendName(g.dev->backend()), why.c_str());
  }
  return ok ? 1 : 0;
}
// Something happened: music, quiet, lights-down, lights-up, fire-on, fire-off.
EMSCRIPTEN_KEEPALIVE void nx_event(const char* e) {
  const std::string ev = e;
  if (ev == "music") g.clock.music = true, g.clock.beats = 0;
  if (ev == "quiet") g.clock.music = false;
  g.rig.send(ev, emscripten_get_now() / 1000.0);
}
// For the test: channel k's brightness this frame (its colour's average), and how many channels the rig plays.
EMSCRIPTEN_KEEPALIVE float nx_level(int k) {
  if (k < 0 || static_cast<size_t>(k) >= g.level.size()) return -1;
  const Vec3& v = g.level[static_cast<size_t>(k)];
  return (v.x + v.y + v.z) / 3;
}
EMSCRIPTEN_KEEPALIVE int nx_rigChannels() {
  int n = 0;
  for (int r : g.rigChannel) n += r >= 0;
  return n;
}
EMSCRIPTEN_KEEPALIVE int nx_ready() { return g.ready ? 1 : 0; }
// For the test: where you stand.
EMSCRIPTEN_KEEPALIVE float nx_eyeX() { return g.eye.x; }
EMSCRIPTEN_KEEPALIVE float nx_eyeZ() { return g.eye.z; }
}

int main() {
  draw::create("#canvas", js_forceGl() != 0, start);
  emscripten_set_main_loop(frame, 0, false);
  return 0;
}
