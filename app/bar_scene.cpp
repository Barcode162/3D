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

// Palette
constexpr Vec3 kFloorWood{0.16f, 0.09f, 0.05f};
constexpr Vec3 kDarkWood{0.20f, 0.10f, 0.05f};
constexpr Vec3 kPolishedWood{0.38f, 0.18f, 0.08f};
// Brick colors come from the texture itself; mid-gray keeps them as is.
constexpr Vec3 kBrickTint{0.5f, 0.5f, 0.5f};
constexpr float kBrickScale = 1.0f / 0.9f;  // one brick tile = 0.9 m
constexpr Vec3 kCeiling{0.07f, 0.055f, 0.05f};
constexpr Vec3 kLeather{0.45f, 0.06f, 0.05f};
constexpr Vec3 kChrome{0.65f, 0.66f, 0.70f};
constexpr Vec3 kBrass{0.80f, 0.58f, 0.24f};
constexpr Vec3 kWarmBulb{1.0f, 0.68f, 0.38f};
constexpr Vec3 kNeonPink{1.0f, 0.18f, 0.55f};
constexpr Vec3 kNeonCyan{0.2f, 0.85f, 1.0f};

struct Builder {
  BarScene& scene;

  SceneObject& add(engine::MeshData mesh, Vec3 pos, Vec3 color, float shininess = 0.1f,
                   float emissive = 0.0f, Quat rot = {}) {
    scene.objects.push_back({std::move(mesh), pos, rot, color, emissive, shininess});
    return scene.objects.back();
  }

  void block(Vec3 center, float halfX, float halfZ) {
    scene.blockers.push_back({center.x - halfX, center.z - halfZ, center.x + halfX, center.z + halfZ});
  }

  void light(Vec3 pos, Vec3 color, float intensity, float range) {
    scene.lights.push_back({pos, color, intensity, range});
  }
};

void buildRoom(Builder& b) {
  const float w = kRoomHalfX * 2, d = kRoomHalfZ * 2, t = 0.2f;
  b.add(makeBox({w, 0.1f, d}), {0, -0.05f, 0}, kFloorWood, 0.35f).textured(kTexWoodPlanks, 0.8f);
  b.add(makeBox({w, 0.1f, d}), {0, kRoomHeight + 0.05f, 0}, kCeiling).textured(kTexPlaster, 0.4f);
  b.add(makeBox({w, kRoomHeight, t}), {0, kRoomHeight / 2, -kRoomHalfZ - t / 2}, kBrickTint).textured(kTexBrick, kBrickScale);
  b.add(makeBox({w, kRoomHeight, t}), {0, kRoomHeight / 2, kRoomHalfZ + t / 2}, kBrickTint).textured(kTexBrick, kBrickScale);
  b.add(makeBox({t, kRoomHeight, d}), {-kRoomHalfX - t / 2, kRoomHeight / 2, 0}, kBrickTint).textured(kTexBrick, kBrickScale);
  b.add(makeBox({t, kRoomHeight, d}), {kRoomHalfX + t / 2, kRoomHeight / 2, 0}, kBrickTint).textured(kTexBrick, kBrickScale);

  // Dark wood paneling on the lower part of the side and front walls.
  const float panel = 1.0f, inset = 0.03f;
  b.add(makeBox({w, panel, inset}), {0, panel / 2, kRoomHalfZ - inset / 2}, kDarkWood, 0.2f).textured(kTexWoodGrain, 1);
  b.add(makeBox({inset, panel, d}), {-kRoomHalfX + inset / 2, panel / 2, 0}, kDarkWood, 0.2f).textured(kTexWoodGrain, 1);
  b.add(makeBox({inset, panel, d}), {kRoomHalfX - inset / 2, panel / 2, 0}, kDarkWood, 0.2f).textured(kTexWoodGrain, 1);

  // Entrance door on the front wall.
  b.add(makeBox({1.1f, 2.2f, 0.06f}), {-3.5f, 1.1f, kRoomHalfZ - 0.03f}, {0.12f, 0.06f, 0.03f}, 0.3f)
      .textured(kTexWoodGrain, 1.2f);
  b.add(makeSphere(0.04f, 12, 8), {-3.1f, 1.05f, kRoomHalfZ - 0.08f}, kBrass, 0.9f);

  // Walls block walking.
  b.block({0, 0, -kRoomHalfZ - 0.5f}, kRoomHalfX + 1, 0.5f);
  b.block({0, 0, kRoomHalfZ + 0.5f}, kRoomHalfX + 1, 0.5f);
  b.block({-kRoomHalfX - 0.5f, 0, 0}, 0.5f, kRoomHalfZ + 1);
  b.block({kRoomHalfX + 0.5f, 0, 0}, 0.5f, kRoomHalfZ + 1);
}

