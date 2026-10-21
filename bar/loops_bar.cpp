#include "bar/loops_bar.h"

#include <algorithm>
#include <cmath>

#include "looped/fetch.h"
#include "looped/json.h"
#include "looped/log.h"
#include "looped/net.h"

namespace app {

namespace {
constexpr float kShyDistance = 1.2f;  // metres: closer than this, shy things dart away
}

void BarLoops::load() {
  if (requested_) return;
  requested_ = true;
  engine::fetchFile("assets/loops/bar.json", [this](bool ok, std::vector<uint8_t> bytes) {
    if (!ok) {
      LOG_ERROR("Loops: assets/loops/bar.json didn't load.");
      return;
    }
    const std::string text(bytes.begin(), bytes.end());
    std::string error;
    loops_ = engine::loops::parse(text, &error);
    if (!error.empty()) LOG_ERROR("Loops: %s", error.c_str());
    const engine::Json root = engine::Json::parse(text);
    for (const auto& [file, song] : root["songs"].members()) {
      if (!song.isObject()) continue;
      tempos_.push_back({file, song["bpm"].number(0), song["firstBeat"].number(0)});
    }
    away_.clear();
    for (const auto& r : loops_) away_.emplace_back(static_cast<size_t>(r.kind == engine::loops::Kind::kSwarm ? r.count : 0));
    LOG_INFO("Loops: %zu playing (%zu songs with a beat).", loops_.size(), tempos_.size());
  });
}

void BarLoops::setLightBase(int light, float scale) {
  if (light < 0) return;
  if (base_.size() <= static_cast<size_t>(light)) base_.resize(static_cast<size_t>(light) + 1, 1.0f);
  base_[static_cast<size_t>(light)] = scale;
}

void BarLoops::update(const Inputs& in, std::vector<SceneRenderer::Box>& boxes, SceneRenderer& renderer) {
  if (loops_.empty()) return;
  const float dt = lastMs_ > 0 ? static_cast<float>(std::clamp((in.nowMs - lastMs_) / 1000.0, 0.0, 0.1)) : 0.0f;
  lastMs_ = in.nowMs;
  // The shared clock: the room's when known, else this phone's.
  engine::loops::Clock clock;
  const double server = engine::net::serverTime();
  clock.seconds = server > 0 ? server : in.nowMs / 1000.0;
  if (in.songSeconds >= 0) {
    const std::string file = in.songUrl.substr(in.songUrl.find_last_of('/') + 1);
    for (const Tempo& t : tempos_) {
      if (t.file != file || t.bpm <= 0) continue;
      clock.music = true;
      clock.beats = engine::loops::beatsAt(in.songSeconds, t.bpm, t.firstBeat);
    }
  }

  std::vector<float> factor;  // per light, this frame
  std::vector<int> driven;
  for (size_t i = 0; i < loops_.size(); ++i) {
    const engine::loops::Recipe& r = loops_[i];
    if (!r.enabled || (r.when == "fire" && !in.fireOn)) continue;
    if (r.kind == engine::loops::Kind::kSwarm) {
      std::vector<engine::Vec3>& away = away_[i];
      for (int m = 0; m < r.count; ++m) {
        engine::Vec3 p = engine::loops::swarmPosition(r, clock, m);
        // Shy: dart away from you, then drift back once you've gone.
        engine::Vec3 target{};
        if (r.shy > 0) {
          const engine::Vec3 d = p - in.eye;
          const float dist = engine::length(d);
          if (dist < kShyDistance && dist > 1e-3f) target = d * (r.shy * (1.0f - dist / kShyDistance) / dist);
        }
        engine::Vec3& a = away[static_cast<size_t>(m)];
        a = a + (target - a) * std::min(1.0f, dt * (engine::length(target) > engine::length(a) ? 8.0f : 1.5f));
        p = p + a;
        SceneRenderer::Box b;
        b.center = p;
        b.size = {r.size, r.size, r.size};
        b.color = r.color;
        b.glow = r.glow * engine::loops::swarmTwinkle(r, clock, m);
        b.roughness = 1;
        if (boxes.size() < SceneRenderer::kMaxBoxes) boxes.push_back(b);
      }
      continue;
    }
    for (size_t k = 0; k < r.lights.size(); ++k) {
      const int light = r.lights[k];
      if (light < 0) continue;
      const float v = r.kind == engine::loops::Kind::kPulse ? engine::loops::pulse(r, clock, static_cast<int>(k))
                                                             : engine::loops::flicker(r, clock, static_cast<int>(k));
      if (factor.size() <= static_cast<size_t>(light)) factor.resize(static_cast<size_t>(light) + 1, 1.0f);
      factor[static_cast<size_t>(light)] *= v;
      if (std::find(driven.begin(), driven.end(), light) == driven.end()) driven.push_back(light);
    }
  }
  auto base = [&](int l) { return static_cast<size_t>(l) < base_.size() ? base_[static_cast<size_t>(l)] : 1.0f; };
  for (int l : driven) renderer.setLightScale(static_cast<size_t>(l), base(l) * factor[static_cast<size_t>(l)]);
  // Lights a loop let go of (a condition ended) go back to the bar's own setting.
  for (int l : driven_) {
    if (std::find(driven.begin(), driven.end(), l) == driven.end()) renderer.setLightScale(static_cast<size_t>(l), base(l));
  }
  driven_ = std::move(driven);
}

}  // namespace app
