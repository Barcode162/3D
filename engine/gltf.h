#pragma once
// Reads glTF 2.0 binary model files (.glb), the standard format Blender and
// most 3D tools export. Written from scratch: the JSON part is read with
// engine/json.h, geometry is copied out of the binary chunk, and the whole
// node tree is flattened so every primitive is already in model space.
//
// Supported: triangle meshes with positions, normals, texture coordinates
// and tangents (computed if missing), PBR metal/roughness materials with
// base color, metal/roughness, normal, occlusion and emissive values
// (with KHR_materials_emissive_strength).
// Not (yet): animation, skins, morph targets, cameras, lights.
#include <cstdint>
#include <string>
#include <vector>

#include "engine/math.h"

namespace engine {

struct ModelVertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  Vec4 tangent;  // xyz = direction of +u on the surface, w = handedness (+1/-1)
};

struct ModelPrimitive {
  std::vector<ModelVertex> vertices;
  std::vector<uint32_t> indices;
  int material = -1;  // index into ModelData::materials, -1 = default
};

// A glTF material. Image indices point into ModelData::images (-1 = none).
struct ModelMaterial {
  std::string name;
  float baseColor[4] = {1, 1, 1, 1};
  float metallic = 1, roughness = 1;
  Vec3 emissive;
  float normalScale = 1, occlusionStrength = 1;
  int baseColorImage = -1, metalRoughImage = -1, normalImage = -1, occlusionImage = -1;
  bool blend = false;  // see-through (glTF alphaMode BLEND), e.g. glass
};

struct ModelImage {
  std::vector<uint8_t> bytes;  // the PNG/JPEG file as stored
  std::string mimeType;
};

struct ModelData {
  std::vector<ModelPrimitive> primitives;
  std::vector<ModelMaterial> materials;
  std::vector<ModelImage> images;
  Vec3 boundsMin, boundsMax;
};

// Parses a .glb file. Returns false and fills `error` if it can't.
bool parseGlb(const std::vector<uint8_t>& file, ModelData& out, std::string& error);

}  // namespace engine
