#include "app/darts.h"

#include <emscripten/emscripten.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "engine/frame_time.h"

using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

constexpr double kFlightMs = 280;
constexpr float kSpread = 0.022f;  // meters: how far from the aim a dart lands (on average)
const int kNumbers[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};

// Gaussian-ish random number (average of a few uniforms), roughly -1..1.
float wobble() {
  float s = 0;
  for (int i = 0; i < 4; ++i) s += static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f;
  return s;
}

// The board faces -z (into the room); the thrower looks toward +z, so
// their right is world -x.
Vec3 boardPoint(Vec3 board, float u, float v, float out = 0) { return board + Vec3{-u, v, -out}; }

int encode(float u, float v) {
  return static_cast<int>(std::lround((u + 0.5f) * 1000)) * 1000 + static_cast<int>(std::lround((v + 0.5f) * 1000));
}
void decode(float e, float& u, float& v) {
  const int n = static_cast<int>(e);
  u = static_cast<float>(n / 1000) / 1000.0f - 0.5f;
  v = static_cast<float>(n % 1000) / 1000.0f - 0.5f;
}

void dartBoxes(std::vector<SceneRenderer::Box>& out, Vec3 tip, Vec3 back, Vec3 flightColor) {
  const Vec3 d = engine::normalize(back - tip);
  // The box's z axis along the dart.
  const float yaw = std::atan2(d.x, d.z), pitch = -std::asin(engine::clampf(d.y, -1, 1));
  const engine::Quat q = engine::Quat::axisAngle({0, 1, 0}, yaw) * engine::Quat::axisAngle({1, 0, 0}, pitch);
  SceneRenderer::Box barrel;
  barrel.center = tip + d * 0.05f;
  barrel.size = {0.006f, 0.006f, 0.1f};
  barrel.rotation = q;
  barrel.color = {0.6f, 0.6f, 0.65f};
  barrel.metallic = 1;
  barrel.roughness = 0.3f;
  out.push_back(barrel);
  SceneRenderer::Box flight = barrel;
  flight.center = tip + d * 0.115f;
  flight.size = {0.035f, 0.035f, 0.03f};
  flight.color = flightColor;
  flight.metallic = 0;
  flight.glow = 0.2f;
  out.push_back(flight);
}

}  // namespace

int Darts::score(float u, float v, std::string* label) {
  const float d = std::sqrt(u * u + v * v);
  if (d > 0.170f) {
    if (label) *label = "Miss";
    return 0;
  }
  if (d < 0.0064f) {
    if (label) *label = "BULL";
    return 50;
  }
  if (d < 0.016f) {
    if (label) *label = "25";
    return 25;
  }
  // Clockwise from the top, as the thrower sees it.
  float ang = std::atan2(u, v) * 180.0f / engine::kPi;
  if (ang < 0) ang += 360;
  const int sector = static_cast<int>((ang + 9.0f) / 18.0f) % 20;
  const int n = kNumbers[sector];
  int mult = 1;
  const char* prefix = "";
  if (d > 0.099f && d < 0.107f) {
    mult = 3;
    prefix = "T";
  } else if (d > 0.162f) {
    mult = 2;
    prefix = "D";
  }
  if (label) *label = prefix + std::to_string(n);
  return n * mult;
}

void Darts::init(Vec3 boardCenter, engine::Sound thunk, engine::Sound whoosh, engine::Sound cheer) {
  board_ = boardCenter;
  thunk_ = thunk;
  whoosh_ = whoosh;
  cheer_ = cheer;
}

void Darts::start() {
  playing_ = true;
  if (scoreLeft_ <= 0) scoreLeft_ = 301;
  message_ = "301: tap the board to throw";
  messageMs_ = engine::frameTime().nowMs;
}

void Darts::throwAt(Vec3 eye, Vec3 dir, double nowMs) {
  if (darts_.size() >= 3 || nowMs < lastThrowMs_ + kFlightMs) return;  // turn over or a dart still flying
  // Where the tap ray meets the board's plane, plus your hand's wobble.
  if (dir.z <= 0.05f) return;
  const float t = (board_.z - eye.z) / dir.z;
  const Vec3 hit = eye + dir * t;
  Dart dart;
  dart.u = -(hit.x - board_.x) + wobble() * kSpread * 2;
  dart.v = (hit.y - board_.y) + wobble() * kSpread * 2;
  dart.thrownMs = nowMs;
  dart.from = eye + Vec3{-0.15f, -0.12f, 0.3f};
  darts_.push_back(dart);
  lastThrowMs_ = nowMs;
  {
    const Vec3 p = eye;
    whoosh_.play(0.5f, &p);
  }
}

void Darts::endTurn() {
  int total = 0;
  for (const Dart& d : darts_) total += score(d.u, d.v, nullptr);
  bestTurn_ = std::max(bestTurn_, total);
  darts_.clear();
  turnStart_ = scoreLeft_;
}

