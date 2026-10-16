#pragma once
// The other people in the bar (multiplayer). Your position and look
// direction go out about 10 times a second through engine/net; theirs come
// in at the same rate and are played back slightly delayed (about 0.25 s)
// so their movement looks smooth instead of jumping between updates. Their
// walk animation is driven by how far they move, like yours.
#include <deque>
#include <string>
#include <vector>

#include "app/avatar.h"
#include "engine/math.h"

namespace app {

struct OtherPlayer {
  int id = 0;
  std::string name;
  engine::Vec3 eye;  // where their eyes are (their camera)
  float yaw = 0, pitch = 0;
  AvatarMotion motion;

  // Received states, oldest first, on the sender's clock.
  struct Sample {
    double time;
    engine::Vec3 eye;
    float yaw, pitch;
  };
  std::deque<Sample> samples;
  double clockOffset = 0;  // their clock minus ours, estimated
  bool placed = false;     // has a position yet
};

class OtherPlayers {
 public:
  void start();
  bool online() const { return on_; }
  // Call every frame with your camera.
  void sendSelf(double nowMs, engine::Vec3 eye, float yaw, float pitch);
  void update(double nowMs, float dt, float walkSpeed);
  const std::vector<OtherPlayer>& players() const { return players_; }
  // For the status line, e.g. "online: 2"; empty when multiplayer is off.
  std::string statusText() const;

 private:
  bool on_ = false;
  std::vector<OtherPlayer> players_;
};

}  // namespace app
