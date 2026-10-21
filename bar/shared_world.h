#pragma once
// The shared bar: glasses and the jukebox song look the same
// for everyone in the room. Each thing is a small "world entry" in the room
// (looped/net.h). Glasses are moved by the room's own physics
// (cloud/games/glasses.js): this phone asks to grab, put down or throw one,
// shows the glass in your hand at once, and otherwise follows the poses the
// room sends; their resting spots are world entries g<n>. The jukebox entry is the song and the
// server time it started, so late joiners hear the same part of the song.
#include <vector>

#include "bar/bar_scene.h"
#include "bar/jukebox.h"
#include "looped/net.h"
#include "looped/physics.h"

namespace app {

class SharedWorld {
 public:
  void start(bool online, size_t bodyCount, size_t mirrorCount);

  // Local actions to share.
  void pickedUp(int body);
  // You put it down (where it stands now) or threw it (its velocity now):
  // the room's physics takes it from here.
  void letGo(int body, engine::Body& b, bool putDown);
  // False if someone else is holding it right now.
  bool canPickUp(int body) const;

  // Call every frame before the physics step. `held` is the body in your
  // hand; it's set to -1 if another player takes it from you.
  void update(double nowMs, float dt, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox, int& held);
  // World entries this class doesn't handle itself (piano, lights, drinks).
  std::vector<engine::net::WorldEntry> takeOther() { return std::move(other_); }

 private:
  enum class Mode { kIdle, kMine, kRemote };
  enum State { kMoving = 0, kHeld = 1, kResting = 2 };
  struct Glass {
    Mode mode = Mode::kIdle;
    bool settled = false;  // has come to rest once since the page loaded
    int claim = 0;          // the room's answer to "may I hold it?" (engine::net::claim)
    bool claimed = false;   // we asked for its claim (released when it comes to rest)
    // Latest state from the player moving it.
    engine::Vec3 position, velocity;
    engine::Quat rotation;
    int state = kMoving;
    double receivedMs = 0;
  };

  void receive(double nowMs, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox, int& held);
  void applyJukebox(Jukebox& jukebox);

  bool on_ = false;
  std::vector<engine::net::WorldEntry> other_;
  std::vector<Glass> glasses_;
  int jukeboxChanges_ = 0;  // Jukebox::localChanges() already shared
  double songStart_ = 0;    // server time the current song started
  // A song another player chose, waiting for sound to unlock (first tap).
  bool songPending_ = false;
  int pendingSong_ = -1;
  double pendingStart_ = 0;
};

}  // namespace app
