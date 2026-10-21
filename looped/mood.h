#pragma once
// A scene's mood (looped/post.h draws it): no GPU code here, so game layouts
// that pick a mood stay buildable by the native tests.
#include "looped/math.h"

namespace engine {

// How a scene feels after it's drawn. The defaults change nothing but add
// a little glow.
struct Mood {
  float glow = 0.6f;            // how strongly bright things bleed light
  float glowThreshold = 0.55f;  // how bright (linear, 0..1) something must be to glow
  float glowKnee = 0.25f;       // how softly the glow fades in above the threshold
  Vec3 tint{1, 1, 1};           // e.g. warm amber (1.05, 1.0, 0.93)
  float saturation = 1;
  float contrast = 1;
  float lift = 0;               // raises the darkest parts a little (0..0.1)
  float vignette = 0.2f;        // darker corners (0 = none)
};

}  // namespace engine
