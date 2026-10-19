#pragma once
// The lounge's chessboard, playable (upgrade plan item 13). The pieces are
// real models on the coffee table (tools/assets/chess.py) that slide and hop
// between squares. Tap the board: you play white against the computer (or
// black, against whoever is already playing white: then it's a two-player
// game). The camera looks down at the board from your side; tap a piece,
// then a highlighted square. The board is shared through the room (world
// key "ch", claims machine.chess.w / .b), so everyone sees the game.
// Rules: app/chess_rules.*.
#include <string>
#include <vector>

#include "app/bar_scene.h"
#include "app/chess_rules.h"
#include "engine/camera.h"
#include "engine/input.h"
#include "engine/text.h"

namespace app {
class SceneRenderer;

class ChessGame {
 public:
  void init(const BarScene& scene);
  bool isOpen() const { return open_; }
  // Sit down at the board. Returns a message for the player ("" = fine).
  std::string open(bool online);
  void close();
  // Each frame (also while closed: the pieces move for spectators too).
  // While open: handles taps (screen positions 0..1) and returns true when
  // you chose to leave.
  bool update(double nowMs, float dt, const std::vector<engine::Vec2>& taps, const engine::FirstPersonCamera& cam,
              float aspect);
  // Puts the pieces where they are this frame.
  void pose(SceneRenderer& renderer) const;
  // While open: where your eyes are (looking down at the board from your side).
  void camera(engine::FirstPersonCamera& cam) const;
  // Highlights on the board (world quads) and the HUD (screen).
  void draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;
  // The room's chess state changed (world key "ch").
  void onRoom(const std::vector<double>& values);
  // What others see you doing: [playing (1/0), your side]
  void stateFor(float out[5]) const;

 private:
  struct Instance {
    size_t model = 0;
    int8_t piece = 0;          // chess::make(kind, color)
    engine::Vec3 shown, from;  // where it's drawn; where it started its hop
    engine::Vec3 target;
    double moveStartMs = -1;
    bool hidden = false;
  };
  engine::Vec3 squareCenter(int sq) const;
  int squareAt(engine::Vec3 p) const;  // -1 off the board
  void place(const chess::Move* hint, double nowMs);
  void playMove(const chess::Move& m, double nowMs, bool local);
  void newGame(double nowMs, bool twoPlayers);
  void send();
  bool myTurn() const;
  std::string statusText() const;
  // Taps on the HUD buttons (screen 0..1). -1 none, 0 leave, 1 new game.
  int buttonAt(engine::Vec2 tap, float aspect) const;

  bool ready_ = false, open_ = false, online_ = false;
  engine::Vec3 center_;  // the board's middle, on its surface
  float square_ = 0.055f;
  std::vector<Instance> pieces_;
  int at_[64];  // instance on each square (-1 = none)
  mutable std::vector<engine::Vec3> posed_;  // last sent to the renderer (re-sent only on change)
  chess::Position pos_ = chess::Position::start();
  chess::Color mySide_ = chess::kWhite;
  bool twoPlayers_ = false;   // else you play the computer (white plays it, on white's phone)
  int selected_ = -1;
  std::vector<chess::Move> legal_;
  int lastFrom_ = -1, lastTo_ = -1;
  double computerAtMs_ = 0;
  chess::MoveSearch search_;  // the computer thinking, a slice per frame   // when the computer will move (0 = not waiting)
  double nowMs_ = 0;
  int seq_ = 0;               // moves played (orders the room's updates)
  std::string message_;
  double messageMs_ = 0;
  uint32_t seed_ = 1;
};

}  // namespace app
