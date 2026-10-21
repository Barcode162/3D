#pragma once
// The bar as things (docs/ENGINE.md E2, first pass): every model and light
// of the bar's layout (bar/bar_scene.cpp) becomes a thing of a kind from
// assets/world/kinds/bar.json, plus the small living things (moths at the
// lamps, sparks over the fire). Later the layout itself moves into a place
// file; until then this is the bridge. Pure data, no GPU (tested natively:
// tools/native/bar_world_test.cpp).
#include <string>

#include "bar/bar_scene.h"
#include "looped/world/kind.h"

namespace app {

// A model's kind: its file's name without ".glb" (bar_stool.glb →
// "bar_stool"), or the nearest shorter name that is a kind (chess_king_w →
// "chess"). "" if none.
std::string kindForModel(const engine::world::Library& kinds, const std::string& file);

// Which light of the bar plays which kind of loop (fire, jukebox, the
// arcade's neon wave...); the rest are warm bulbs or coloured neon.
std::string kindForLight(const BarScene& scene, size_t light, float* phase);

// Every thing in the bar. Each carries a "slot" part saying what it drives:
// {"model": i} (BarScene::models), {"light": i} (BarScene::lights), or
// nothing (it draws itself: its "glowbox" part).
engine::world::Place barPlace(const BarScene& scene, const engine::world::Library& kinds);

}  // namespace app
