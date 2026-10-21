#pragma once
// Words for the bar's enums in place and kind files (assets/world/): what a
// thing is made of ("leather"), what tapping it does ("jukebox"), what kind
// of seat it is ("sofa"), which glass it is ("pint").
#include <string>
#include <string_view>

#include "bar/bar_scene.h"

namespace app {

const char* materialName(MaterialId m);
bool materialFromName(std::string_view name, MaterialId* out);
const char* actionName(Action a);
bool actionFromName(std::string_view name, Action* out);
const char* seatKindName(SeatKind k);
bool seatKindFromName(std::string_view name, SeatKind* out);
const char* propName(Prop p);  // "" for none
bool propFromName(std::string_view name, Prop* out);

}  // namespace app
