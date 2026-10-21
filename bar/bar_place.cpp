#include "bar/bar_place.h"

#include <cstdio>
#include <unordered_map>

namespace app {

namespace {

using engine::Vec3;
using engine::world::Library;
using engine::world::Place;
using engine::world::ThingSpec;

// Lights with a loop of their own, by index in the bar's layout (as the
// first loops had them; tools/native/bar_world_test.cpp checks they're
// still where these names say).
constexpr int kGrooveLights[] = {7, 8, 9};             // the ceiling over the floor
constexpr int kNeonWave[] = {22, 24, 23, 25};          // the arcade, in wave order
constexpr int kArcadeSignLight = 28;                   // by the ARCADE sign

// The small living things, around a point: how many, of which kind, and
// whether they sit on the nearest model (moths sway with their lamp).
struct Extra {
  const char* kind;
  Vec3 center;
  int count;
  bool onModel;
};
constexpr Extra kExtras[] = {
    {"moth", {-4.3f, 2.4f, -4.6f}, 3, true},  // the second lamp over the counter
    {"moth", {0.2f, 2.4f, -4.6f}, 2, true},   // the last lamp over the counter
    {"ember", {-8.15f, 0.95f, 0.8f}, 6, false},  // over the fire (not on the flames: they stretch)
};

std::string at(const char* prefix, Vec3 p) {
  char buf[96];
  std::snprintf(buf, sizeof buf, "%s@%.1f,%.1f", prefix, p.x, p.z);
  return buf;
}

engine::Json slot(const char* what, size_t i) {
  return engine::Json::parse("{\"" + std::string(what) + "\": " + std::to_string(i) + "}");
}

}  // namespace

std::string kindForModel(const Library& kinds, const std::string& file) {
  std::string name = file.substr(0, file.rfind('.'));
  while (!name.empty()) {
    if (kinds.find(name)) return name;
    const size_t cut = name.rfind('_');
    if (cut == std::string::npos) break;
    name.resize(cut);
  }
  return "";
}

std::string kindForLight(const BarScene& scene, size_t light, float* phase) {
  *phase = 0;
  const int l = static_cast<int>(light);
  if (l == scene.fireLight) return "light-fire";
  for (int g : kGrooveLights)
    if (g == l) return "light-groove";
  for (size_t k = 0; k < std::size(kNeonWave); ++k)
    if (kNeonWave[k] == l) {
      *phase = static_cast<float>(k) / static_cast<float>(std::size(kNeonWave));
      return "light-neon-wave";
    }
  if (l == kArcadeSignLight) return "light-sign";
  // The jukebox's own glow: the light within 1.5 m of it.
  for (const ModelPlacement& m : scene.models)
    if (m.file == "jukebox.glb" && engine::distance(m.position, scene.lights[light].position) < 1.5f)
      return "light-jukebox";
  const Vec3 c = scene.lights[light].color;
  return c.z < 0.5f ? "light-bulb" : "light-neon";  // warm, or coloured
}

Place barPlace(const BarScene& scene, const Library& kinds) {
  Place place;
  place.id = "bar";
  place.about = "The bar (from its layout, bar/bar_scene.cpp).";
  std::unordered_map<std::string, int> used;
  auto unique = [&](std::string id) {
    const int n = ++used[id];
    return n == 1 ? id : id + "#" + std::to_string(n);
  };
  std::vector<std::string> modelIds(scene.models.size());
  for (size_t i = 0; i < scene.models.size(); ++i) {
    const ModelPlacement& m = scene.models[i];
    ThingSpec t;
    t.kind = kindForModel(kinds, m.file);
    if (t.kind.empty()) t.kind = m.file;  // reported as a missing kind
    t.id = modelIds[i] = unique(at(t.kind.c_str(), m.position));
    t.at = m.position;
    t.rotation = m.rotation;
    t.scale = {m.scale, m.scale, m.scale};
    t.parts.emplace_back("slot", slot("model", i));
    place.things.push_back(std::move(t));
  }
  for (size_t i = 0; i < scene.lights.size(); ++i) {
    ThingSpec t;
    t.kind = kindForLight(scene, i, &t.phase);
    t.id = unique(at(t.kind.c_str(), scene.lights[i].position));
    t.at = scene.lights[i].position;
    t.parts.emplace_back("slot", slot("light", i));
    place.things.push_back(std::move(t));
  }
  for (const Extra& e : kExtras) {
    // The model it sits on: the nearest one within 1.5 m.
    int parent = -1;
    float best = 1.5f;
    if (e.onModel)
      for (size_t i = 0; i < scene.models.size(); ++i) {
        const float d = engine::distance(scene.models[i].position, e.center);
        if (d < best) best = d, parent = static_cast<int>(i);
      }
    for (int k = 0; k < e.count; ++k) {
      ThingSpec t;
      t.kind = e.kind;
      t.id = unique(at(e.kind, e.center) + "-" + std::to_string(k + 1));
      if (parent >= 0) {
        const ModelPlacement& m = scene.models[static_cast<size_t>(parent)];
        t.parent = modelIds[static_cast<size_t>(parent)];
        t.at = engine::conjugate(m.rotation).rotate(e.center - m.position) * (1.0f / m.scale);
      } else {
        t.at = e.center;
      }
      place.things.push_back(std::move(t));
    }
  }
  return place;
}

}  // namespace app
