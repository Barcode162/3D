#include "bar/studio_mode.h"

#include <cmath>
#include <string>

namespace barapp {

namespace {

struct Studio {
  bool on = false;
  // The orbit camera: looks at `target` from `dist` metres away.
  Vec3 target{-1, 1, -1};
  float dist = 4.5f, yaw = 0.6f, pitch = -0.35f;
  // Fingers down last frame (id → screen position), for drags.
  std::vector<engine::Pointer> last;
} S;

void send(const std::string& json) { engine::bridge::op("[\"studio\"," + json + "]"); }

std::string num(float v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.3f", static_cast<double>(v));
  return b;
}

void placeCamera() {
  engine::FirstPersonCamera& cam = g_app.camera;
  cam.yaw = S.yaw;
  cam.pitch = S.pitch;
  cam.position = S.target - cam.forward() * S.dist;
}

const engine::Pointer* find(const std::vector<engine::Pointer>& list, int id) {
  for (const engine::Pointer& p : list)
    if (p.id == id) return &p;
  return nullptr;
}

}  // namespace

bool studioOn() { return S.on; }

void studioStart() {
  S.on = true;
  g_app.input.setCapture(true);  // touches are the Studio's (no walking, no tapping things)
  S.target = g_app.scene.spawnPosition;
  S.target.y = 1;
  placeCamera();
  // The world, for the page's lists: index, id, kind, parent, where.
  const engine::world::World& w = g_app.world.world();
  std::string things = "[";
  for (size_t i = 0; i < w.size(); ++i) {
    const engine::world::Thing& t = w.thing(static_cast<int>(i));
    const Mat4& m = t.world;
    if (i) things += ",";
    things += "[" + engine::jsonQuote(t.id) + "," + engine::jsonQuote(t.kind ? t.kind->id : "") + "," +
              std::to_string(t.parent) + "," + num(m.m[12]) + "," + num(m.m[13]) + "," + num(m.m[14]) + "]";
  }
  things += "]";
  send("{\"ev\":\"ready\",\"place\":" + engine::jsonQuote(g_app.scene.place) + ",\"things\":" + things + "}");
  LOG_INFO("Studio: ready (%zu things).", w.size());
}

void studioInput(float) {
  const engine::InputFrame& in = g_app.input.frame();
  // One finger: orbit around the target.
  if (in.pointers.size() == 1) {
    const engine::Pointer& p = in.pointers[0];
    if (const engine::Pointer* q = find(S.last, p.id)) {
      S.yaw += (p.pos.x - q->pos.x) * 4.0f;
      S.pitch = engine::clampf(S.pitch - (p.pos.y - q->pos.y) * 3.0f, engine::radians(-89.0f), engine::radians(60.0f));
    }
  }
  S.last = in.pointers;
  placeCamera();
}

bool studioCommand(const engine::ui::Command& c) {
  if (c.name.rfind("st.", 0) != 0) return false;
  LOG_INFO("Studio: unknown command %s.", c.name.c_str());
  return true;
}

void studioPresent(std::vector<app::SceneRenderer::Box>&, double) {}

}  // namespace barapp
