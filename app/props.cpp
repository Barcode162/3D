#include "app/props.h"

#include <algorithm>
#include <cmath>

namespace app {
namespace {

using engine::BodySphere;
using engine::Vec3;

// A glass as rings of small spheres following its outline, from the base
// (radius rBottom at y = 0) to the rim (rTop at `height`).
std::vector<BodySphere> glassSpheres(float rBottom, float rTop, float height, int rings, int perRing) {
  const float s = 0.008f;  // sphere radius: about the wall thickness plus a bit
  std::vector<BodySphere> out;
  for (int ring = 0; ring < rings; ++ring) {
    const float t = static_cast<float>(ring) / static_cast<float>(rings - 1);
    const float y = s + t * (height - 2 * s);
    const float r = rBottom + (rTop - rBottom) * (y / height) - s;
    for (int k = 0; k < perRing; ++k) {
      const float a = engine::kPi * 2 * (static_cast<float>(k) + 0.5f * static_cast<float>(ring % 2)) /
                      static_cast<float>(perRing);
      out.push_back({{r * std::cos(a), y, r * std::sin(a)}, s});
    }
  }
  out.push_back({{0, s, 0}, s});  // fill the middle of the base
  return out;
}

// Walls + base of a glass, in cubic meters, for its mass.
float glassVolume(float rBottom, float rTop, float height, float wall, float base) {
  const float rMid = (rBottom + rTop) / 2;
  return engine::kPi * rBottom * rBottom * base + 2 * engine::kPi * rMid * height * wall;
}

// A fixed box from a scene object: its mesh's bounds, turned with it.
engine::StaticBox boxFor(const SceneObject& o) {
  Vec3 lo = o.mesh.vertices[0].position, hi = lo;
  for (const engine::Vertex& v : o.mesh.vertices) {
    lo = {std::min(lo.x, v.position.x), std::min(lo.y, v.position.y), std::min(lo.z, v.position.z)};
    hi = {std::max(hi.x, v.position.x), std::max(hi.y, v.position.y), std::max(hi.z, v.position.z)};
  }
  const Vec3 localCenter = (lo + hi) * 0.5f;
  return {o.position + o.rotation.rotate(localCenter), (hi - lo) * 0.5f, o.rotation, &material(o.material)};
}

}  // namespace

void buildPhysics(const BarScene& scene, engine::PhysicsWorld& world, std::vector<int>& bodyOfModel) {
  for (const SceneObject& o : scene.objects) {
    if (!o.mesh.vertices.empty()) world.addStaticBox(boxFor(o));
  }
  for (const Collider& c : scene.colliders) {
    world.addStaticBox({c.center, c.half, c.rotation, &material(c.material)});
  }
  // The room: everything fixed, plus a margin. A body outside it (or NaN)
  // is put back where it last was fine.
  engine::Aabb room = engine::Aabb::empty();
  const auto addCorners = [&](Vec3 center, Vec3 half, engine::Quat rot) {
    for (int k = 0; k < 8; ++k) {
      const Vec3 corner{k & 1 ? half.x : -half.x, k & 2 ? half.y : -half.y, k & 4 ? half.z : -half.z};
      room.add(center + rot.rotate(corner));
    }
  };
  for (const SceneObject& o : scene.objects) {
    if (o.mesh.vertices.empty()) continue;
    const engine::StaticBox b = boxFor(o);
    addCorners(b.center, b.half, b.rotation);
  }
  for (const Collider& c : scene.colliders) addCorners(c.center, c.half, c.rotation);
  room.min -= Vec3{0.5f, 0.2f, 0.5f};
  room.min.y = -0.05f;  // the floor is at y = 0: below it means it fell through
  room.max += Vec3{0.5f, 0.5f, 0.5f};
  world.setBounds(room);
  bodyOfModel.assign(scene.models.size(), -1);
  for (size_t i = 0; i < scene.models.size(); ++i) {
    const ModelPlacement& m = scene.models[i];
    std::vector<BodySphere> spheres;
    float volume = 0;
    switch (m.prop) {
      case Prop::kNone: continue;
      case Prop::kPintGlass:
        spheres = glassSpheres(0.030f, 0.043f, 0.150f, 4, 8);
        volume = glassVolume(0.030f, 0.043f, 0.150f, 0.0028f, 0.012f);
        break;
      case Prop::kRocksGlass:
        spheres = glassSpheres(0.040f, 0.040f, 0.090f, 3, 8);
        volume = glassVolume(0.040f, 0.040f, 0.090f, 0.0035f, 0.018f);
        break;
    }
    bodyOfModel[i] = static_cast<int>(world.addBody(m.position, m.rotation, std::move(spheres), volume,
                                                    &material(m.material)));
  }
}

}  // namespace app
