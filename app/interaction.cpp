#include "app/interaction.h"

namespace app {

namespace {
float bias(TargetKind k) {
  switch (k) {
    case TargetKind::kProp: return kPropBias;
    case TargetKind::kPlayer: return kPlayerBias;
    default: return 0;
  }
}
int order(TargetKind k) { return static_cast<int>(k); }  // ties: earlier kinds first
}  // namespace

TapTarget pickTap(const std::vector<TapTarget>& candidates) {
  TapTarget best;
  float bestScore = 0;
  for (const TapTarget& c : candidates) {
    if (c.kind == TargetKind::kNone) continue;
    const float score = c.distance + bias(c.kind);
    if (best.kind == TargetKind::kNone || score < bestScore ||
        (score == bestScore && order(c.kind) < order(best.kind))) {
      best = c;
      bestScore = score;
    }
  }
  return best;
}

}  // namespace app
