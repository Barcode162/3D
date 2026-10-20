#pragma once
// Things you can pick up and throw (glasses...), and building the physics
// world from the bar's layout.
#include <vector>

#include "bar/bar_scene.h"
#include "looped/physics.h"

namespace app {

// Fills `world` with the bar's fixed surfaces and one body per movable
// model. `bodyOfModel[i]` is the body for BarScene::models[i] (-1 = fixed).
void buildPhysics(const BarScene& scene, engine::PhysicsWorld& world, std::vector<int>& bodyOfModel);

}  // namespace app
