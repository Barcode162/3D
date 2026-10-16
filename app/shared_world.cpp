#include "app/shared_world.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "engine/audio.h"
#include "engine/log.h"
#include "engine/net.h"

namespace app {

using engine::Quat;
using engine::Vec3;

namespace {

constexpr double kSendEveryMs = 90;     // a moving glass's updates (the relay ticks every 100 ms)
constexpr double kExtrapolateMs = 300;  // guess ahead at most this far past the last update
constexpr double kGoneMs = 3000;        // no update this long: the mover left, let it fall
constexpr double kQuietMs = 1500;       // after a glass is placed by the network
constexpr float kFollow = 14.0f;        // how fast a remote glass catches up (per second)
constexpr double kSongSlack = 2.0;      // seconds; closer song starts count as the same

Quat nlerp(Quat a, Quat b, float t) {
  if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0) b = {-b.x, -b.y, -b.z, -b.w};
  Quat q{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
  const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  return {q.x / len, q.y / len, q.z / len, q.w / len};
}

std::string glassKey(int index) { return "g" + std::to_string(index); }

}  // namespace

void SharedWorld::start(bool online, size_t bodyCount, size_t /*mirrorCount*/) {
  on_ = online;
  glasses_.assign(bodyCount, {});
}

bool SharedWorld::canPickUp(int body) const {
  if (!on_ || body < 0) return true;
  const Glass& g = glasses_[static_cast<size_t>(body)];
  return !(g.mode == Mode::kRemote && g.state == kHeld);
}

void SharedWorld::pickedUp(int body) {
  if (!on_) return;
  Glass& g = glasses_[static_cast<size_t>(body)];
  g.mode = Mode::kMine;
  g.settled = true;
  g.lastSentMs = -1e9;  // send right away
}

void SharedWorld::mirrorToggled(int mirror, bool on) {
  if (!on_) return;
  const double v = on ? 1 : 0;
  engine::net::setWorld("m" + std::to_string(mirror), &v, 1);
}

void SharedWorld::sendGlass(int index, const engine::Body& b, int state) {
  const double v[11] = {b.position.x, b.position.y, b.position.z, b.rotation.x, b.rotation.y, b.rotation.z,
                        b.rotation.w, b.velocity.x, b.velocity.y, b.velocity.z, static_cast<double>(state)};
  engine::net::setWorld(glassKey(index), v, 11);
}

void SharedWorld::applyJukebox(Jukebox& jukebox) {
  if (!engine::audio::ready()) return;  // phones only play sound after a tap
  songPending_ = false;
  if (pendingSong_ < 0) {
    jukebox.stop(true);
    return;
  }
  const double now = engine::net::serverTime();
  double offset = now > 0 ? now - pendingStart_ : 0;
  if (offset < 0 || offset > 3600) offset = 0;
  songStart_ = pendingStart_;
  jukebox.play(pendingSong_, offset, true);
}

void SharedWorld::receive(double nowMs, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox,
                          int& held) {
  static std::vector<engine::net::WorldEntry> entries;
  entries.clear();
  engine::net::receiveWorld(entries);
  for (const engine::net::WorldEntry& e : entries) {
    const std::vector<double>& v = e.values;
    if (e.key.size() >= 2 && e.key[0] == 'm' && !v.empty()) {
      const int i = std::atoi(e.key.c_str() + 1);
      if (i < 0 || i >= static_cast<int>(scene.mirrors.size())) continue;
      Mirror& m = scene.mirrors[static_cast<size_t>(i)];
      if (m.on != (v[0] != 0)) LOG_INFO("Multiplayer: mirror %d turned %s by another player.", i, v[0] != 0 ? "on" : "off");
      m.on = v[0] != 0;
    } else if (e.key == "jb" && v.size() >= 2) {
      const int song = static_cast<int>(v[0]);
      const int count = static_cast<int>(jukebox.songs().size());
      if (song >= count) continue;
      // Same song started at (nearly) the same moment: already playing it.
      if (song >= 0 && song == jukebox.current() && std::fabs(v[1] - songStart_) < kSongSlack) continue;
      pendingSong_ = song;
      pendingStart_ = v[1];
      songPending_ = true;
    } else if (e.key.size() >= 2 && e.key[0] == 'g' && v.size() >= 11) {
      const int i = std::atoi(e.key.c_str() + 1);
      if (i < 0 || i >= static_cast<int>(glasses_.size())) continue;
      Glass& g = glasses_[static_cast<size_t>(i)];
      engine::Body& b = physics.body(static_cast<size_t>(i));
      const int state = static_cast<int>(v[10]);
      if (g.mode == Mode::kMine) {
        // Both of us are moving it. Someone holding it beats a glass that's
        // just flying; otherwise the player with the lower id wins.
        const bool mineHeld = b.held;
        const bool theyWin = (state == kHeld && !mineHeld) || ((state == kHeld) == mineHeld && e.fromLowerId);
        if (!theyWin) continue;
        if (held == i) {
          held = -1;
          LOG_INFO("Multiplayer: someone else took the glass.");
        }
      }
      b.held = false;
      g.position = {static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])};
      g.rotation = Quat{static_cast<float>(v[3]), static_cast<float>(v[4]), static_cast<float>(v[5]),
                        static_cast<float>(v[6])};
      g.velocity = {static_cast<float>(v[7]), static_cast<float>(v[8]), static_cast<float>(v[9])};
      g.state = state;
      g.receivedMs = nowMs;
      if (state == kResting && g.mode != Mode::kRemote) {
        // Not watching it move (e.g. you just joined): put it there.
        if (engine::length(g.position - b.position) > 0.01f) {
          LOG_INFO("Multiplayer: glass %d is at %.2f, %.2f, %.2f.", i, g.position.x, g.position.y, g.position.z);
        }
        b.position = g.position;
        b.rotation = g.rotation;
        b.velocity = b.angularVelocity = {};
        b.kinematic = false;
        b.asleep = true;
        g.mode = Mode::kIdle;
        g.settled = true;
        g.quietUntilMs = nowMs + kQuietMs;
        continue;
      }
      g.mode = Mode::kRemote;
      b.kinematic = true;
      b.asleep = false;
    }
  }
}

