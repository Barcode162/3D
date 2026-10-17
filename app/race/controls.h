#pragma once
// Turns every kind of input into driving: touch, keyboard, mouse, gamepad,
// steering wheel. Whatever was used last is in charge.
//
// Touch (landscape): the left half is a steering pad. Put a thumb down
// anywhere and slide: a full lock is a long slide (about a fifth of the
// screen width), softer in the middle for small corrections. Sliding past
// full lock drags the center along, so coming back reacts at once. Right
// side: brake and gas pedals; a press is a smooth, quick ramp, and sliding
// down on a pedal eases it off (analog). Buttons: leave, camera, reset,
// look back, handbrake, gears (manual).
//
// Steering assist (touch, keys, mouse, pad): full input means "as much as
// the tires can take at this speed", so a thumb can't overdo it. Wheels
// steer 1:1.
#include <string>
#include <vector>

#include "engine/gamepad.h"
#include "engine/input.h"
#include "engine/vehicle.h"

namespace app::race {

class Controls {
 public:
  enum class Source { kTouch, kKeyboard, kMouse, kGamepad, kWheel };
  struct Rect {
    float x, y, w, h;  // screen fractions
    bool contains(engine::Vec2 p) const { return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h; }
  };
  enum Button { kLeave, kCamera, kReset, kLookBack, kHandbrake, kGearDown, kGearUp, kButtonCount };

  void reset();
  // speed in m/s; `aspect` = width / height of the screen.
  void update(float dt, const engine::InputFrame& input, float speed, const engine::CarSpec& spec, bool manualGears,
              float aspect);

  engine::CarInput car;          // what goes to the car this frame
  bool pressed[kButtonCount] = {};  // buttons pressed this frame
  bool lookingBack = false;
  bool mouseSteering = false;    // toggled with M on a computer
  Source source = Source::kTouch;

  // For drawing the touch controls.
  Rect buttonRect(Button b, float aspect, bool manualGears) const;
  // The notch and home bar (screen fractions, from the page's safe area):
  // nothing to press goes under them. The steering pad also keeps clear of
  // the screen's left edge (Safari's back swipe).
  void setSafeArea(float left, float right, float bottom) {
    safeL_ = left;
    safeR_ = right;
    safeB_ = bottom;
  }
  Rect steerZone() const { return {safeL_ + 0.02f, 0.16f, 0.48f - safeL_, 0.84f - safeB_}; }
  Rect brakePedal(float aspect) const;
  Rect gasPedal(float aspect) const;
  bool steering() const { return steerFinger_ >= 0; }
  float steerAnchor() const { return anchorX_; }  // screen fraction
  float rawSteer() const { return targetSteer_; }
  float gasValue() const { return gas_; }
  float brakeValue() const { return brake_; }
  bool buttonHeld(Button b) const { return held_[b]; }
  std::string deviceName() const { return deviceName_; }

 private:
  float steer_ = 0, targetSteer_ = 0, gas_ = 0, brake_ = 0;
  int steerFinger_ = -1, gasFinger_ = -1, brakeFinger_ = -1;
  float anchorX_ = 0;
  float safeL_ = 0, safeR_ = 0, safeB_ = 0;
  bool held_[kButtonCount] = {};
  bool padWas_[20] = {};
  std::string deviceName_;
};

}  // namespace app::race
