#include "bar/scene_names.h"

#include <iterator>

namespace app {

namespace {

template <typename E, size_t N>
const char* nameOf(const char* const (&names)[N], E e) {
  const auto i = static_cast<size_t>(e);
  return i < N ? names[i] : "";
}

template <typename E, size_t N>
bool fromName(const char* const (&names)[N], std::string_view name, E* out) {
  for (size_t i = 0; i < N; ++i)
    if (name == names[i]) {
      *out = static_cast<E>(i);
      return true;
    }
  return false;
}

// In each enum's order (bar/bar_scene.h, bar/materials.h).
constexpr const char* kMaterials[] = {"oak-floor", "dark-wood", "varnished-wood", "door-wood", "brick",
                                      "plaster",   "leather",   "chrome",         "brass",     "black-steel",
                                      "bottle-glass", "lacquer", "glow-glass"};
static_assert(std::size(kMaterials) == kMaterialCount);
constexpr const char* kActions[] = {"none",       "jukebox",  "mirror", "mirror-mode", "rhythm", "darts",
                                    "bartender",  "drink",    "bell",   "guitar",      "mic",    "light-switch",
                                    "fireplace",  "door",     "tv",     "books",       "chess",  "tip-jar",
                                    "taps",       "coffee",   "piano-songs", "arcade"};
static_assert(std::size(kActions) == static_cast<size_t>(Action::kArcade) + 1);
constexpr const char* kSeatKinds[] = {"stool", "chair", "sofa", "piano-bench", "sim"};
constexpr const char* kProps[] = {"", "pint", "rocks"};

}  // namespace

const char* materialName(MaterialId m) { return nameOf(kMaterials, m); }
bool materialFromName(std::string_view n, MaterialId* out) { return fromName(kMaterials, n, out); }
const char* actionName(Action a) { return nameOf(kActions, a); }
bool actionFromName(std::string_view n, Action* out) { return fromName(kActions, n, out); }
const char* seatKindName(SeatKind k) { return nameOf(kSeatKinds, k); }
bool seatKindFromName(std::string_view n, SeatKind* out) { return fromName(kSeatKinds, n, out); }
const char* propName(Prop p) { return nameOf(kProps, p); }
bool propFromName(std::string_view n, Prop* out) { return fromName(kProps, n, out); }

}  // namespace app
