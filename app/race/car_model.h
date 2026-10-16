#pragma once
// The car's look, built in code for now (a Blender model replaces it in the
// polish step; the physics doesn't care). Car space like engine/vehicle.h:
// x = right, y = up, -z = forward, origin = center of mass (0.5 m up).
#include "app/race/mesh.h"
#include "engine/vehicle.h"

namespace app::race {

// Body, cabin glass, headlights. Paint parts use kCarPaint (tinted per car).
MeshBuilder buildCarBody(const engine::CarSpec& spec);
// Tail lights alone (their glow changes when braking).
MeshBuilder buildTailLights();
// One wheel (tire, rim, spokes) centered at the origin, axle along x, the
// rim face on +x (mirror it for the left side).
MeshBuilder buildWheel(float radius);

}  // namespace app::race
