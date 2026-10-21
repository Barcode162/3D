#pragma once
// Darts at the board on the front wall: stand at the oche, tap where you
// want the dart to go (your hand isn't perfect: it lands near there), and
// it flies and sticks. Real scoring: 20 sectors, doubles and triples rings,
// outer bull 25, bull 50. Game: 301, three darts a turn, finish on exactly
// zero (going below is a bust: the turn doesn't count).
#include <string>
#include <vector>

#include "bar/renderer.h"
#include "looped/audio.h"
#include "looped/math.h"
#include "looped/text.h"

namespace app {

class Darts {
 public:
  void init(engine::Vec3 boardCenter, engine::Sound thunk, engine::Sound whoosh, engine::Sound cheer);
  bool playing() const { return playing_; }
  void start();
  void stop() { playing_ = false; }
  // Online, the room scores every dart (cloud/games/darts.js): a throw sends
  // the aim, the dart lands where the room says. Offline, this phone scores.
  void setOnline(bool on) { online_ = on; }

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
  // Goes up each time a game paid you credits.
  int payouts() const { return payouts_; }
  // Darts that have hit the board since the start (the board shudders, E3.5).
  int hits() const { return hits_; }

  // Score of a spot on the board (u = right, v = up, meters from the
  // center, as seen by the thrower); `label` gets e.g. "T20".
  static int score(float u, float v, std::string* label);

 private:
  int payouts_ = 0;
  int hits_ = 0;
  struct Dart {
    float u = 0, v = 0;  // where it sticks (board coordinates)
    double thrownMs = 0;
    bool landed = false;
    engine::Vec3 from;   // where it left your hand
    int request = 0;     // waiting for the room's answer (online)
    // The room's answer: points, label, what's left, bust/won, turn over.
    int pts = 0, left = 0;
    bool bust = false, won = false, turnOver = false, scored = false;
    int paid = -1;  // cents the room credited for a win (-1: nothing paid)
    bool guest = false, capped = false;
    std::string label;
  };
  void endTurn();

  engine::Vec3 board_;
  engine::Sound thunk_, whoosh_, cheer_;
  bool playing_ = false;
  int scoreLeft_ = 301;
  int turnStart_ = 301;
  std::vector<Dart> darts_;  // this turn's darts
  std::string message_;
  double messageMs_ = 0;
  double turnEndsMs_ = 0;   // pull the darts out at this time
  double lastThrowMs_ = -1e9;
  int games_ = 0, bestTurn_ = 0;
  bool online_ = false;
};

}  // namespace app
