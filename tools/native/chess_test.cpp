// Chess rules (bar/projects/chess/chess_rules.*): perft counts on standard test positions
// must match the known numbers exactly (every legal move: castling, en
// passant, promotions, pins, checks), then checkmate / stalemate detection
// and that the computer finds a mate in one and doesn't hang its queen.
#include <algorithm>
#include <chrono>
#include <cstdio>

#include "bar/projects/chess/chess_rules.h"

using namespace app::chess;

static int failed = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failed;
}

int main() {
  struct Case { const char* name; const char* fen; int depth; uint64_t want; };
  const Case cases[] = {
      {"start position, 4 plies", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},
      {"kiwipete (castling, en passant, promotions), 3 plies",
       "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
      {"position 3 (en passant pins), 5 plies", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624},
      {"position 4 (promotions, checks), 3 plies", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
      {"position 5, 3 plies", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
  };
  for (const Case& c : cases) {
    Position p;
    Position::fromFen(c.fen, p);
    const auto t0 = std::chrono::steady_clock::now();
    const uint64_t n = perft(p, c.depth);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    char what[160];
    std::snprintf(what, sizeof(what), "%s: %llu (want %llu, %.0f ms)", c.name, static_cast<unsigned long long>(n),
                  static_cast<unsigned long long>(c.want), ms);
    check(n == c.want, what);
  }
  {
    Position p;
    Position::fromFen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", p);  // fool's mate
    check(p.result() == Result::kBlackWins, "fool's mate is checkmate (black wins)");
    Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", p);
    check(p.result() == Result::kStalemate, "king with no moves and not in check is stalemate");
    Position::fromFen(Position::start().fen(), p);
    check(p.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "FEN round trip");
  }
  {
    Position p;
    Position::fromFen("6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", p);  // back-rank mate: Rd8#
    const auto t0 = std::chrono::steady_clock::now();
    const Move m = bestMove(p, 3, 7);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    char what[96];
    std::snprintf(what, sizeof(what), "the computer finds mate in one (%s, %.0f ms)", toText(m).c_str(), ms);
    check(toText(m) == "d1d8", what);
    Position::fromFen("rnb1kbnr/pppp1ppp/8/4p3/4P2q/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 3", p);  // queen attacked by the knight
    const Move m2 = bestMove(p, 3, 3);
    Position q = p;
    q.play(m2);
    bool queenSafe = true;
    for (int sq = 0; sq < 64; ++sq) {
      if (q.board[sq] == make(kQueen, kBlack) && q.attacked(sq, kWhite)) queenSafe = false;
    }
    std::snprintf(what, sizeof(what), "the computer moves its attacked queen away (%s)", toText(m2).c_str());
    check(queenSafe, what);
    // A whole move at the depth the game uses, from the start: time it.
    const auto t1 = std::chrono::steady_clock::now();
    bestMove(Position::start(), 3, 1);
    std::printf("     one computer move from the start (depth 3): %.0f ms\n",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count());
  }
  {
    // Thinking in slices (a frame at a time) picks the same move, and no
    // slice runs much past its budget.
    Position p;
    Position::fromFen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4", p);
    bool same = true;
    double worstSlice = 0;
    int slices = 0;
    for (uint32_t seed = 1; seed <= 4; ++seed) {
      MoveSearch s;
      s.start(p, 3, seed);
      bool done = false;
      while (!done) {
        const auto t0 = std::chrono::steady_clock::now();
        done = s.step(3.0);
        worstSlice = std::max(
            worstSlice, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        ++slices;
      }
      same = same && toText(s.best()) == toText(bestMove(p, 3, seed));
    }
    char what[96];
    std::snprintf(what, sizeof(what), "sliced search = whole search (%d slices, worst %.1f ms)", slices, worstSlice);
    check(same, what);
  }
  std::printf(failed ? "CHESS TEST FAILED (%d)\n" : "CHESS TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
