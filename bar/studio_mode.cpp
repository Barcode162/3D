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
  bool readySent = false;
  int layers = 0;
  std::vector<int> posed;  // things the Studio moved: their models follow them every frame
  // The gizmo: arrows on the selection (shown when `gizmo`), one being dragged.
  bool gizmo = false;
  int tool = 0;  // 0 move, 1 turn, 2 size
  int dragAxis = -1;
  bool sliding = false;  // the selection follows the finger over surfaces
  bool locked = false;
  // Several fingers tapping together (two: undo, three: redo, like drawing apps).
  int fingers = 0;
  bool multi = false;  // the last touch was several fingers (not a tap)
  double fingersMs = 0;
  float fingersMoved = 0;
  int dragId = -1;
  engine::Vec2 dragLast;  // X-ray: 1 tap areas, 2 shapes you bump into, 4 seats, 8 lights, 16 where people appear  // the world's list goes to the page after its first update
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

// The selection's middle, and its arrows' screen ends (fractions), for drags.
bool screenOf(Vec3 p, engine::Vec2* out) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  const Mat4 vp = cam.projection(aspect) * cam.view();
  const float x = vp.m[0] * p.x + vp.m[4] * p.y + vp.m[8] * p.z + vp.m[12];
  const float y = vp.m[1] * p.x + vp.m[5] * p.y + vp.m[9] * p.z + vp.m[13];
  const float w = vp.m[3] * p.x + vp.m[7] * p.y + vp.m[11] * p.z + vp.m[15];
  if (w <= 0.01f) return false;
  *out = {(x / w + 1) * 0.5f, (1 - y / w) * 0.5f};
  return true;
}
Vec3 axisDir(int a) { return a == 0 ? Vec3{1, 0, 0} : a == 1 ? Vec3{0, 1, 0} : Vec3{0, 0, 1}; }
float arrowLength(Vec3 mid) { return engine::clampf(engine::length(mid - g_app.camera.position) * 0.18f, 0.15f, 2.0f); }

// Which arrow a finger at `at` is on (-1: none).
int arrowAt(engine::Vec2 at) {
  if (!S.gizmo || S.selected < 0) return -1;
  Vec3 mid;
  float size;
  boundsOf(S.selected, &mid, &size);
  engine::Vec2 o;
  if (!screenOf(mid, &o)) return -1;
  const float aspect = static_cast<float>(g_app.gpu.cssWidth) / std::max(1.0, g_app.gpu.cssHeight);
  int best = -1;
  float bestD = 0.035f;
  for (int a = 0; a < 3; ++a) {
    engine::Vec2 tip;
    if (!screenOf(mid + axisDir(a) * arrowLength(mid), &tip)) continue;
    // Distance from the finger to the arrow's segment, in screen widths.
    const float vx = (tip.x - o.x) * aspect, vy = tip.y - o.y, px = (at.x - o.x) * aspect, py = at.y - o.y;
    const float len2 = vx * vx + vy * vy;
    const float t = len2 > 1e-6f ? engine::clampf((px * vx + py * vy) / len2, 0.2f, 1.15f) : 0;
    const float d = std::hypot(px - vx * t, py - vy * t);
    if (d < bestD) bestD = d, best = a;
  }
  return best;
}

// Moves a thing's models with it (the world already moved it).
void poseModels() {
  const engine::world::World& w = g_app.world.world();
  for (size_t m = 0; m < g_app.scene.models.size() && m < g_app.scene.modelThing.size(); ++m)
    for (int t = g_app.scene.modelThing[m]; t >= 0; t = w.thing(t).parent)
      if (std::find(S.posed.begin(), S.posed.end(), t) != S.posed.end()) {
        g_app.renderer.setModelPose(m, w.thing(g_app.scene.modelThing[m]).world);
        break;
      }
}

