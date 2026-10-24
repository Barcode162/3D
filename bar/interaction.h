#pragma once
// What a tap lands on: everything the tap's ray hits (a thing to pick up,
// another player, something to use, a seat) is a candidate, and the nearest
// wins after a small bias per kind:
// - things to pick up always win (a glass on the counter, not the counter);
// - another player must be clearly nearer than a thing (kPlayerBias), so a
//   player standing behind the jukebox doesn't steal the tap;
// - on an exact tie a thing to use beats a seat;
// - touching a living thing (a lamp, a table: its kind has a "tap" rule,
//   docs/archive/ENGINE.md) comes last: it must be clearly nearer than the rest
//   (kTouchBias), so a stool's seat or the jukebox still wins.
// Pure logic (no engine): tested by tools/native/interaction_test.cpp.
#include <vector>

namespace app {

enum class TargetKind { kNone, kProp, kPlayer, kThing, kSeat, kTouch };

struct TapTarget {
  TargetKind kind = TargetKind::kNone;
  int index = -1;       // body, player, interactable, seat or world thing
  float distance = 0;   // along the tap's ray, metres
};

constexpr float kPropBias = -100.0f;  // always first
constexpr float kPlayerBias = 0.35f;  // metres a player must be nearer by
constexpr float kTouchBias = 0.3f;    // ... and a thing to touch

// The winner among `candidates` (kind kNone when there are none).
TapTarget pickTap(const std::vector<TapTarget>& candidates);

}  // namespace app
