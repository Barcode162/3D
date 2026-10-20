#include "app/entities.h"

#include <cmath>
#include <cstdio>

namespace app {

namespace {

// Decimetres: a thing has to move 5 cm to get a new id.
std::string at(engine::Vec3 p, bool withY) {
  char buf[64];
  auto r = [](float v) { return std::round(v * 10.0f) / 10.0f + 0.0f; };  // (+0: no "-0")
  if (withY) std::snprintf(buf, sizeof(buf), "@%.1f,%.1f,%.1f", r(p.x), r(p.y), r(p.z));
  else std::snprintf(buf, sizeof(buf), "@%.1f,%.1f", r(p.x), r(p.z));
  return buf;
}

const char* seatKind(SeatKind k) {
  switch (k) {
    case SeatKind::kStool: return "stool";
    case SeatKind::kChair: return "chair";
    case SeatKind::kSofa: return "sofa";
    case SeatKind::kPianoBench: return "bench";
    case SeatKind::kSim: return "rig";
  }
  return "seat";
}

const char* actionName(Action a) {
  static const char* kNames[] = {"none",   "jukebox", "mirror", "mirrormode", "rhythm", "darts",  "bartender",
                                 "drink",  "bell",    "guitar", "mic",        "lights", "fire",   "door",
                                 "tv",     "books",   "chess",  "tips",       "taps",   "coffee", "sheets",
                                 "arcade"};
  const auto i = static_cast<size_t>(a);
  return i < sizeof(kNames) / sizeof(kNames[0]) ? kNames[i] : "thing";
}

}  // namespace

uint16_t netIdOf(const std::string& id) {
  uint32_t h = 2166136261u;
  for (const char c : id) {
    h ^= static_cast<uint8_t>(c);
    h *= 16777619u;
  }
  const auto folded = static_cast<uint16_t>((h >> 16) ^ (h & 0xffff));
  return folded ? folded : 1;
}

std::vector<Entity> listEntities(const BarScene& scene) {
  std::vector<Entity> out;
  auto add = [&](Entity::Kind kind, size_t index, std::string id) {
    const uint16_t net = netIdOf(id);
    out.push_back({kind, index, std::move(id), net});
  };
  for (size_t i = 0; i < scene.seats.size(); ++i) {
    const Seat& s = scene.seats[i];
    add(Entity::Kind::kSeat, i, std::string("seat.") + seatKind(s.kind) + at(s.hip, false));
  }
  for (size_t i = 0; i < scene.interactables.size(); ++i) {
    const Interactable& it = scene.interactables[i];
    const engine::Vec3 c = (it.boundsMin + it.boundsMax) * 0.5f;
    std::string id = std::string("thing.") + actionName(it.action) + at(c, true);
    if (it.action == Action::kMirrorMode || it.action == Action::kArcade) id += "#" + std::to_string(it.value);
    add(Entity::Kind::kThing, i, std::move(id));
  }
  for (size_t i = 0; i < scene.mirrors.size(); ++i) {
    add(Entity::Kind::kMirror, i, "mirror" + at(scene.mirrors[i].center, true));
  }
  for (size_t i = 0; i < scene.models.size(); ++i) {
    const ModelPlacement& m = scene.models[i];
    if (m.prop == Prop::kNone) continue;
    add(Entity::Kind::kProp, i, std::string("prop.") + (m.prop == Prop::kPintGlass ? "pint" : "rocks") + at(m.position, true));
  }
  return out;
}

}  // namespace app
