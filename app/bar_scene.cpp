#include "app/bar_scene.h"

#include <algorithm>
#include <cmath>

using engine::Quat;
using engine::Vec3;

namespace app {
namespace {

// Room interior: x from -6 to 6, z from -5 to 5, ceiling at 3.2 m.
constexpr float kRoomHalfX = 6.0f;
constexpr float kRoomHalfZ = 5.0f;
constexpr float kRoomHeight = 3.2f;

// Light and glow colors.
constexpr Vec3 kWarmBulb{1.0f, 0.68f, 0.38f};
constexpr Vec3 kNeonPink{1.0f, 0.18f, 0.55f};

struct Builder {
  BarScene& scene;

  // Places a mesh made of material `m`.
  SceneObject& add(engine::MeshData mesh, Vec3 pos, MaterialId m, Quat rot = {}) {
    const engine::Material& mat = material(m);
    scene.objects.push_back({std::move(mesh), pos, rot, m, mat.color, mat.emissive, mat.textureScale});
    return scene.objects.back();
  }

  // Places a .glb model (assets/models/<file>); returns its index.
  size_t model(const char* file, Vec3 pos, MaterialId m, Quat rot = {}, Prop prop = Prop::kNone) {
    scene.models.push_back({file, pos, rot, 1, m, prop});
    return scene.models.size() - 1;
  }

  // An invisible solid box for physics (a model's shape).
  void solid(Vec3 center, Vec3 half, MaterialId m, Quat rot = {}) {
    scene.colliders.push_back({center, half, rot, m});
  }

  void block(Vec3 center, float halfX, float halfZ) {
    scene.blockers.push_back({center.x - halfX, center.z - halfZ, center.x + halfX, center.z + halfZ});
  }