void buildBar(Builder& b) {
  const float barZ = -3.2f, barLen = 7.0f;
  b.add(makeBox({barLen, 1.05f, 0.7f}), {0, 0.525f, barZ}, kDarkWood, 0.25f).textured(kTexWoodGrain, 1);
  b.add(makeBox({barLen + 0.3f, 0.06f, 0.9f}), {0, 1.08f, barZ + 0.05f}, kPolishedWood, 0.9f)
      .textured(kTexWoodGrain, 0.7f);
  // Brass foot rail along the front.
  b.add(makeCylinder(0.025f, barLen, 16), {0, 0.2f, barZ + 0.48f}, kBrass, 0.85f, 0,
        Quat::axisAngle({0, 0, 1}, engine::kPi / 2));
  // Counter plus the bartender's area behind it.
  b.block({0, 0, (barZ + 0.5f + -kRoomHalfZ) / 2}, barLen / 2 + 0.15f,
          (barZ + 0.5f + kRoomHalfZ) / 2);

  // Stools in front of the bar.
  for (int i = 0; i < 6; ++i) {
    const float x = -2.5f + static_cast<float>(i);
    const float z = barZ + 0.95f;
    b.add(makeCylinder(0.2f, 0.08f), {x, 0.78f, z}, kLeather, 0.45f).textured(kTexLeather, 4);
    b.add(makeCylinder(0.035f, 0.74f, 12), {x, 0.37f, z}, kChrome, 0.9f);
    b.add(makeCylinder(0.18f, 0.03f), {x, 0.015f, z}, kChrome, 0.9f);
    b.add(makeCylinder(0.15f, 0.02f, 16), {x, 0.3f, z}, kChrome, 0.9f);  // foot ring
  }

  // Back bar: three backlit shelves of bottles against the back wall.
  const float shelfZ = -kRoomHalfZ + 0.2f;
  const Vec3 bottleColors[] = {
      {0.55f, 0.25f, 0.05f}, {0.1f, 0.35f, 0.12f}, {0.6f, 0.6f, 0.55f},
      {0.45f, 0.08f, 0.1f},  {0.7f, 0.45f, 0.1f},  {0.15f, 0.25f, 0.45f},
  };
  for (int s = 0; s < 3; ++s) {
    const float y = 1.3f + 0.45f * static_cast<float>(s);
    b.add(makeBox({6.0f, 0.04f, 0.35f}), {0, y, shelfZ}, kDarkWood, 0.3f).textured(kTexWoodGrain, 1.5f);
    b.add(makeBox({6.0f, 0.015f, 0.02f}), {0, y - 0.03f, shelfZ + 0.16f}, kWarmBulb, 0, 1.2f);
    for (int i = 0; i < 16; ++i) {
      const float x = -2.7f + 0.36f * static_cast<float>(i) + 0.05f * static_cast<float>(s % 2);
      const Vec3 color = bottleColors[(i * 7 + s * 3) % 6];
      const float h = 0.24f + 0.06f * static_cast<float>((i + s) % 3);
      b.add(makeCylinder(0.04f, h, 12), {x, y + 0.02f + h / 2, shelfZ}, color, 0.95f, 0.25f);
      b.add(makeCylinder(0.014f, 0.08f, 8), {x, y + 0.02f + h + 0.04f, shelfZ}, color, 0.95f, 0.25f);
    }
  }
  b.light({0, 1.9f, shelfZ + 0.9f}, kWarmBulb, 0.9f, 4.0f);

  // Pendant lamps over the counter.
  for (int i = -1; i <= 1; ++i) {
    const float x = 2.4f * static_cast<float>(i);
    b.add(makeSphere(0.08f), {x, 2.28f, barZ}, kWarmBulb, 0, 8.0f);
    b.add(makeCylinder(0.2f, 0.18f), {x, 2.42f, barZ}, {0.08f, 0.08f, 0.08f}, 0.6f);
    b.add(makeCylinder(0.008f, 0.7f, 6), {x, 2.86f, barZ}, {0.05f, 0.05f, 0.05f});
    b.light({x, 2.2f, barZ}, kWarmBulb, 2.4f, 6.0f);
  }

  // Neon sign above the shelves: a pink frame with a cyan line inside.
  const float signY = 2.78f, signZ = -kRoomHalfZ + 0.05f;
  b.add(makeBox({2.2f, 0.04f, 0.04f}), {0, signY + 0.28f, signZ}, kNeonPink, 0, 3.0f);
  b.add(makeBox({2.2f, 0.04f, 0.04f}), {0, signY - 0.28f, signZ}, kNeonPink, 0, 3.0f);
  b.add(makeBox({0.04f, 0.6f, 0.04f}), {-1.1f, signY, signZ}, kNeonPink, 0, 3.0f);
  b.add(makeBox({0.04f, 0.6f, 0.04f}), {1.1f, signY, signZ}, kNeonPink, 0, 3.0f);
  b.add(makeBox({1.5f, 0.035f, 0.035f}), {0, signY, signZ}, kNeonCyan, 0, 3.0f);
  b.light({0, signY, signZ + 0.4f}, kNeonPink, 1.2f, 4.5f);
}

