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

// What each player sends about 10 times a second (engine/net state): where
// they are and look, plus what they're doing. Append new slots only.
namespace slot {
enum : int {
  kX, kY, kZ, kYaw, kPitch,  // eyes and look direction
  kActivity,                 // app::Activity
  kParam,                    // seat index, machine, drink... (depends on the activity)
  kLook,                     // avatar look (avatarLooks() index)
  kVoice,                    // how loud they're talking right now (0..1)
  kEmote,                    // app::Emote
  kEmoteCount,               // goes up by one per emote, so a repeat restarts it
  kData,                     // 5 numbers for the activity (scores, steering...)
  kHeldDrink = kData + 5,    // drink in hand (drink type + 1, 0 = none)
  kFlags,                    // bit 0 = bartender on shift, bit 1 = on the stage mic, bit 2 = away (kFlagAway)
  kCount,
};
}

// Set by the page (web/net.js) while that player's game is in the background.
constexpr int kFlagAway = 4;

struct OtherPlayer {
  int id = 0;
  std::string name;
  std::string account;  // "" = a guest
  int avatarRev = 0;     // profile picture revision (0 = none)
  engine::Vec3 eye;  // where their eyes are (their camera)
  float yaw = 0, pitch = 0;
  AvatarMotion motion;
  float values[slot::kCount] = {};  // their newest state
  int look = 0;
  float voice = 0;  // talking level (smoothed)
  int emoteCount = -1;
  bool away = false;  // their game is in the background (kFlagAway)
  double activitySinceMs = 0;  // when their current activity/emote started (our clock)

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
  // `extra` = the slots after kPitch (activity, look, ...).
  void sendSelf(double nowMs, engine::Vec3 eye, float yaw, float pitch, const float* extra);
  void update(double nowMs, float dt, float walkSpeed);
  const std::vector<OtherPlayer>& players() const { return players_; }
  // For the status line, e.g. "online: 2"; empty when multiplayer is off.
  std::string statusText() const;

 private:
  bool on_ = false;
  std::vector<OtherPlayer> players_;
};

}  // namespace app
