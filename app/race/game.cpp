#include "app/race/game.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "engine/log.h"

using engine::Vec2;
using engine::Vec3;
using engine::Vec4;

namespace app::race {
namespace {

constexpr int kBots = 7;
constexpr int kLaps = 3;
const char* kCameraNames[] = {"CHASE", "FAR", "HOOD", "BUMPER"};

std::string timeText(double ms) {
  if (ms <= 0) return "-:--.---";
  char b[32];
  const int total = static_cast<int>(ms);
  std::snprintf(b, sizeof(b), "%d:%02d.%03d", total / 60000, (total / 1000) % 60, total % 1000);
  return b;
}

float wrapAngle(float a) { return std::remainder(a, 2 * engine::kPi); }

}  // namespace

void Game::start(int rig, double nowMs) {
  if (!ready_) {
    const double t0 = emscripten_get_now();
    world_.init();
    ready_ = view_.init(*gpu_, world_.track(), sportsCar());
    LOG_INFO("Race track ready in %.0f ms.", emscripten_get_now() - t0);
    if (!ready_) return;
  }
  rig_ = rig;
  open_ = true;
  startMs_ = nowMs;
  resultsMs_ = 0;
  // Left rig starts 4th, right rig 5th (side by side when two play).
  world_.start(3 + (rig & 1), kBots, kLaps, nowMs);
  controls_.reset();
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  camYaw_ = engine::Vehicle::headingOf(me.car.forward());
  camPos_ = me.car.position;
  if (hum_ && !humVoice_) humVoice_ = hum_->play(0.0f, nullptr, 0, 0.6f, engine::Sound::kEffects, true);
  // The page's top bar (menu, mic, log) would sit on the race buttons.
  char build[32] = {};
  EM_ASM({
    document.body.classList.add('racing');
    var b = document.getElementById('build');
    stringToUTF8(b ? b.textContent : "", $0, $1);
  }, build, sizeof(build));
  build_ = build;
}

void Game::close() {
  open_ = false;
  EM_ASM({ document.body.classList.remove('racing'); });
  engine::Sound::stop(humVoice_, 0, 0.3f);
  humVoice_ = 0;
}

bool Game::update(double nowMs, float dt, engine::Input& input) {
  if (!open_) return false;
  Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const float W = static_cast<float>(gpu_->width), H = static_cast<float>(gpu_->height);
  controls_.update(dt, input, me.car.speed(), me.car.spec(), manualGears_, W / std::max(H, 1.0f));
  const bool* pressed = controls_.pressed;
  if (pressed[Controls::kLeave]) return true;
  if (pressed[Controls::kCamera]) cameraMode_ = (cameraMode_ + 1) % 4;
  if (pressed[Controls::kReset] && world_.started(nowMs)) world_.resetCar(world_.you());
  me.input = controls_.car;
  me.assists.autoGears = !manualGears_;
  // After the finish: tap anywhere on the results to race again.
  if (me.finished && resultsMs_ == 0) resultsMs_ = nowMs;
  if (resultsMs_ > 0 && nowMs - resultsMs_ > 1500) {
    for (const engine::Input::Pointer& p : input.pointers) {
      if (p.pos.x > 0.3f && p.pos.x < 0.7f && p.pos.y > 0.3f && p.pos.y < 0.8f && nowMs - p.startMs < 100) {
        start(rig_, nowMs);
        return false;
      }
    }
    if (input.keyDown("Enter")) start(rig_, nowMs);
  }
  world_.update(nowMs, dt);
  updateCamera(dt);
  // Engine sound from the revs and the throttle.
  const float revs = (me.car.rpm - 900) / 6600;
  engine::Sound::set(humVoice_, 0.1f + 0.25f * me.car.throttleUsed + 0.05f * revs, 0.45f + 1.6f * revs);
  return false;
}

void Game::updateCamera(float dt) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const engine::Vehicle& car = me.car;
  const float speed = car.speed();
  const float aspect = static_cast<float>(gpu_->width) / std::max(static_cast<float>(gpu_->height), 1.0f);
  // About 90 degrees across in landscape; a bit wider at speed.
  const float hfov = engine::radians(88.0f + engine::clampf(std::fabs(speed) * 0.12f, 0, 10));
  camera_.fovY = std::min(2 * std::atan(std::tan(hfov * 0.5f) / aspect), engine::radians(80.0f));
  const Vec3 fwd = car.forward();
  float heading = engine::Vehicle::headingOf(fwd);
  if (controls_.lookingBack) heading += engine::kPi;
  if (cameraMode_ <= 1) {
    // Chase: the camera trails the car's heading with a little lag, so turns
    // and slides read clearly.
    const float lag = 1 - std::exp(-dt * 6.0f);
    camYaw_ += wrapAngle(heading - camYaw_) * lag;
    const Vec3 back{std::sin(camYaw_), 0, std::cos(camYaw_)};  // behind = opposite of forward
    const float distance = cameraMode_ == 0 ? 5.4f : 8.5f, height = cameraMode_ == 0 ? 1.65f : 2.6f;
    Vec3 eye = car.position + back * distance + Vec3{0, height, 0};
    const Vec3 target = car.position + Vec3{-back.x, 0, -back.z} * 2.5f + Vec3{0, 0.75f, 0};
    // Keep above the ground.
    float s, lat;
    int hint = me.hint;
    world_.track().locate(eye, s, lat, hint);
    const float ground = world_.track().surfacePoint(s, lat).y;
    eye.y = std::max(eye.y, ground + 0.6f);
    camera_.eye = eye;
    camera_.target = target;
    camera_.up = {0, 1, 0};
  } else {
    // Hood / bumper: fixed to the car (the horizon follows the car's roll a little).
    const Vec3 local = cameraMode_ == 2 ? Vec3{0, 0.42f, -0.55f} : Vec3{0, -0.12f, -2.3f};
    const Vec3 eye = car.worldPoint(local);
    Vec3 dir = controls_.lookingBack ? -fwd : fwd;
    camera_.eye = eye;
    camera_.target = eye + dir * 10.0f;
    camera_.up = engine::normalize(Vec3{0, 1, 0} * 3.0f + car.up());
  }
}

