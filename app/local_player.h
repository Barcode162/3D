#pragma once
// You: the camera and walking, picking things up and putting them down,
// and your avatar's pose this frame (moved out of main.cpp).
#include "app/app_state.h"

namespace barapp {

void updateCamera(float dt);
void putDownOrThrow(Vec3 dir);
int pickableAt(Vec3 dir, float* dist);
void pickUp(int body);
void teleport(Vec3 eye, float yaw);
void updateSelfMotion(double nowMs);

}  // namespace barapp
