#pragma once
// You: the camera and walking, picking things up and putting them down,
// and your avatar's pose this frame (moved out of main.cpp).
#include "app/app_state.h"

namespace barapp {

void updateCamera(float dt);
void putDownOrThrow(Vec3 dir);
bool tryPickUp(Vec3 dir);
void updateSelfMotion(double nowMs);

}  // namespace barapp
