#pragma once
// The credit coin popping out when you earn credits (the model
// assets/models/credit_coin.glb, tools/assets/credit_coin.py): it appears
// in front of you, rises and spins, hangs a moment, then shoots up and is
// gone. One coin at a time; a new win starts it again.
#include <string>

#include "bar/renderer.h"
#include "looped/math.h"

namespace app {

class CoinPop {
 public:
  // `eye` and `ahead` (flat, unit) place it in front of you.
  void start(double nowMs, engine::Vec3 eye, engine::Vec3 ahead);
  // Moves the coin model (`model` = its BarScene::models index) each frame.
  void update(double nowMs, SceneRenderer& renderer, int model);
  bool active() const { return startMs_ >= 0; }

  static constexpr double kDurationMs = 2600;

 private:
  double startMs_ = -1;
  engine::Vec3 at_;
  float yaw_ = 0;
  bool shown_ = false;
};

// Credits as people read them: "5", "47.50" (the ledger counts cents).
std::string creditText(int cents);

}  // namespace app
