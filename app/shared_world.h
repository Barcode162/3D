#pragma once
// The shared bar (multiplayer M2): glasses, the jukebox song and the mirrors
// look the same for everyone in the room. Each thing is a small "world
// entry" on the relay (engine/net.h). Whoever is moving a glass (holding
// it, or it's flying after a throw or a knock) sends where it is about 10
// times a second; everyone else's copy follows smoothly and still knocks
// their other glasses around. When it comes to rest, its final spot is
// sent once and it's free again. The jukebox entry is the song and the
// server time it started, so late joiners hear the same part of the song.
#include <vector>

#include "app/bar_scene.h"
#include "app/jukebox.h"
#include "engine/net.h"
#include "engine/physics.h"

namespace app {

class SharedWorld {
 public:
  void start(bool online, size_t bodyCount, size_t mirrorCount);

  // Local actions to share.
  void pickedUp(int body);
  void mirrorChanged(int mirror, bool on, bool avatarsOnly);
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
    double lastSentMs = -1e9;
    double quietUntilMs = 0;  // don't claim it before this (just placed by the network)
    // Latest state from the player moving it.
    engine::Vec3 position, velocity;
    engine::Quat rotation;
    int state = kMoving;
    double receivedMs = 0;
  };

  void receive(double nowMs, engine::PhysicsWorld& physics, BarScene& scene, Jukebox& jukebox, int& held);
  void sendGlass(int index, const engine::Body& b, int state);
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
