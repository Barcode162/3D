#pragma once
// Chess rules for the lounge's chessboard (upgrade plan item 13): the
// position, every legal move (castling, en passant, promotion), check,
// checkmate, stalemate and the simple draws, plus a small computer opponent.
// No engine code: plain C++, checked by tools/native/chess_test.cpp (perft
// counts on standard positions).
#include <cstdint>
#include <string>
#include <vector>

namespace app::chess {

// Squares 0..63: a1 = 0, b1 = 1, ..., h8 = 63 (file = sq % 8, rank = sq / 8).
enum Piece : int8_t { kEmpty = 0, kPawn = 1, kKnight, kBishop, kRook, kQueen, kKing };
enum Color : int8_t { kWhite = 0, kBlack = 1 };

// A square's content: 0 = empty, else piece + 8 for black (1..6 white, 9..14 black).
inline int pieceOf(int8_t c) { return c & 7; }
inline Color colorOf(int8_t c) { return (c & 8) ? kBlack : kWhite; }
inline int8_t make(Piece p, Color col) { return static_cast<int8_t>(p | (col == kBlack ? 8 : 0)); }

struct Move {
  int8_t from = 0, to = 0;
  int8_t promo = 0;  // a piece type when a pawn promotes (queen unless chosen)
  bool operator==(const Move& o) const { return from == o.from && to == o.to && promo == o.promo; }
};

enum class Result { kPlaying, kWhiteWins, kBlackWins, kStalemate, kDraw };

struct Position {
  int8_t board[64] = {};
  Color turn = kWhite;
  uint8_t castling = 15;  // bits: 1 white king side, 2 white queen side, 4 black king side, 8 black queen side
  int8_t epSquare = -1;   // the square a pawn just skipped (en passant target), -1 = none
  int halfmoves = 0;      // since the last capture or pawn move (50-move rule)
  int fullmove = 1;

  static Position start();
  // FEN, for tests and for sending a position around.
  static bool fromFen(const std::string& fen, Position& out);
  std::string fen() const;

  bool attacked(int sq, Color by) const;  // is `sq` attacked by `by`?
  bool inCheck(Color c) const;
  int king(Color c) const;
  void legalMoves(std::vector<Move>& out) const;
  bool isLegal(const Move& m) const;
  // Plays a move (assumed legal). Returns the captured piece (0 = none).
  int8_t play(const Move& m);
  Result result() const;  // checkmate, stalemate, 50 moves, bare kings
};

// The computer's move: a short search (alpha-beta, `depth` plies plus
// captures) with material and simple piece-square tables. `seed` varies its
// choice among equal moves so games differ.
Move bestMove(const Position& p, int depth, uint32_t seed);

// The same search, done a little at a time (a few root moves per call), so
// the computer thinks across several frames instead of freezing one.
// Gives exactly the move bestMove would.
class MoveSearch {
 public:
  void start(const Position& p, int depth, uint32_t seed);
  // Searches for about `budgetMs`; true once the move is chosen.
  bool step(double budgetMs);
  void cancel() { running_ = false; }
  bool running() const { return running_; }
  Move best() const { return best_; }

 private:
  Position root_;
  std::vector<Move> moves_;
  size_t next_ = 0;
  Move best_{};
  int bestScore_ = 0;
  int depth_ = 0;
  bool running_ = false;
};

// "e2e4"-style text for a move (and back).
std::string toText(const Move& m);
bool fromText(const std::string& s, Move& out);
std::string squareName(int sq);

// Counts the leaf nodes of the move tree `depth` plies deep (perft).
uint64_t perft(const Position& p, int depth);

}  // namespace app::chess
