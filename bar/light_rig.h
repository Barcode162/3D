#pragma once
// The bar's light rig (J116.2, the language: looped/words/rig.h): the
// place's lamps as a rig names them, the events the bar sends it, and its
// file (assets/world/rigs/<place>.looped). Plain C++ (no GPU).
#include <string>
#include <vector>

#include "bar/bar_scene.h"
#include "looped/words/rig.h"

namespace app {

// The place's lamps, in the scene's light order (BarScene::lights).
std::vector<engine::words::LampInfo> rigLamps(const BarScene& scene);
// What can happen: music, quiet, fire-on, fire-off, lights-down, lights-up.
const std::vector<std::string>& rigEvents();
// assets/world/rigs/<place>.looped
std::string rigPath(const std::string& place);

}  // namespace app
