#pragma once
// Stable ids for every thing in the bar that other players can share: seats,
// things you use, mirrors, movable props (RESTRUCTURE 4.2). An id is a
// readable string made from what the thing is and where it stands
// ("seat.stool@-2.5,-1.3"), so it doesn't change when the layout code adds
// or reorders things; its 16-bit net id is a hash of that string (what the
// room protocol will carry, docs/PROTOCOL.md §3.3).
// tools/native/entity_test.cpp checks they're unique and equal the saved list
// app/entity_ids.txt (a changed id there is a deliberate layout change).
// Pure logic (no engine): the native test builds the real scene.
#include <cstdint>
#include <string>
#include <vector>

#include "app/bar_scene.h"

namespace app {

struct Entity {
  enum class Kind { kSeat, kThing, kMirror, kProp };
  Kind kind;
  size_t index;      // into scene.seats / interactables / mirrors / models
  std::string id;
  uint16_t netId;
};

// FNV-1a over the id, folded to 16 bits (0 is never used: it means "none").
uint16_t netIdOf(const std::string& id);
// Every shareable thing in the scene, in a fixed order (seats, things,
// mirrors, props).
std::vector<Entity> listEntities(const BarScene& scene);

}  // namespace app
