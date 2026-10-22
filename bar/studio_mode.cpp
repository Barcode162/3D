#include "bar/studio_mode.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace barapp {

namespace {

struct Studio {
  bool on = false;
  int mode = 0;  // 0 orbit, 1 fly, 2 top (a map), 3 walk (eye height, on the floor)
  // Orbit and top: the camera looks at `target` from `dist` metres away.
  Vec3 target{-1, 1, -1};
  float dist = 4.5f, yaw = 0.6f, pitch = -0.35f;
  // Fingers down last frame, for drags and pinches.
  std::vector<engine::Pointer> last;
} S;

void send(const std::string& json) { engine::bridge::op("[\"studio\"," + json + "]"); }

std::string num(float v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.3f", static_cast<double>(v));
  return b;
}

constexpr float kEye = 1.65f;

// Where the camera is, from the mode's own numbers.
void placeCamera() {
  engine::FirstPersonCamera& cam = g_app.camera;
  cam.fovY = engine::radians(70.0f);
  if (S.mode == 0 || S.mode == 2) {
    cam.yaw = S.yaw;
    cam.pitch = S.mode == 2 ? engine::radians(-89.0f) : S.pitch;
    cam.position = S.target - cam.forward() * S.dist;
  }
  if (S.mode == 2) {
    // A map from just under the ceiling of the room below: further out is a
    // wider lens, not a higher camera (the roof would hide everything).
    float ceiling = 1e9f;
    for (const engine::Aabb& r : g_app.scene.rooms)
      if (S.target.x >= r.min.x && S.target.x <= r.max.x && S.target.z >= r.min.z && S.target.z <= r.max.z)
        ceiling = std::min(ceiling, r.max.y - 0.2f);
    const float height = std::max(0.5f, std::min(S.dist, ceiling - S.target.y));
    cam.position.y = S.target.y + height;
    const float half = std::atan(std::tan(engine::radians(35.0f)) * S.dist / height);
    cam.fovY = std::min(2 * half, engine::radians(130.0f));
  }
  if (S.mode == 3) cam.position.y = app::groundHeight(g_app.scene, cam.position) + kEye;
}

void setMode(int mode) {
  engine::FirstPersonCamera& cam = g_app.camera;
  const bool wasAround = S.mode == 0 || S.mode == 2, around = mode == 0 || mode == 2;
  if (wasAround && !around) {  // keep looking from where the camera is
    cam.yaw = S.yaw;
  } else if (!wasAround && around) {  // look at what's in front
    S.yaw = cam.yaw;
    S.pitch = engine::clampf(cam.pitch, engine::radians(-85.0f), engine::radians(60.0f));
    S.target = cam.position + cam.forward() * S.dist;
  }
  if (mode == 2) S.dist = std::max(S.dist, 9.0f);
  if (mode == 3) cam.pitch = 0;
  S.mode = mode;
  placeCamera();
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
  g_app.introEndMs = 0;  // no "Left thumb: walk" here
  const engine::InputFrame& in = g_app.input.frame();
  engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 right = engine::normalize(engine::cross(cam.forward(), {0, 1, 0}));
  const Vec3 up = engine::cross(right, cam.forward());
  if (in.pointers.size() == 1) {
    // One finger: orbit (orbit), look around (fly, walk), slide the map (top).
    const engine::Pointer& p = in.pointers[0];
    if (const engine::Pointer* q = find(S.last, p.id)) {
      const float dx = p.pos.x - q->pos.x, dy = p.pos.y - q->pos.y;
      if (S.mode == 0) {
        S.yaw += dx * 4.0f;
        S.pitch = engine::clampf(S.pitch - dy * 3.0f, engine::radians(-89.0f), engine::radians(60.0f));
      } else if (S.mode == 2) {
        const float k = S.dist * 1.1f;
        S.target = S.target - right * (dx * k) + cam.flatForward() * (dy * k);
      } else {
        cam.look(-dx * 3.0f, dy * 2.4f);
      }
    }
  } else if (in.pointers.size() >= 2) {
    // Two fingers: move with them (pan), pinch to come closer.
    const engine::Pointer &a = in.pointers[0], &b = in.pointers[1];
    const engine::Pointer *qa = find(S.last, a.id), *qb = find(S.last, b.id);
    if (qa && qb) {
      const float dx = (a.pos.x + b.pos.x - qa->pos.x - qb->pos.x) * 0.5f;
      const float dy = (a.pos.y + b.pos.y - qa->pos.y - qb->pos.y) * 0.5f;
      const float spread = std::hypot(a.pos.x - b.pos.x, a.pos.y - b.pos.y);
      const float lastSpread = std::hypot(qa->pos.x - qb->pos.x, qa->pos.y - qb->pos.y);
      const float pinch = lastSpread > 0.01f ? spread / lastSpread : 1.0f;
      if (S.mode == 0 || S.mode == 2) {
        const float k = S.dist * 1.1f;
        S.target = S.target - right * (dx * k) + (S.mode == 2 ? cam.flatForward() * (dy * k) : up * (dy * k));
        S.dist = engine::clampf(S.dist / pinch, 0.4f, 60.0f);
      } else {
        const Vec3 ahead = S.mode == 3 ? cam.flatForward() : cam.forward();
        cam.position = cam.position - right * (dx * 4.0f) + ahead * (-dy * 4.0f + (pinch - 1.0f) * 3.0f);
      }
    }
  }
  S.last = in.pointers;
  placeCamera();
}

bool studioCommand(const engine::ui::Command& c) {
  if (c.name.rfind("st.", 0) != 0) return false;
  engine::FirstPersonCamera& cam = g_app.camera;
  if (c.name == "st.cam") {
    setMode(std::clamp(static_cast<int>(c.a), 0, 3));
  } else if (c.name == "st.getview") {
    send("{\"ev\":\"view\",\"view\":[" + num(cam.position.x) + "," + num(cam.position.y) + "," + num(cam.position.z) + "," +
         num(cam.yaw) + "," + num(cam.pitch) + "," + std::to_string(S.mode) + "," + num(S.dist) + "]}");
  } else if (c.name == "st.view") {  // text: x,y,z,yaw,pitch,mode,dist (as st.getview gives)
    float v[7] = {};
    if (std::sscanf(c.text.c_str(), "%f,%f,%f,%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]) == 7) {
      S.mode = std::clamp(static_cast<int>(v[5]), 0, 3);
      cam.position = {v[0], v[1], v[2]};
      cam.yaw = S.yaw = v[3];
      cam.pitch = S.pitch = v[4];
      S.dist = engine::clampf(v[6], 0.4f, 60.0f);
      S.target = cam.position + cam.forward() * S.dist;
      placeCamera();
    }
  } else {
    LOG_INFO("Studio: unknown command %s.", c.name.c_str());
  }
  return true;
}

void studioPresent(std::vector<app::SceneRenderer::Box>&, double) {}

}  // namespace barapp
