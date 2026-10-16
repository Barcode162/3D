#include "engine/input.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
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

constexpr double kTapMaxMs = 350;
constexpr float kTapMaxMove = 12;  // CSS pixels

Input::Press* findPress(Input& in, int id) {
  for (Input::Press& p : in.presses) {
    if (p.id == id) return &p;
  }
  return nullptr;
}

void pressStart(Input& in, int id, Vec2 p) {
  Input::Press* slot = findPress(in, Input::kNone);
  if (slot) *slot = {id, p, emscripten_get_now(), 0};
}

void pressMove(Input& in, int id, Vec2 p) {
  if (Input::Press* press = findPress(in, id)) {
    const float dx = p.x - press->start.x, dy = p.y - press->start.y;
    press->moved = std::max(press->moved, std::sqrt(dx * dx + dy * dy));
  }
}

void pressEnd(Input& in, int id, Vec2 p, bool cancelled) {
  Input::Press* press = findPress(in, id);
  if (!press) return;
  pressMove(in, id, p);
  if (!cancelled && press->moved < kTapMaxMove && emscripten_get_now() - press->startMs < kTapMaxMs) {
    double w = 0, h = 0;
    emscripten_get_element_css_size("#canvas", &w, &h);
    if (w > 0 && h > 0) in.taps.push_back({static_cast<float>(p.x / w), static_cast<float>(p.y / h)});
  }
  press->id = Input::kNone;
}

void trackPointer(Input& in, int id, Vec2 pos, int type, bool mouse) {
  auto it = std::find_if(in.pointers.begin(), in.pointers.end(), [&](const Input::Pointer& p) { return p.id == id; });
  if (type == EMSCRIPTEN_EVENT_TOUCHSTART || type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
    if (it == in.pointers.end()) in.pointers.push_back({id, pos, pos, emscripten_get_now(), mouse});
    else it->pos = pos;
  } else if (type == EMSCRIPTEN_EVENT_TOUCHMOVE || type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
    if (it != in.pointers.end()) it->pos = pos;
  } else if (it != in.pointers.end()) {
    in.pointers.erase(it);
  }
}

bool onTouch(int type, const EmscriptenTouchEvent* e, void* user) {
  auto& in = *static_cast<Input*>(user);
  double canvasW = 0, canvasH = 0;
  emscripten_get_element_css_size("#canvas", &canvasW, &canvasH);
  if (in.capture) {
    const double now = emscripten_get_now();
    for (int i = 0; i < e->numTouches; ++i) {
      const EmscriptenTouchPoint& t = e->touches[i];
      if (!t.isChanged || canvasW <= 0 || canvasH <= 0) continue;
      trackPointer(in, static_cast<int>(t.identifier),
                   {static_cast<float>(t.targetX / canvasW), static_cast<float>(t.targetY / canvasH)}, type, false);
      if (type == EMSCRIPTEN_EVENT_TOUCHMOVE) continue;
      in.rawPresses.push_back({{static_cast<float>(t.targetX / canvasW), static_cast<float>(t.targetY / canvasH)}, now,
                               static_cast<int>(t.identifier), type == EMSCRIPTEN_EVENT_TOUCHSTART, -1});
    }
    return true;
  }

  for (int i = 0; i < e->numTouches; ++i) {
    const EmscriptenTouchPoint& t = e->touches[i];
    if (!t.isChanged) continue;
    const Vec2 p{static_cast<float>(t.targetX), static_cast<float>(t.targetY)};
    const int id = static_cast<int>(t.identifier);

    if (type == EMSCRIPTEN_EVENT_TOUCHSTART) pressStart(in, id, p);
    else if (type == EMSCRIPTEN_EVENT_TOUCHMOVE) pressMove(in, id, p);
    else pressEnd(in, id, p, type == EMSCRIPTEN_EVENT_TOUCHCANCEL);

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
  const Vec2 p{static_cast<float>(e->targetX), static_cast<float>(e->targetY)};
  if (in.capture) {
    double w = 0, h = 0;
    emscripten_get_element_css_size("#canvas", &w, &h);
    if (w <= 0 || h <= 0) return true;
    const Vec2 f{static_cast<float>(p.x / w), static_cast<float>(p.y / h)};
    if (type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
      if (f.x != in.mousePos.x || f.y != in.mousePos.y) in.mouseMoved = true;
      in.mousePos = f;
      if (in.mouseButtons & 1) trackPointer(in, Input::kMouseId, f, type, true);
      return true;
    }
    const int bit = e->button == 0 ? 1 : e->button == 2 ? 2 : 4;
    if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) in.mouseButtons |= bit;
    if (type == EMSCRIPTEN_EVENT_MOUSEUP) in.mouseButtons &= ~bit;
    in.mousePos = f;
    if (e->button == 0) trackPointer(in, Input::kMouseId, f, type, true);
    if (e->button == 0) {
      in.rawPresses.push_back({{static_cast<float>(p.x / w), static_cast<float>(p.y / h)}, emscripten_get_now(),
                               Input::kMouseId, type == EMSCRIPTEN_EVENT_MOUSEDOWN, -1});
    }
    return true;
  }
  if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
    in.mouseLooking = true;
    pressStart(in, Input::kMouseId, p);
  }
  if (type == EMSCRIPTEN_EVENT_MOUSEUP && in.mouseLooking) {
    in.mouseLooking = false;
    pressEnd(in, Input::kMouseId, p, false);
  }
  if (type == EMSCRIPTEN_EVENT_MOUSEMOVE && in.mouseLooking) {
    pressMove(in, Input::kMouseId, p);
    in.lookDelta.x += static_cast<float>(e->movementX);
    in.lookDelta.y += static_cast<float>(e->movementY);
  }
  return true;
}

