#include "app/chess.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app/renderer.h"
#include "engine/log.h"
#include "engine/net.h"
#include "engine/ui.h"

using engine::Vec2;
using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

using namespace chess;
constexpr double kHopMs = 380;          // a piece's move, lifted in an arc
constexpr double kComputerDelayMs = 700;
constexpr int kComputerDepth = 3;
constexpr double kThinkMsPerFrame = 3.0;

// The room's "ch": board in 8 numbers (8 squares x 4 bits each), then turn,
// castling, en passant, halfmoves, fullmove, last from, last to, two players,
// move number.
constexpr int kStateCount = 17;

}  // namespace

void ChessGame::init(const BarScene& scene) {
  center_ = scene.chessCenter;
  square_ = scene.chessSquare;
  pieces_.clear();
  for (const ChessPiece& p : scene.chessPieces) {
    Instance in;
    in.model = p.model;
    in.piece = p.piece;
    in.shown = in.target = in.from = scene.models[p.model].position;
    pieces_.push_back(in);
  }
  ready_ = !pieces_.empty();
  std::fill(std::begin(at_), std::end(at_), -1);
  pos_ = Position::start();
  place(nullptr, 0);
  for (Instance& in : pieces_) in.shown = in.target;
}

Vec3 ChessGame::squareCenter(int sq) const {
  const float f = static_cast<float>(sq % 8), r = static_cast<float>(sq / 8);
  return center_ + Vec3{(f - 3.5f) * square_, 0, (3.5f - r) * square_};
}

int ChessGame::squareAt(Vec3 p) const {
  const int f = static_cast<int>(std::floor((p.x - center_.x) / square_ + 4));
  const int r = static_cast<int>(std::floor(4 - (p.z - center_.z) / square_));
  if (f < 0 || f > 7 || r < 0 || r > 7) return -1;
  return r * 8 + f;
}

// Which model stands where: the piece that moved keeps its model, pieces
// that stayed keep theirs, the rest go to the nearest free model of their
// kind. Models left over are captured (lined up beside the board) or spare
// queens for promotions (hidden under the table).
void ChessGame::place(const Move* hint, double nowMs) {
  int newAt[64];
  std::fill(std::begin(newAt), std::end(newAt), -1);
  std::vector<char> used(pieces_.size(), 0);
  const auto take = [&](int sq, int inst) {
    newAt[sq] = inst;
    used[static_cast<size_t>(inst)] = 1;
  };
  if (hint) {
    const int it = at_[hint->from];
    if (it >= 0 && pieces_[static_cast<size_t>(it)].piece == pos_.board[hint->to]) take(hint->to, it);
    // Castling: the rook moves too.
    if (pieceOf(pos_.board[hint->to]) == kKing && std::abs(hint->to - hint->from) == 2) {
      const bool kingSide = hint->to > hint->from;
      const int rf = kingSide ? hint->from + 3 : hint->from - 4, rt = kingSide ? hint->from + 1 : hint->from - 1;
      if (at_[rf] >= 0) take(rt, at_[rf]);
    }
  }
  for (int sq = 0; sq < 64; ++sq) {
    const int it = at_[sq];
    if (newAt[sq] < 0 && pos_.board[sq] && it >= 0 && !used[static_cast<size_t>(it)] &&
        pieces_[static_cast<size_t>(it)].piece == pos_.board[sq]) {
      take(sq, it);
    }
  }
  for (int sq = 0; sq < 64; ++sq) {
    if (!pos_.board[sq] || newAt[sq] >= 0) continue;
    float best = 1e9f;
    int pick = -1;
    for (size_t i = 0; i < pieces_.size(); ++i) {
      if (used[i] || pieces_[i].piece != pos_.board[sq]) continue;
      const Vec3 d = pieces_[i].shown - squareCenter(sq);
      const float dd = engine::dot(d, d) + (pieces_[i].hidden ? 100.0f : 0.0f);  // spares last
      if (dd < best) {
        best = dd;
        pick = static_cast<int>(i);
      }
    }
    if (pick >= 0) take(sq, pick);
  }
  std::copy(std::begin(newAt), std::end(newAt), std::begin(at_));
  // Targets: on their squares; the rest beside the board (white's captured
  // pieces on the +x side, black's on the -x side) or hidden.
  int lostCount[2] = {0, 0};
  int onBoard[2][7] = {};
  for (int8_t c : pos_.board) {
    if (c) ++onBoard[colorOf(c)][pieceOf(c)];
  }
  constexpr int kStart[7] = {0, 8, 2, 2, 2, 1, 1};
  int lostLeft[2][7];
  for (int col = 0; col < 2; ++col) {
    for (int k = 0; k < 7; ++k) lostLeft[col][k] = std::max(0, kStart[k] - onBoard[col][k]);
  }
  for (size_t i = 0; i < pieces_.size(); ++i) {
    Instance& in = pieces_[i];
    Vec3 target;
    bool hide = false;
    if (used[i]) {
      for (int sq = 0; sq < 64; ++sq) {
        if (at_[sq] == static_cast<int>(i)) target = squareCenter(sq);
      }
    } else {
      const int col = colorOf(in.piece), kind = pieceOf(in.piece);
      if (lostLeft[col][kind] > 0) {
        --lostLeft[col][kind];
        const int n = lostCount[col]++;
        const float side = col == kWhite ? 1.0f : -1.0f;
        target = center_ + Vec3{side * (4.0f * square_ + 0.035f + static_cast<float>(n / 8) * 0.03f), -0.0205f,
                                (static_cast<float>(n % 8) - 3.5f) * square_ * 0.9f};
      } else {
        target = center_ + Vec3{0, -0.6f, 0};  // a spare: under the table
        hide = true;
      }
    }
    if (engine::length(target - in.target) > 1e-4f) {
      in.from = in.shown;
      in.moveStartMs = nowMs;
      in.target = target;
    }
    in.hidden = hide;
  }
}

