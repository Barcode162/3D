#include "bar/bar_scene.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "bar/scene_names.h"
#include "bar/world_files.h"
#include "looped/json.h"
#include "looped/world/place_layout.h"

using engine::Quat;
using engine::Vec3;

namespace app {
namespace {

using engine::Json;
using engine::world::readVec3;

float yawOf(Quat r) { return 2 * std::atan2(r.y, r.w); }  // a turn about y

engine::Aabb box(const Json& j) { return {readVec3(j["min"], {}), readVec3(j["max"], {})}; }

// The bar's materials for the engine's place layout ("" = plaster, plain boxes' default).
const engine::Material* materialById(const std::string& id) {
  MaterialId m = kPlaster;
  if (!id.empty() && !materialFromName(id, &m)) return nullptr;
  return &app::material(m);
}

}  // namespace

BarScene buildBarScene(const std::string& placeName) {
  BarScene scene;
  scene.place = placeName;
  std::string problems = loadMaterials();
  engine::world::Library kinds;
  const std::string placeFile = "places/" + placeName + ".json";
  const Json placeJson = Json::parse(worldFile(placeFile), &problems, {64, 1 << 16});
  if (!placeJson.isObject()) problems += "no place \"" + placeName + "\"\n";
  for (const Json& f : placeJson["kinds"].items()) kinds.add(worldFile(f.string()), f.string());
  kinds.resolve();
  problems += kinds.errors();
  const engine::world::Place place = engine::world::parsePlace(worldFile(placeFile), &problems);
  auto world = std::make_shared<engine::world::World>();
  world->build(kinds, place, &problems);

  // What any program draws (the engine's place layout, J86): the air, where
  // you come in, the rooms and doorways, models, boxes, lights, mirrors.
  engine::world::PlaceLayout lay = engine::world::layOut(*world, placeJson, materialById);
  problems += lay.problems;
  scene.ambient = lay.ambient;
  scene.fogColor = lay.fogColor;
  scene.fogDensity = lay.fogDensity;
  scene.spawnPosition = lay.spawnPosition;
  scene.spawnYaw = lay.spawnYaw;
  scene.lightGridMin = lay.lightGridMin;
  scene.lightGridMax = lay.lightGridMax;
  scene.rooms = lay.rooms;
  scene.doorways = lay.doorways;
  scene.lights = lay.lights;
  scene.lightThing = lay.lightThing;
  for (size_t l = 0; l < lay.lights.size(); ++l) {
    if (lay.lightDims[l]) scene.roomLights.push_back(static_cast<int>(l));
    if (world->thing(lay.lightThing[l]).kind->id == "light-fire") scene.fireLight = static_cast<int>(l);
  }
  scene.mirrors = lay.mirrors;
  scene.mirrorThing = lay.mirrorThing;
  const engine::Aabb arcade = box(placeJson["areas"]["arcade"]);
  scene.arcadeMin = arcade.min;
  scene.arcadeMax = arcade.max;
  scene.arcadeDoor = readVec3(placeJson["points"]["arcade-door"], {});

  // Every thing, in the file's order: its parts become the scene's lists.
  std::map<std::string, int> mirrorOf;
  for (size_t m = 0; m < lay.mirrorThing.size(); ++m) mirrorOf[world->thing(lay.mirrorThing[m]).id] = static_cast<int>(m);
  struct Pending {
    size_t use;
    std::string mirror;
  };
  std::vector<Pending> mirrorUses;
  size_t nextBox = 0;
  for (size_t n = 0; n < world->size(); ++n) {
    const int i = static_cast<int>(n);
    const engine::world::Thing& t = world->thing(i);
    Vec3 pos;
    Quat rot;
    engine::world::restPose(*world, i, &pos, &rot);
    auto at = [&](const Json& j) { return pos + rot.rotate(readVec3(j, {})); };
    auto material = [&](const Json& j, MaterialId fallback) {
      MaterialId m = fallback;
      if (!j.isNull() && !materialFromName(j.string(), &m)) problems += t.id + ": no material \"" + j.string() + "\"\n";
      return m;
    };
    const int model = lay.modelOfThing[n];
    if (model >= 0) {
      const engine::world::PlaceLayout::Model& lm = lay.models[static_cast<size_t>(model)];
      ModelPlacement p;
      p.file = lm.placed.file;
      p.position = lm.placed.position;
      p.rotation = lm.placed.rotation;
      p.scale = lm.placed.scale;
      p.room = lm.placed.room;
      p.material = material(world->part(i, "model")["material"], kDarkWood);
      if (const Json& pr = world->part(i, "prop"); pr.isString() && !propFromName(pr.string(), &p.prop))
        problems += t.id + ": no prop \"" + pr.string() + "\"\n";
      scene.models.push_back(p);
      scene.modelThing.push_back(i);
    }
    for (const Json& s : world->part(i, "solids").items()) {
      const float turn = static_cast<float>(s["turn"].number(0));
      scene.colliders.push_back({at(s["at"]), readVec3(s["half"], {}),
                                 engine::normalized(rot * Quat::axisAngle({0, 1, 0}, engine::radians(turn))),
                                 material(s["material"], kDarkWood)});
      scene.colliderThing.push_back(i);
    }
    // Plain boxes (E7.1), laid out by the engine; solid unless "solid": false.
    for (; nextBox < lay.boxes.size() && lay.boxes[nextBox].thing == i; ++nextBox) {
      engine::world::PlaceLayout::Box& b = lay.boxes[nextBox];
      MaterialId m = kPlaster;
      if (!b.material.empty()) materialFromName(b.material, &m);
      SceneObject o{std::move(b.mesh), b.position, b.rotation, m, b.color, b.glow, b.textureScale};
      scene.objects.push_back(std::move(o));
      if (b.solid) {
        scene.colliders.push_back({b.position, b.size * 0.5f, b.rotation, m});
        scene.colliderThing.push_back(i);
      }
    }
    for (const Json& b : world->part(i, "blocks").items())
      scene.blockers.push_back({pos.x + static_cast<float>(b[0].number()), pos.z + static_cast<float>(b[1].number()),
                                pos.x + static_cast<float>(b[2].number()), pos.z + static_cast<float>(b[3].number())});
    for (const Json& f : world->part(i, "floors").items())
      scene.floors.push_back({pos.x + static_cast<float>(f[0].number()), pos.z + static_cast<float>(f[1].number()),
                              pos.x + static_cast<float>(f[2].number()), pos.z + static_cast<float>(f[3].number()),
                              pos.y + static_cast<float>(f[4].number())});
    for (const Json& s : world->part(i, "seats").items()) {
      Seat st;
      st.name = s["name"].string();
      st.hip = at(s["hip"]);
      const float f = engine::radians(static_cast<float>(s["facing"].number(0)));
      const Vec3 d = rot.rotate({std::sin(f), 0, -std::cos(f)});
      st.yaw = std::atan2(d.x, -d.z);
      st.lowSeat = static_cast<float>(s["low"].number(0));
      if (!seatKindFromName(s["kind"].string(), &st.kind)) problems += t.id + ": no seat kind \"" + s["kind"].string() + "\"\n";
      st.model = s["lights"].boolean(false) ? model : -1;
      st.machine = s["machine"].integer(-1);
      st.boundsMin = {st.hip.x - 0.28f, 0, st.hip.z - 0.28f};
      st.boundsMax = {st.hip.x + 0.28f, st.hip.y + 0.12f, st.hip.z + 0.28f};
      scene.seats.push_back(st);
      scene.seatThing.push_back(i);
    }
    for (const Json& u : world->part(i, "uses").items()) {
      Interactable it;
      it.name = u["name"].string();
      it.prompt = u["prompt"].string();
      it.tagPosition = at(u["tag"]);
      it.boundsMin = pos + readVec3(u["box"][0], {});
      it.boundsMax = pos + readVec3(u["box"][1], {});
      if (!actionFromName(u["action"].string(), &it.action)) problems += t.id + ": no action \"" + u["action"].string() + "\"\n";
      it.value = u["value"].integer(0);
      it.direct = u["direct"].boolean(false);
      if (u["lights"].boolean(false) && model >= 0) it.firstModel = static_cast<size_t>(model), it.modelCount = 1;
      if (u.has("mirror")) mirrorUses.push_back({scene.interactables.size(), u["mirror"].string()});
      scene.interactables.push_back(it);
      scene.useThing.push_back(i);
    }
    // The bar's special spots.
    if (const Json& d = world->part(i, "dartboard"); d.isObject()) {
      scene.dartBoard = at(d["face"]);
      scene.dartOche = at(d["oche"]);
    }
    if (const Json& r = world->part(i, "rig"); r.isObject()) {
      const int machine = r["machine"].integer(0);
      if (machine >= 0 && machine < 2) scene.simPosition[machine] = pos;
    }
    if (const Json& s = world->part(i, "screen"); s.isArray())
      for (int k = 0; k < 4; ++k) scene.rhythmScreen[k] = at(s[k]);
    if (const Json& s = world->part(i, "speaker"); s.isObject())
      (s["music"].string() == "arcade" ? scene.arcadeSpeaker : scene.jukeboxSpeaker) = at(s["at"]);
    if (const Json& d = world->part(i, "drink"); d.isObject()) {
      const int slot = d["slot"].integer(-1);
      if (slot >= 0 && slot < 8) scene.barTopFront[slot] = pos;
    }
    if (const Json& b = world->part(i, "bartender"); b.isObject()) scene.bartenderSpot = at(b["stand"]);
    if (const Json& c = world->part(i, "chessboard"); c.isObject()) {
      scene.chessCenter = at(c["center"]);
      scene.chessSquare = static_cast<float>(c["square"].number(0.055));
    }
    if (const Json& c = world->part(i, "chess"); c.isObject() && model >= 0)
      scene.chessPieces.push_back({static_cast<size_t>(model), static_cast<int8_t>(c["piece"].integer(0))});
    if (world->part(i, "piano").isObject()) {
      scene.pianoPosition = pos;
      scene.pianoYaw = yawOf(rot);
    }
    if (world->part(i, "coin").isObject()) scene.coinModel = model;
  }
  for (const Pending& p : mirrorUses) {
    const auto it = mirrorOf.find(p.mirror);
    if (it == mirrorOf.end()) problems += "a use names mirror \"" + p.mirror + "\", which has none\n";
    else scene.interactables[p.use].mirror = it->second;
  }
  scene.world = world;
  scene.problems = problems;
  return scene;
}

std::vector<engine::MaterialImages> buildBarTextures() {
  constexpr int kSize = 512;
  std::vector<engine::MaterialImages> m;
  m.push_back(engine::makeFabric(kSize, 61));      // kTexFabric
  m.push_back(engine::makeDenim(kSize, 67));       // kTexDenim
  m.push_back(engine::makeHair(kSize, 71));        // kTexHair
  return m;
}

namespace {
// Where you can walk: a 0.1 m grid over the floor, flood-filled from the
// door (cells where a 0.3 m walker isn't pushed by any blocker).
struct WalkMap {
  float minX = 0, minZ = 0, cell = 0.1f;
  int w = 0, d = 0;
  std::vector<uint8_t> reach;
  bool cellOf(Vec3 p, int& i, int& k) const {
    i = static_cast<int>(std::floor((p.x - minX) / cell));
    k = static_cast<int>(std::floor((p.z - minZ) / cell));
    return i >= 0 && k >= 0 && i < w && k < d;
  }
  Vec3 center(int i, int k) const {
    return {minX + (static_cast<float>(i) + 0.5f) * cell, 0, minZ + (static_cast<float>(k) + 0.5f) * cell};
  }
};

WalkMap buildWalkMap(const BarScene& scene) {
  constexpr float kRadius = 0.3f;
  WalkMap m;
  float maxX = -1e9f, maxZ = -1e9f;
  m.minX = m.minZ = 1e9f;
  for (const Blocker& b : scene.blockers) {
    m.minX = std::min(m.minX, b.minX);
    m.minZ = std::min(m.minZ, b.minZ);
    maxX = std::max(maxX, b.maxX);
    maxZ = std::max(maxZ, b.maxZ);
  }
  m.w = static_cast<int>((maxX - m.minX) / m.cell) + 1;
  m.d = static_cast<int>((maxZ - m.minZ) / m.cell) + 1;
  std::vector<uint8_t> open(static_cast<size_t>(m.w * m.d));
  for (int k = 0; k < m.d; ++k) {
    for (int i = 0; i < m.w; ++i) {
      const Vec3 p = m.center(i, k);
      const Vec3 q = resolveCollisions(scene, p, kRadius);
      open[static_cast<size_t>(k * m.w + i)] = std::fabs(q.x - p.x) + std::fabs(q.z - p.z) < 1e-3f;
    }
  }
  m.reach.assign(open.size(), 0);
  int si, sk;
  if (!m.cellOf(scene.spawnPosition, si, sk)) return m;
  std::vector<int> queue;
  // Start from the nearest open cell to the door.
  for (int r = 0; r < 10 && queue.empty(); ++r) {
    for (int dk = -r; dk <= r && queue.empty(); ++dk) {
      for (int di = -r; di <= r && queue.empty(); ++di) {
        const int i = si + di, k = sk + dk;
        if (i >= 0 && k >= 0 && i < m.w && k < m.d && open[static_cast<size_t>(k * m.w + i)]) queue.push_back(k * m.w + i);
      }
    }
  }
  for (int c : queue) m.reach[static_cast<size_t>(c)] = 1;
  for (size_t q = 0; q < queue.size(); ++q) {
    const int c = queue[q], i = c % m.w, k = c / m.w;
    const int ni[4] = {i + 1, i - 1, i, i}, nk[4] = {k, k, k + 1, k - 1};
    for (int n = 0; n < 4; ++n) {
      if (ni[n] < 0 || nk[n] < 0 || ni[n] >= m.w || nk[n] >= m.d) continue;
      const int nc = nk[n] * m.w + ni[n];
      if (!open[static_cast<size_t>(nc)] || m.reach[static_cast<size_t>(nc)]) continue;
      m.reach[static_cast<size_t>(nc)] = 1;
      queue.push_back(nc);
    }
  }
  return m;
}
}  // namespace

bool reachableOnFoot(const BarScene& scene, Vec3 p) {
  static const BarScene* built = nullptr;
  static WalkMap map;
  if (built != &scene) { map = buildWalkMap(scene); built = &scene; }
  int i, k;
  return map.cellOf(p, i, k) && map.reach[static_cast<size_t>(k * map.w + i)];
}

Vec3 standSpot(const BarScene& scene, const Seat& s) {
  constexpr float kRadius = 0.3f;
  // Sofas and armchairs: stand up in front. Stools, the piano bench and the
  // racing rigs face a counter, a piano or screens: step off behind you.
  const Vec3 forward{std::sin(s.yaw), 0, -std::cos(s.yaw)};
  const bool front = s.kind == SeatKind::kSofa || s.kind == SeatKind::kChair;
  Vec3 want = s.hip + forward * (front ? 0.65f : -0.6f);
  want.y = 0;
  const Vec3 p = resolveCollisions(scene, want, kRadius);
  if (reachableOnFoot(scene, p)) return p;
  // Boxed in there: the nearest floor you can walk away from (rings out to 3 m).
  Vec3 best = p;
  float bestD = 1e9f;
  for (float r = 0.1f; r <= 3.0f && bestD > 1e8f; r += 0.1f) {
    for (int a = 0; a < 48; ++a) {
      const float t = engine::kPi * 2 * static_cast<float>(a) / 48.0f;
      const Vec3 c = want + Vec3{std::cos(t) * r, 0, std::sin(t) * r};
      if (!reachableOnFoot(scene, c)) continue;
      const Vec3 q = resolveCollisions(scene, c, kRadius);
      const float d = engine::length(q - want);
      if (reachableOnFoot(scene, q) && d < bestD) { bestD = d; best = q; }
    }
  }
  return best;
}

float groundHeight(const BarScene& scene, Vec3 p) {
  float h = 0;
  for (const FloorArea& f : scene.floors) {
    if (p.x >= f.minX && p.x <= f.maxX && p.z >= f.minZ && p.z <= f.maxZ) h = std::max(h, f.height);
  }
  return h;
}

Vec3 resolveCollisions(const BarScene& scene, Vec3 p, float radius) {
  for (int pass = 0; pass < 2; ++pass) {
    for (const Blocker& bl : scene.blockers) {
      const float cx = std::clamp(p.x, bl.minX, bl.maxX);
      const float cz = std::clamp(p.z, bl.minZ, bl.maxZ);
      const float dx = p.x - cx, dz = p.z - cz;
      const float dist2 = dx * dx + dz * dz;
      if (dist2 >= radius * radius) continue;
      if (dist2 > 1e-8f) {
        const float dist = std::sqrt(dist2);
        p.x = cx + dx / dist * radius;
        p.z = cz + dz / dist * radius;
      } else {
        // Center is inside the blocker: push out along the shortest way.
        const float pushes[4] = {p.x - bl.minX, bl.maxX - p.x, p.z - bl.minZ, bl.maxZ - p.z};
        const int side = static_cast<int>(std::min_element(pushes, pushes + 4) - pushes);
        if (side == 0) p.x = bl.minX - radius;
        if (side == 1) p.x = bl.maxX + radius;
        if (side == 2) p.z = bl.minZ - radius;
        if (side == 3) p.z = bl.maxZ + radius;
      }
    }
  }
  return p;
}

}  // namespace app