void buildSeating(Builder& b) {
  // Two round tables with stools on the right side.
  for (float z : {-0.2f, 2.4f}) {
    const Vec3 c{3.6f, 0, z};
    b.add(makeCylinder(0.45f, 0.05f, 32), {c.x, 0.76f, c.z}, kPolishedWood, 0.7f).textured(kTexWoodGrain, 1.5f);
    b.add(makeCylinder(0.05f, 0.74f, 12), {c.x, 0.37f, c.z}, {0.06f, 0.06f, 0.06f}, 0.5f);
    b.add(makeCylinder(0.25f, 0.03f), {c.x, 0.015f, c.z}, {0.06f, 0.06f, 0.06f}, 0.5f);
    b.add(makeSphere(0.035f, 12, 8), {c.x, 0.82f, c.z}, kWarmBulb, 0, 4.0f);  // candle glow
    b.block(c, 0.5f, 0.5f);
    for (float dx : {-0.8f, 0.8f}) {
      b.add(makeCylinder(0.19f, 0.07f), {c.x + dx, 0.5f, c.z}, kLeather, 0.45f).textured(kTexLeather, 4);
      b.add(makeCylinder(0.03f, 0.47f, 12), {c.x + dx, 0.235f, c.z}, kChrome, 0.9f);
    }
  }
  b.light({3.6f, 2.5f, 1.1f}, kWarmBulb, 1.6f, 6.0f);

  // A long leather booth against the left wall with two small tables.
  const float boothX = -kRoomHalfX + 0.35f, boothZ = 0.6f, boothLen = 3.6f;
  b.add(makeBox({0.65f, 0.45f, boothLen}), {boothX, 0.225f, boothZ}, kLeather, 0.4f).textured(kTexLeather, 3);
  b.add(makeBox({0.15f, 0.75f, boothLen}), {-kRoomHalfX + 0.1f, 0.8f, boothZ}, kLeather, 0.4f)
      .textured(kTexLeather, 3);
  b.block({boothX, 0, boothZ}, 0.35f, boothLen / 2);
  for (float z : {-0.3f, 1.5f}) {
    b.add(makeBox({0.75f, 0.05f, 0.75f}), {-4.75f, 0.74f, z}, kPolishedWood, 0.7f).textured(kTexWoodGrain, 1.5f);
    b.add(makeCylinder(0.05f, 0.72f, 12), {-4.75f, 0.36f, z}, {0.06f, 0.06f, 0.06f}, 0.5f);
    b.add(makeSphere(0.035f, 12, 8), {-4.75f, 0.8f, z}, kWarmBulb, 0, 4.0f);
    b.block({-4.75f, 0, z}, 0.42f, 0.42f);
  }
  b.light({-4.8f, 2.4f, 0.6f}, kWarmBulb, 1.5f, 6.0f);
}

