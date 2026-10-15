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
  kTexFabric,  // avatar shirt and socks
  kTexDenim,   // avatar jeans
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

// Something you can use. It has a floating name tag; walking close and
// looking at it highlights it and shows a prompt; tapping it runs `action`.
enum class Action { kNone, kJukebox, kMirror };

struct Interactable {
  std::string name;               // floating name tag
  std::string prompt;             // shown when you're at it (if no action overrides it)
  engine::Vec3 tagPosition;
  engine::Vec3 boundsMin, boundsMax;  // world box that taps hit
  size_t firstObject = 0, objectCount = 0;  // parts that light up when focused
  Action action = Action::kNone;
  int mirror = -1;  // for Action::kMirror: index into BarScene::mirrors
};

// A mirror on a wall. Like VRChat's mirror options: kFull reflects the whole
// room; kAvatarsOnly shows just avatars on a plain backdrop (much cheaper).
enum class MirrorMode { kFull, kAvatarsOnly };

struct Mirror {
  engine::Vec3 center;  // middle of the glass surface
  engine::Vec3 normal;  // which way the glass faces (into the room)
  engine::Vec3 right, up;  // unit directions along the glass
  float halfWidth, halfHeight;
  MirrorMode mode;
  bool on = true;
};

struct BarScene {
  std::vector<SceneObject> objects;
  std::vector<PointLight> lights;
  std::vector<Blocker> blockers;
  std::vector<Interactable> interactables;
  std::vector<Mirror> mirrors;
  engine::Vec3 ambient;
  engine::Vec3 fogColor;
  float fogDensity;
  engine::Vec3 jukeboxSpeaker;  // where the music comes from
  engine::Vec3 spawnPosition;
  float spawnYaw;
};

BarScene buildBarScene();

// Generates the surface textures (one per Material, in order).
std::vector<engine::MaterialImages> buildBarMaterials();

// Moves a walker (circle of `radius` on the floor) out of any blocker.
engine::Vec3 resolveCollisions(const BarScene& scene, engine::Vec3 position, float radius);

}  // namespace app
