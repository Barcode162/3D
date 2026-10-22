#include "looped/scene_geometry.h"

#include <cmath>

#include "looped/json.h"

namespace engine {

namespace {
Vec3 vec3Of(const Json& j, Vec3 fallback) {
  if (!j.isArray() || j.size() < 3) return fallback;
  return {static_cast<float>(j[0].number()), static_cast<float>(j[1].number()), static_cast<float>(j[2].number())};
}
// A material's value from glTF alone: the colour factor, as if its texture
// averaged mid grey (textures are usually lighter than that; the table knows).
GeoMaterial fromGltf(const ModelMaterial& m) {
  GeoMaterial g;
  const float t = m.baseColorImage >= 0 ? 0.35f : 1.0f;
  g.albedo = {m.baseColor[0] * t, m.baseColor[1] * t, m.baseColor[2] * t};
  g.emission = m.emissive;
  g.metal = m.metallic;
  g.seeThrough = m.blend;
  return g;
}
}  // namespace

bool AlbedoTable::parse(const std::string& text, std::string* error) {
  const Json root = Json::parse(text, error);
  if (!root.isObject()) return false;
  for (const auto& [file, mats] : root.members()) {
    auto& slot = models_[file];
    for (const auto& [name, v] : mats.members()) {
      Entry e;
      e.fix = static_cast<float>(v["fix"].number(1));
      e.m.albedo = vec3Of(v["albedo"], {0.5f, 0.5f, 0.5f}) * e.fix;
      e.m.emission = vec3Of(v["emissive"], {});
      e.m.metal = static_cast<float>(v["metal"].number(0));
      e.m.seeThrough = v["seeThrough"].boolean(false);
      slot[name] = e;
    }
  }
  return true;
}

GeoMaterial AlbedoTable::lookup(const std::string& file, const ModelMaterial& m) const {
  const auto f = models_.find(file);
  if (f != models_.end()) {
    const auto e = f->second.find(m.name);
    if (e != f->second.end()) return e->second.m;
  }
  return fromGltf(m);
}

float AlbedoTable::fix(const std::string& file, const std::string& material) const {
  const auto f = models_.find(file);
  if (f == models_.end()) return 1;
  const auto e = f->second.find(material);
  return e == f->second.end() ? 1 : e->second.fix;
}

void SceneGeometry::add(const ModelData& model, const Mat4& world, const std::string& file, const AlbedoTable& table,
                        uint32_t modelId) {
  // The model's materials, once each (plus the default for parts without one).
  const auto firstMaterial = static_cast<uint32_t>(materials.size());
  for (const ModelMaterial& m : model.materials) materials.push_back(table.lookup(file, m));
  materials.push_back(GeoMaterial{});
  const auto defaultMaterial = static_cast<uint32_t>(materials.size() - 1);
  for (const ModelPrimitive& p : model.primitives) {
    const bool has = p.material >= 0 && static_cast<size_t>(p.material) < model.materials.size();
    const uint32_t mat = has ? firstMaterial + static_cast<uint32_t>(p.material) : defaultMaterial;
    if (materials[mat].seeThrough) continue;
    const auto base = static_cast<uint32_t>(positions.size());
    for (const ModelVertex& v : p.vertices) {
      const Vec4 w = transformPoint(world, v.position);
      positions.push_back({w.x, w.y, w.z});
    }
    for (size_t i = 0; i + 2 < p.indices.size(); i += 3) {
      indices.push_back(base + p.indices[i]);
      indices.push_back(base + p.indices[i + 1]);
      indices.push_back(base + p.indices[i + 2]);
      triMaterial.push_back(mat);
      triModel.push_back(modelId);
    }
  }
}

Vec3 SceneGeometry::normal(uint32_t tri) const {
  const Vec3 a = corner(tri, 0), b = corner(tri, 1), c = corner(tri, 2);
  const Vec3 n = cross(b - a, c - a);
  const float l = length(n);
  return l > 1e-12f ? n * (1.0f / l) : Vec3{0, 1, 0};
}

float SceneGeometry::area(uint32_t tri) const {
  const Vec3 a = corner(tri, 0), b = corner(tri, 1), c = corner(tri, 2);
  return 0.5f * length(cross(b - a, c - a));
}

}  // namespace engine
