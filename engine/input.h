#pragma once
// Turns touches (and mouse/keyboard on a computer) into movement and look
// input:
// - Touch on the left half of the screen: a virtual joystick for walking.
// - Drag on the right half: look around.
// - Computer: WASD/arrow keys to walk, drag the mouse to look.
#include "engine/math.h"

namespace engine {

class Input {
 public:
  void attach(const char* canvasSelector);

  // Walking direction from the joystick or keys: x = strafe right, y = forward.
  // Length is 0..1.
  Vec2 move() const;

  // Look movement since the last call, in CSS pixels. Resets after reading.
  Vec2 takeLookDelta();

  // Internal state, updated by the event callbacks in input.cpp.
  static constexpr int kNone = -1;
  int stickTouchId = kNone;
  int lookTouchId = kNone;
  Vec2 stickOrigin, stickPos;
  Vec2 lastLookPos;
  Vec2 lookDelta;
  bool mouseLooking = false;
  bool keyForward = false, keyBack = false, keyLeft = false, keyRight = false;

  static constexpr float kStickRadius = 60.0f;  // CSS pixels
};

}  // namespace engine
