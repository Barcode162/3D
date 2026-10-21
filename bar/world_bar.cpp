#include "bar/world_bar.h"

#include <algorithm>
#include <cmath>

#include "bar/bar_place.h"
#include "looped/fetch.h"
#include "looped/json.h"
#include "looped/log.h"
#include "looped/net.h"

namespace app {

namespace {
constexpr float kShyDistance = 1.2f;  // metres: closer than this, shy things dart away
constexpr char kKindsFile[] = "assets/world/kinds/bar.json";
constexpr char kSongsFile[] = "assets/world/songs.json";
}  // namespace

void BarWorld::load(const BarScene& scene) {
  if (requested_) return;
  requested_ = true;
  scene_ = &scene;
  engine::fetchFile(kKindsFile, [this](bool ok, std::vector<uint8_t> bytes) {
    if (!ok) LOG_ERROR("World: %s didn't load.", kKindsFile);
    kindsText_.assign(bytes.begin(), bytes.end());
    if (++files_ == 2) build();
  });
  engine::fetchFile(kSongsFile, [this](bool ok, std::vector<uint8_t> bytes) {
    if (ok) {
      const engine::Json root = engine::Json::parse(std::string(bytes.begin(), bytes.end()));
      for (const auto& [file, song] : root.members())
        if (song.isObject()) tempos_.push_back({file, song["bpm"].number(0), song["firstBeat"].number(0)});
    }
    if (++files_ == 2) build();
  });
}

void BarWorld::build() {
  if (kindsText_.empty()) return;
  engine::world::Library kinds;
  kinds.add(kindsText_, kKindsFile);
  kinds.resolve();
  if (!kinds.errors().empty()) LOG_ERROR("World: %s", kinds.errors().c_str());
  std::string errors;
  world_.build(kinds, barPlace(*scene_, kinds), &errors);
  if (!errors.empty()) LOG_ERROR("World: %s", errors.c_str());
  for (const std::string& p : world_.check()) LOG_ERROR("World: %s", p.c_str());
  drawn_.clear();
  for (size_t i = 0; i < world_.size(); ++i) {
    const int t = static_cast<int>(i);
    const engine::world::Thing& th = world_.thing(t);
    if (!th.kind->still.empty()) continue;  // nothing to play
    Drawn d;
    d.thing = t;
    const engine::Json& slot = world_.part(t, "slot");
    d.model = slot["model"].integer(-1);
    d.light = slot["light"].integer(-1);
    d.glowbox = world_.part(t, "glowbox").isObject();
    if (d.model >= 0 || d.light >= 0 || d.glowbox) drawn_.push_back(d);
  }
  built_ = true;
  LOG_INFO("World: %zu things alive (%zu kinds, %zu songs with a beat).", drawn_.size(), kinds.kinds().size(),
           tempos_.size());
}

void BarWorld::setLightBase(int light, float scale) {
  if (light < 0) return;
  if (base_.size() <= static_cast<size_t>(light)) base_.resize(static_cast<size_t>(light) + 1, 1.0f);
  base_[static_cast<size_t>(light)] = scale;
}

void BarWorld::update(const Inputs& in, std::vector<SceneRenderer::Box>& boxes, SceneRenderer& renderer) {
  if (!built_) return;
  const float dt = lastMs_ > 0 ? static_cast<float>(std::clamp((in.nowMs - lastMs_) / 1000.0, 0.0, 0.1)) : 0.0f;
  lastMs_ = in.nowMs;
  // The shared clock: the room's when known, else this phone's.
  engine::world::Clock clock;
  const double server = engine::net::serverTime();
  clock.seconds = server > 0 ? server : in.nowMs / 1000.0;
  if (in.songSeconds >= 0) {
    const std::string file = in.songUrl.substr(in.songUrl.find_last_of('/') + 1);
    for (const Tempo& t : tempos_) {
      if (t.file != file || t.bpm <= 0) continue;
      clock.music = true;
      clock.beats = (in.songSeconds - t.firstBeat) * t.bpm / 60.0;
    }
  }
  // The bar's events, to every thing (those with a rule for it follow it).
  const bool music = in.songSeconds >= 0;
  if (music != music_) {
    music_ = music;
    world_.sendAll(music ? "music" : "quiet", clock.seconds);
  }
  if (in.fireOn != fire_) {
    fire_ = in.fireOn;
    world_.sendAll(fire_ ? "fire-on" : "fire-off", clock.seconds);
  }
  world_.takeChanges();  // every phone makes these itself (the jukebox and fire are shared already)
  world_.update(clock);

  auto base = [&](int l) { return static_cast<size_t>(l) < base_.size() ? base_[static_cast<size_t>(l)] : 1.0f; };
  for (Drawn& d : drawn_) {
    const engine::world::Thing& t = world_.thing(d.thing);
    const engine::world::Motion& m = t.motion;
    if (d.model >= 0) {
      if (!world_.stateOf(d.thing).free) renderer.setModelPose(static_cast<size_t>(d.model), t.world);
      const engine::Vec3 tint = m.tint();
      renderer.setModelLook(static_cast<size_t>(d.model), 1 + m.ch[engine::world::kGlow],
                            {1 + tint.x, 1 + tint.y, 1 + tint.z});
    }
    if (d.light >= 0)
      renderer.setLightScale(static_cast<size_t>(d.light), base(d.light) * std::max(0.0f, 1 + m.ch[engine::world::kLight]));
    if (d.glowbox) {
      const engine::Json& g = world_.part(d.thing, "glowbox");
      const float size = static_cast<float>(g["size"].number(0.01)) * (1 + m.ch[engine::world::kScaleX]);
      if (size <= 1e-4f) continue;
      engine::Vec3 p{t.world.at(3, 0), t.world.at(3, 1), t.world.at(3, 2)};
      // Shy: dart away from you, then drift back once you've gone.
      const float shy = static_cast<float>(g["shy"].number(0));
      engine::Vec3 target{};
      if (shy > 0) {
        const engine::Vec3 away = p - in.eye;
        const float dist = engine::length(away);
        if (dist < kShyDistance && dist > 1e-3f) target = away * (shy * (1.0f - dist / kShyDistance) / dist);
      }
      d.away = d.away + (target - d.away) * std::min(1.0f, dt * (engine::length(target) > engine::length(d.away) ? 8.0f : 1.5f));
      SceneRenderer::Box b;
      b.center = p + d.away;
      b.size = {size, size, size};
      b.color = engine::world::readVec3(g["color"], {1, 1, 1});
      b.glow = static_cast<float>(g["glow"].number(1)) * std::max(0.0f, 1 + m.ch[engine::world::kGlow]);
      b.roughness = 1;
      if (boxes.size() < SceneRenderer::kMaxBoxes) boxes.push_back(b);
    }
  }
}

}  // namespace app