// A point in the world → on a thing (its parent's space: what "at" means in the files).
Vec3 toParent(int thing, Vec3 p) {
  const engine::world::World& w = g_app.world.world();
  const int parent = w.thing(thing).parent;
  if (parent < 0) return p;
  const Mat4& m = w.thing(parent).world;
  // Affine inverse: columns 0-2 are the parent's turned (and sized) axes.
  const Vec3 a{m.m[0], m.m[1], m.m[2]}, b{m.m[4], m.m[5], m.m[6]}, c{m.m[8], m.m[9], m.m[10]};
  const Vec3 d = p - Vec3{m.m[12], m.m[13], m.m[14]};
  const Vec3 bc = engine::cross(b, c), ca = engine::cross(c, a), ab = engine::cross(a, b);
  const float det = engine::dot(a, bc);
  if (std::fabs(det) < 1e-8f) return p;
  return Vec3{engine::dot(d, bc), engine::dot(d, ca), engine::dot(d, ab)} / det;
}
bool inSubtree(int thing, int root) {
  const engine::world::World& w = g_app.world.world();
  for (int t = thing; t >= 0; t = w.thing(t).parent)
    if (t == root) return true;
  return false;
}
// The nearest surface along a ray that isn't `skip` (or on it): things' boxes,
// shapes you bump into, the floor. Returns the distance (-1: nothing).
float surfaceAlong(Vec3 eye, Vec3 dir, int skip) {
  const app::BarScene& sc = g_app.scene;
  float best = -1;
  auto take = [&](float t) { if (t > 0.001f && (best < 0 || t < best)) best = t; };
  for (size_t m = 0; m < sc.models.size() && m < sc.modelThing.size(); ++m) {
    const int t = sc.modelThing[m];
    if (inSubtree(t, skip)) continue;
    const engine::world::Thing& th = g_app.world.world().thing(t);
    if (th.kind && (th.kind->id == "room" || th.kind->is == "room")) continue;  // shells: their colliders and the floor
    engine::Aabb box;
    if (g_app.renderer.modelBox(m, &box)) {
      const float d = rayBoxDistance(eye, dir, box.min, box.max);
      if (d > 0) take(d);
    }
  }
  for (size_t k = 0; k < sc.colliders.size() && k < sc.colliderThing.size(); ++k) {
    if (inSubtree(sc.colliderThing[k], skip)) continue;
    const float d = rayTurnedBoxDistance(eye, dir, sc.colliders[k]);
    if (d > 0) take(d);
  }
  if (dir.y < -1e-4f) take((0 - eye.y) / dir.y);  // the floor (raised floors come from shapes)
  return best;
}
void sendRest(const char* ev, int thing, Vec3 world) {
  const Vec3 r = toParent(thing, world);
  send(std::string("{\"ev\":\"") + ev + "\",\"thing\":" + std::to_string(thing) + ",\"rest\":[" + num(r.x) + "," + num(r.y) + "," + num(r.z) + "]}");
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
  // A finger that went down on an arrow drags along it (the page moves the thing).
  for (const engine::RawPress& r : in.presses) {
    if (r.key >= 0) continue;
    if (r.down && S.dragAxis < 0 && !S.sliding) {
      const int a = arrowAt(r.pos);
      if (a >= 0) {
        S.dragAxis = a, S.dragId = r.id, S.dragLast = r.pos;
      } else if (S.gizmo && S.selected >= 0 && S.tool == 0) {
        // A finger that lands on the selection slides it over the surfaces under it.
        const std::vector<Hit> hits = pickAll(r.pos);
        if (!hits.empty() && inSubtree(hits[0].thing, S.selected)) S.sliding = true, S.dragId = r.id;
      }
    } else if (!r.down && r.id == S.dragId && (S.dragAxis >= 0 || S.sliding)) {
      send(S.sliding ? "{\"ev\":\"slideEnd\"}" : "{\"ev\":\"dragEnd\"}");
      S.dragAxis = -1, S.dragId = -1, S.sliding = false;
    }
  }
  if (S.sliding) {
    for (const engine::Pointer& p : in.pointers) {
      if (p.id != S.dragId) continue;
      const Vec3 eye = g_app.camera.position, dir = screenRay(p.pos);
      const float t = surfaceAlong(eye, dir, S.selected);
      if (t > 0) sendRest("slide", S.selected, eye + dir * t);
    }
    S.last = in.pointers;
    S.downs.clear();
    placeCamera();
    return;
  }
  if (S.dragAxis >= 0) {
    for (const engine::Pointer& p : in.pointers) {
      if (p.id != S.dragId) continue;
      Vec3 mid;
      float size;
      boundsOf(S.selected, &mid, &size);
      engine::Vec2 o, tip;
      if (screenOf(mid, &o) && screenOf(mid + axisDir(S.dragAxis), &tip)) {
        // Finger movement along the arrow's screen direction → metres along the axis.
        const float vx = tip.x - o.x, vy = tip.y - o.y, len2 = vx * vx + vy * vy;
        const float d = len2 > 1e-8f ? ((p.pos.x - S.dragLast.x) * vx + (p.pos.y - S.dragLast.y) * vy) / len2 : 0;
        if (std::fabs(d) > 1e-5f)
          send("{\"ev\":\"drag\",\"axis\":" + std::to_string(S.dragAxis) + ",\"by\":" + num(engine::clampf(d, -2, 2)) + "}");
      }
      S.dragLast = p.pos;
    }
    S.last = in.pointers;
    S.downs.clear();
    placeCamera();
    return;
  }
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
  // Several fingers at once: count them while any are down; a quick, still
  // touch of two or three fingers is an undo or a redo.
  if (!in.pointers.empty()) {
    if (S.fingers == 0) S.fingersMs = engine::frameTime().nowMs, S.fingersMoved = 0;
    S.fingers = std::max(S.fingers, static_cast<int>(in.pointers.size()));
    for (const engine::Pointer& p : in.pointers)
      S.fingersMoved = std::max(S.fingersMoved, std::hypot(p.pos.x - p.start.x, p.pos.y - p.start.y));
  } else if (S.fingers > 0) {
    if (S.fingers >= 2 && S.fingersMoved < 0.03f && engine::frameTime().nowMs - S.fingersMs < 450)
      send(S.fingers == 2 ? "{\"ev\":\"undo\"}" : "{\"ev\":\"redo\"}");
    S.multi = S.fingers >= 2;
    S.fingers = 0;
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
      if (!r.cancelled && r.ms - d.ms < 400 && moved < 0.02f && in.pointers.empty() && !S.multi && S.fingers < 2) tapped(r.pos, r.ms);
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
  } else if (c.name == "st.pose") {  // a: thing; text: x,y,z,turn°,sx,sy,sz on its parent (as in the files)
    float v[7] = {0, 0, 0, 0, 1, 1, 1};
    const int t = static_cast<int>(c.a);
    if (t >= 0 && t < static_cast<int>(g_app.world.world().size()) &&
        std::sscanf(c.text.c_str(), "%f,%f,%f,%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]) == 7) {
      engine::world::World& w = g_app.world.editWorld();
      w.setRest(t, {v[0], v[1], v[2]}, engine::Quat::axisAngle({0, 1, 0}, engine::radians(v[3])));
      w.setScale(t, {v[4], v[5], v[6]});
      if (std::find(S.posed.begin(), S.posed.end(), t) == S.posed.end()) S.posed.push_back(t);
    }
  } else if (c.name == "st.attach") {  // a: thing, b: its new parent (-1: the place)
    const int t = static_cast<int>(c.a), p = static_cast<int>(c.b);
    const int n = static_cast<int>(g_app.world.world().size());
    if (t >= 0 && t < n && p >= -1 && p < n && p != t) {
      g_app.world.editWorld().attach(t, p);
      if (std::find(S.posed.begin(), S.posed.end(), t) == S.posed.end()) S.posed.push_back(t);
      const engine::world::Thing& th = g_app.world.world().thing(t);
      const float turn = 2.0f * std::atan2(th.rot.y, th.rot.w) * 180.0f / engine::kPi;  // about Y: all the files hold
      send("{\"ev\":\"attached\",\"thing\":" + std::to_string(t) + ",\"at\":[" + num(th.at.x) + "," + num(th.at.y) + "," + num(th.at.z) +
           "],\"turn\":" + num(turn) + ",\"parent\":" + std::to_string(p) + "}");
    }
  } else if (c.name == "st.drop") {  // a: thing → where it lands if let go straight down
    const int t = static_cast<int>(c.a);
    if (t >= 0 && t < static_cast<int>(g_app.world.world().size())) {
      const Mat4& m = g_app.world.world().thing(t).world;
      const Vec3 from{m.m[12], m.m[13] + 0.02f, m.m[14]};
      const float d = surfaceAlong(from, {0, -1, 0}, t);
      if (d > 0) sendRest("drop", t, from + Vec3{0, -d, 0});
    }
  } else if (c.name == "st.gizmo") {  // a: 1 = arrows on the selection; b: tool (0 move, 1 turn, 2 size)
    S.gizmo = c.a > 0;
    S.tool = static_cast<int>(c.b);
  } else if (c.name == "st.where") {  // a: thing → where it is in the world now
    const int t = static_cast<int>(c.a);
    if (t >= 0 && t < static_cast<int>(g_app.world.world().size())) {
      const Mat4& m = g_app.world.world().thing(t).world;
      send("{\"ev\":\"where\",\"thing\":" + std::to_string(t) + ",\"at\":[" + num(m.m[12]) + "," + num(m.m[13]) + "," + num(m.m[14]) + "]}");
    }
  } else if (c.name == "st.layers") {  // a: which X-ray layers (bits, see Studio::layers)
    S.layers = static_cast<int>(c.a);
  } else if (c.name == "st.frame") {
    frameThing(c.a >= 0 ? static_cast<int>(c.a) : S.selected);
  } else {
    LOG_INFO("Studio: unknown command %s.", c.name.c_str());
  }
  return true;
}

// X-ray (docs/STUDIO.md 5.3): the invisible parts as glowing edges, the
// nearest first (the renderer draws at most so many boxes).
struct Region {
  Vec3 center, half;
  engine::Quat rot;
  Vec3 color;
};
void xray(std::vector<app::SceneRenderer::Box>& boxes) {
  if (!S.layers) return;
  const app::BarScene& sc = g_app.scene;
  std::vector<Region> regions;
  auto box = [&](Vec3 mn, Vec3 mx, Vec3 color) { regions.push_back({(mn + mx) * 0.5f, (mx - mn) * 0.5f, {}, color}); };
  if (S.layers & 1)
    for (const app::Interactable& it : sc.interactables) box(it.boundsMin, it.boundsMax, {1.0f, 0.55f, 0.17f});
  if (S.layers & 2)
    for (const app::Collider& c : sc.colliders) regions.push_back({c.center, c.half, c.rotation, {0.36f, 0.61f, 1.0f}});
  if (S.layers & 4)
    for (const app::Seat& st : sc.seats) box(st.boundsMin, st.boundsMax, {0.34f, 0.81f, 0.53f});
  if (S.layers & 8)
    for (const auto& l : sc.lights) {
      const float r = std::min(l.range * 0.25f, 1.2f);
      box(l.position - Vec3{r, r, r}, l.position + Vec3{r, r, r}, l.color);
    }
  if (S.layers & 16) {
    const Vec3 p = sc.spawnPosition;
    box({p.x - 0.3f, p.y - 1.65f, p.z - 0.3f}, {p.x + 0.3f, p.y + 0.2f, p.z + 0.3f}, {0.95f, 0.78f, 0.27f});
  }
  const Vec3 eye = g_app.camera.position;
  std::sort(regions.begin(), regions.end(), [&](const Region& a, const Region& b) {
    return engine::lengthSq(a.center - eye) < engine::lengthSq(b.center - eye);
  });
  const float t = 0.012f;  // edge thickness, metres
  for (const Region& r : regions) {
    if (boxes.size() + 12 > app::SceneRenderer::kMaxBoxes) break;
    const Vec3 h = r.half;
    // 4 edges along each axis.
    for (int axis = 0; axis < 3; ++axis)
      for (int k = 0; k < 4; ++k) {
        const float s1 = (k & 1) ? 1.0f : -1.0f, s2 = (k & 2) ? 1.0f : -1.0f;
        Vec3 off, size;
        if (axis == 0) off = {0, s1 * h.y, s2 * h.z}, size = {h.x * 2 + t, t, t};
        if (axis == 1) off = {s1 * h.x, 0, s2 * h.z}, size = {t, h.y * 2 + t, t};
        if (axis == 2) off = {s1 * h.x, s2 * h.y, 0}, size = {t, t, h.z * 2 + t};
        app::SceneRenderer::Box b;
        b.center = r.center + r.rot.rotate(off);
        b.rotation = r.rot;
        b.size = size;
        b.color = r.color;
        b.glow = 0.7f;
        boxes.push_back(b);
      }
  }
}

void studioPresent(std::vector<app::SceneRenderer::Box>& boxes, double nowMs) {
  if (!S.readySent && g_app.entered && !g_app.loader.wantsWarmUpFrame()) {
    S.readySent = true;
    sendReady();
  }
  // The selection breathes (docs/STUDIO.md 4.0: the selected thing pulses).
  const float glow = 0.6f + 0.35f * std::sin(static_cast<float>(nowMs) * 0.005f);
  for (size_t m : S.glowing) g_app.renderer.setGlow(0, 0, m, 1, glow);
  poseModels();
  xray(boxes);
  if (S.gizmo && S.selected >= 0) {
    Vec3 mid;
    float size;
    boundsOf(S.selected, &mid, &size);
    const float len = arrowLength(mid), t = len * 0.035f;
    const Vec3 colors[3] = {{0.94f, 0.29f, 0.2f}, {0.34f, 0.81f, 0.53f}, {0.36f, 0.61f, 1.0f}};
    for (int a = 0; a < 3; ++a) {
      const Vec3 d = axisDir(a);
      app::SceneRenderer::Box shaft, tip;
      shaft.center = mid + d * (len * 0.5f);
      shaft.size = {a == 0 ? len : t, a == 1 ? len : t, a == 2 ? len : t};
      shaft.color = colors[a];
      shaft.glow = a == S.dragAxis ? 2.0f : 1.0f;
      tip = shaft;
      tip.center = mid + d * len;
      tip.size = {t * 3, t * 3, t * 3};
      if (S.tool == 2) tip.size = tip.size * 1.3f;  // size: square ends
      boxes.push_back(shaft);
      boxes.push_back(tip);
    }
  }
}

}  // namespace barapp
