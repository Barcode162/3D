#include "app/chess_rules.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace app::chess {
namespace {

constexpr int kKnightJumps[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
constexpr int kKingSteps[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
constexpr int kRookDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr int kBishopDirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

inline bool onBoard(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }
inline int sqOf(int f, int r) { return r * 8 + f; }

// Pseudo-legal moves (may leave the king in check).
void pseudoMoves(const Position& p, std::vector<Move>& out) {
  const Color us = p.turn, them = us == kWhite ? kBlack : kWhite;
  const int fwd = us == kWhite ? 1 : -1;
  for (int sq = 0; sq < 64; ++sq) {
    const int8_t c = p.board[sq];
    if (!c || colorOf(c) != us) continue;
    const int f = sq % 8, r = sq / 8;
    const int kind = pieceOf(c);
    const auto add = [&](int to) {
      if ((kind == kPawn) && (to / 8 == 7 || to / 8 == 0)) {
        for (int8_t pr : {kQueen, kRook, kBishop, kKnight}) out.push_back({static_cast<int8_t>(sq), static_cast<int8_t>(to), pr});
      } else {
        out.push_back({static_cast<int8_t>(sq), static_cast<int8_t>(to), 0});
      }
    };
    if (kind == kPawn) {
      const int r1 = r + fwd;
      if (onBoard(f, r1) && !p.board[sqOf(f, r1)]) {
        add(sqOf(f, r1));
        const int startRank = us == kWhite ? 1 : 6;
        if (r == startRank && !p.board[sqOf(f, r + 2 * fwd)]) add(sqOf(f, r + 2 * fwd));
      }
      for (int df : {-1, 1}) {
        if (!onBoard(f + df, r1)) continue;
        const int to = sqOf(f + df, r1);
        const int8_t t = p.board[to];
        if ((t && colorOf(t) == them) || to == p.epSquare) add(to);
      }
    } else if (kind == kKnight || kind == kKing) {
      const auto& steps = kind == kKnight ? kKnightJumps : kKingSteps;
      for (const auto& d : steps) {
        const int nf = f + d[0], nr = r + d[1];
        if (!onBoard(nf, nr)) continue;
        const int8_t t = p.board[sqOf(nf, nr)];
        if (!t || colorOf(t) == them) add(sqOf(nf, nr));
      }
      if (kind == kKing) {
        // Castling: the squares between empty, the king not in check, not
        // passing through or landing on an attacked square.
        const int home = us == kWhite ? 4 : 60;
        if (sq == home && !p.attacked(home, them)) {
          const uint8_t ks = us == kWhite ? 1 : 4, qs = us == kWhite ? 2 : 8;
          if ((p.castling & ks) && !p.board[home + 1] && !p.board[home + 2] && p.board[home + 3] == make(kRook, us) &&
              !p.attacked(home + 1, them) && !p.attacked(home + 2, them)) {
            out.push_back({static_cast<int8_t>(home), static_cast<int8_t>(home + 2), 0});
          }
          if ((p.castling & qs) && !p.board[home - 1] && !p.board[home - 2] && !p.board[home - 3] &&
              p.board[home - 4] == make(kRook, us) && !p.attacked(home - 1, them) && !p.attacked(home - 2, them)) {
            out.push_back({static_cast<int8_t>(home), static_cast<int8_t>(home - 2), 0});
          }
        }
      }
    } else {
      const bool straight = kind == kRook || kind == kQueen, diagonal = kind == kBishop || kind == kQueen;
      for (int k = 0; k < 2; ++k) {
        if ((k == 0 && !straight) || (k == 1 && !diagonal)) continue;
        const auto& dirs = k == 0 ? kRookDirs : kBishopDirs;
        for (const auto& d : dirs) {
          int nf = f + d[0], nr = r + d[1];
          while (onBoard(nf, nr)) {
            const int8_t t = p.board[sqOf(nf, nr)];
            if (t && colorOf(t) == us) break;
            add(sqOf(nf, nr));
            if (t) break;
            nf += d[0];
            nr += d[1];
          }
        }
      }
    }
  }
}

// ---- Evaluation: material + piece-square tables (from white's side, a1 = 0) ----
constexpr int kValue[7] = {0, 100, 320, 330, 500, 900, 0};
constexpr int kPawnTable[64] = {0,  0,  0,  0,   0,   0,  0,  0,  5,  10, 10, -20, -20, 10, 10, 5,  5, -5, -10, 0,  0,  -10,
                                -5, 5,  0,  0,  0,  20,  20,  0,  0,  0,  5,  5,  10,  25,  25, 10, 5,  10, 10, 10, 20, 30,
                                30, 20, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 0,   0,   0,   0,  0,  0,  0,  0};
constexpr int kKnightTable[64] = {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   5,   5,   0,   -20, -40,
                                  -30, 5,   10,  15,  15,  10,  5,   -30, -30, 0,   15,  20,  20,  15,  0,   -30,
                                  -30, 5,   15,  20,  20,  15,  5,   -30, -30, 0,   10,  15,  15,  10,  0,   -30,
                                  -40, -20, 0,   0,   0,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
constexpr int kCenterTable[64] = {-10, -5, -5, -5, -5, -5, -5, -10, -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0,  5,  5,  5,  5,
                                  0,   -5, -5, 0,  5,  10, 10, 5,  0,  -5, -5, 0,  5,  10, 10, 5,  0,  -5, -5, 0,  5,  5,
                                  5,   5,  0,  -5, -5, 0,  0,  0,  0,  0,  0,  -5, -10, -5, -5, -5, -5, -5, -5, -10};
constexpr int kKingTable[64] = {20,  30,  10,  0,   0,   10,  30,  20,  20,  20,  0,   0,   0,   0,   20,  20,
                                -10, -20, -20, -20, -20, -20, -20, -10, -20, -30, -30, -40, -40, -30, -30, -20,
                                -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
                                -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};

int evaluate(const Position& p) {  // from the side to move's view
  int score = 0;
  for (int sq = 0; sq < 64; ++sq) {
    const int8_t c = p.board[sq];
    if (!c) continue;
    const int kind = pieceOf(c);
    const Color col = colorOf(c);
    const int t = col == kWhite ? sq : (7 - sq / 8) * 8 + sq % 8;  // mirror for black
    int v = kValue[kind];
    switch (kind) {
      case kPawn: v += kPawnTable[t]; break;
      case kKnight: v += kKnightTable[t]; break;
      case kKing: v += kKingTable[t]; break;
      default: v += kCenterTable[t]; break;
    }
    score += col == kWhite ? v : -v;
  }
  return p.turn == kWhite ? score : -score;
}

constexpr int kMate = 100000;

int quiesce(const Position& p, int alpha, int beta, int depth) {
  const int stand = evaluate(p);
  if (depth <= 0) return stand;
  if (stand >= beta) return beta;
  alpha = std::max(alpha, stand);
  std::vector<Move> moves;
  p.legalMoves(moves);
  for (const Move& m : moves) {
    if (!p.board[m.to] && m.to != p.epSquare && !m.promo) continue;  // captures and promotions only
    Position q = p;
    q.play(m);
    const int s = -quiesce(q, -beta, -alpha, depth - 1);
    if (s >= beta) return beta;
    alpha = std::max(alpha, s);
  }
  return alpha;
}

int search(const Position& p, int depth, int alpha, int beta, int ply) {
  std::vector<Move> moves;
  p.legalMoves(moves);
  if (moves.empty()) return p.inCheck(p.turn) ? -kMate + ply : 0;
  if (depth <= 0) return quiesce(p, alpha, beta, 4);
  // Captures first (most valuable victim): better cut-offs.
  std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
    return kValue[pieceOf(p.board[a.to])] + a.promo * 50 > kValue[pieceOf(p.board[b.to])] + b.promo * 50;
  });
  for (const Move& m : moves) {
    Position q = p;
    q.play(m);
    const int s = -search(q, depth - 1, -beta, -alpha, ply + 1);
    if (s >= beta) return beta;
    alpha = std::max(alpha, s);
  }
  return alpha;
}

}  // namespace

Position Position::start() {
  Position p;
  fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", p);
  return p;
}

bool Position::fromFen(const std::string& fen, Position& out) {
  Position p;
  std::istringstream in(fen);
  std::string placement, side, castle, ep;
  in >> placement >> side >> castle >> ep >> p.halfmoves >> p.fullmove;
  int r = 7, f = 0;
  for (char ch : placement) {
    if (ch == '/') { --r; f = 0; continue; }
    if (ch >= '1' && ch <= '8') { f += ch - '0'; continue; }
    const char lower = static_cast<char>(ch | 32);
    const int kind = std::string("pnbrqk").find(lower) + 1;
    if (kind <= 0 || !onBoard(f, r)) return false;
    p.board[sqOf(f, r)] = make(static_cast<Piece>(kind), ch == lower ? kBlack : kWhite);
    ++f;
  }
  p.turn = side == "b" ? kBlack : kWhite;
  p.castling = 0;
  for (char ch : castle) {
    if (ch == 'K') p.castling |= 1;
    if (ch == 'Q') p.castling |= 2;
    if (ch == 'k') p.castling |= 4;
    if (ch == 'q') p.castling |= 8;
  }
  p.epSquare = ep.size() == 2 ? static_cast<int8_t>(sqOf(ep[0] - 'a', ep[1] - '1')) : -1;
  if (p.fullmove < 1) p.fullmove = 1;
  out = p;
  return true;
}

std::string Position::fen() const {
  std::string s;
  for (int r = 7; r >= 0; --r) {
    int empty = 0;
    for (int f = 0; f < 8; ++f) {
      const int8_t c = board[sqOf(f, r)];
      if (!c) { ++empty; continue; }
      if (empty) { s += static_cast<char>('0' + empty); empty = 0; }
      const char ch = " pnbrqk"[pieceOf(c)];
      s += colorOf(c) == kWhite ? static_cast<char>(ch & ~32) : ch;
    }
    if (empty) s += static_cast<char>('0' + empty);
    if (r) s += '/';
  }
  s += turn == kWhite ? " w " : " b ";
  std::string c;
  if (castling & 1) c += 'K';
  if (castling & 2) c += 'Q';
  if (castling & 4) c += 'k';
  if (castling & 8) c += 'q';
  s += c.empty() ? "-" : c;
  s += " " + (epSquare >= 0 ? squareName(epSquare) : std::string("-"));
  s += " " + std::to_string(halfmoves) + " " + std::to_string(fullmove);
  return s;
}

bool Position::attacked(int sq, Color by) const {
  const int f = sq % 8, r = sq / 8;
  // Pawns attack diagonally forward (from their side).
  const int pr = by == kWhite ? r - 1 : r + 1;
  for (int df : {-1, 1}) {
    if (onBoard(f + df, pr) && board[sqOf(f + df, pr)] == make(kPawn, by)) return true;
  }
  for (const auto& d : kKnightJumps) {
    if (onBoard(f + d[0], r + d[1]) && board[sqOf(f + d[0], r + d[1])] == make(kKnight, by)) return true;
  }
  for (const auto& d : kKingSteps) {
    if (onBoard(f + d[0], r + d[1]) && board[sqOf(f + d[0], r + d[1])] == make(kKing, by)) return true;
  }
  for (int k = 0; k < 2; ++k) {
    const auto& dirs = k == 0 ? kRookDirs : kBishopDirs;
    const int8_t a = make(k == 0 ? kRook : kBishop, by), q = make(kQueen, by);
    for (const auto& d : dirs) {
      int nf = f + d[0], nr = r + d[1];
      while (onBoard(nf, nr)) {
        const int8_t t = board[sqOf(nf, nr)];
        if (t) {
          if (t == a || t == q) return true;
          break;
        }
        nf += d[0];
        nr += d[1];
      }
    }
  }
  return false;
}

int Position::king(Color c) const {
  for (int sq = 0; sq < 64; ++sq) {
    if (board[sq] == make(kKing, c)) return sq;
  }
  return -1;
}

bool Position::inCheck(Color c) const {
  const int k = king(c);
  return k >= 0 && attacked(k, c == kWhite ? kBlack : kWhite);
}

void Position::legalMoves(std::vector<Move>& out) const {
  out.clear();
  std::vector<Move> pseudo;
  pseudoMoves(*this, pseudo);
  for (const Move& m : pseudo) {
    Position q = *this;
    q.play(m);
    if (!q.inCheck(turn)) out.push_back(m);
  }
}

bool Position::isLegal(const Move& m) const {
  std::vector<Move> moves;
  legalMoves(moves);
  return std::find(moves.begin(), moves.end(), m) != moves.end();
}

int8_t Position::play(const Move& m) {
  const int8_t piece = board[m.from];
  int8_t captured = board[m.to];
  const int kind = pieceOf(piece);
  const Color us = colorOf(piece);
  // En passant: the captured pawn is beside, not on the target square.
  if (kind == kPawn && m.to == epSquare && !captured) {
    const int victim = m.to + (us == kWhite ? -8 : 8);
    captured = board[victim];
    board[victim] = 0;
  }
  board[m.to] = m.promo && kind == kPawn ? make(static_cast<Piece>(m.promo), us) : piece;
  board[m.from] = 0;
  // Castling moves the rook too.
  if (kind == kKing && std::abs(m.to - m.from) == 2) {
    const bool kingSide = m.to > m.from;
    const int rookFrom = kingSide ? m.from + 3 : m.from - 4, rookTo = kingSide ? m.from + 1 : m.from - 1;
    board[rookTo] = board[rookFrom];
    board[rookFrom] = 0;
  }
  // Castling rights: lost when the king or a rook moves, or a rook is taken.
  const auto lose = [&](int sq) {
    if (sq == 4) castling &= ~3;
    if (sq == 60) castling &= ~12;
    if (sq == 7) castling &= ~1;
    if (sq == 0) castling &= ~2;
    if (sq == 63) castling &= ~4;
    if (sq == 56) castling &= ~8;
  };
  lose(m.from);
  lose(m.to);
  epSquare = kind == kPawn && std::abs(m.to - m.from) == 16 ? static_cast<int8_t>((m.from + m.to) / 2) : -1;
  halfmoves = kind == kPawn || captured ? 0 : halfmoves + 1;
  if (us == kBlack) ++fullmove;
  turn = us == kWhite ? kBlack : kWhite;
  return captured;
}

Result Position::result() const {
  std::vector<Move> moves;
  legalMoves(moves);
  if (moves.empty()) {
    if (!inCheck(turn)) return Result::kStalemate;
    return turn == kWhite ? Result::kBlackWins : Result::kWhiteWins;
  }
  if (halfmoves >= 100) return Result::kDraw;
  // Bare kings (or a single minor piece): nobody can mate.
  int others = 0, minors = 0;
  for (int8_t c : board) {
    if (!c || pieceOf(c) == kKing) continue;
    ++others;
    if (pieceOf(c) == kKnight || pieceOf(c) == kBishop) ++minors;
  }
  if (others == 0 || (others == 1 && minors == 1)) return Result::kDraw;
  return Result::kPlaying;
}

void MoveSearch::start(const Position& p, int depth, uint32_t seed) {
  root_ = p;
  moves_.clear();
  p.legalMoves(moves_);
  // Shuffle first (seeded), so equal moves aren't always picked the same way.
  for (size_t i = moves_.size(); i > 1; --i) {
    seed = seed * 1664525u + 1013904223u;
    std::swap(moves_[i - 1], moves_[(seed >> 8) % i]);
  }
  next_ = 0;
  best_ = moves_.empty() ? Move{} : moves_[0];
  bestScore_ = -kMate * 2;
  depth_ = depth;
  running_ = true;
}

bool MoveSearch::step(double budgetMs) {
  if (!running_) return false;
  const auto t0 = std::chrono::steady_clock::now();
  while (next_ < moves_.size()) {
    const Move& m = moves_[next_++];
    Position q = root_;
    q.play(m);
    const int s = -search(q, depth_ - 1, -kMate * 2, -bestScore_, 1);
    if (s > bestScore_) {
      bestScore_ = s;
      best_ = m;
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (ms >= budgetMs) break;
  }
  if (next_ < moves_.size()) return false;
  running_ = false;
  return true;
}

Move bestMove(const Position& p, int depth, uint32_t seed) {
  MoveSearch s;
  s.start(p, depth, seed);
  while (!s.step(1e9)) {
  }
  return s.best();
}

std::string squareName(int sq) {
  return {static_cast<char>('a' + sq % 8), static_cast<char>('1' + sq / 8)};
}

std::string toText(const Move& m) {
  std::string s = squareName(m.from) + squareName(m.to);
  if (m.promo) s += " nbrq"[m.promo - 1];
  return s;
}

bool fromText(const std::string& s, Move& out) {
  if (s.size() < 4) return false;
  const int f0 = s[0] - 'a', r0 = s[1] - '1', f1 = s[2] - 'a', r1 = s[3] - '1';
  if (!onBoard(f0, r0) || !onBoard(f1, r1)) return false;
  out = {static_cast<int8_t>(sqOf(f0, r0)), static_cast<int8_t>(sqOf(f1, r1)), 0};
  if (s.size() >= 5) {
    const size_t k = std::string("nbrq").find(s[4]);
    if (k != std::string::npos) out.promo = static_cast<int8_t>(k + 2);
  }
  return true;
}

uint64_t perft(const Position& p, int depth) {
  if (depth == 0) return 1;
  std::vector<Move> moves;
  p.legalMoves(moves);
  if (depth == 1) return moves.size();
  uint64_t n = 0;
  for (const Move& m : moves) {
    Position q = p;
    q.play(m);
    n += perft(q, depth - 1);
  }
  return n;
}

}  // namespace app::chess
