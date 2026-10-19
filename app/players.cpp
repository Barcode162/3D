#include "app/players.h"

#include <algorithm>
#include <cmath>

#include "engine/bridge.h"
#include "engine/log.h"
#include "engine/net.h"

namespace app {

using engine::Vec3;

namespace {

constexpr double kPlaybackDelayMs = 250;  // behind their newest state, to smooth gaps
constexpr double kKeepMs = 2000;          // older samples are dropped

// Shortest way from angle a to b (radians), so turning across ±180° doesn't spin.

}  // namespace

void OtherPlayers::start() {
  on_ = engine::net::start();
  engine::net::setAwayFlag(slot::kFlags, kFlagAway);
}

void OtherPlayers::sendSelf(double nowMs, Vec3 eye, float yaw, float pitch, const float* extra) {
  if (!on_) return;
  float state[slot::kCount] = {eye.x, eye.y, eye.z, engine::wrapAngle(yaw), pitch};
  for (int i = slot::kActivity; i < slot::kCount; ++i) state[i] = extra[i - slot::kActivity];
  engine::net::setState(nowMs, state, slot::kCount);
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
    // What they're doing: taken as it comes (no smoothing).
    const int oldActivity = static_cast<int>(o.values[slot::kActivity]);
    const int oldParam = static_cast<int>(o.values[slot::kParam]);
    for (int k = 0; k < slot::kCount; ++k) o.values[k] = k < p.count ? p.values[k] : 0;
    o.look = static_cast<int>(o.values[slot::kLook]);
    const int emoteCount = static_cast<int>(o.values[slot::kEmoteCount]);
    if (static_cast<int>(o.values[slot::kActivity]) != oldActivity || static_cast<int>(o.values[slot::kParam]) != oldParam ||
        emoteCount != o.emoteCount) {
      o.activitySinceMs = nowMs;
    }
    o.emoteCount = emoteCount;
    const bool away = (static_cast<int>(o.values[slot::kFlags]) & kFlagAway) != 0;
    if (away != o.away) {
      o.away = away;
      LOG_INFO("Multiplayer: %s is %s.", o.name.empty() ? "a player" : o.name.c_str(), away ? "away" : "back");
    }
    o.samples.push_back({p.time, {p.values[0], p.values[1], p.values[2]}, p.values[3], p.values[4]});
    OtherPlayer::DataSample d{p.time, {}};
    for (int k = 0; k < 5; ++k) d.v[k] = o.values[slot::kData + k];
    o.data.push_back(d);
  }

  // Gone players disappear; the rest are played back a little in the past.
  std::erase_if(players_, [](const OtherPlayer& o) { return !engine::net::present(o.id); });
  for (OtherPlayer& o : players_) {
    // Names and pictures can change (accounts); the roster is in memory.
    if (const engine::bridge::RosterEntry* e = engine::bridge::rosterFind(o.id)) {
      o.name = e->name;
      o.account = e->account;
      o.avatarRev = e->avatarRev;
    }
    while (o.samples.size() > 2 && o.samples[1].time < nowMs + o.clockOffset - kKeepMs) o.samples.pop_front();
    while (o.data.size() > 2 && o.data[1].time < nowMs + o.clockOffset - kKeepMs) o.data.pop_front();
    if (o.samples.empty()) continue;
    const double t = nowMs + o.clockOffset - kPlaybackDelayMs;
    size_t i = 0;
    while (i + 1 < o.samples.size() && o.samples[i + 1].time <= t) ++i;
    const OtherPlayer::Sample& a = o.samples[i];
    const OtherPlayer::Sample& b = o.samples[std::min(i + 1, o.samples.size() - 1)];
    const float f = b.time > a.time ? engine::clampf(static_cast<float>((t - a.time) / (b.time - a.time)), 0, 1) : 0;
    const Vec3 eye = a.eye + (b.eye - a.eye) * f;
    const float yaw = engine::angleLerp(a.yaw, b.yaw, f);
    if (o.placed) {
      const Vec3 moved = eye - o.eye;
      const Vec3 forward{std::sin(yaw), 0, -std::cos(yaw)}, right{std::cos(yaw), 0, std::sin(yaw)};
      advanceWalk(o.motion, engine::dot(moved, forward), engine::dot(moved, right), dt, walkSpeed);
    }
    o.eye = eye;
    o.yaw = yaw;
    o.pitch = a.pitch + (b.pitch - a.pitch) * f;
    o.motion.pitch = o.pitch;
    o.motion.activity = static_cast<Activity>(static_cast<int>(o.values[slot::kActivity]));
    o.motion.emote = static_cast<Emote>(static_cast<int>(o.values[slot::kEmote]));
    o.motion.actionTime = static_cast<float>((nowMs - o.activitySinceMs) / 1000.0);
    o.voice += (o.values[slot::kVoice] - o.voice) * std::min(1.0f, dt * 10.0f);
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

double OtherPlayers::dataAt(int id, double nowMs, double delayMs, float out[5], int angleIndex) const {
  auto it = std::find_if(players_.begin(), players_.end(), [&](const OtherPlayer& o) { return o.id == id; });
  if (it == players_.end() || it->data.empty()) return -1;
  const auto& d = it->data;
  const double t = nowMs + it->clockOffset - delayMs;
  if (t >= d.back().time || d.size() == 1) {
    for (int k = 0; k < 5; ++k) out[k] = d.back().v[k];
    return std::max(0.0, t - d.back().time);
  }
  size_t i = 0;
  while (i + 2 < d.size() && d[i + 1].time <= t) ++i;
  const auto& a = d[i];
  const auto& b = d[i + 1];
  const float f = static_cast<float>(std::clamp((t - a.time) / std::max(b.time - a.time, 1.0), 0.0, 1.0));
  for (int k = 0; k < 5; ++k) {
    out[k] = k == angleIndex ? engine::angleLerp(a.v[k], b.v[k], f) : a.v[k] + (b.v[k] - a.v[k]) * f;
  }
  return 0;
}

}  // namespace app
