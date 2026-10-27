#include "bar/light_rig.h"

#include "looped/light_units.h"
#include "looped/world/world.h"

namespace app {

std::vector<engine::words::LampInfo> rigLamps(const BarScene& scene) {
  std::vector<engine::words::LampInfo> out;
  for (size_t i = 0; i < scene.lights.size(); ++i) {
    const engine::PointLight& l = scene.lights[i];
    engine::words::LampInfo m;
    const int thing = i < scene.lightThing.size() ? scene.lightThing[i] : -1;
    if (thing >= 0 && scene.world) {
      const engine::world::Thing& t = scene.world->thing(thing);
      m.id = t.id;
      m.kind = t.kind ? t.kind->id : "";
      m.seed = t.seed;
    } else {
      m.id = "light-" + std::to_string(i);
    }
    m.lumens = l.lumens > 0 ? l.lumens : engine::light::lumensFromClassic(l.intensity);
    m.kelvin = l.kelvin;
    out.push_back(std::move(m));
  }
  return out;
}

const std::vector<std::string>& rigEvents() {
  static const std::vector<std::string> events = {"music", "quiet", "fire-on", "fire-off", "lights-down", "lights-up"};
  return events;
}

std::string rigPath(const std::string& place) { return "assets/world/rigs/" + place + ".looped"; }

}  // namespace app
