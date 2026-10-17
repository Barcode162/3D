#include "app/race/controls.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace app::race {
namespace {

float approach(float v, float target, float up, float down, float dt) {
  if (target > v) return std::min(target, v + up * dt);
  return std::max(target, v - down * dt);
}
float curve(float x, float power) { return std::copysign(std::pow(std::fabs(x), power), x); }
float deadzone(float x, float dz) { return std::fabs(x) < dz ? 0 : std::copysign((std::fabs(x) - dz) / (1 - dz), x); }

constexpr float kSteerSpan = 0.2f;  // screen widths of slide for full lock
constexpr float kSteerSmoothing = 0.025f;  // seconds (touch and gamepad)

}  // namespace

void Controls::reset() {
  car = {};
  steer_ = targetSteer_ = gas_ = brake_ = 0;
  steerFinger_ = gasFinger_ = brakeFinger_ = -1;
  for (bool& h : held_) h = false;
  for (bool& p : pressed) p = false;
  lookingBack = false;
}

Controls::Rect Controls::brakePedal(float aspect) const {
  const float w = std::min(0.15f, 0.36f / aspect);
  return {0.80f - w - 0.015f - safeR_, 0.42f, w, 0.55f - safeB_};
}
Controls::Rect Controls::gasPedal(float aspect) const {
  const float w = std::min(0.17f, 0.4f / aspect);
  return {0.985f - w - safeR_, 0.32f, w, 0.65f - safeB_};
}

Controls::Rect Controls::buttonRect(Button b, float aspect, bool manual) const {
  const float bw = std::min(0.1f, 0.22f / aspect), bh = 0.11f;
  switch (b) {
    case kLeave: return {0.012f + safeL_, 0.02f, bw, bh};
    case kCamera: return {0.988f - 2 * bw - 0.01f - safeR_, 0.02f, bw, bh};
    case kReset: return {0.988f - bw - safeR_, 0.02f, bw, bh};
    case kLookBack: return {0.988f - 3 * bw - 0.02f - safeR_, 0.02f, bw, bh};
    case kHandbrake: {
      const Rect brake = brakePedal(aspect);
      return {brake.x, brake.y - 0.15f, brake.w, 0.12f};
    }
    case kGearDown:
    case kGearUp: {
      if (!manual) return {-1, -1, 0, 0};
      const Rect gas = gasPedal(aspect);
      return {b == kGearUp ? gas.x : gas.x - bw - 0.012f, 0.17f, b == kGearUp ? gas.w : bw, 0.12f};
    }
    default: return {-1, -1, 0, 0};
  }
}

