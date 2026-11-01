// The draw lab (J116.4): the light lab's baked room drawn by the new
// skeleton's drawing piece (looped/draw/), on WebGPU or WebGL2, with the one
// light formula (labs/draw/scene_shader.h). Its lights play their loops
// (the engine's own: looped/world/loop.h) from the room's world file, and
// the page's buttons dim them as the room's words say.
//   the page: labs/web/draw/index.html (?gl=1 forces WebGL2)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "labs/draw/scene_shader.h"
#include "looped/draw/draw.h"
#include "looped/json.h"
#include "looped/math.h"
#include "looped/world/loop.h"

using namespace engine;

namespace {

struct SceneUniforms {
  float viewProj[16];
  float eye[4];  // xyz, exposure
  float w[4][4];
  float lampPos[16][4];
  float lampCol[16][4];
  float lampAx[16][4];
  float lampAx2[16][4];
};
static_assert(sizeof(SceneUniforms) == 1168, "the std140 block");

struct LampInfo {
  Vec3 pos, color, axis, axis2;
  float cd = 0, radius = 0, shape = 0, oneSided = 0;
  int channel = 0;
};

struct Lab {
  std::unique_ptr<draw::Device> dev;
  draw::Handle pipe = 0, vb = 0, ib = 0, atlas = 0;
  uint32_t indices = 0;
  int layers = 0, channels = 0;
  std::vector<float> basis;  // layers x 3·channels
  std::vector<LampInfo> lamps;
  std::vector<std::string> ids;
  std::vector<world::Loop> loops;  // per channel (may be empty)
  std::vector<float> manual;       // per channel, from the page
  std::vector<float> dims;         // per channel: the level when the lights go down
  bool down = false, alive = true;
  float yaw = 0, pitch = -12, exposure = 1;
  Vec3 eye{0, 1.65f, 2.9f};
  bool dragging = false;
  double lastX = 0, lastY = 0;
  double fpsT0 = 0;
  int frames = 0;
  bool ready = false;
};
Lab g;

EM_JS(void, js_status, (const char* backend, const char* problem), {
  if (window['drawLabStatus']) window['drawLabStatus'](UTF8ToString(backend), UTF8ToString(problem));
});
EM_JS(void, js_fps, (double fps), { if (window['drawLabFps']) window['drawLabFps'](fps); });
EM_JS(int, js_forceGl, (), { return /[?&]gl=1(&|$)/.test(location.search) ? 1 : 0; });

template <class T>
T take(const uint8_t*& p) {
  T v;
  std::memcpy(&v, p, sizeof v);
  p += sizeof v;
  return v;
}

bool loadScene(const uint8_t* data, size_t size) {
  if (size < 36 || std::memcmp(data, "LSCN", 4) != 0) return false;
  const uint8_t* p = data + 4;
  take<uint32_t>(p);  // version
  const uint32_t nv = take<uint32_t>(p), ni = take<uint32_t>(p), nl = take<uint32_t>(p);
  g.layers = static_cast<int>(take<uint32_t>(p));
  g.channels = static_cast<int>(take<uint32_t>(p));
  const uint32_t aw = take<uint32_t>(p), ah = take<uint32_t>(p);
  const size_t vbytes = static_cast<size_t>(nv) * 72;
  g.vb = g.dev->vertexBuffer(p, vbytes);
  p += vbytes;
  g.ib = g.dev->indexBuffer(reinterpret_cast<const uint32_t*>(p), ni);
  g.indices = ni;
  p += static_cast<size_t>(ni) * 4;
  g.atlas = g.dev->texture(static_cast<int>(aw), static_cast<int>(ah), draw::Format::kRgba16f, p);
  p += static_cast<size_t>(aw) * ah * 8;
  g.basis.resize(static_cast<size_t>(g.layers) * 3 * g.channels);
  for (float& b : g.basis) b = take<float>(p);
  g.lamps.resize(nl);
  for (LampInfo& l : g.lamps) {
    float f[18];
    for (float& x : f) x = take<float>(p);
    l.pos = {f[0], f[1], f[2]}, l.color = {f[3], f[4], f[5]}, l.cd = f[6], l.radius = f[7], l.shape = f[8];
    l.axis = {f[9], f[10], f[11]}, l.axis2 = {f[12], f[13], f[14]}, l.oneSided = f[15], l.channel = static_cast<int>(f[16]);
  }
  for (int c = 0; c < g.channels; ++c) {
    const uint16_t n = take<uint16_t>(p);
    g.ids.emplace_back(reinterpret_cast<const char*>(p), n);
    p += n;
  }
  g.manual.assign(static_cast<size_t>(g.channels), 1.0f);
  g.dims.assign(static_cast<size_t>(g.channels), 1.0f);
  g.loops.resize(static_cast<size_t>(g.channels));
  return static_cast<size_t>(p - data) <= size;
}

// The room's world file: each light's loop, and which lights dim (and how far) when the lights go down.
void loadPlace(const char* text, int size) {
  std::string err;
  const Json j = Json::parse(std::string_view(text, static_cast<size_t>(size)), &err);
  for (const Json& t : j["things"].items()) {
    for (int c = 0; c < g.channels; ++c) {
      if (g.ids[static_cast<size_t>(c)] != t["id"].string()) continue;
      for (const auto& [name, lj] : t["loops"].members()) g.loops[static_cast<size_t>(c)] = world::parseLoop(lj, name, &err);
      if (t["parts"]["light"]["dims"].boolean(false)) g.dims[static_cast<size_t>(c)] = 0.25f;  // the words: dim to a quarter
    }
  }
}

void perspective(float* m, float fovY, float aspect, float zn, float zf, bool zeroToOne) {
  const float f = 1 / std::tan(fovY / 2);
  std::memset(m, 0, 64);
  m[0] = f / aspect;
  m[5] = f;
  if (zeroToOne) m[10] = zf / (zn - zf), m[14] = zn * zf / (zn - zf);
  else m[10] = (zf + zn) / (zn - zf), m[14] = 2 * zf * zn / (zn - zf);
  m[11] = -1;
}

void mul(const float* a, const float* b, float* o) {  // column-major: o = a · b
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r) {
      float s = 0;
      for (int k = 0; k < 4; ++k) s += a[k * 4 + r] * b[c * 4 + k];
      o[c * 4 + r] = s;
    }
}