std::string ChessGame::open(bool online) {
  if (!ready_) return "The chess set isn't here yet.";
  online_ = online;
  mySide_ = kWhite;
  if (online) {
    const int white = engine::net::claimOwner("machine.chess.w");
    const int black = engine::net::claimOwner("machine.chess.b");
    if (white > 0 && black > 0) return "Two people are already playing chess.";
    if (white > 0) {
      mySide_ = kBlack;
      engine::net::claim("machine.chess.b");
    } else {
      engine::net::claim("machine.chess.w");
    }
  }
  open_ = true;
  selected_ = -1;
  engine::ui::setBodyClass("racing", true);  // (hides the page's top bar: the game has the screen)
  if (mySide_ == kBlack) {
    newGame(nowMs_, true);  // you joined white's game: a fresh game between you two
    message_ = "You play black against " + engine::net::playerName(engine::net::claimOwner("machine.chess.w")) + ".";
  } else if (pos_.result() != Result::kPlaying || twoPlayers_) {
    newGame(nowMs_, false);
    message_ = "You play white against the computer.";
  } else {
    message_ = "You play white against the computer.";
  }
  messageMs_ = nowMs_;
  if (!myTurn() && !twoPlayers_) computerAtMs_ = nowMs_ + kComputerDelayMs;
  LOG_INFO("Chess: playing %s%s.", mySide_ == kWhite ? "white" : "black", twoPlayers_ ? " (two players)" : " vs the computer");
  return "";
}

void ChessGame::close() {
  if (!open_) return;
  open_ = false;
  selected_ = -1;
  computerAtMs_ = 0;
  search_.cancel();
  engine::ui::setBodyClass("racing", false);
  if (online_) {
    engine::net::release(mySide_ == kWhite ? "machine.chess.w" : "machine.chess.b");
    if (twoPlayers_) {  // the other player keeps the board, against the computer
      twoPlayers_ = false;
      send();
    }
  }
}

void ChessGame::newGame(double nowMs, bool twoPlayers) {
  pos_ = Position::start();
  twoPlayers_ = twoPlayers;
  lastFrom_ = lastTo_ = -1;
  selected_ = -1;
  computerAtMs_ = 0;
  search_.cancel();
  ++seq_;
  place(nullptr, nowMs);
  send();
}

bool ChessGame::myTurn() const { return open_ && pos_.turn == mySide_; }

void ChessGame::playMove(const Move& m, double nowMs, bool local) {
  pos_.play(m);
  lastFrom_ = m.from;
  lastTo_ = m.to;
  ++seq_;
  place(&m, nowMs);
  if (local) send();
  const Result r = pos_.result();
  if (r != Result::kPlaying) {
    message_ = r == Result::kStalemate ? "Stalemate: a draw." : r == Result::kDraw ? "A draw."
             : (r == Result::kWhiteWins) == (mySide_ == kWhite) ? "Checkmate! You win!" : "Checkmate. You lose.";
    if (!open_) message_.clear();
    messageMs_ = nowMs;
    LOG_INFO("Chess: %s", r == Result::kWhiteWins ? "white wins." : r == Result::kBlackWins ? "black wins." : "draw.");
  } else if (pos_.inCheck(pos_.turn)) {
    message_ = "Check!";
    messageMs_ = nowMs;
  }
}