bool onKey(int type, const EmscriptenKeyboardEvent* e, void* user) {
  auto& in = *static_cast<Input*>(user);
  const bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
  const char* c = e->code;
  // Typing in a form (the account screens): leave the keys alone.
  if (EM_ASM_INT({
        var a = document.activeElement;
        return a && (a.tagName === 'INPUT' || a.tagName === 'TEXTAREA') ? 1 : 0;
      })) {
    return false;
  }
  if (in.capture) {
    // Every key is tracked (the racing game reads them by name).
    const std::string code = c;
    auto held = std::find(in.keysHeld.begin(), in.keysHeld.end(), code);
    if (down && held == in.keysHeld.end()) {
      in.keysHeld.push_back(code);
      in.keyPresses.push_back(code);
    } else if (!down && held != in.keysHeld.end()) {
      in.keysHeld.erase(held);
    }
    // D F J K (and the arrows left, right, down, up) are keys 0..3, Escape 4.
    static const char* kLaneKeys[9] = {"KeyD", "KeyF", "KeyJ", "KeyK", "Escape", "ArrowLeft", "ArrowRight", "ArrowDown", "ArrowUp"};
    for (int i = 0; i < 9; ++i) {
      if (std::strcmp(c, kLaneKeys[i]) != 0) continue;
      const int k = i < 5 ? i : i - 5;
      if (e->repeat) return true;
      in.rawPresses.push_back({{0.125f + 0.25f * static_cast<float>(std::min(k, 3)), 0.9f}, emscripten_get_now(),
                               -100 - k, down, k});
      return true;
    }
    // Keep game keys from scrolling the page; leave browser shortcuts alone.
    return !(e->ctrlKey || e->metaKey) && (code == "Space" || code.rfind("Key", 0) == 0 || code.rfind("Shift", 0) == 0);
  }
  if (!std::strcmp(c, "KeyW") || !std::strcmp(c, "ArrowUp")) in.keyForward = down;
  else if (!std::strcmp(c, "KeyS") || !std::strcmp(c, "ArrowDown")) in.keyBack = down;
  else if (!std::strcmp(c, "KeyA") || !std::strcmp(c, "ArrowLeft")) in.keyLeft = down;
  else if (!std::strcmp(c, "KeyD") || !std::strcmp(c, "ArrowRight")) in.keyRight = down;
  else return false;
  return true;
}

}  // namespace

void Input::attach(const char* canvas) {
  // Right-click is a game button (mouse steering: brake), not a menu.
  EM_ASM({
    var c = document.querySelector(UTF8ToString($0));
    if (c) c.addEventListener('contextmenu', function(e) { e.preventDefault(); });
  }, canvas);
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

void Input::setCapture(bool on) {
  if (capture == on) return;
  capture = on;
  // Let go of everything the other mode was tracking.
  stickTouchId = lookTouchId = kNone;
  showStick(false, {}, {});
  mouseLooking = false;
  keyForward = keyBack = keyLeft = keyRight = false;
  for (Press& p : presses) p.id = kNone;
  taps.clear();
  rawPresses.clear();
  pointers.clear();
  keysHeld.clear();
  keyPresses.clear();
  mouseButtons = 0;
  mouseMoved = false;
  lookDelta = {};
}

std::vector<Input::RawPress> Input::takeRawPresses() {
  std::vector<RawPress> r;
  r.swap(rawPresses);
  return r;
}

bool Input::keyDown(const char* code) const {
  for (const std::string& k : keysHeld) {
    if (k == code) return true;
  }
  return false;
}

std::vector<std::string> Input::takeKeyPresses() {
  std::vector<std::string> k;
  k.swap(keyPresses);
  return k;
}

std::vector<Vec2> Input::takeTaps() {
  std::vector<Vec2> t;
  t.swap(taps);
  return t;
}

Vec2 Input::takeLookDelta() {
  const Vec2 d = lookDelta;
  lookDelta = {};
  return d;
}

}  // namespace engine
