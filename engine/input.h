#pragma once
// Turns touches (and mouse/keyboard on a computer) into movement and look
// input:
// - Touch on the left half of the screen: a virtual joystick for walking.
// - Drag on the right half: look around.
// - Computer: WASD/arrow keys to walk, drag the mouse to look.
// - A quick tap or click that doesn't move counts as a "tap" (to use things).
#include <string>
#include <vector>

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

  // Taps since the last call, as screen fractions (0,0 = top-left,
  // 1,1 = bottom-right). Resets after reading.
  std::vector<Vec2> takeTaps();

  // Game input (the rhythm game): while capturing, touches don't walk, look
  // or tap. Every finger going down or up is reported instead, with when
  // it happened (emscripten_get_now ms) and where (screen fractions). On a
  // computer the keys D F J K act as presses in the four quarters of the
  // screen (key = 0..3) and Escape as key 4.
  struct RawPress {
    Vec2 pos;
    double ms = 0;
    int id = 0;     // finger (or key) id, to pair downs with ups
    bool down = true;
    int key = -1;   // -1 = a touch or click
  };
  // The canvas size in CSS pixels; set once per frame (Gpu::resizeIfNeeded
  // measures it), so touch events never have to measure the page.
  void setCanvasSize(double w, double h) { canvasW = w; canvasH = h; }
  double canvasW = 0, canvasH = 0;
  // Moves the joystick rings on the page, if they changed (once per frame).
  void flushStick();
  // Handles the touches queued since the last frame (call once per frame,
  // before reading input).
  void pollTouches();
  bool stickVisible = false, stickDirty = false;
  Vec2 stickShownOrigin, stickShownKnob;
  void setCapture(bool on);
  bool capturing() const { return capture; }
  std::vector<RawPress> takeRawPresses();

  // Also while capturing: every finger currently down (and the mouse while
  // a button is held), with where it is now and where it started (screen
  // fractions), for controls that follow a finger (steering, pedals).
  struct Pointer {
    int id = 0;
    Vec2 pos, start;
    double startMs = 0;
    bool mouse = false;
  };
  std::vector<Pointer> pointers;
  // Keys held while capturing, by KeyboardEvent.code ("KeyW", "ArrowUp",
  // "Space", "ShiftLeft"...), and the ones pressed since the last call.
  bool keyDown(const char* code) const;
  std::vector<std::string> takeKeyPresses();
  std::vector<std::string> keysHeld, keyPresses;
  // The mouse while capturing: position (screen fractions) and buttons
  // held (bit 0 = left, bit 1 = right, bit 2 = middle).
  Vec2 mousePos{0.5f, 0.5f};
  int mouseButtons = 0;
  bool mouseMoved = false;  // since capture began (to tell mouse users apart)

  // Internal state, updated by the event callbacks in input.cpp.
  bool capture = false;
  std::vector<RawPress> rawPresses;
  static constexpr int kNone = -1;
  int stickTouchId = kNone;
  int lookTouchId = kNone;
  Vec2 stickOrigin, stickPos;
  Vec2 lastLookPos;
  Vec2 lookDelta;
  bool mouseLooking = false;
  bool keyForward = false, keyBack = false, keyLeft = false, keyRight = false;

  // Tap detection: where/when each finger (or the mouse) went down, and
  // how far it has wandered since.
  struct Press {
    int id = kNone;
    Vec2 start;
    double startMs = 0;
    float moved = 0;
  };
  static constexpr int kMaxPresses = 5;
  static constexpr int kMouseId = -2;
  Press presses[kMaxPresses];
  std::vector<Vec2> taps;

  static constexpr float kStickRadius = 60.0f;  // CSS pixels
};

}  // namespace engine