void ChessGame::send() {
  if (!online_ || engine::net::status() != engine::net::Status::kConnected) return;
  double v[kStateCount] = {};
  for (int k = 0; k < 8; ++k) {
    uint32_t word = 0;
    for (int j = 0; j < 8; ++j) word |= static_cast<uint32_t>(pos_.board[k * 8 + j] & 15) << (4 * j);
    v[k] = static_cast<double>(word);
  }
  v[8] = pos_.turn;
  v[9] = pos_.castling;
  v[10] = pos_.epSquare;
  v[11] = pos_.halfmoves;
  v[12] = pos_.fullmove;
  v[13] = lastFrom_;
  v[14] = lastTo_;
  v[15] = twoPlayers_ ? 1 : 0;
  v[16] = seq_;
  engine::net::setWorld("ch", v, kStateCount);
}

void ChessGame::onRoom(const std::vector<double>& v) {
  if (v.size() < kStateCount || !ready_) return;
  if (static_cast<int>(v[16]) < seq_ && open_) return;  // older than what we have
  Position p;
  for (int k = 0; k < 8; ++k) {
    const auto word = static_cast<uint32_t>(v[k]);
    for (int j = 0; j < 8; ++j) p.board[k * 8 + j] = static_cast<int8_t>((word >> (4 * j)) & 15);
  }
  p.turn = v[8] != 0 ? kBlack : kWhite;
  p.castling = static_cast<uint8_t>(v[9]);
  p.epSquare = static_cast<int8_t>(v[10]);
  p.halfmoves = static_cast<int>(v[11]);
  p.fullmove = static_cast<int>(v[12]);
  const bool changed = p.fen() != pos_.fen();
  const int from = static_cast<int>(v[13]), to = static_cast<int>(v[14]);
  twoPlayers_ = v[15] != 0;
  seq_ = std::max(seq_, static_cast<int>(v[16]));
  if (!changed) return;
  pos_ = p;
  lastFrom_ = from;
  lastTo_ = to;
  Move hint{static_cast<int8_t>(std::clamp(from, 0, 63)), static_cast<int8_t>(std::clamp(to, 0, 63)), 0};
  place(from >= 0 && to >= 0 ? &hint : nullptr, nowMs_);
  selected_ = -1;
  if (open_ && pos_.inCheck(pos_.turn)) {
    message_ = "Check!";
    messageMs_ = nowMs_;
  }
}

int ChessGame::buttonAt(Vec2 tap, float aspect) const {
  (void)aspect;
  if (tap.y > 0.88f && tap.y < 0.97f) {
    if (tap.x > 0.04f && tap.x < 0.46f) return 0;
    if (tap.x > 0.54f && tap.x < 0.96f) return 1;
  }
  return -1;
}

