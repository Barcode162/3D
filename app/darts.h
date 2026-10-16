#pragma once
// Darts at the board on the front wall: stand at the oche, tap where you
// want the dart to go (your hand isn't perfect: it lands near there), and
// it flies and sticks. Real scoring: 20 sectors, doubles and triples rings,
// outer bull 25, bull 50. Game: 301, three darts a turn, finish on exactly
// zero (going below is a bust: the turn doesn't count).
#include <string>
#include <vector>

#include "app/renderer.h"
#include "engine/audio.h"
#include "engine/math.h"
#include "engine/text.h"

namespace app {

class Darts {
 public:
  void init(engine::Vec3 boardCenter, const engine::Sound* thunk, const engine::Sound* whoosh,
            const engine::Sound* cheer);
  bool playing() const { return playing_; }
  void start();
  void stop() { playing_ = false; }

  // A tap while playing: `eye` and `dir` = the ray through the tap.
  void throwAt(engine::Vec3 eye, engine::Vec3 dir, double nowMs);
  void update(double nowMs);

  // Darts in the board / in flight (yours, or another player's from their
  // state), for drawing.
  void addBoxes(std::vector<SceneRenderer::Box>& out, double nowMs) const;
  static void addStuckDarts(std::vector<SceneRenderer::Box>& out, engine::Vec3 board, const float* encoded, int count);
  void drawHud(engine::TextRenderer& text, float W, float H, float px) const;
  // What to show others: [score left, dart 1, dart 2, dart 3, darts thrown].
  void stateFor(float out[5]) const;
  // When the dart in your hand leaves it (for the throw animation).
  double lastThrowMs() const { return lastThrowMs_; }

  // Score of a spot on the board (u = right, v = up, meters from the
  // center, as seen by the thrower); `label` gets e.g. "T20".
  static int score(float u, float v, std::string* label);

 private:
  struct Dart {
    float u = 0, v = 0;  // where it sticks (board coordinates)
    double thrownMs = 0;
    bool landed = false;
    engine::Vec3 from;   // where it left your hand
  };
  void endTurn();

  engine::Vec3 board_;
  const engine::Sound* thunk_ = nullptr;
  const engine::Sound* whoosh_ = nullptr;
  const engine::Sound* cheer_ = nullptr;
  bool playing_ = false;
  int scoreLeft_ = 301;
  int turnStart_ = 301;
  std::vector<Dart> darts_;  // this turn's darts
  std::string message_;
  double messageMs_ = 0;
  double turnEndsMs_ = 0;   // pull the darts out at this time
  double lastThrowMs_ = -1e9;
  int games_ = 0, bestTurn_ = 0;
};

}  // namespace app
