#include "app/bar_scene.h"

#include <algorithm>
#include <cmath>

using engine::makeBox;
using engine::makeCylinder;
using engine::makeSphere;
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
constexpr Vec3 kNeonCyan{0.2f, 0.85f, 1.0f};

struct Builder {
  BarScene& scene;

  // Places a mesh made of material `m`.
  SceneObject& add(engine::MeshData mesh, Vec3 pos, MaterialId m, Quat rot = {}) {
    const engine::Material& mat = material(m);
    scene.objects.push_back({std::move(mesh), pos, rot, m, mat.color, mat.emissive, mat.textureScale});
    return scene.objects.back();
  }

  void block(Vec3 center, float halfX, float halfZ) {
    scene.blockers.push_back({center.x - halfX, center.z - halfZ, center.x + halfX, center.z + halfZ});
  }

  void light(Vec3 pos, Vec3 color, float intensity, float range, float radius = 0.1f) {
    scene.lights.push_back({pos, color, intensity, range, radius});
  }
};

void buildRoom(Builder& b) {
  const float w = kRoomHalfX * 2, d = kRoomHalfZ * 2, t = 0.2f;
  b.add(makeBox({w, 0.1f, d}), {0, -0.05f, 0}, kOakFloor);
  b.add(makeBox({w, 0.1f, d}), {0, kRoomHeight + 0.05f, 0}, kPlaster);
  b.add(makeBox({w, kRoomHeight, t}), {0, kRoomHeight / 2, -kRoomHalfZ - t / 2}, kBrick);
  b.add(makeBox({w, kRoomHeight, t}), {0, kRoomHeight / 2, kRoomHalfZ + t / 2}, kBrick);
  b.add(makeBox({t, kRoomHeight, d}), {-kRoomHalfX - t / 2, kRoomHeight / 2, 0}, kBrick);
  b.add(makeBox({t, kRoomHeight, d}), {kRoomHalfX + t / 2, kRoomHeight / 2, 0}, kBrick);

  // Dark wood paneling on the lower part of the side and front walls.
  const float panel = 1.0f, inset = 0.03f;
  b.add(makeBox({w, panel, inset}), {0, panel / 2, kRoomHalfZ - inset / 2}, kDarkWood);
  b.add(makeBox({inset, panel, d}), {-kRoomHalfX + inset / 2, panel / 2, 0}, kDarkWood);
  b.add(makeBox({inset, panel, d}), {kRoomHalfX - inset / 2, panel / 2, 0}, kDarkWood);

  // Entrance door on the front wall.
  b.add(makeBox({1.1f, 2.2f, 0.06f}), {-3.5f, 1.1f, kRoomHalfZ - 0.03f}, kDoorWood);
  b.add(makeSphere(0.04f, 12, 8), {-3.1f, 1.05f, kRoomHalfZ - 0.08f}, kBrass);

  // Walls block walking.
  b.block({0, 0, -kRoomHalfZ - 0.5f}, kRoomHalfX + 1, 0.5f);
  b.block({0, 0, kRoomHalfZ + 0.5f}, kRoomHalfX + 1, 0.5f);
  b.block({-kRoomHalfX - 0.5f, 0, 0}, 0.5f, kRoomHalfZ + 1);
  b.block({kRoomHalfX + 0.5f, 0, 0}, 0.5f, kRoomHalfZ + 1);
}

