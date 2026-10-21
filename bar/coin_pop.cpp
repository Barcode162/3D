#include "bar/coin_pop.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace app {

namespace {
const engine::Vec3 kHidden{0, -20, 0};  // under the floor while not shown
}

std::string creditText(int cents) {
  char text[32];
  if (cents % 100 == 0) std::snprintf(text, sizeof(text), "%d", cents / 100);
  else std::snprintf(text, sizeof(text), "%d.%02d", cents / 100, std::abs(cents % 100));
  return text;
}

void CoinPop::start(double nowMs, engine::Vec3 eye, engine::Vec3 ahead) {
  startMs_ = nowMs;
  at_ = eye + ahead * 0.6f + engine::Vec3{0, -0.12f, 0};
  yaw_ = std::atan2(-ahead.x, -ahead.z) + 3.14159265f;  // the face turned to you
}

void CoinPop::update(double nowMs, SceneRenderer& renderer, int model) {
  if (model < 0) return;
  if (startMs_ < 0) {
    if (shown_) renderer.setModelTransform(static_cast<size_t>(model), kHidden, {});
    shown_ = false;
    return;
  }
  const float t = static_cast<float>((nowMs - startMs_) / kDurationMs);
  if (t >= 1) {
    startMs_ = -1;
    return;
  }
  // Pops up quickly with an overshoot, floats, then shoots up and away.
  const float pop = t < 0.18f ? std::sin(t / 0.18f * 1.9f) / std::sin(1.9f) : 1.0f;
  float y = -0.25f + 0.33f * pop + 0.02f * std::sin(t * 12.0f);
  if (t > 0.78f) {
    const float u = (t - 0.78f) / 0.22f;
    y += u * u * 1.6f;
  }
  // Spins fast first and settles to a slow turn (ending face-on mid-way).
  const float spin = yaw_ + 6.2831853f * (2.5f * (1 - std::exp(-t * 6.0f)) + 0.4f * t);
  if (!shown_) renderer.setModelBaseGlow(static_cast<size_t>(model), 0.35f);  // a reward should shine
  renderer.setModelTransform(static_cast<size_t>(model), at_ + engine::Vec3{0, y, 0},
                             engine::Quat::axisAngle({0, 1, 0}, spin));
  shown_ = true;
}

}  // namespace app