void Game::render(const wgpu::TextureView& target, engine::TextRenderer& text, double nowMs) {
  if (!open_ || !ready_) return;
  const float W = static_cast<float>(gpu_->width), H = static_cast<float>(gpu_->height);
  queueHud(text, W, H, gpu_->pixelRatio, nowMs);
  text.upload(gpu_->queue);
  std::vector<CarDraw> cars;
  for (const Racer& r : world_.racers()) cars.push_back(View::carDraw(r.car, r.paint, r.car.brakeUsed));
  view_.render(target, gpu_->depthView, gpu_->width, gpu_->height, camera_, cars, world_.lightsOn(nowMs),
               static_cast<float>(nowMs / 1000), &text);
}

float Game::steer() const {
  if (!open_) return 0;
  return world_.racers()[static_cast<size_t>(world_.you())].car.wheels[0].steer / 0.5f;
}

void Game::stateFor(float out[5]) const {
  for (int i = 0; i < 5; ++i) out[i] = 0;
  if (!open_) return;
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  out[0] = me.car.position.x;
  out[1] = me.car.position.z;
  out[2] = engine::Vehicle::headingOf(me.car.forward());
  out[3] = me.car.speed();
  out[4] = engine::clampf(me.input.steer, -1, 1);
}

void Game::queueHud(engine::TextRenderer& text, float W, float H, float px, double nowMs) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const engine::Vehicle& car = me.car;
  const float aspect = W / std::max(H, 1.0f);
  const float u = std::min(W, H) / 400.0f;  // HUD unit: scales with the screen
  engine::TextStyle st;
  st.color = {1, 1, 1, 1};
  const auto rect = [&](const Controls::Rect& r, Vec4 color) { text.addRect(r.x * W, r.y * H, r.w * W, r.h * H, color); };
  const auto label = [&](const Controls::Rect& r, const char* s, float size) {
    engine::TextStyle t;
    t.align = 0.5f;
    t.color = {1, 1, 1, 0.92f};
    text.addScreen(s, (r.x + r.w * 0.5f) * W, (r.y + r.h * 0.5f) * H - size * 0.5f, size, t);
  };

  // Position, lap, times (top middle).
  char line[96];
  st.align = 0.5f;
  std::snprintf(line, sizeof(line), "P%d/%d   LAP %d/%d", me.place, static_cast<int>(world_.racers().size()),
                std::min(me.lap + 1, world_.laps()), world_.laps());
  text.addRect(W * 0.5f - 95 * u, 6 * u, 190 * u, 40 * u, {0, 0, 0, 0.45f});
  text.addScreen(line, W * 0.5f, 11 * u, 13 * u, st);
  const double lapMs = world_.started(nowMs) && !me.finished && me.lapStartMs > 0 ? nowMs - me.lapStartMs : 0;
  std::snprintf(line, sizeof(line), "%s   BEST %s", timeText(lapMs).c_str(), timeText(me.bestLapMs).c_str());
  st.color = {1, 0.9f, 0.6f, 1};
  text.addScreen(line, W * 0.5f, 30 * u, 8 * u, st);

  // Speed, gear and revs (bottom middle, between the thumbs).
  const float cx = W * 0.5f + W * 0.06f, by = H - 18 * u;
  const float rpmFrac = engine::clampf((car.rpm - 900) / (car.spec().maxRpm - 900), 0, 1);
  const float barW = 120 * u;
  text.addRect(cx - barW * 0.5f, by - 64 * u, barW, 7 * u, {0, 0, 0, 0.5f});
  const Vec4 rpmColor = rpmFrac > 0.92f ? Vec4{1, 0.15f, 0.1f, 1} : rpmFrac > 0.75f ? Vec4{1, 0.75f, 0.1f, 1} : Vec4{0.3f, 0.9f, 1, 1};
  text.addRect(cx - barW * 0.5f, by - 64 * u, barW * rpmFrac, 7 * u, rpmColor);
  st.color = {1, 1, 1, 1};
  st.align = 0.5f;
  std::snprintf(line, sizeof(line), "%d", static_cast<int>(std::fabs(car.speed()) * 3.6f + 0.5f));
  text.addScreen(line, cx - 14 * u, by - 50 * u, 26 * u, st);
  st.color = {0.8f, 0.8f, 0.8f, 1};
  text.addScreen("KM/H", cx - 14 * u, by - 18 * u, 6 * u, st);
  const std::string gear = car.gear < 0 ? "R" : car.gear == 0 ? "N" : std::to_string(car.gear);
  st.color = rpmColor;
  text.addScreen(gear, cx + 44 * u, by - 50 * u, 22 * u, st);
  st.color = {0.8f, 0.8f, 0.8f, 1};
  text.addScreen(manualGears_ ? "MAN" : "AUTO", cx + 44 * u, by - 18 * u, 6 * u, st);

  // Standings (top left, under the leave button): the cars around you.
  {
    std::vector<const Racer*> order;
    for (const Racer& r : world_.racers()) order.push_back(&r);
    std::sort(order.begin(), order.end(), [](const Racer* a, const Racer* b) { return a->place < b->place; });
    engine::TextStyle t;
    float y = 0.16f * H;
    for (const Racer* r : order) {
      t.color = r == &me ? Vec4{1, 0.85f, 0.3f, 1} : Vec4{1, 1, 1, 0.85f};
      std::snprintf(line, sizeof(line), "%d %s", r->place, r->name.c_str());
      text.addRect(0.012f * W, y - 2 * u, 70 * u, 13 * u, {0, 0, 0, 0.35f});
      text.addRect(0.012f * W, y - 2 * u, 3 * u, 13 * u, {r->paint.x, r->paint.y, r->paint.z, 1});
      text.addScreen(line, 0.012f * W + 7 * u, y, 7 * u, t);
      y += 15 * u;
    }
  }

  // Start: lights are on the gantry; a big GO when they go out.
  const double go = world_.goMs();
  if (nowMs < go) {
    const int lit = world_.lightsOn(nowMs);
    for (int k = 0; k < 5; ++k) {
      text.addShape(0, W * 0.5f + (static_cast<float>(k) - 2) * 26 * u, H * 0.3f, 10 * u,
                    k < lit ? Vec4{1, 0.05f, 0.03f, 1} : Vec4{0.15f, 0.02f, 0.02f, 0.9f}, {0, 0, 0, 1}, 2 * u);
    }
    st.color = {1, 1, 1, 0.9f};
    text.addScreen(world_.lightsOn(nowMs) == 0 ? "GET READY" : "", W * 0.5f, H * 0.36f, 12 * u, st);
  } else if (nowMs < go + 1200) {
    st.color = {0.3f, 1, 0.3f, 1};
    text.addScreen("GO!", W * 0.5f, H * 0.28f, 40 * u, st);
  }

  // Results.
  if (me.finished) {
    const float bw = 260 * u, bh = 170 * u;
    text.addRect(W * 0.5f - bw * 0.5f, H * 0.5f - bh * 0.5f, bw, bh, {0, 0, 0, 0.75f});
    st.color = {1, 0.85f, 0.3f, 1};
    std::snprintf(line, sizeof(line), "FINISHED P%d", me.place);
    text.addScreen(line, W * 0.5f, H * 0.5f - bh * 0.5f + 10 * u, 16 * u, st);
    std::vector<const Racer*> order;
    for (const Racer& r : world_.racers()) order.push_back(&r);
    std::sort(order.begin(), order.end(), [](const Racer* a, const Racer* b) { return a->place < b->place; });
    engine::TextStyle t;
    float y = H * 0.5f - bh * 0.5f + 36 * u;
    for (const Racer* r : order) {
      t.color = r == &me ? Vec4{1, 0.85f, 0.3f, 1} : Vec4{1, 1, 1, 0.9f};
      std::snprintf(line, sizeof(line), "%d. %-8s %s  best %s", r->place, r->name.c_str(),
                    r->finished ? timeText(r->finishMs).c_str() : "racing", timeText(r->bestLapMs).c_str());
      text.addScreen(line, W * 0.5f - bw * 0.5f + 14 * u, y, 7 * u, t);
      y += 13 * u;
    }
    st.color = {0.6f, 1, 0.6f, 1};
    text.addScreen("TAP HERE TO RACE AGAIN", W * 0.5f, H * 0.5f + bh * 0.5f - 18 * u, 8 * u, st);
  }

  // Touch controls (drawn unless driving with a pad/wheel/keys).
  const bool touch = controls_.source == Controls::Source::kTouch;
  for (int b = 0; b < Controls::kButtonCount; ++b) {
    const auto button = static_cast<Controls::Button>(b);
    const Controls::Rect r = controls_.buttonRect(button, aspect, manualGears_);
    if (r.w <= 0) continue;
    if (!touch && b != Controls::kLeave) continue;
    rect(r, controls_.buttonHeld(button) ? Vec4{1, 0.42f, 0.78f, 0.6f} : Vec4{0.05f, 0.05f, 0.08f, 0.45f});
    static const char* kLabels[] = {"LEAVE", "CAM", "RESET", "BACK", "HANDBRAKE", "GEAR -", "GEAR +"};
    label(r, b == Controls::kCamera ? kCameraNames[cameraMode_] : kLabels[b], 7 * u);
  }
  if (touch) {
    const Controls::Rect gas = controls_.gasPedal(aspect), brake = controls_.brakePedal(aspect);
    rect(brake, {0.05f, 0.05f, 0.08f, 0.35f});
    rect({brake.x, brake.y + brake.h * (1 - controls_.brakeValue()), brake.w, brake.h * controls_.brakeValue()},
         {1, 0.2f, 0.15f, 0.45f});
    label(brake, "BRAKE", 9 * u);
    rect(gas, {0.05f, 0.05f, 0.08f, 0.35f});
    rect({gas.x, gas.y + gas.h * (1 - controls_.gasValue()), gas.w, gas.h * controls_.gasValue()},
         {0.2f, 1, 0.4f, 0.45f});
    label(gas, "GAS", 11 * u);
    // Steering: a bar showing the wheel, plus where your thumb set the center.
    const float sy = H - 22 * u, sw = 0.36f * W, sx = 0.04f * W;
    text.addRect(sx, sy, sw, 6 * u, {0, 0, 0, 0.4f});
    text.addRect(sx + sw * 0.5f - 1 * u, sy - 4 * u, 2 * u, 14 * u, {1, 1, 1, 0.6f});
    const float st01 = 0.5f + 0.5f * controls_.rawSteer();
    text.addShape(0, sx + sw * st01, sy + 3 * u, 8 * u,
                  controls_.steering() ? Vec4{1, 0.42f, 0.78f, 0.95f} : Vec4{1, 1, 1, 0.5f});
    if (!controls_.steering() && nowMs - startMs_ < 9000) {
      engine::TextStyle t;
      t.align = 0.5f;
      t.color = {1, 1, 1, 0.75f};
      text.addScreen("THUMB DOWN ANYWHERE HERE\nAND SLIDE TO STEER", 0.25f * W, 0.55f * H, 8 * u, t);
    }
  } else {
    engine::TextStyle t;
    t.color = {1, 1, 1, 0.6f};
    const std::string help = controls_.source == Controls::Source::kKeyboard || controls_.source == Controls::Source::kMouse
                                 ? "W/S gas brake  A/D steer  SPACE handbrake  E/Q gears  C camera  R reset  M mouse steer"
                                 : controls_.deviceName();
    text.addScreen(help, 0.012f * W, H - 14 * u, 6 * u, t);
  }
  {
    engine::TextStyle t;
    t.align = 1;
    t.color = {1, 1, 1, 0.45f};
    text.addScreen("build " + build_, W - 6 * u, H - 9 * u, 5 * u, t);
  }
  if (H > W) {
    engine::TextStyle t;
    t.align = 0.5f;
    t.color = {1, 1, 0.6f, 1};
    text.addRect(0, H * 0.42f, W, 40 * u, {0, 0, 0, 0.6f});
    text.addScreen("TURN YOUR PHONE SIDEWAYS", W * 0.5f, H * 0.42f + 12 * u, 12 * u, t);
  }
  (void)px;
}

}  // namespace app::race