void buildBar(Builder& b) {
  const float barZ = -3.2f, barLen = 7.0f;
  b.add(makeBox({barLen, 1.05f, 0.7f}), {0, 0.525f, barZ}, kDarkWood);
  b.add(makeBox({barLen + 0.3f, 0.06f, 0.9f}), {0, 1.08f, barZ + 0.05f}, kVarnishedWood).scale(0.7f);
  // Brass foot rail along the front.
  b.add(makeCylinder(0.025f, barLen, 16), {0, 0.2f, barZ + 0.48f}, kBrass,
        Quat::axisAngle({0, 0, 1}, engine::kPi / 2));
  // Counter plus the bartender's area behind it.
  b.block({0, 0, (barZ + 0.5f + -kRoomHalfZ) / 2}, barLen / 2 + 0.15f,
          (barZ + 0.5f + kRoomHalfZ) / 2);

  // Stools in front of the bar.
  for (int i = 0; i < 6; ++i) {
    const float x = -2.5f + static_cast<float>(i);
    const float z = barZ + 0.95f;
    b.add(makeCylinder(0.2f, 0.08f), {x, 0.78f, z}, kLeather);
    b.add(makeCylinder(0.035f, 0.74f, 12), {x, 0.37f, z}, kChrome);
    b.add(makeCylinder(0.18f, 0.03f), {x, 0.015f, z}, kChrome);
    b.add(makeCylinder(0.15f, 0.02f, 16), {x, 0.3f, z}, kChrome);  // foot ring
  }

  // Back bar: three backlit shelves of bottles against the back wall.
  const float shelfZ = -kRoomHalfZ + 0.2f;
  const Vec3 bottleColors[] = {
      {0.55f, 0.25f, 0.05f}, {0.1f, 0.35f, 0.12f}, {0.6f, 0.6f, 0.55f},
      {0.45f, 0.08f, 0.1f},  {0.7f, 0.45f, 0.1f},  {0.15f, 0.25f, 0.45f},
  };
  for (int s = 0; s < 3; ++s) {
    const float y = 1.3f + 0.45f * static_cast<float>(s);
    b.add(makeBox({6.0f, 0.04f, 0.35f}), {0, y, shelfZ}, kDarkWood).scale(1.5f);
    b.add(makeBox({6.0f, 0.015f, 0.02f}), {0, y - 0.03f, shelfZ + 0.16f}, kGlowGlass).tint(kWarmBulb).glow(1.2f);
    for (int i = 0; i < 16; ++i) {
      const float x = -2.7f + 0.36f * static_cast<float>(i) + 0.05f * static_cast<float>(s % 2);
      const Vec3 color = bottleColors[(i * 7 + s * 3) % 6];
      const float h = 0.24f + 0.06f * static_cast<float>((i + s) % 3);
      // A little glow stands in for the shelf light shining through the glass.
      b.add(makeCylinder(0.04f, h, 12), {x, y + 0.02f + h / 2, shelfZ}, kBottleGlass).tint(color).glow(0.25f);
      b.add(makeCylinder(0.014f, 0.08f, 8), {x, y + 0.02f + h + 0.04f, shelfZ}, kBottleGlass).tint(color).glow(0.25f);
    }
  }
  b.light({0, 1.9f, shelfZ + 0.9f}, kWarmBulb, 0.9f, 4.0f, 0.5f);

  // Pendant lamps over the counter.
  for (int i = -1; i <= 1; ++i) {
    const float x = 2.4f * static_cast<float>(i);
    b.add(makeSphere(0.08f), {x, 2.28f, barZ}, kGlowGlass).tint(kWarmBulb).glow(8.0f);
    b.add(makeCylinder(0.2f, 0.18f), {x, 2.42f, barZ}, kBlackSteel);
    b.add(makeCylinder(0.008f, 0.7f, 6), {x, 2.86f, barZ}, kBlackSteel);
    b.light({x, 2.2f, barZ}, kWarmBulb, 2.4f, 6.0f, 0.08f);
  }

  // Neon sign above the shelves: a pink frame with a cyan line inside.
  const float signY = 2.78f, signZ = -kRoomHalfZ + 0.05f;
  const auto neon = [&](Vec3 size, Vec3 pos, Vec3 color) {
    b.add(makeBox(size), pos, kGlowGlass).tint(color).glow(3.0f);
  };
  neon({2.2f, 0.04f, 0.04f}, {0, signY + 0.28f, signZ}, kNeonPink);
  neon({2.2f, 0.04f, 0.04f}, {0, signY - 0.28f, signZ}, kNeonPink);
  neon({0.04f, 0.6f, 0.04f}, {-1.1f, signY, signZ}, kNeonPink);
  neon({0.04f, 0.6f, 0.04f}, {1.1f, signY, signZ}, kNeonPink);
  neon({1.5f, 0.035f, 0.035f}, {0, signY, signZ}, kNeonCyan);
  b.light({0, signY, signZ + 0.4f}, kNeonPink, 1.2f, 4.5f, 0.6f);
}