bool ChessGame::update(double nowMs, float dt, const std::vector<Vec2>& taps, const engine::FirstPersonCamera& cam,
                       float aspect) {
  nowMs_ = nowMs;
  (void)dt;
  // Pieces hop toward their squares.
  for (Instance& in : pieces_) {
    if (in.moveStartMs < 0) {
      in.shown = in.target;
      continue;
    }
    const float t = static_cast<float>(std::clamp((nowMs - in.moveStartMs) / kHopMs, 0.0, 1.0));
    const float s = t * t * (3 - 2 * t);
    in.shown = engine::lerp(in.from, in.target, s);
    const float dist = engine::length(in.target - in.from);
    if (dist < 0.4f) in.shown.y += std::sin(t * 3.14159f) * std::min(0.06f, 0.02f + dist * 0.15f);
    if (t >= 1) in.moveStartMs = -1;
  }
  if (!open_) return false;
  // Someone else took our place, or the other player left.
  if (online_ && twoPlayers_ && mySide_ == kWhite && engine::net::claimOwner("machine.chess.b") < 0 &&
      engine::net::status() == engine::net::Status::kConnected) {
    twoPlayers_ = false;
    message_ = "Your opponent left: the computer plays on.";
    messageMs_ = nowMs;
    send();
  }
  // The computer's move (white's phone plays it).
  if (!twoPlayers_ && pos_.turn != mySide_ && pos_.result() == Result::kPlaying) {
    if (computerAtMs_ == 0) computerAtMs_ = nowMs + kComputerDelayMs;
    if (nowMs >= computerAtMs_ && !search_.running()) {
      seed_ = seed_ * 1664525u + 1013904223u + static_cast<uint32_t>(nowMs);
      search_.start(pos_, kComputerDepth, seed_);
    }
    // A few milliseconds of thinking per frame, so the game never freezes.
    if (search_.running() && search_.step(kThinkMsPerFrame)) {
      computerAtMs_ = 0;
      const Move m = search_.best();
      LOG_INFO("Chess: the computer plays %s.", toText(m).c_str());
      playMove(m, nowMs, true);
    }
  }
  for (const Vec2 tap : taps) {
    const int b = buttonAt(tap, aspect);
    if (b == 0) return true;
    if (b == 1) {
      if (twoPlayers_ && mySide_ == kBlack) continue;  // white starts new games
      newGame(nowMs, twoPlayers_);
      message_ = "New game.";
      messageMs_ = nowMs;
      continue;
    }
    if (!myTurn() || pos_.result() != Result::kPlaying) continue;
    // The tap as a ray from the eye, onto the board's surface.
    const float th = std::tan(cam.fovY * 0.5f);
    const Vec3 fwd = cam.forward();
    const Vec3 right = engine::normalize(engine::cross(fwd, {0, 1, 0}));
    const Vec3 up = engine::cross(right, fwd);
    const Vec3 dir = engine::normalize(fwd + right * ((tap.x * 2 - 1) * th * aspect) + up * ((1 - tap.y * 2) * th));
    if (dir.y > -1e-3f) continue;
    const float t = (center_.y - cam.position.y) / dir.y;
    const int sq = squareAt(cam.position + dir * t);
    if (sq < 0) {
      selected_ = -1;
      continue;
    }
    // A highlighted square: move there (pawns reaching the end become queens).
    bool moved = false;
    if (selected_ >= 0) {
      for (const Move& m : legal_) {
        if (m.from == selected_ && m.to == sq && (m.promo == 0 || m.promo == kQueen)) {
          playMove(m, nowMs, true);
          selected_ = -1;
          moved = true;
          break;
        }
      }
    }
    if (moved) continue;
    const int8_t c = pos_.board[sq];
    selected_ = c && colorOf(c) == mySide_ ? sq : -1;
    if (selected_ >= 0) pos_.legalMoves(legal_);
  }
  return false;
}

void ChessGame::pose(SceneRenderer& renderer) const {
  posed_.resize(pieces_.size(), Vec3{1e9f, 0, 0});
  for (size_t i = 0; i < pieces_.size(); ++i) {
    const Instance& in = pieces_[i];
    if (posed_[i] == in.shown) continue;
    posed_[i] = in.shown;
    const bool whiteKnight = pieceOf(in.piece) == kKnight && colorOf(in.piece) == kWhite;
    renderer.setModelTransform(in.model, in.shown, engine::Quat::axisAngle({0, 1, 0}, whiteKnight ? engine::kPi : 0));
  }
}

void ChessGame::camera(engine::FirstPersonCamera& cam) const {
  // Above your edge of the board, looking down at it.
  const float side = mySide_ == kWhite ? 1.0f : -1.0f;
  cam.position = center_ + Vec3{0, 0.5f, side * 0.38f};
  cam.yaw = mySide_ == kWhite ? 0.0f : engine::kPi;  // white looks toward -z
  cam.pitch = -0.92f;
}

std::string ChessGame::statusText() const {
  const Result r = pos_.result();
  if (r == Result::kWhiteWins || r == Result::kBlackWins) {
    return (r == Result::kWhiteWins) == (mySide_ == kWhite) ? "CHECKMATE - YOU WIN!" : "CHECKMATE - YOU LOSE";
  }
  if (r != Result::kPlaying) return r == Result::kStalemate ? "STALEMATE - DRAW" : "DRAW";
  if (pos_.turn == mySide_) return pos_.inCheck(mySide_) ? "CHECK! YOUR MOVE" : "YOUR MOVE";
  return twoPlayers_ ? "THEIR MOVE" : "COMPUTER IS THINKING...";
}