void Controls::update(float dt, const engine::InputFrame& input, float speed, const engine::CarSpec& spec, bool manual,
                      float aspect) {
  for (bool& p : pressed) p = false;
  bool wasHeld[kButtonCount];
  for (int i = 0; i < kButtonCount; ++i) wasHeld[i] = held_[i];
  for (bool& h : held_) h = false;

  // --- Touch: fingers keep the job they started with.
  bool touchGas = false, touchBrake = false;
  float gasTarget = 0, brakeTarget = 0;
  bool steerAlive = false, gasAlive = false, brakeAlive = false;
  const Rect gasR = gasPedal(aspect), brakeR = brakePedal(aspect);
  for (const engine::Pointer& p : input.pointers) {
    if (p.mouse && mouseSteering) continue;
    // Buttons (by where the finger is now).
    bool onButton = false;
    for (int b = 0; b < kButtonCount; ++b) {
      if (buttonRect(static_cast<Button>(b), aspect, manual).contains(p.start)) {
        held_[b] = true;
        onButton = true;
      }
    }
    if (onButton) continue;
    if (p.id == steerFinger_ || (steerFinger_ < 0 && steerZone().contains(p.start) && p.id != gasFinger_ &&
                                 p.id != brakeFinger_)) {
      if (steerFinger_ != p.id) {
        steerFinger_ = p.id;
        anchorX_ = p.start.x;
      }
      steerAlive = true;
      float raw = (p.pos.x - anchorX_) / kSteerSpan;
      // Past full lock: the center follows the finger, so reversing is instant.
      if (raw > 1) anchorX_ = p.pos.x - kSteerSpan;
      if (raw < -1) anchorX_ = p.pos.x + kSteerSpan;
      raw = engine::clampf(raw, -1, 1);
      targetSteer_ = curve(raw, 1.35f);
      source = Source::kTouch;
      continue;
    }
    // Pedals: the press is the base value, sliding down eases off.
    const float ease = engine::clampf((p.pos.y - p.start.y) / 0.35f, 0, 0.85f);
    if (p.id == gasFinger_ || (gasFinger_ < 0 && gasR.contains(p.start))) {
      gasFinger_ = p.id;
      gasAlive = touchGas = true;
      gasTarget = 1 - ease;
      source = Source::kTouch;
    } else if (p.id == brakeFinger_ || (brakeFinger_ < 0 && brakeR.contains(p.start))) {
      brakeFinger_ = p.id;
      brakeAlive = touchBrake = true;
      brakeTarget = 1 - ease;
      source = Source::kTouch;
    }
  }
  if (!steerAlive) steerFinger_ = -1;
  if (!gasAlive) gasFinger_ = -1;
  if (!brakeAlive) brakeFinger_ = -1;

  // --- Keyboard.
  const auto key = [&](const char* a, const char* b = nullptr) { return input.keyDown(a) || (b && input.keyDown(b)); };
  const bool kLeft = key("ArrowLeft", "KeyA"), kRight = key("ArrowRight", "KeyD");
  const bool kGas = key("ArrowUp", "KeyW"), kBrake = key("ArrowDown", "KeyS");
  for (const std::string& k : input.keyPresses) {
    if (k == "KeyM") mouseSteering = !mouseSteering;
    if (k == "KeyC") pressed[kCamera] = true;
    if (k == "KeyR") pressed[kReset] = true;
    if (k == "Escape") pressed[kLeave] = true;
    if (k == "KeyE" || k == "ShiftLeft" || k == "ShiftRight") pressed[kGearUp] = true;
    if (k == "KeyQ" || k == "ControlLeft" || k == "ControlRight") pressed[kGearDown] = true;
    source = Source::kKeyboard;
  }
  if (key("Space")) held_[kHandbrake] = true;
  if (key("KeyB")) held_[kLookBack] = true;
  const bool keyboardDriving = kLeft || kRight || kGas || kBrake;
  if (keyboardDriving) source = Source::kKeyboard;

  // --- Gamepad / wheel.
  const engine::Gamepad pad = engine::readGamepad();
  bool padActive = false;
  float padSteer = 0, padGas = 0, padBrake = 0;
  if (pad.connected) {
    std::string id = pad.id;
    for (char& ch : id) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    bool wheel = false;
    for (const char* w : {"wheel", "g29", "g920", "g923", "t300", "t150", "t248", "tmx", "fanatec", "moza",
                          "thrustmaster", "simucube", "driving force"}) {
      wheel = wheel || id.find(w) != std::string::npos;
    }
    deviceName_ = pad.id;
    const auto button = [&](int i) { return i < pad.buttonCount ? pad.buttons[i] : 0.0f; };
    if (wheel) {
      padSteer = pad.axes[0];
      // Pedals rest at +1 or -1 depending on the wheel; map both to 0..1.
      const auto pedal = [&](int axis) {
        if (axis >= pad.axisCount) return 0.0f;
        const float a = pad.axes[axis];
        return engine::clampf((1 - a) * 0.5f, 0, 1);
      };
      padGas = pedal(2);
      padBrake = pedal(pad.axisCount > 5 ? 5 : 3);
      if (button(4) > 0.5f && !padWas_[4]) pressed[kGearUp] = true;
      if (button(5) > 0.5f && !padWas_[5]) pressed[kGearDown] = true;
    } else {
      padSteer = curve(deadzone(pad.axes[0], 0.08f), 1.5f);
      padGas = button(7);
      padBrake = button(6);
      if (button(0) > 0.5f) held_[kHandbrake] = true;
      if (button(1) > 0.5f) held_[kLookBack] = true;
      if (button(2) > 0.5f && !padWas_[2]) pressed[kReset] = true;
      if (button(3) > 0.5f && !padWas_[3]) pressed[kCamera] = true;
      if (button(5) > 0.5f && !padWas_[5]) pressed[kGearUp] = true;
      if (button(4) > 0.5f && !padWas_[4]) pressed[kGearDown] = true;
      if (button(8) > 0.5f && !padWas_[8]) pressed[kLeave] = true;
    }
    for (int i = 0; i < 20; ++i) padWas_[i] = button(i) > 0.5f;
    padActive = std::fabs(padSteer) > 0.05f || padGas > 0.05f || padBrake > 0.05f;
    if (padActive) source = wheel ? Source::kWheel : Source::kGamepad;
  }

  // --- Mouse steering (computer): left/right = wheel, buttons = gas/brake.
  if (mouseSteering && input.mouseMoved) {
    source = Source::kMouse;
  }

  // --- Mix: whatever was used last drives.
  float steerTarget = 0, steerRateUp = 30, steerRateDown = 30;
  switch (source) {
    case Source::kTouch:
      steerTarget = steerAlive ? targetSteer_ : 0;
      gasTarget = touchGas ? gasTarget : 0;
      brakeTarget = touchBrake ? brakeTarget : 0;
      break;
    case Source::kKeyboard: {
      // Keys ramp the wheel in (slower at speed) and let it return faster.
      steerTarget = (kRight ? 1.0f : 0.0f) - (kLeft ? 1.0f : 0.0f);
      steerRateUp = 3.2f / (1 + std::max(speed, 0.0f) / 40);
      steerRateDown = 6;
      gasTarget = kGas ? 1.0f : 0.0f;
      brakeTarget = kBrake ? 1.0f : 0.0f;
      break;
    }
    case Source::kMouse:
      steerTarget = curve(engine::clampf((input.mousePos.x - 0.5f) / 0.32f, -1, 1), 1.2f);
      gasTarget = (input.mouseButtons & 1) ? 1.0f : 0.0f;
      brakeTarget = (input.mouseButtons & 2) ? 1.0f : 0.0f;
      if (kGas) gasTarget = 1;
      if (kBrake) brakeTarget = 1;
      break;
    case Source::kGamepad:
    case Source::kWheel:
      steerTarget = padSteer;
      gasTarget = padGas;
      brakeTarget = padBrake;
      break;
  }
  if (source == Source::kTouch || source == Source::kKeyboard) {
    // A brake press bites at once (to 40% of the press), then ramps in.
    if (brakeTarget > 0 && brake_ < 0.4f * brakeTarget) brake_ = 0.4f * brakeTarget;
    gas_ = approach(gas_, gasTarget, 7, 12, dt);
    brake_ = approach(brake_, brakeTarget, 8, 14, dt);
  } else {
    gas_ = gasTarget;
    brake_ = brakeTarget;
  }
  if (source == Source::kWheel) {
    steer_ = steerTarget;
  } else if (source == Source::kTouch || source == Source::kGamepad) {
    // A light filter (25 ms): removes thumb jitter without feeling laggy,
    // the same at any frame rate.
    steer_ += (steerTarget - steer_) * (1.0f - std::exp(-dt / kSteerSmoothing));
  } else {
    steer_ = std::fabs(steerTarget) > std::fabs(steer_) && steerTarget * steer_ >= 0
                 ? approach(steer_, steerTarget, steerRateUp, steerRateUp, dt)
                 : approach(steer_, steerTarget, steerRateDown, steerRateDown, dt);
  }

  // Steering assist: at speed, full input = the steering angle that gives
  // the most grip (about the tires' peak slip plus the turn itself).
  float steerOut = steer_;
  if (source != Source::kWheel) {
    const float v = std::max(std::fabs(speed), 1.0f);
    const float wheelbase = spec.front.z - spec.rear.z;
    const float best = wheelbase * 1.15f * 9.81f / (v * v) + spec.tire.peakSlipAngle * 0.95f;
    steerOut *= engine::clampf(best / (spec.maxSteer * 0.85f), 0.22f, 1.0f);
  }

  // Button presses (touch: on release-to-press edge).
  for (int b = 0; b < kButtonCount; ++b) {
    if (held_[b] && !wasHeld[b] && b != kHandbrake && b != kLookBack) pressed[b] = true;
  }
  car.steer = steerOut;
  car.throttle = gas_;
  car.brake = brake_;
  car.handbrake = held_[kHandbrake] ? 1.0f : 0.0f;
  car.shift = (pressed[kGearUp] ? 1 : 0) - (pressed[kGearDown] ? 1 : 0);
  lookingBack = held_[kLookBack];
}

}  // namespace app::race
