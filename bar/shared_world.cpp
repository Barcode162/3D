#include "bar/shared_world.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

#include "looped/audio.h"
#include "looped/frame_time.h"
#include "looped/log.h"
#include "looped/net.h"

namespace app {

using engine::Quat;
using engine::Vec3;

namespace {

constexpr double kExtrapolateMs = 300;  // guess ahead at most this far past the last update
constexpr double kGoneMs = 3000;        // no update this long: the mover left, let it fall
constexpr float kFollow = 14.0f;        // how fast a remote glass catches up (per second)
constexpr double kSongSlack = 2.0;      // seconds; closer song starts count as the same


std::string glassClaim(int index) { return "glass." + std::to_string(index); }

}  // namespace

void SharedWorld::start(bool online, size_t bodyCount) {
  on_ = online;
  glasses_.assign(bodyCount, {});
}

bool SharedWorld::canPickUp(int body) const {
  if (!on_ || body < 0) return true;
  const Glass& g = glasses_[static_cast<size_t>(body)];
  if (engine::net::claimOwner(glassClaim(body)) > 0) return false;  // the room says someone has it
  return !(g.mode == Mode::kRemote && g.state == kHeld);
}

void SharedWorld::pickedUp(int body) {
  if (!on_) return;
  Glass& g = glasses_[static_cast<size_t>(body)];
  g.mode = Mode::kMine;
  g.settled = true;
  // The room's physics moves every glass (cloud/games/glasses.js): ask to
  // grab it, and hold it in your hand meanwhile (the others see the room's).
  g.claim = engine::net::request("grab", "{\"g\":" + std::to_string(body) + "}");
  g.claimed = true;
}

void SharedWorld::letGo(int body, engine::Body& b, bool putDown) {
  if (!on_ || body < 0 || static_cast<size_t>(body) >= glasses_.size()) return;
  Glass& g = glasses_[static_cast<size_t>(body)];
  char args[160];
  if (putDown) {
    std::snprintf(args, sizeof(args), "{\"g\":%d,\"pos\":[%.4f,%.4f,%.4f]}", body, b.position.x, b.position.y, b.position.z);
  } else {
    std::snprintf(args, sizeof(args), "{\"g\":%d,\"vel\":[%.3f,%.3f,%.3f]}", body, b.velocity.x, b.velocity.y, b.velocity.z);
  }
  engine::net::request(putDown ? "drop" : "throw", args);
  // From now on it goes where the room's physics says.
  g.claim = 0;
  g.claimed = false;
  g.mode = Mode::kRemote;
  g.state = kMoving;
  g.position = b.position;
  g.rotation = b.rotation;
  g.velocity = b.velocity;
  g.receivedMs = engine::frameTime().nowMs;
  b.kinematic = true;
  b.target = b.position;
  b.targetRotation = b.rotation;
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
  (void)held;  // a refused grab is handled in update()
  static std::vector<engine::net::WorldEntry> entries;
  entries.clear();
  engine::net::receiveWorld(entries);
  for (const engine::net::WorldEntry& e : entries) {
    const std::vector<double>& v = e.values;
    if (e.key == "jb" && v.size() >= 2) {
      const int song = static_cast<int>(v[0]);
      const int count = static_cast<int>(jukebox.songs().size());
      if (song >= count) continue;
      // Same song started at (nearly) the same moment: already playing it.
      if (song >= 0 && song == jukebox.current() && std::fabs(v[1] - songStart_) < kSongSlack) continue;
      pendingSong_ = song;
      pendingStart_ = v[1];
      songPending_ = true;
    } else if (e.key.size() >= 2 && e.key[0] == 'g' && std::isdigit(static_cast<unsigned char>(e.key[1])) && v.size() >= 11) {
      const int i = std::atoi(e.key.c_str() + 1);
      if (i < 0 || i >= static_cast<int>(glasses_.size())) continue;
      Glass& g = glasses_[static_cast<size_t>(i)];
      engine::Body& b = physics.body(static_cast<size_t>(i));
      bool finite = true;
      for (size_t k = 0; k < 11; ++k) finite = finite && std::isfinite(v[k]);
      if (!finite) continue;  // a broken message: ignore it
      const int state = static_cast<int>(v[10]);
      if (g.mode == Mode::kMine) continue;  // in your hand: your phone shows it there (the room agrees, or says no)
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
        physics.teleport(static_cast<size_t>(i), g.position, g.rotation);
        b.velocity = b.angularVelocity = {};
        b.kinematic = true;  // online the room's physics moves it, not this phone's
        b.target = g.position;
        b.targetRotation = g.rotation;
        b.asleep = true;
        g.mode = Mode::kIdle;
        g.settled = true;
        continue;
      }
      if (g.mode != Mode::kRemote) {
        b.target = b.position;  // glides from here
        b.targetRotation = b.rotation;
      }
      g.mode = Mode::kRemote;
      b.kinematic = true;
      b.asleep = false;
    } else {
      other_.push_back(e);  // piano, lights, drinks...: handled by their own code
    }
  }
}

void SharedWorld::update(double nowMs, float dt, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox,
                         int& held) {
  if (!on_) return;
  jukebox.followRoom(engine::net::status() == engine::net::Status::kConnected);
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
        // Online the room's physics moves glasses: this phone doesn't push them.
        if (!b.held && !b.kinematic) {
          b.kinematic = true;
          b.target = b.position;
          b.targetRotation = b.rotation;
        }
        break;
      case Mode::kMine:
        if (g.claim) {
          const int answer = engine::net::claimResult(g.claim);
          if (answer >= 0) g.claim = 0;
          if (answer == 0) {  // someone else got it first (or it's out of reach): let go
            if (held == index) held = -1;
            b.held = false;
            g.claimed = false;
            g.mode = Mode::kIdle;
            LOG_INFO("Multiplayer: the room said no to glass %d.", index);
          }
        }
        break;
      case Mode::kRemote: {
        const double age = nowMs - g.receivedMs;
        if (age > kGoneMs && g.state != kResting) {
          g.mode = Mode::kIdle;  // no news from the room for a while: it stays where it is
          break;
        }
        Vec3 target = g.position;
        if (g.state == kMoving) target += g.velocity * static_cast<float>(std::min(age, kExtrapolateMs) / 1000.0);
        // Ease the physics target toward where they say it is; the physics
        // step moves the glass there (and knocks others on the way).
        const float f = 1.0f - std::exp(-kFollow * dt);
        b.target = b.target + (target - b.target) * f;
        b.targetRotation = nlerp(b.targetRotation, g.rotation, f);
        b.asleep = false;
        if (g.state == kResting && (engine::length(target - b.position) < 0.002f || age > 600)) {
          LOG_INFO("Multiplayer: glass %d came to rest at %.2f, %.2f, %.2f.", index, g.position.x, g.position.y,
                   g.position.z);
          physics.teleport(i, g.position, g.rotation);
          b.velocity = {};
          b.kinematic = true;
          b.target = g.position;
          b.targetRotation = g.rotation;
          b.asleep = true;
          g.mode = Mode::kIdle;
          g.settled = true;
          }
        break;
      }
    }
  }
}

}  // namespace app
