#pragma once
// On-screen text: the frame rate, prompts, name tags and pictures over
// heads, the machines' screens, banners and toasts (moved out of main.cpp).
#include "bar/app_state.h"

namespace barapp {

void updateFps(double nowMs);
std::string promptFor(const app::Interactable& it);
int pictureSlotFor(const app::OtherPlayer& o, double nowMs);
void simMonitor(int rig, Vec3 out[4]);
void queueMachineScreens(double nowMs);
void queueText(double nowMs, const Mat4& viewProj);

}  // namespace barapp
