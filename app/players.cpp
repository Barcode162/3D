#include "app/players.h"

#include <algorithm>
#include <cmath>

#include "engine/net.h"

namespace app {

using engine::Vec3;

namespace {

constexpr double kPlaybackDelayMs = 250;  // behind their newest state, to smooth gaps
constexpr double kKeepMs = 2000;          // older samples are dropped

// Shortest way from angle a to b (radians), so turning across ±180° doesn't spin.
float angleLerp(float a, float b, float t) {
  float d = std::fmod(b - a, 2.0f * engine::kPi);
  if (d > engine::kPi) d -= 2.0f * engine::kPi;
  if (d < -engine::kPi) d += 2.0f * engine::kPi;
  return a + d * t;
}

}  // namespace

void OtherPlayers::start() { on_ = engine::net::start(); }

void OtherPlayers::sendSelf(double nowMs, Vec3 eye, float yaw, float pitch) {
  if (!on_) return;
  const float state[5] = {eye.x, eye.y, eye.z, std::remainder(yaw, 2.0f * engine::kPi), pitch};
  engine::net::setState(nowMs, state, 5);
}

void OtherPlayers::update(double nowMs, float dt, float walkSpeed) {
  if (!on_) return;
  static std::vector<engine::net::Packet> packets;
  packets.clear();
  engine::net::receive(packets);
  for (const engine::net::Packet& p : packets) {
    if (p.count < 5) continue;
    auto it = std::find_if(players_.begin(), players_.end(), [&](const OtherPlayer& o) { return o.id == p.player; });
    const double offset = p.time - nowMs;
    if (it == players_.end()) {
      OtherPlayer o;
      o.id = p.player;
      o.name = engine::net::playerName(p.player);
      o.clockOffset = offset;
      players_.push_back(o);
      it = players_.end() - 1;
    }
    OtherPlayer& o = *it;
    // The quickest delivery tells us the clock difference best; drift slowly
    // toward later ones so a changed delay is picked up too.
    o.clockOffset = offset > o.clockOffset ? offset : o.clockOffset + (offset - o.clockOffset) * 0.02;
    if (!o.samples.empty() && p.time <= o.samples.back().time) continue;
    o.samples.push_back({p.time, {p.values[0], p.values[1], p.values[2]}, p.values[3], p.values[4]});
  }

  // Gone players disappear; the rest are played back a little in the past.
  std::erase_if(players_, [](const OtherPlayer& o) { return !engine::net::present(o.id); });
  for (OtherPlayer& o : players_) {
    if (nowMs - o.infoMs > 1000) {  // names and pictures can change (accounts)
      o.infoMs = nowMs;
      o.name = engine::net::playerName(o.id);
      o.account = engine::net::playerAccount(o.id);
      o.avatarRev = engine::net::playerAvatarRev(o.id);
    }
    while (o.samples.size() > 2 && o.samples[1].time < nowMs + o.clockOffset - kKeepMs) o.samples.pop_front();
    if (o.samples.empty()) continue;
    const double t = nowMs + o.clockOffset - kPlaybackDelayMs;
    size_t i = 0;
    while (i + 1 < o.samples.size() && o.samples[i + 1].time <= t) ++i;
    const OtherPlayer::Sample& a = o.samples[i];
    const OtherPlayer::Sample& b = o.samples[std::min(i + 1, o.samples.size() - 1)];
    const float f = b.time > a.time ? engine::clampf(static_cast<float>((t - a.time) / (b.time - a.time)), 0, 1) : 0;
    const Vec3 eye = a.eye + (b.eye - a.eye) * f;
    const float yaw = angleLerp(a.yaw, b.yaw, f);
    if (o.placed) {
      const Vec3 moved = eye - o.eye;
      const Vec3 forward{std::sin(yaw), 0, -std::cos(yaw)}, right{std::cos(yaw), 0, std::sin(yaw)};
      advanceWalk(o.motion, engine::dot(moved, forward), engine::dot(moved, right), dt, walkSpeed);
    }
    o.eye = eye;
    o.yaw = yaw;
    o.pitch = a.pitch + (b.pitch - a.pitch) * f;
    o.motion.pitch = o.pitch;
    o.motion.time = static_cast<float>(nowMs / 1000.0) + static_cast<float>(o.id) * 1.7f;  // out of step with you
    o.placed = true;
  }
}

std::string OtherPlayers::statusText() const {
  switch (engine::net::status()) {
    case engine::net::Status::kOff: return "";
    case engine::net::Status::kConnecting: return "connecting...";
    case engine::net::Status::kFailing: return "offline";
    case engine::net::Status::kConnected: break;
  }
  return "online: " + std::to_string(players_.size() + 1);
}

}  // namespace app