void buildFeatures(Builder& b) {
  // Jukebox in the back-right corner.
  const Vec3 j{4.9f, 0, -4.45f};
  const size_t jukeboxFirst = b.scene.objects.size();
  b.add(makeBox({0.9f, 1.4f, 0.6f}), {j.x, 0.7f, j.z}, {0.32f, 0.06f, 0.08f}, 0.8f);
  b.add(makeCylinder(0.45f, 0.6f, 32), {j.x, 1.4f, j.z}, {0.32f, 0.06f, 0.08f}, 0.8f, 0,
        Quat::axisAngle({1, 0, 0}, engine::kPi / 2));
  b.add(makeBox({0.7f, 0.45f, 0.02f}), {j.x, 1.05f, j.z + 0.31f}, {1.0f, 0.55f, 0.2f}, 0, 2.0f);
  b.add(makeBox({0.6f, 0.04f, 0.02f}), {j.x, 0.6f, j.z + 0.31f}, {0.6f, 0.3f, 1.0f}, 0, 3.0f);
  b.block(j, 0.5f, 0.35f);
  b.light({j.x, 1.2f, j.z + 0.7f}, {0.7f, 0.35f, 1.0f}, 1.0f, 3.5f);
  b.scene.interactables.push_back({"JUKEBOX", "Jukebox", {j.x, 2.1f, j.z}, {j.x - 0.5f, 0, j.z - 0.35f},
                                   {j.x + 0.5f, 1.9f, j.z + 0.35f}, jukeboxFirst,
                                   b.scene.objects.size() - jukeboxFirst, Action::kJukebox});
  b.scene.jukeboxSpeaker = {j.x, 1.1f, j.z + 0.3f};

  // Mirror frame on the right wall (the reflection comes in Stage 5).
  const Vec3 m{kRoomHalfX - 0.03f, 1.45f, -2.2f};
  const size_t mirrorFirst = b.scene.objects.size();
  b.add(makeBox({0.04f, 2.1f, 1.5f}), m, {0.75f, 0.6f, 0.3f}, 0.9f);
  b.add(makeBox({0.02f, 1.95f, 1.35f}), {m.x - 0.02f, m.y, m.z}, {0.5f, 0.55f, 0.6f}, 1.0f);
  b.scene.interactables.push_back({"MIRROR", "Mirror\nReflections coming soon", {m.x - 0.3f, 2.65f, m.z},
                                   {m.x - 0.1f, m.y - 1.05f, m.z - 0.75f}, {m.x + 0.05f, m.y + 1.05f, m.z + 0.75f},
                                   mirrorFirst, b.scene.objects.size() - mirrorFirst, Action::kNone});
}

}  // namespace

std::vector<engine::MaterialImages> buildBarMaterials() {
  constexpr int kSize = 512;
  std::vector<engine::MaterialImages> m;
  m.push_back(engine::makeWoodPlanks(kSize, 11));  // kTexWoodPlanks
  m.push_back(engine::makeWoodGrain(kSize, 23));   // kTexWoodGrain
  m.push_back(engine::makeBrick(kSize, 37));       // kTexBrick
  m.push_back(engine::makeLeather(kSize, 41));     // kTexLeather
  m.push_back(engine::makePlaster(kSize, 53));     // kTexPlaster
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
