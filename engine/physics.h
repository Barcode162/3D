#pragma once
// Rigid-body physics, written from scratch.
//
// - The world is made of fixed boxes (walls, floor, counter, tables...).
// - Moving bodies are built from a few spheres glued together ("compound
//   spheres"). A glass is rings of small spheres: it can stand on its base,
//   tip over, roll on its side and clink into other glasses. Sphere contacts
//   are cheap and very stable, which suits a phone.
// - Each body and box has an engine::Material, so weight (density),
//   friction and bounciness come from what things are made of.
// - Impacts are reported as events so the sound engine can play them.
//
// The solver: many small substeps per frame, each applying contact impulses
// (with friction) and pushing overlapping shapes apart.
#include <vector>

#include "engine/material.h"
#include "engine/math.h"

namespace engine {

struct StaticBox {
  Vec3 center;
  Vec3 half;       // half size along the box's own axes
  Quat rotation;
  const Material* material = nullptr;
};

struct BodySphere {
  Vec3 center;  // in body space
  float radius;
};

struct Body {
  // State
  Vec3 position;  // body origin (for a glass: bottom center)
  Quat rotation;
  Vec3 velocity, angularVelocity;
  bool asleep = false;

  // Shape and mass
  std::vector<BodySphere> spheres;
  Vec3 centerOfMass;  // in body space
  float boundRadius = 0;  // around the center of mass, covers all spheres
  float inverseMass = 0;
  Vec3 inverseInertia;  // diagonal, body space, about the center of mass
  const Material* material = nullptr;

  // Held in the player's hand: pulled toward `holdTarget`, no gravity.
  bool held = false;
  Vec3 holdTarget;

  float sleepTimer = 0;
  Vec3 worldCenter() const { return position + rotation.rotate(centerOfMass); }
};

// A hard hit, for impact sounds: where, how fast the surfaces met, and what
// they're made of (`other` is null for nothing/unknown).
struct Impact {
  Vec3 point;
  float speed;
  const Material* a;
  const Material* b;
};

class PhysicsWorld {
 public:
  static constexpr float kStep = 1.0f / 60.0f;  // fixed time step, seconds
  static constexpr int kSubsteps = 8;
  static constexpr int kMaxStepsPerFrame = 8;   // slow frames slow the world instead of breaking it

  void addStaticBox(const StaticBox& box) { statics_.push_back(box); }
  // Adds a body made of `spheres`. Mass comes from the material's density
  // times `volume` (the real solid volume, e.g. a glass's walls, in m^3).
  size_t addBody(Vec3 position, Quat rotation, std::vector<BodySphere> spheres, float volume,
                 const Material* material);

  // Advances the world by `dt` seconds of frame time, in fixed steps.
  void step(float dt);

  Body& body(size_t i) { return bodies_[i]; }
  const Body& body(size_t i) const { return bodies_[i]; }
  size_t bodyCount() const { return bodies_.size(); }

  // The nearest body whose bounding sphere the ray hits (-1 = none), and the
  // distance along the ray.
  int raycastBodies(Vec3 origin, Vec3 dir, float maxDist, float* hitDist) const;
  // The nearest fixed box the ray hits: returns false if none.
  bool raycastStatic(Vec3 origin, Vec3 dir, float maxDist, float* hitDist, Vec3* hitNormal) const;

  // Impacts since the last call.
  std::vector<Impact> takeImpacts();

 private:
  void substep(float h);
  void collideStatic(Body& b, float h);
  void collidePair(Body& a, Body& b);
  void applyContact(Body& a, Body* b, Vec3 point, Vec3 normal, float depth, const Material* otherMaterial);
  void updateSleep(Body& b, float dt);

  std::vector<StaticBox> statics_;
  std::vector<Body> bodies_;
  std::vector<Impact> impacts_;
  float pending_ = 0;  // frame time not yet simulated
};

}  // namespace engine