  void light(Vec3 pos, Vec3 color, float intensity, float range, float radius = 0.1f) {
    scene.lights.push_back({pos, color, intensity, range, radius});
  }
};

Quat turnY(float radians) { return Quat::axisAngle({0, 1, 0}, radians); }

void buildRoom(Builder& b) {
  // The shell is one model (tools/assets/room.py): plank floor, brick
  // walls, plaster ceiling with beams and crown molding, wainscot with a
  // chair rail, baseboards, and the entrance door on the front wall.
  b.model("room.glb", {0, 0, 0}, kBrick);
  const float hx = kRoomHalfX, hz = kRoomHalfZ, h = kRoomHeight;
  b.solid({0, -0.05f, 0}, {hx, 0.05f, hz}, kOakFloor);
  b.solid({0, h + 0.05f, 0}, {hx, 0.05f, hz}, kPlaster);
  b.solid({0, h / 2, -hz - 0.1f}, {hx + 0.2f, h / 2, 0.1f}, kBrick);
  b.solid({0, h / 2, hz + 0.1f}, {hx + 0.2f, h / 2, 0.1f}, kBrick);
  b.solid({-hx - 0.1f, h / 2, 0}, {0.1f, h / 2, hz}, kBrick);
  b.solid({hx + 0.1f, h / 2, 0}, {0.1f, h / 2, hz}, kBrick);

  // Walls block walking.
  b.block({0, 0, -hz - 0.5f}, hx + 1, 0.5f);
  b.block({0, 0, hz + 0.5f}, hx + 1, 0.5f);
  b.block({-hx - 0.5f, 0, 0}, 0.5f, hz + 1);
  b.block({hx + 0.5f, 0, 0}, 0.5f, hz + 1);
}

void buildBar(Builder& b) {
  const float barZ = -3.2f, barLen = 7.0f;
  // The counter (tools/assets/bar_counter.py): paneled front, mahogany top
  // with a drink rail, brass foot rail on brackets.
  b.model("bar_counter.glb", {0, 0, barZ}, kVarnishedWood);
  // Its solid shape: the top, the raised drink rail and the body below.
  b.solid({0, 1.07f, barZ + 0.05f}, {3.65f, 0.03f, 0.47f}, kVarnishedWood);
  b.solid({0, 1.1175f, barZ + 0.46f}, {3.65f, 0.0175f, 0.06f}, kVarnishedWood);
  b.solid({0, 0.52f, barZ}, {3.52f, 0.52f, 0.37f}, kDarkWood);

  // Glasses on the bar top (its surface is 1.10 m up) to pick up and throw.
  const float topY = 1.10f, glassZ = barZ + 0.08f;
  for (float x : {-2.3f, -0.4f, 1.3f, 2.6f}) b.model("pint_glass.glb", {x, topY, glassZ}, kBottleGlass, {}, Prop::kPintGlass);
  for (float x : {-1.4f, 0.6f}) {
    b.model("rocks_glass.glb", {x, topY, glassZ - 0.1f}, kBottleGlass, {}, Prop::kRocksGlass);
  }
  // Beer taps on the bartender's side (the faucets face the back bar).
  const float tapX = -0.9f, tapZ = barZ - 0.2f;
  b.model("beer_taps.glb", {tapX, topY, tapZ}, kChrome, turnY(engine::kPi));
  b.solid({tapX, topY + 0.19f, tapZ}, {0.04f, 0.19f, 0.04f}, kChrome);
  b.solid({tapX, topY + 0.37f, tapZ}, {0.28f, 0.035f, 0.035f}, kChrome);
  // Coasters in front of each stool, a napkin holder and a menu.
  for (int i = 0; i < 6; ++i) {
    b.model("coaster.glb", {-2.5f + static_cast<float>(i) + 0.12f, topY, barZ + 0.3f}, kPlaster, turnY(0.3f * i));
  }
  b.model("napkin_holder.glb", {2.05f, topY, barZ + 0.05f}, kChrome, turnY(0.2f));
  b.solid({2.05f, topY + 0.06f, barZ + 0.05f}, {0.075f, 0.06f, 0.045f}, kChrome, turnY(0.2f));
  b.model("menu_stand.glb", {-3.0f, topY, barZ + 0.12f}, kDarkWood, turnY(0.15f));
  // Counter plus the bartender's area behind it.
  b.block({0, 0, (barZ + 0.5f + -kRoomHalfZ) / 2}, barLen / 2 + 0.15f, (barZ + 0.5f + kRoomHalfZ) / 2);

  // Bar stools: chrome pedestal, foot ring, tufted leather seat.
  for (int i = 0; i < 6; ++i) {
    const float x = -2.5f + static_cast<float>(i), z = barZ + 0.95f;
    b.model("bar_stool.glb", {x, 0, z}, kLeather, turnY(0.4f * static_cast<float>(i)));
    b.solid({x, 0.77f, z}, {0.2f, 0.05f, 0.2f}, kLeather);
    b.solid({x, 0.37f, z}, {0.03f, 0.33f, 0.03f}, kChrome);
    b.solid({x, 0.023f, z}, {0.2f, 0.023f, 0.2f}, kChrome);
  }

  // Back bar: cabinet, smoked mirror, three shelves of bottles with LED strips.
  const float backZ = -kRoomHalfZ;
  b.model("back_bar.glb", {0, 0, backZ}, kDarkWood);
  b.solid({0, 0.5f, backZ + 0.29f}, {3.23f, 0.5f, 0.29f}, kDarkWood);
  b.solid({0, 1.74f, backZ + 0.006f}, {3.2f, 0.74f, 0.006f}, kGlowGlass);
  for (float y : {1.30f, 1.75f, 2.20f}) b.solid({0, y, backZ + 0.17f}, {3.2f, 0.0175f, 0.16f}, kDarkWood);
  b.light({0, 1.9f, backZ + 0.9f}, kWarmBulb, 0.9f, 4.0f, 0.5f);

  // Pendant lamps over the counter (each hangs from the ceiling).
  for (int i = -1; i <= 1; ++i) {
    const float x = 2.4f * static_cast<float>(i);
    b.model("pendant_lamp.glb", {x, kRoomHeight, barZ}, kBlackSteel);
    b.light({x, 2.3f, barZ}, kWarmBulb, 2.4f, 6.0f, 0.08f);
  }

  // Neon sign above the back bar: "BAR" with martini glasses.
  const float signY = 2.88f;
  b.model("neon_sign.glb", {0, signY, backZ}, kGlowGlass);
  b.light({0, signY, backZ + 0.4f}, kNeonPink, 1.2f, 4.5f, 0.6f);
}

void buildSeating(Builder& b) {
  // Two round pub tables with low stools on the right side.
  for (float z : {-0.2f, 2.4f}) {
    const Vec3 c{3.6f, 0, z};
    const float topY = 0.785f;
    b.model("round_table.glb", c, kVarnishedWood);
    b.solid({c.x, topY - 0.0225f, c.z}, {0.45f, 0.0225f, 0.45f}, kVarnishedWood);
    b.solid({c.x, 0.37f, c.z}, {0.05f, 0.37f, 0.05f}, kBlackSteel);
    b.solid({c.x, 0.025f, c.z}, {0.25f, 0.025f, 0.25f}, kBlackSteel);
    b.block(c, 0.5f, 0.5f);
    b.model("candle.glb", {c.x, topY, c.z}, kBottleGlass);
    b.solid({c.x, topY + 0.04f, c.z}, {0.037f, 0.04f, 0.037f}, kBottleGlass);
    b.model("rocks_glass.glb", {c.x + 0.22f, topY, c.z + 0.12f}, kBottleGlass, {}, Prop::kRocksGlass);
    b.model("menu_stand.glb", {c.x - 0.2f, topY, c.z - 0.18f}, kDarkWood, turnY(-engine::kPi / 2 + 0.3f));
    for (float dx : {-0.8f, 0.8f}) {
      b.model("table_stool.glb", {c.x + dx, 0, c.z}, kLeather, turnY(dx));
      b.solid({c.x + dx, 0.5f, c.z}, {0.18f, 0.04f, 0.18f}, kLeather);
      b.solid({c.x + dx, 0.23f, c.z}, {0.13f, 0.22f, 0.13f}, kBlackSteel);
    }
  }
  // Over the tables, a bit toward the darts corner on the front wall.
  b.light({3.3f, 2.5f, 1.8f}, kWarmBulb, 1.7f, 7.0f);

  // A long channel-tufted leather booth against the left wall, two small
  // tables, and two framed posters above it.
  const float boothX = -kRoomHalfX + 0.03f, boothZ = 0.6f, boothLen = 3.6f;
  b.model("booth.glb", {boothX, 0, boothZ}, kLeather);
  b.solid({boothX + 0.33f, 0.23f, boothZ}, {0.33f, 0.23f, boothLen / 2}, kLeather);
  b.solid({boothX + 0.1f, 0.83f, boothZ}, {0.1f, 0.38f, boothLen / 2}, kLeather);
  b.block({-kRoomHalfX + 0.35f, 0, boothZ}, 0.35f, boothLen / 2);
  for (float z : {-0.3f, 1.5f}) {
    const float topY = 0.765f;
    b.model("booth_table.glb", {-4.75f, 0, z}, kVarnishedWood);
    b.solid({-4.75f, topY - 0.02f, z}, {0.375f, 0.02f, 0.375f}, kVarnishedWood);
    b.solid({-4.75f, 0.36f, z}, {0.05f, 0.36f, 0.05f}, kBlackSteel);
    b.model("candle.glb", {-4.75f, topY, z}, kBottleGlass);
    b.solid({-4.75f, topY + 0.04f, z}, {0.037f, 0.04f, 0.037f}, kBottleGlass);
    b.model("napkin_holder.glb", {-4.95f, topY, z + 0.2f}, kChrome, turnY(engine::kPi / 2));
    b.block({-4.75f, 0, z}, 0.42f, 0.42f);
  }
  b.model("poster_jazz.glb", {-kRoomHalfX, 1.9f, -0.3f}, kDarkWood, turnY(engine::kPi / 2));
  b.model("poster_cocktails.glb", {-kRoomHalfX, 1.9f, 1.5f}, kDarkWood, turnY(engine::kPi / 2));
  b.light({-4.8f, 2.4f, 0.6f}, kWarmBulb, 1.5f, 6.0f);
}

// A mirror on the right wall (facing -x) in a framed model; `m` is the
// center of the frame's back, on the wall.
void addMirror(Builder& b, Vec3 m, MirrorMode mode, const char* frame, MaterialId frameMat, const char* name) {
  const size_t model = b.model(frame, m, frameMat, turnY(-engine::kPi / 2));
  b.solid({m.x - 0.03f, m.y, m.z}, {0.03f, 1.05f, 0.75f}, frameMat);
  const float glassW = 1.35f, glassH = 1.95f;
  // The glass sits just in front of the frame's backing and is drawn by the renderer.
  b.scene.mirrors.push_back({{m.x - 0.025f, m.y, m.z}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, glassW / 2, glassH / 2,
                             mode});
  Interactable it;
  it.name = name;
  it.tagPosition = {m.x - 0.3f, 2.65f, m.z};
  it.boundsMin = {m.x - 0.1f, m.y - 1.05f, m.z - 0.75f};
  it.boundsMax = {m.x + 0.05f, m.y + 1.05f, m.z + 0.75f};
  it.firstModel = model;
  it.modelCount = 1;
  it.action = Action::kMirror;
  it.mirror = static_cast<int>(b.scene.mirrors.size() - 1);
  b.scene.interactables.push_back(it);
}

void buildFeatures(Builder& b) {
  // Jukebox in the back-right corner: arched walnut cabinet, glow tubes,
  // the record behind a domed window, chrome grille.
  const Vec3 j{4.9f, 0, -4.45f};
  const size_t jukebox = b.model("jukebox.glb", j, kLacquer);
  b.solid({j.x, 0.73f, j.z - 0.015f}, {0.45f, 0.73f, 0.27f}, kLacquer);
  b.block(j, 0.5f, 0.35f);
  b.light({j.x, 1.2f, j.z + 0.7f}, {1.0f, 0.45f, 0.25f}, 1.0f, 3.5f, 0.3f);
  Interactable it{"JUKEBOX", "Jukebox", {j.x, 2.1f, j.z}, {j.x - 0.5f, 0, j.z - 0.35f}, {j.x + 0.5f, 1.9f, j.z + 0.35f}};
  it.firstModel = jukebox;
  it.modelCount = 1;
  it.action = Action::kJukebox;
  b.scene.interactables.push_back(it);
  b.scene.jukeboxSpeaker = {j.x, 0.6f, j.z + 0.3f};

  // Two mirrors on the right wall: a full one in a brass frame, and an
  // avatars-only one in a black frame (VRChat style).
  addMirror(b, {kRoomHalfX, 1.45f, -2.2f}, MirrorMode::kFull, "mirror_frame_brass.glb", kBrass, "MIRROR");
  addMirror(b, {kRoomHalfX, 1.45f, 1.1f}, MirrorMode::kAvatarsOnly, "mirror_frame_black.glb", kBlackSteel,
            "AVATAR MIRROR");

  // Darts corner on the front wall: the dartboard cabinet and a chalkboard.
  b.model("dartboard.glb", {1.6f, 1.73f, kRoomHalfZ}, kDarkWood, turnY(engine::kPi));
  b.solid({1.6f, 1.73f, kRoomHalfZ - 0.04f}, {0.66f, 0.32f, 0.04f}, kDarkWood);
  b.model("chalkboard.glb", {3.3f, 1.6f, kRoomHalfZ}, kDarkWood, turnY(engine::kPi));
}

}  // namespace

std::vector<engine::MaterialImages> buildBarTextures() {
  constexpr int kSize = 512;
  std::vector<engine::MaterialImages> m;
  m.push_back(engine::makeWoodPlanks(kSize, 11));  // kTexWoodPlanks
  m.push_back(engine::makeWoodGrain(kSize, 23));   // kTexWoodGrain
  m.push_back(engine::makeBrick(kSize, 37));       // kTexBrick
  m.push_back(engine::makeLeather(kSize, 41));     // kTexLeather
  m.push_back(engine::makePlaster(kSize, 53));     // kTexPlaster
  m.push_back(engine::makeFabric(kSize, 61));      // kTexFabric
  m.push_back(engine::makeDenim(kSize, 67));       // kTexDenim
  return m;
}

BarScene buildBarScene() {
  BarScene scene;
  scene.ambient = {0.08f, 0.055f, 0.05f};
  scene.fogColor = {0.025f, 0.015f, 0.015f};
  scene.fogDensity = 0.015f;
  scene.spawnPosition = {0, 1.65f, 3.6f};
  scene.spawnYaw = 0;
  scene.lightGridMin = {-kRoomHalfX - 1, 0, -kRoomHalfZ - 1};
  scene.lightGridMax = {kRoomHalfX + 1, 0, kRoomHalfZ + 1};

  Builder b{scene};
  buildRoom(b);
  buildBar(b);
  buildSeating(b);
  buildFeatures(b);
  return scene;
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
