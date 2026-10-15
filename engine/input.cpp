#include "engine/input.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstring>

namespace engine {
namespace {

// Shows the joystick rings (HTML elements in web/index.html) at the touch spot.
void showStick(bool visible, Vec2 origin, Vec2 knob) {
  EM_ASM({
    var base = document.getElementById('stickBase');
    var knob = document.getElementById('stickKnob');
    if (!base || !knob) return;
    base.style.display = knob.style.display = $0 ? 'block' : 'none';
    base.style.transform = 'translate(' + $1 + 'px,' + $2 + 'px)';
    knob.style.transform = 'translate(' + $3 + 'px,' + $4 + 'px)';
  }, visible, origin.x, origin.y, knob.x, knob.y);
}

Vec2 clampToStick(Vec2 origin, Vec2 p) {
  Vec2 d{p.x - origin.x, p.y - origin.y};
  const float len = std::sqrt(d.x * d.x + d.y * d.y);
  if (len > Input::kStickRadius) {
    d.x *= Input::kStickRadius / len;
    d.y *= Input::kStickRadius / len;
  }
  return {origin.x + d.x, origin.y + d.y};
}

bool onTouch(int type, const EmscriptenTouchEvent* e, void* user) {
  auto& in = *static_cast<Input*>(user);
  double canvasW = 0, canvasH = 0;
  emscripten_get_element_css_size("#canvas", &canvasW, &canvasH);

  for (int i = 0; i < e->numTouches; ++i) {
    const EmscriptenTouchPoint& t = e->touches[i];
    if (!t.isChanged) continue;
    const Vec2 p{static_cast<float>(t.targetX), static_cast<float>(t.targetY)};
    const int id = static_cast<int>(t.identifier);

    if (type == EMSCRIPTEN_EVENT_TOUCHSTART) {
      if (p.x < canvasW * 0.5 && in.stickTouchId == Input::kNone) {
        in.stickTouchId = id;
        in.stickOrigin = in.stickPos = p;
        showStick(true, p, p);
      } else if (in.lookTouchId == Input::kNone) {
        in.lookTouchId = id;
        in.lastLookPos = p;
      }
    } else if (type == EMSCRIPTEN_EVENT_TOUCHMOVE) {
      if (id == in.stickTouchId) {
        in.stickPos = clampToStick(in.stickOrigin, p);
        showStick(true, in.stickOrigin, in.stickPos);
      } else if (id == in.lookTouchId) {
        in.lookDelta.x += p.x - in.lastLookPos.x;
        in.lookDelta.y += p.y - in.lastLookPos.y;
        in.lastLookPos = p;
      }
    } else {  // touch end / cancel
      if (id == in.stickTouchId) {
        in.stickTouchId = Input::kNone;
        showStick(false, {}, {});
      } else if (id == in.lookTouchId) {
        in.lookTouchId = Input::kNone;
      }
    }
  }
  return true;  // prevents page scrolling / zooming
}

bool onMouse(int type, const EmscriptenMouseEvent* e, void* user) {
  auto& in = *static_cast<Input*>(user);
  if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) in.mouseLooking = true;
  if (type == EMSCRIPTEN_EVENT_MOUSEUP) in.mouseLooking = false;
  if (type == EMSCRIPTEN_EVENT_MOUSEMOVE && in.mouseLooking) {
    in.lookDelta.x += static_cast<float>(e->movementX);
    in.lookDelta.y += static_cast<float>(e->movementY);
  }
  return true;
}

bool onKey(int type, const EmscriptenKeyboardEvent* e, void* user) {
  auto& in = *static_cast<Input*>(user);
  const bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
  const char* c = e->code;
  if (!std::strcmp(c, "KeyW") || !std::strcmp(c, "ArrowUp")) in.keyForward = down;
  else if (!std::strcmp(c, "KeyS") || !std::strcmp(c, "ArrowDown")) in.keyBack = down;
  else if (!std::strcmp(c, "KeyA") || !std::strcmp(c, "ArrowLeft")) in.keyLeft = down;
  else if (!std::strcmp(c, "KeyD") || !std::strcmp(c, "ArrowRight")) in.keyRight = down;
  else return false;
  return true;
}

}  // namespace

void Input::attach(const char* canvas) {
  emscripten_set_touchstart_callback(canvas, this, true, onTouch);
  emscripten_set_touchmove_callback(canvas, this, true, onTouch);
  emscripten_set_touchend_callback(canvas, this, true, onTouch);
  emscripten_set_touchcancel_callback(canvas, this, true, onTouch);
  emscripten_set_mousedown_callback(canvas, this, true, onMouse);
  emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, onMouse);
  emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, onMouse);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, onKey);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, onKey);
}

Vec2 Input::move() const {
  Vec2 m;
  if (stickTouchId != kNone) {
    m.x = (stickPos.x - stickOrigin.x) / kStickRadius;
    m.y = -(stickPos.y - stickOrigin.y) / kStickRadius;  // screen up = forward
  }
  m.x += (keyRight ? 1.0f : 0.0f) - (keyLeft ? 1.0f : 0.0f);
  m.y += (keyForward ? 1.0f : 0.0f) - (keyBack ? 1.0f : 0.0f);
  const float len = std::sqrt(m.x * m.x + m.y * m.y);
  if (len > 1.0f) {
    m.x /= len;
    m.y /= len;
  }
  return m;
}

Vec2 Input::takeLookDelta() {
  const Vec2 d = lookDelta;
  lookDelta = {};
  return d;
}

}  // namespace engine