void buildSeating(Builder& b) {
  // Two round tables with stools on the right side.
  for (float z : {-0.2f, 2.4f}) {
    const Vec3 c{3.6f, 0, z};
    b.add(makeCylinder(0.45f, 0.05f, 32), {c.x, 0.76f, c.z}, kVarnishedWood).scale(1.5f);
    b.add(makeCylinder(0.05f, 0.74f, 12), {c.x, 0.37f, c.z}, kBlackSteel);
    b.add(makeCylinder(0.25f, 0.03f), {c.x, 0.015f, c.z}, kBlackSteel);
    b.add(makeSphere(0.035f, 12, 8), {c.x, 0.82f, c.z}, kGlowGlass).tint(kWarmBulb).glow(4.0f);  // candle glow
    b.block(c, 0.5f, 0.5f);
    for (float dx : {-0.8f, 0.8f}) {
      b.add(makeCylinder(0.19f, 0.07f), {c.x + dx, 0.5f, c.z}, kLeather);
      b.add(makeCylinder(0.03f, 0.47f, 12), {c.x + dx, 0.235f, c.z}, kChrome);
    }
  }
  b.light({3.6f, 2.5f, 1.1f}, kWarmBulb, 1.6f, 6.0f);

  // A long leather booth against the left wall with two small tables.
  const float boothX = -kRoomHalfX + 0.35f, boothZ = 0.6f, boothLen = 3.6f;
  b.add(makeBox({0.65f, 0.45f, boothLen}), {boothX, 0.225f, boothZ}, kLeather).scale(3);
  b.add(makeBox({0.15f, 0.75f, boothLen}), {-kRoomHalfX + 0.1f, 0.8f, boothZ}, kLeather).scale(3);
  b.block({boothX, 0, boothZ}, 0.35f, boothLen / 2);
  for (float z : {-0.3f, 1.5f}) {
    b.add(makeBox({0.75f, 0.05f, 0.75f}), {-4.75f, 0.74f, z}, kVarnishedWood).scale(1.5f);
    b.add(makeCylinder(0.05f, 0.72f, 12), {-4.75f, 0.36f, z}, kBlackSteel);
    b.add(makeSphere(0.035f, 12, 8), {-4.75f, 0.8f, z}, kGlowGlass).tint(kWarmBulb).glow(4.0f);
    b.block({-4.75f, 0, z}, 0.42f, 0.42f);
  }
  b.light({-4.8f, 2.4f, 0.6f}, kWarmBulb, 1.5f, 6.0f);
}

// A mirror on the right wall (facing -x) with its frame; `m` is the frame center.
void addMirror(Builder& b, Vec3 m, MirrorMode mode, MaterialId frame, const char* name) {
  const size_t first = b.scene.objects.size();
  b.add(makeBox({0.04f, 2.1f, 1.5f}), m, frame);
  const float glassW = 1.35f, glassH = 1.95f;
  // The glass sits just in front of the frame and is drawn by the renderer.
  b.scene.mirrors.push_back({{m.x - 0.025f, m.y, m.z}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, glassW / 2, glassH / 2,
                             mode});
  Interactable it;
  it.name = name;
  it.tagPosition = {m.x - 0.3f, 2.65f, m.z};
  it.boundsMin = {m.x - 0.1f, m.y - 1.05f, m.z - 0.75f};
  it.boundsMax = {m.x + 0.05f, m.y + 1.05f, m.z + 0.75f};
  it.firstObject = first;
  it.objectCount = b.scene.objects.size() - first;
  it.action = Action::kMirror;
  it.mirror = static_cast<int>(b.scene.mirrors.size() - 1);
  b.scene.interactables.push_back(it);
}

void buildFeatures(Builder& b) {
  // Jukebox in the back-right corner.
  const Vec3 j{4.9f, 0, -4.45f};
  const size_t jukeboxFirst = b.scene.objects.size();
  b.add(makeBox({0.9f, 1.4f, 0.6f}), {j.x, 0.7f, j.z}, kLacquer);
  b.add(makeCylinder(0.45f, 0.6f, 32), {j.x, 1.4f, j.z}, kLacquer, Quat::axisAngle({1, 0, 0}, engine::kPi / 2));
  b.add(makeBox({0.7f, 0.45f, 0.02f}), {j.x, 1.05f, j.z + 0.31f}, kGlowGlass).tint({1.0f, 0.55f, 0.2f}).glow(2.0f);
  b.add(makeBox({0.6f, 0.04f, 0.02f}), {j.x, 0.6f, j.z + 0.31f}, kGlowGlass).tint({0.6f, 0.3f, 1.0f}).glow(3.0f);
  b.block(j, 0.5f, 0.35f);
  b.light({j.x, 1.2f, j.z + 0.7f}, {0.7f, 0.35f, 1.0f}, 1.0f, 3.5f, 0.3f);
  b.scene.interactables.push_back({"JUKEBOX", "Jukebox", {j.x, 2.1f, j.z}, {j.x - 0.5f, 0, j.z - 0.35f},
                                   {j.x + 0.5f, 1.9f, j.z + 0.35f}, jukeboxFirst,
                                   b.scene.objects.size() - jukeboxFirst, Action::kJukebox});
  b.scene.jukeboxSpeaker = {j.x, 1.1f, j.z + 0.3f};

  // Two mirrors on the right wall: a full one in a brass frame, and an
  // avatars-only one in a black frame (VRChat style).
  addMirror(b, {kRoomHalfX - 0.03f, 1.45f, -2.2f}, MirrorMode::kFull, kBrass, "MIRROR");
  addMirror(b, {kRoomHalfX - 0.03f, 1.45f, 1.1f}, MirrorMode::kAvatarsOnly, kBlackSteel, "AVATAR MIRROR");
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
