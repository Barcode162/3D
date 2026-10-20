#pragma once
// What the scene renderer draws (looped/scene_renderer.h), described without
// any game: shapes with their look, placed .glb models, point lights, mirrors,
// rooms and the doorways between them, the air (ambient light, fog) and the
// surface textures. A game builds one from its own layout (the bar:
// bar/bar_scene.cpp → renderSceneOf).
#include <functional>
#include <string>
#include <vector>

#include "looped/culling.h"
#include "looped/math.h"
#include "looped/mesh_data.h"
#include "looped/post.h"
#include "looped/procedural.h"

namespace engine {

// A shape drawn with one look: base color, how metallic and rough it is, a
// surface texture (a layer of RenderScene::textures, -1 = none) repeating
// `textureScale` times per metre, and a glow of its own (neon, bulbs).
struct DrawObject {
  MeshData mesh;
  Vec3 position;
  Quat rotation;
  Vec3 color{0.8f, 0.8f, 0.8f};
  float metallic = 0;
  float roughness = 0.6f;
  int texture = -1;
  float emissive = 0;
  float textureScale = 1;
};

// A .glb model from assets/models/. `room` (an index into RenderScene::rooms,
// -1 = none) makes it load only when you come near that room. Without a
// `critical` model there's nothing to enter (the loading screen says so).
struct PlacedModel {
  std::string file;  // e.g. "bar_counter.glb"
  Vec3 position;
  Quat rotation;
  float scale = 1;
  int room = -1;
  bool critical = false;
};

struct PointLight {
  Vec3 position;
  Vec3 color;
  float intensity;
  float range;
  float radius = 0.1f;  // size of the glowing part: bigger = softer, wider shine
};

// A mirror on a wall. Like VRChat's mirror options: kFull reflects the whole
// room; kAvatarsOnly shows just avatars on a plain backdrop (much cheaper).
enum class MirrorMode { kFull, kAvatarsOnly };

struct Mirror {
  Vec3 center;  // middle of the glass surface
  Vec3 normal;  // which way the glass faces (into the room)
  Vec3 right, up;  // unit directions along the glass
  float halfWidth, halfHeight;
  MirrorMode mode;
  bool on = true;
  // The game's lit buttons that show the mode (placed models; -1 = none):
  // full, avatars only, off. The renderer doesn't use them.
  int buttonModels[3] = {-1, -1, -1};
};

// Two rooms joined by an opening: a room is drawn only when you're in it or
// can see into it through a doorway, and then only what shows through it.
struct Doorway {
  int a = 0, b = 0;      // the rooms it joins (indices into RenderScene::rooms)
  Vec3 corners[4];       // the opening, a rectangle (a little generous)
};

struct RenderScene {
  std::vector<DrawObject> objects;
  std::vector<PlacedModel> models;
  std::vector<PointLight> lights;
  size_t mirrorCount = 0;  // the mirrors themselves come with each frame
  // Anything not wholly inside one room (walls between rooms, things in
  // doorways) is always considered.
  std::vector<Aabb> rooms;
  std::vector<Doorway> doorways;
  Vec3 lightGridMin, lightGridMax;  // floor area the light grid covers (x, z)
  Vec3 ambient;
  Vec3 fogColor;
  float fogDensity = 0;
  Vec3 spawnPosition;  // models of the room you start in load first
  // Makes the surface textures, one layer each (DrawObject::texture). Called
  // once the model downloads have started (making them takes a moment).
  std::function<std::vector<MaterialImages>()> makeTextures;
  // How often the textures repeat on the avatars, per metre.
  float avatarTextureScale = 1;
  // Glow and mood after the scene (looped/post.h).
  Mood mood;
};

}  // namespace engine
