#pragma once
// The car's look, built in code for now (a Blender model replaces it in the
// polish step; the physics doesn't care). Car space like looped/vehicle.h:
// x = right, y = up, -z = forward, origin = center of mass (0.5 m up).
#include "bar/projects/racing/cars.h"
#include "bar/projects/racing/mesh.h"
#include "looped/vehicle.h"

namespace app::race {

// Body, cabin glass, headlights. Paint parts use kCarPaint (tinted per car).
// Built on the GT's shape, then scaled to `style` (each model's spec has its
// axles scaled the same way, so the wheels sit in the arches).
MeshBuilder buildCarBody(const CarStyle& style);
// Tail lights alone (their glow changes when braking).
MeshBuilder buildTailLights(const CarStyle& style);
// One wheel (tire, rim, spokes) centered at the origin, axle along x, the
// rim face on +x (mirror it for the left side).
MeshBuilder buildWheel(float radius);

}  // namespace app::race
