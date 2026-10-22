#include "bar/studio_mode.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace barapp {

float rayTurnedBoxDistance(Vec3 origin, Vec3 dir, const app::Collider& c);  // game.cpp

namespace {

struct Studio {
  bool on = false;
  int mode = 0;  // 0 orbit, 1 fly, 2 top (a map), 3 walk (eye height, on the floor)
  // Orbit and top: the camera looks at `target` from `dist` metres away.
  Vec3 target{-1, 1, -1};
  float dist = 4.5f, yaw = 0.6f, pitch = -0.35f;
  // Fingers down last frame, for drags and pinches.
  std::vector<engine::Pointer> last;
  // Taps: where each finger went down, how far it wandered; the last tap.
  struct Down { int id; engine::Vec2 pos; double ms; float moved; };
  std::vector<Down> downs;
  double lastTapMs = -1e9;
  engine::Vec2 lastTapPos;
  // The selection and the models glowing for it.
  int selected = -1;
  std::vector<size_t> glowing;
  bool readySent = false;  // the world's list goes to the page after its first update
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

struct Hit {
  int thing;
  float dist;
};

// Every thing under a screen point (fractions), nearest first, once each:
// models (their boxes), shapes you bump into, tap areas, seats, lights.
std::vector<Hit> pickAll(engine::Vec2 at) {
  const Vec3 eye = g_app.camera.position, dir = screenRay(at);
  std::vector<Hit> hits;
  auto add = [&](int thing, float t) {
    if (thing < 0 || t < 0) return;
    for (Hit& h : hits)
      if (h.thing == thing) {
        h.dist = std::min(h.dist, t);
        return;
      }
    hits.push_back({thing, t});
  };
  const app::BarScene& sc = g_app.scene;
  for (size_t m = 0; m < sc.models.size() && m < sc.modelThing.size(); ++m) {
    engine::Aabb box;
    if (g_app.renderer.modelBox(m, &box)) add(sc.modelThing[m], rayBoxDistance(eye, dir, box.min, box.max));
  }
  for (size_t k = 0; k < sc.colliders.size() && k < sc.colliderThing.size(); ++k)
    add(sc.colliderThing[k], rayTurnedBoxDistance(eye, dir, sc.colliders[k]));
  for (size_t k = 0; k < sc.interactables.size() && k < sc.useThing.size(); ++k)
    add(sc.useThing[k], rayBoxDistance(eye, dir, sc.interactables[k].boundsMin, sc.interactables[k].boundsMax));
  for (size_t k = 0; k < sc.seats.size() && k < sc.seatThing.size(); ++k)
    add(sc.seatThing[k], rayBoxDistance(eye, dir, sc.seats[k].boundsMin, sc.seats[k].boundsMax));
  for (size_t k = 0; k < sc.lights.size() && k < sc.lightThing.size(); ++k) {
    const Vec3 p = sc.lights[k].position, r{0.12f, 0.12f, 0.12f};
    add(sc.lightThing[k], rayBoxDistance(eye, dir, p - r, p + r));
  }
  // A room's shell (walls, floor) and boxes around the camera: only after the rest.
  std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.dist < b.dist; });
  const engine::world::World& w = g_app.world.world();
  std::stable_partition(hits.begin(), hits.end(), [&](const Hit& h) {
    const engine::world::Thing& t = w.thing(h.thing);
    // Boxes the camera is inside of (the hall around you) aren't what you tapped.
    return h.dist > 0.01f && !(t.kind && (t.kind->id == "room" || t.kind->is == "room"));
  });
  return hits;
}

