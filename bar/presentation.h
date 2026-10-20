#pragma once
// What the frame shows, worked out right before drawing: moving shapes and
// avatar poses (bar/presentation.cpp).
#include "bar/app_state.h"

namespace barapp {

// Moving shapes: piano lights, darts, drinks (yours in hand, others').
void presentScene(double nowMs);
// Your avatar's pose and the other players' (sent to the renderer).
void presentAvatars(double nowMs);

}  // namespace barapp
