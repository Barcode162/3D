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

#include "labs/draw/room_view.h"
#include "labs/draw/scene_shader.h"
#include "looped/draw/draw.h"
#include "looped/json.h"
#include "looped/math.h"
#include "looped/world/loop.h"

using namespace engine;

namespace {

struct Lab {
  std::unique_ptr<draw::Device> dev;
  draw::Handle pipe = 0, vb = 0, ib = 0, atlas = 0;
  lab::BakedRoom room;
  std::vector<world::Loop> loops;  // per channel (may be empty)
  std::vector<float> manual;       // per channel, from the page
  std::vector<float> dims;         // per channel: the level when the lights go down
  bool down = false, alive = true;
  float yaw = 0, pitch = -12, exposure = 0.7f;
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

bool loadScene(const uint8_t* data, size_t size) {
  if (!lab::parseBakedRoom(data, size, g.room)) return false;
  g.vb = g.dev->vertexBuffer(g.room.vertices.data(), g.room.vertices.size());
  g.ib = g.dev->indexBuffer(g.room.indices.data(), g.room.indices.size());
  g.atlas = g.dev->texture(g.room.atlasW, g.room.atlasH, draw::Format::kRgba16f, g.room.atlas.data());
  const size_t c = static_cast<size_t>(g.room.channels);
  g.manual.assign(c, 1.0f);
  g.dims.assign(c, 1.0f);
  g.loops.resize(c);
  return true;
}

// The room's world file: each light's loop, and which lights dim (and how far) when the lights go down.
void loadPlace(const char* text, int size) {
  std::string err;
  const Json j = Json::parse(std::string_view(text, static_cast<size_t>(size)), &err);
  for (const Json& t : j["things"].items()) {
    for (int c = 0; c < g.room.channels; ++c) {
      if (g.room.ids[static_cast<size_t>(c)] != t["id"].string()) continue;
      for (const auto& [name, lj] : t["loops"].members()) g.loops[static_cast<size_t>(c)] = world::parseLoop(lj, name, &err);
      if (t["parts"]["light"]["dims"].boolean(false)) g.dims[static_cast<size_t>(c)] = 0.25f;  // the words: dim to a quarter
    }
  }
}

void frame() {
  if (!g.ready) return;
  uint32_t w = 1, h = 1;
  g.dev->resize(&w, &h);
  const double now = emscripten_get_now() / 1000.0;
  // Each channel's brightness now: its loop, the lights going down, the page's slider.
  std::vector<float> s(static_cast<size_t>(g.room.channels), 1.0f);
  world::Clock clock;
  clock.seconds = now;
  for (int c = 0; c < g.room.channels; ++c) {
    float v = 1;
    const world::Loop& L = g.loops[static_cast<size_t>(c)];
    if (g.alive && L.live()) {
      world::Motion m;
      const uint32_t seed = world::hash32(g.room.ids[static_cast<size_t>(c)]);
      L.add(L.cycles(clock, 0, seed), seed, 1, m);
      v += m.ch[world::kLight];
    }
    if (g.down) v *= g.dims[static_cast<size_t>(c)];
    s[static_cast<size_t>(c)] = std::max(0.0f, v * g.manual[static_cast<size_t>(c)]);
  }
  lab::SceneUniforms u{};
  lab::camera(u, g.eye, g.yaw, g.pitch, static_cast<float>(w) / static_cast<float>(h), g.dev->backend() == draw::Backend::kWebGpu, g.exposure);
  lab::lights(u, g.room, s);
  const float clear[4] = {0.02f, 0.02f, 0.025f, 1};
  if (!g.dev->begin(clear)) return;
  g.dev->drawIndexed(g.pipe, g.vb, g.ib, static_cast<uint32_t>(g.room.indices.size()), &u, sizeof u, &g.atlas, 1);
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