// The middle and size of a thing (its models' boxes, else where it is).
void boundsOf(int thing, Vec3* mid, float* size) {
  const app::BarScene& sc = g_app.scene;
  engine::Aabb all{{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
  bool any = false;
  for (size_t m = 0; m < sc.models.size() && m < sc.modelThing.size(); ++m) {
    engine::Aabb box;
    if (sc.modelThing[m] != thing || !g_app.renderer.modelBox(m, &box)) continue;
    all.min = {std::min(all.min.x, box.min.x), std::min(all.min.y, box.min.y), std::min(all.min.z, box.min.z)};
    all.max = {std::max(all.max.x, box.max.x), std::max(all.max.y, box.max.y), std::max(all.max.z, box.max.z)};
    any = true;
  }
  if (any) {
    *mid = (all.min + all.max) * 0.5f;
    *size = engine::length(all.max - all.min);
  } else {
    const Mat4& m = g_app.world.world().thing(thing).world;
    *mid = {m.m[12], m.m[13], m.m[14]};
    *size = 0.5f;
  }
}

void frameThing(int thing) {
  if (thing < 0 || thing >= static_cast<int>(g_app.world.world().size())) return;
  Vec3 mid;
  float size;
  boundsOf(thing, &mid, &size);
  if (S.mode == 1 || S.mode == 3) setMode(0);
  S.target = mid;
  S.dist = engine::clampf(size * 1.6f, 0.6f, 25.0f);
  placeCamera();
}

void select(int thing) {
  if (thing >= static_cast<int>(g_app.world.world().size())) thing = -1;
  for (size_t m : S.glowing) g_app.renderer.setGlow(0, 0, m, 1, 0);
  S.glowing.clear();
  S.selected = thing;
  if (thing < 0) return;
  const engine::world::World& w = g_app.world.world();
  for (size_t m = 0; m < g_app.scene.models.size() && m < g_app.scene.modelThing.size(); ++m) {
    // The thing and everything sitting on it.
    for (int t = g_app.scene.modelThing[m]; t >= 0; t = w.thing(t).parent)
      if (t == thing) {
        S.glowing.push_back(m);
        break;
      }
  }
}

void tapped(engine::Vec2 at, double ms) {
  const std::vector<Hit> hits = pickAll(at);
  const bool twice = ms - S.lastTapMs < 350 && std::hypot(at.x - S.lastTapPos.x, at.y - S.lastTapPos.y) < 0.05f;
  S.lastTapMs = twice ? -1e9 : ms;
  S.lastTapPos = at;
  std::string list = "[";
  for (size_t k = 0; k < hits.size() && k < 8; ++k)
    list += (k ? "," : "") + std::string("[") + std::to_string(hits[k].thing) + "," + num(hits[k].dist) + "]";
  list += "]";
  send("{\"ev\":\"pick\",\"hits\":" + list + ",\"twice\":" + (twice ? "true" : "false") + "}");
  if (twice && !hits.empty()) frameThing(S.selected >= 0 ? S.selected : hits[0].thing);
}

void sendReady() {
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

}  // namespace

bool studioOn() { return S.on; }

void studioStart() {
  S.on = true;
  g_app.input.setCapture(true);  // touches are the Studio's (no walking, no tapping things)
  S.target = g_app.scene.spawnPosition;
  S.target.y = 1;
  placeCamera();
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
  // Taps: a finger that went down and up quickly without wandering.
  for (const engine::Pointer& p : in.pointers)
    for (Studio::Down& d : S.downs)
      if (d.id == p.id) d.moved = std::max(d.moved, std::hypot(p.pos.x - d.pos.x, p.pos.y - d.pos.y));
  for (const engine::RawPress& r : in.presses) {
    if (r.key >= 0) continue;
    if (r.down) {
      S.downs.push_back({r.id, r.pos, r.ms, 0});
      continue;
    }
    for (size_t k = 0; k < S.downs.size(); ++k) {
      if (S.downs[k].id != r.id) continue;
      const Studio::Down d = S.downs[k];
      S.downs.erase(S.downs.begin() + static_cast<long>(k));
      const float moved = std::max(d.moved, std::hypot(r.pos.x - d.pos.x, r.pos.y - d.pos.y));
      if (!r.cancelled && r.ms - d.ms < 400 && moved < 0.02f && in.pointers.empty()) tapped(r.pos, r.ms);
      break;
    }
  }
  if (in.pointers.empty() && S.downs.size() > 4) S.downs.clear();
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
  } else if (c.name == "st.select") {  // a: thing index (-1: none); b: 1 = and fly to it
    select(static_cast<int>(c.a));
    if (c.b > 0) frameThing(S.selected);
  } else if (c.name == "st.check") {  // the world's own checks (every thing alive, rules that point somewhere)
    std::string list = "[";
    for (const std::string& p : g_app.world.world().check()) list += (list.size() > 1 ? "," : "") + engine::jsonQuote(p);
    send("{\"ev\":\"check\",\"problems\":" + list + "]}");
  } else if (c.name == "st.frame") {
    frameThing(c.a >= 0 ? static_cast<int>(c.a) : S.selected);
  } else {
    LOG_INFO("Studio: unknown command %s.", c.name.c_str());
  }
  return true;
}

void studioPresent(std::vector<app::SceneRenderer::Box>&, double nowMs) {
  if (!S.readySent) {
    S.readySent = true;
    sendReady();
  }
  // The selection breathes (docs/STUDIO.md 4.0: the selected thing pulses).
  const float glow = 0.6f + 0.35f * std::sin(static_cast<float>(nowMs) * 0.005f);
  for (size_t m : S.glowing) g_app.renderer.setGlow(0, 0, m, 1, glow);
}

}  // namespace barapp