void SharedWorld::update(double nowMs, float dt, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox,
                         int& held) {
  if (!on_) return;
  receive(nowMs, physics, scene, jukebox, held);

  // Jukebox: share songs you choose (built-in ones; your own files stay on
  // your phone), and play the ones others choose.
  if (jukebox.localChanges() != jukeboxChanges_) {
    jukeboxChanges_ = jukebox.localChanges();
    const Song* song = jukebox.currentSong();
    if (!song || song->builtIn) {
      songStart_ = engine::net::serverTime();
      const double v[2] = {song ? static_cast<double>(jukebox.current()) : -1.0, songStart_};
      engine::net::setWorld("jb", v, 2);
      songPending_ = false;
    }
  }
  if (songPending_) applyJukebox(jukebox);

  for (size_t i = 0; i < glasses_.size(); ++i) {
    Glass& g = glasses_[i];
    engine::Body& b = physics.body(i);
    const int index = static_cast<int>(i);
    switch (g.mode) {
      case Mode::kIdle:
        if (b.asleep) {
          g.settled = true;
        } else if (g.settled && !b.held && nowMs > g.quietUntilMs &&
                   engine::length(b.velocity) > 0.05f) {
          g.mode = Mode::kMine;  // knocked by something here: we move it now
          g.lastSentMs = -1e9;
        }
        break;
      case Mode::kMine:
        if (b.asleep) {
          sendGlass(index, b, kResting);
          g.mode = Mode::kIdle;
        } else if (nowMs - g.lastSentMs >= kSendEveryMs) {
          sendGlass(index, b, b.held ? kHeld : kMoving);
          g.lastSentMs = nowMs;
        }
        break;
      case Mode::kRemote: {
        const double age = nowMs - g.receivedMs;
        if (age > kGoneMs && g.state != kResting) {
          b.kinematic = false;  // its mover vanished: let it drop here
          b.asleep = false;
          g.mode = Mode::kIdle;
          break;
        }
        Vec3 target = g.position;
        if (g.state == kMoving) target += g.velocity * static_cast<float>(std::min(age, kExtrapolateMs) / 1000.0);
        const float f = 1.0f - std::exp(-kFollow * dt);
        const Vec3 next = b.position + (target - b.position) * f;
        b.velocity = dt > 0 ? (next - b.position) * (1.0f / dt) : Vec3{};  // physics moves it there
        b.angularVelocity = {};
        b.rotation = nlerp(b.rotation, g.rotation, f);
        b.asleep = false;
        if (g.state == kResting && (engine::length(target - b.position) < 0.002f || age > 600)) {
          LOG_INFO("Multiplayer: glass %d came to rest at %.2f, %.2f, %.2f.", index, g.position.x, g.position.y,
                   g.position.z);
          b.position = g.position;
          b.rotation = g.rotation;
          b.velocity = {};
          b.kinematic = false;
          b.asleep = true;
          g.mode = Mode::kIdle;
          g.settled = true;
          g.quietUntilMs = nowMs + kQuietMs;
        }
        break;
      }
    }
  }
}

}  // namespace app
