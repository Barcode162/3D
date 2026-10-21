// The tap resolver (bar/interaction.*): which of the things a tap hits wins.
#include <cstdio>

#include "bar/interaction.h"

using app::TapTarget;
using app::TargetKind;

static int failed = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failed;
}

int main() {
  check(app::pickTap({}).kind == TargetKind::kNone, "nothing hit: no target");
  {
    const TapTarget t = app::pickTap({{TargetKind::kThing, 4, 1.5f}, {TargetKind::kPlayer, 1, 1.8f}});
    check(t.kind == TargetKind::kThing && t.index == 4, "the jukebox beats a player standing behind it");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kThing, 4, 1.5f}, {TargetKind::kPlayer, 1, 1.3f}});
    check(t.kind == TargetKind::kThing, "...and a player only just in front of it");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kThing, 4, 2.5f}, {TargetKind::kPlayer, 1, 1.0f}});
    check(t.kind == TargetKind::kPlayer && t.index == 1, "a player clearly in front wins (their card opens)");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kThing, 2, 0.9f}, {TargetKind::kProp, 7, 1.2f}});
    check(t.kind == TargetKind::kProp && t.index == 7, "a glass to pick up beats the counter around it");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kSeat, 3, 1.0f}, {TargetKind::kThing, 2, 1.0f}});
    check(t.kind == TargetKind::kThing, "on a tie, a thing to use beats a seat");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kSeat, 3, 0.8f}, {TargetKind::kThing, 2, 1.4f}});
    check(t.kind == TargetKind::kSeat && t.index == 3, "the nearer seat wins over a farther thing");
  }
  {
    const TapTarget t = app::pickTap({{TargetKind::kSeat, 2, 1.2f}, {TargetKind::kTouch, 7, 1.0f}});
    check(t.kind == TargetKind::kSeat, "a stool's seat beats touching the table just in front of it");
    const TapTarget u = app::pickTap({{TargetKind::kSeat, 2, 2.5f}, {TargetKind::kTouch, 7, 1.0f}});
    check(u.kind == TargetKind::kTouch && u.index == 7, "a lamp clearly nearer than a seat gets the touch");
    const TapTarget v = app::pickTap({{TargetKind::kTouch, 7, 1.0f}});
    check(v.kind == TargetKind::kTouch, "a touch alone wins");
  }
  std::printf(failed ? "INTERACTION TEST FAILED (%d)\n" : "INTERACTION TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
