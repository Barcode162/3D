#pragma once
// The bar's layout: which shapes go where, their colors, the lights and the
// walls/furniture you can't walk through. Pure data, no GPU code.
#include <string>
#include <vector>

#include "engine/math.h"
#include "engine/mesh.h"
#include "engine/procedural.h"

namespace app {

// Surface textures, one layer each in the GPU texture array (same order as
// buildBarMaterials()).
enum Material : int {
  kNoTexture = -1,
  kTexWoodPlanks,
  kTexWoodGrain,
  kTexBrick,
  kTexLeather,
  kTexPlaster,
  kMaterialCount,
};

struct SceneObject {
  engine::MeshData mesh;
  engine::Vec3 position;
  engine::Quat rotation;
  engine::Vec3 color;
  float emissive = 0;   // glows by itself (neon, bulbs)
  float shininess = 0;  // 0 = matte, 1 = glossy
  Material texture = kNoTexture;
  float textureScale = 1;  // repeats per meter

  SceneObject& textured(Material m, float scale) {
    texture = m;
    textureScale = scale;
    return *this;
  }
};

struct PointLight {
  engine::Vec3 position;
  engine::Vec3 color;
  float intensity;
  float range;
};

// Axis-aligned rectangle on the floor (x/z) that blocks walking.
struct Blocker {
  float minX, minZ, maxX, maxZ;
};

// A floating name tag over something you'll be able to use. Walking close
// and looking at it shows `prompt` on screen.
struct Label {
  engine::Vec3 position;
  std::string text;
  std::string prompt;
};

struct BarScene {
  std::vector<SceneObject> objects;
  std::vector<PointLight> lights;
  std::vector<Blocker> blockers;
  std::vector<Label> labels;
  engine::Vec3 ambient;
  engine::Vec3 fogColor;
  float fogDensity;
  engine::Vec3 spawnPosition;
  float spawnYaw;
};

BarScene buildBarScene();

// Generates the surface textures (one per Material, in order).
std::vector<engine::MaterialImages> buildBarMaterials();

// Moves a walker (circle of `radius` on the floor) out of any blocker.
engine::Vec3 resolveCollisions(const BarScene& scene, engine::Vec3 position, float radius);

}  // namespace app