void ChessGame::draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  if (!open_) return;
  // On the board: the last move, your selection, where it can go.
  const auto squareQuad = [&](int sq, Vec4 color, float inset) {
    const Vec3 c = squareCenter(sq) + Vec3{0, 0.0015f, 0};
    const float h = square_ * 0.5f - inset;
    const Vec3 q[4] = {c + Vec3{-h, 0, h}, c + Vec3{h, 0, h}, c + Vec3{h, 0, -h}, c + Vec3{-h, 0, -h}};
    text.addWorldQuad(q, color);
  };
  if (lastFrom_ >= 0) squareQuad(lastFrom_, {1, 0.85f, 0.2f, 0.28f}, 0.002f);
  if (lastTo_ >= 0) squareQuad(lastTo_, {1, 0.85f, 0.2f, 0.4f}, 0.002f);
  if (pos_.inCheck(pos_.turn)) squareQuad(pos_.king(pos_.turn), {1, 0.15f, 0.1f, 0.55f}, 0.0f);
  if (selected_ >= 0) {
    squareQuad(selected_, {0.3f, 0.85f, 1, 0.55f}, 0.0f);
    for (const Move& m : legal_) {
      if (m.from != selected_ || (m.promo && m.promo != kQueen)) continue;
      const bool capture = pos_.board[m.to] || m.to == pos_.epSquare;
      squareQuad(m.to, capture ? Vec4{1, 0.3f, 0.3f, 0.5f} : Vec4{0.4f, 1, 0.5f, 0.45f}, capture ? 0.0f : square_ * 0.3f);
    }
  }
  // HUD: who's turn, a message, two buttons (Clash style: outlined, chunky).
  engine::TextStyle st;
  st.align = 0.5f;
  st.outlineWidth = 0.8f;
  const float u = std::min(W, H) / 400.0f;
  text.addRect(W * 0.1f, H * 0.03f, W * 0.8f, 30 * u, {0.12f, 0.28f, 0.62f, 0.92f});
  text.addRect(W * 0.1f, H * 0.03f + 28 * u, W * 0.8f, 2 * u, {0.06f, 0.09f, 0.18f, 1});
  st.color = pos_.inCheck(pos_.turn) ? Vec4{1, 0.5f, 0.4f, 1} : Vec4{1, 1, 1, 1};
  text.addScreen(statusText(), W / 2, H * 0.03f + 9 * u, 12 * u, st);
  st.color = {0.85f, 0.9f, 1, 1};
  char line[96];
  std::snprintf(line, sizeof(line), "%s  -  MOVE %d", mySide_ == kWhite ? "YOU ARE WHITE" : "YOU ARE BLACK", pos_.fullmove);
  text.addScreen(line, W / 2, H * 0.03f + 36 * u, 7 * u, st);
  if (!message_.empty() && nowMs - messageMs_ < 3500) {
    st.color = {1, 0.85f, 0.3f, static_cast<float>(std::min(1.0, (3500 - (nowMs - messageMs_)) / 400.0))};
    text.addScreen(message_, W / 2, H * 0.03f + 50 * u, 8 * u, st);
  }
  const auto button = [&](float x0, float x1, const char* label, Vec4 color) {
    const float y = H * 0.88f, h = H * 0.09f;
    text.addRect(W * x0, y, W * (x1 - x0), h, color);
    text.addRect(W * x0, y + h - 4 * u, W * (x1 - x0), 4 * u, {0, 0, 0, 0.3f});
    text.addRect(W * x0, y, W * (x1 - x0), 2 * u, {1, 1, 1, 0.35f});
    engine::TextStyle b;
    b.align = 0.5f;
    b.outlineWidth = 0.8f;
    const float cap = std::min(h * 0.4f, 13 * u);
    text.addScreen(label, W * (x0 + x1) / 2, y + (h - cap) / 2, cap, b);
  };
  button(0.04f, 0.46f, "LEAVE", {0.27f, 0.62f, 1.0f, 0.95f});
  if (!(twoPlayers_ && mySide_ == kBlack)) button(0.54f, 0.96f, "NEW GAME", {1.0f, 0.78f, 0.15f, 0.95f});
  (void)px;
}

void ChessGame::stateFor(float out[5]) const {
  out[0] = open_ ? 1.0f : 0.0f;
  out[1] = static_cast<float>(mySide_);
  out[2] = out[3] = out[4] = 0;
}

}  // namespace app