void frame() {
  if (!g.ready) return;
  uint32_t w = 1, h = 1;
  g.dev->resize(&w, &h);
  const double now = emscripten_get_now() / 1000.0;
  // Each channel's brightness now: its loop, the lights going down, the page's slider.
  std::vector<float> s(static_cast<size_t>(g.channels), 1.0f);
  world::Clock clock;
  clock.seconds = now;
  for (int c = 0; c < g.channels; ++c) {
    float v = 1;
    const world::Loop& L = g.loops[static_cast<size_t>(c)];
    if (g.alive && L.live()) {
      world::Motion m;
      const uint32_t seed = world::hash32(g.ids[static_cast<size_t>(c)]);
      L.add(L.cycles(clock, 0, seed), seed, 1, m);
      v += m.ch[world::kLight];
    }
    if (g.down) v *= g.dims[static_cast<size_t>(c)];
    s[static_cast<size_t>(c)] = std::max(0.0f, v * g.manual[static_cast<size_t>(c)]);
  }
  SceneUniforms u{};
  // The camera.
  const float yr = radians(g.yaw), pr = radians(g.pitch);
  const Vec3 f{std::sin(yr) * std::cos(pr), std::sin(pr), -std::cos(yr) * std::cos(pr)};
  const Vec3 right = normalize(cross(f, {0, 1, 0})), up = cross(right, f);
  float view[16] = {right.x, up.x, -f.x, 0, right.y, up.y, -f.y, 0, right.z, up.z, -f.z, 0,
                    -dot(right, g.eye), -dot(up, g.eye), dot(f, g.eye), 1};
  float proj[16];
  perspective(proj, radians(70), static_cast<float>(w) / static_cast<float>(h), 0.05f, 50, g.dev->backend() == draw::Backend::kWebGpu);
  mul(proj, view, u.viewProj);
  u.eye[0] = g.eye.x, u.eye[1] = g.eye.y, u.eye[2] = g.eye.z, u.eye[3] = g.exposure;
  // w_k = B_k · s (rgb).
  for (int k = 0; k < g.layers && k < 4; ++k)
    for (int c = 0; c < g.channels; ++c)
      for (int e = 0; e < 3; ++e) u.w[k][e] += g.basis[static_cast<size_t>(k * 3 * g.channels + 3 * c + e)] * s[static_cast<size_t>(c)];
  for (size_t i = 0; i < g.lamps.size() && i < 16; ++i) {
    const LampInfo& l = g.lamps[i];
    const float b = l.cd * s[static_cast<size_t>(l.channel)];
    u.lampPos[i][0] = l.pos.x, u.lampPos[i][1] = l.pos.y, u.lampPos[i][2] = l.pos.z, u.lampPos[i][3] = l.radius;
    u.lampCol[i][0] = l.color.x * b, u.lampCol[i][1] = l.color.y * b, u.lampCol[i][2] = l.color.z * b, u.lampCol[i][3] = l.shape;
    u.lampAx[i][0] = l.axis.x, u.lampAx[i][1] = l.axis.y, u.lampAx[i][2] = l.axis.z, u.lampAx[i][3] = l.oneSided;
    u.lampAx2[i][0] = l.axis2.x, u.lampAx2[i][1] = l.axis2.y, u.lampAx2[i][2] = l.axis2.z;
  }
  const float clear[4] = {0.02f, 0.02f, 0.025f, 1};
  if (!g.dev->begin(clear)) return;
  g.dev->drawIndexed(g.pipe, g.vb, g.ib, g.indices, &u, sizeof u, &g.atlas, 1);
  g.dev->end();
  ++g.frames;
  if (now - g.fpsT0 > 1) {
    js_fps(g.frames / (now - g.fpsT0));
    g.frames = 0;
    g.fpsT0 = now;
  }
}

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
  pd.layout.items = {{draw::Attr::kFloat3, 0},     {draw::Attr::kFloat3, 12},  {draw::Attr::kFloat2, 24},
                     {draw::Attr::kUnorm8x4, 32},  {draw::Attr::kFloat3, 36},  {draw::Attr::kFloat4, 48},
                     {draw::Attr::kUint8x4, 64},   {draw::Attr::kUnorm8x4, 68}};
  g.pipe = g.dev->pipeline(pd);
  js_status(draw::backendName(g.dev->backend()), g.dev->problem.c_str());
  emscripten_async_wget_data(
      "data/scene.bin", nullptr,
      [](void*, void* buf, int size) {
        if (!loadScene(static_cast<const uint8_t*>(buf), static_cast<size_t>(size))) {
          js_status(draw::backendName(g.dev->backend()), "The baked room didn't load.");
          return;
        }
        emscripten_async_wget_data(
            "../../assets/world/places/lab.json", nullptr,
            [](void*, void* b2, int s2) {
              loadPlace(static_cast<const char*>(b2), s2);
              g.ready = true;
            },
            [](void*) { g.ready = true; });
      },
      [](void*) { js_status(draw::backendName(g.dev->backend()), "The baked room didn't load."); });
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
EMSCRIPTEN_KEEPALIVE void dl_lightsDown(int on) { g.down = on != 0; }
EMSCRIPTEN_KEEPALIVE void dl_alive(int on) { g.alive = on != 0; }
EMSCRIPTEN_KEEPALIVE void dl_exposure(float e) { g.exposure = e; }
EMSCRIPTEN_KEEPALIVE void dl_spot(float x, float y, float z, float yaw, float pitch) {
  g.eye = {x, y, z}, g.yaw = yaw, g.pitch = pitch;
}
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