void Darts::update(double nowMs) {
  for (Dart& d : darts_) {
    if (d.landed || nowMs < d.thrownMs + kFlightMs) continue;
    d.landed = true;
    std::string label;
    const int pts = score(d.u, d.v, &label);
    const Vec3 at = boardPoint(board_, d.u, d.v);
    thunk_.play(pts > 0 ? 0.9f : 0.5f, &at);
    char text[96];
    if (scoreLeft_ - pts < 0 || scoreLeft_ - pts == 1) {
      std::snprintf(text, sizeof(text), "%s - BUST! Back to %d", label.c_str(), turnStart_);
      scoreLeft_ = turnStart_;
      turnEndsMs_ = nowMs + 1500;
      for (Dart& other : darts_) other.landed = true;  // the turn ends now
      while (darts_.size() < 3) darts_.push_back({0, -1, nowMs, true, {}});  // fills the turn (off the board)
    } else {
      scoreLeft_ -= pts;
      if (scoreLeft_ == 0) {
        ++games_;
        std::snprintf(text, sizeof(text), "%s - GAME SHOT! You won (%d won so far)", label.c_str(), games_);
        cheer_.play(0.8f, &at);
        turnEndsMs_ = nowMs + 2500;
      } else {
        std::snprintf(text, sizeof(text), "%s = %d", label.c_str(), pts);
        if (darts_.size() == 3) turnEndsMs_ = nowMs + 1500;
      }
    }
    message_ = text;
    messageMs_ = nowMs;
  }
  if (turnEndsMs_ > 0 && nowMs > turnEndsMs_) {
    turnEndsMs_ = 0;
    endTurn();
    if (scoreLeft_ == 0) {
      scoreLeft_ = turnStart_ = 301;
      message_ = "New game: 301";
      messageMs_ = nowMs;
    }
  }
}

void Darts::addBoxes(std::vector<SceneRenderer::Box>& out, double nowMs) const {
  for (const Dart& d : darts_) {
    if (d.v < -0.9f) continue;  // a bust filler
    const Vec3 tip = boardPoint(board_, d.u, d.v);
    if (d.landed || nowMs >= d.thrownMs + kFlightMs) {
      dartBoxes(out, tip, tip + Vec3{0.0f, 0.01f, -0.14f}, {1.0f, 0.3f, 0.6f});
    } else {
      // In flight: along a slight arc from the hand to the board.
      const float k = static_cast<float>((nowMs - d.thrownMs) / kFlightMs);
      Vec3 p = d.from + (tip - d.from) * k;
      p.y += 0.12f * std::sin(k * engine::kPi);
      dartBoxes(out, p, p - engine::normalize(tip - d.from) * 0.14f, {1.0f, 0.3f, 0.6f});
    }
  }
}

void Darts::addStuckDarts(std::vector<SceneRenderer::Box>& out, Vec3 board, const float* encoded, int count) {
  for (int i = 0; i < count; ++i) {
    if (encoded[i] <= 0) continue;
    float u, v;
    decode(encoded[i], u, v);
    const Vec3 tip = boardPoint(board, u, v);
    dartBoxes(out, tip, tip + Vec3{0.0f, 0.01f, -0.14f}, {0.3f, 0.8f, 1.0f});
  }
}

void Darts::stateFor(float out[5]) const {
  out[0] = static_cast<float>(scoreLeft_);
  for (int i = 0; i < 3; ++i) {
    const bool shown = i < static_cast<int>(darts_.size()) && darts_[static_cast<size_t>(i)].landed &&
                       darts_[static_cast<size_t>(i)].v > -0.9f;
    out[1 + i] = shown ? static_cast<float>(encode(darts_[static_cast<size_t>(i)].u, darts_[static_cast<size_t>(i)].v)) : 0;
  }
  out[4] = static_cast<float>(darts_.size());
}

void Darts::drawHud(engine::TextRenderer& text, float W, float H, float px) const {
  text.addRect(W * 0.04f, H * 0.075f, W * 0.92f, H * 0.11f, {0.05f, 0.03f, 0.07f, 0.75f});
  engine::TextStyle s;
  s.align = 0.5f;
  s.color = {1.0f, 0.86f, 0.62f, 1};
  char line[96];
  std::snprintf(line, sizeof(line), "DARTS 301   LEFT: %d   DART %d/3", scoreLeft_,
                std::min(3, static_cast<int>(darts_.size()) + 1));
  text.addScreen(line, W * 0.5f, H * 0.09f, 11 * px, s);
  s.color = {1, 1, 1, 1};
  text.addScreen(message_, W * 0.5f, H * 0.135f, 10 * px, s);
  // A crosshair in the middle helps aiming; taps anywhere throw there.
  text.addRect(W * 0.5f - 9 * px, H * 0.5f - px, 18 * px, 2 * px, {1, 1, 1, 0.6f});
  text.addRect(W * 0.5f - px, H * 0.5f - 9 * px, 2 * px, 18 * px, {1, 1, 1, 0.6f});
  s.color = {0.8f, 0.8f, 0.8f, 1};
  text.addScreen("Tap the board to throw. Walk away to stop.", W * 0.5f, H * 0.9f, 8 * px, s);
}

}  // namespace app
