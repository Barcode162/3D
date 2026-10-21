#include "bar/world_bar.h"

#include <algorithm>
#include <cmath>

#include "looped/fetch.h"
#include "looped/json.h"
#include "looped/log.h"
#include "looped/net.h"

namespace app {

namespace {
constexpr float kShyDistance = 1.2f;  // metres: closer than this, shy things dart away
constexpr char kSongsFile[] = "assets/world/songs.json";
}  // namespace

void BarWorld::load(const BarScene& scene) {
  if (requested_) return;
  requested_ = true;
  scene_ = &scene;
  if (!scene.problems.empty()) LOG_ERROR("World: %s", scene.problems.c_str());
  build();
  engine::fetchFile(kSongsFile, [this](bool ok, std::vector<uint8_t> bytes) {
    if (!ok) return;
    const engine::Json root = engine::Json::parse(std::string(bytes.begin(), bytes.end()));
    for (const auto& [file, song] : root.members())
      if (song.isObject()) tempos_.push_back({file, song["bpm"].number(0), song["firstBeat"].number(0)});
    LOG_INFO("World: %zu songs with a beat.", tempos_.size());
  });
}

void BarWorld::build() {
  if (!scene_->world) return;
  world_ = *scene_->world;  // its own copy: the scene's stays as built
  for (const std::string& p : world_.check()) LOG_ERROR("World: %s", p.c_str());
  std::vector<int> modelOf(world_.size(), -1), lightOf(world_.size(), -1);
  for (size_t m = 0; m < scene_->modelThing.size(); ++m) modelOf[static_cast<size_t>(scene_->modelThing[m])] = static_cast<int>(m);
  for (size_t l = 0; l < scene_->lightThing.size(); ++l) lightOf[static_cast<size_t>(scene_->lightThing[l])] = static_cast<int>(l);
  drawn_.clear();
  for (size_t i = 0; i < world_.size(); ++i) {
    const int t = static_cast<int>(i);
    if (!world_.thing(t).kind->still.empty()) continue;  // nothing to play
    Drawn d;
    d.thing = t;
    d.model = modelOf[i];
    d.light = lightOf[i];
    d.glowbox = world_.part(t, "glowbox").isObject();
    if (d.model >= 0 || d.light >= 0 || d.glowbox) drawn_.push_back(d);
  }
  dims_.assign(scene_->lights.size(), false);
  for (int l : scene_->roomLights) dims_[static_cast<size_t>(l)] = true;
  built_ = true;
  LOG_INFO("World: %zu things, %zu alive.", world_.size(), drawn_.size());
}

bool BarWorld::touch(int i) {
  if (!reacts(i)) return false;
  world_.takeChanges();
  world_.send(i, "tap", now_);
  std::vector<engine::world::Change> changes = world_.takeChanges();
  LOG_INFO("World: you touched %s (%s).", world_.thing(i).id.c_str(), world_.stateOf(i).name.c_str());
  if (engine::net::status() == engine::net::Status::kConnected) {
    const engine::world::Thing& t = world_.thing(i);
    const int request = engine::net::request("thing", "{\"n\":" + std::to_string(t.net) + ",\"e\":\"tap\"}");
    if (request > 0) pending_.push_back({request, std::move(changes)});
  }
  return true;
}

void BarWorld::applyShared(uint16_t net, int state, double start) {
  if (state < 0 || state > 255) return;
  const engine::world::Change c{net, static_cast<uint8_t>(state), start};
  shared_[net] = c;
  if (!built_) return;
  const int i = world_.findNet(net);
  const bool changed = i >= 0 && (world_.thing(i).state != state || world_.thing(i).start != start);
  world_.apply(c);
  if (changed) LOG_INFO("World: %s is %s (the room).", world_.thing(i).id.c_str(), world_.stateOf(i).name.c_str());
}

void BarWorld::undo(const engine::world::Change& c) {
  // The room said no: back to its word (or how the thing started).
  const auto it = shared_.find(c.net);
  if (it != shared_.end()) {
    world_.apply(it->second);
    return;
  }
  const int i = world_.findNet(c.net);
  if (i < 0) return;
  const engine::world::Kind& k = *world_.thing(i).kind;
  const int start = k.start.empty() ? 0 : std::max(0, k.state(k.start));
  world_.apply({c.net, static_cast<uint8_t>(start), 0});
}

std::string BarWorld::stateOf(const char* kind) const {
  for (size_t i = 0; i < world_.size(); ++i)
    if (world_.thing(static_cast<int>(i)).kind->id == kind) return world_.stateOf(static_cast<int>(i)).name;
  return "";
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
  // A new song (E3.4): the song itself is shared already (world key jb), so
  // every phone sends this to its own world at the same moment.
  if (music && in.songUrl != song_) world_.sendAll("song", clock.seconds);
  song_ = music ? in.songUrl : "";
  // Seats (E3.3): who sits where is shared already (the room's seat claims),
  // so every phone tells its own world.
  seated_.resize(in.seated.size(), false);
  for (size_t k = 0; k < in.seated.size() && k < scene_->seatThing.size(); ++k) {
    if (in.seated[k] == seated_[k]) continue;
    seated_[k] = in.seated[k];
    world_.send(scene_->seatThing[k], in.seated[k] ? "sit" : "stand", clock.seconds);
  }
  world_.takeChanges();  // every phone makes these itself (the jukebox's song is shared already)
  now_ = clock.seconds;
  // The room's answers to our touches: a refusal (too far...) takes them back.
  for (size_t k = 0; k < pending_.size();) {
    std::string answer;
    if (!engine::net::takeAnswer(pending_[k].request, answer)) {
      ++k;
      continue;
    }
    if (!engine::Json::parse(answer)["ok"].integer(0))
      for (const engine::world::Change& c : pending_[k].changes) undo(c);
    pending_.erase(pending_.begin() + static_cast<long>(k));
  }
  world_.update(clock);

  // The light switch (a thing with states on / dim) turns the lamps and
  // string lights down to a quarter.
  const bool dim = stateOf("light-switch") == "dim";
  auto base = [&](int l) { return dim && dims_[static_cast<size_t>(l)] ? 0.25f : 1.0f; };
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
