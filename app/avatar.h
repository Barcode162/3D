#pragma once
// Your body in the bar: a skeleton (18 bones) plus one smooth "skinned" mesh
// that bends with it. It stands where you stand, turns with you, and tilts
// its head as you look up and down. Seen in the mirrors.
#include <vector>

#include "engine/math.h"
#include "engine/skeleton.h"
#include "engine/skinned_mesh.h"

namespace app {

// What the body is doing besides walking. Sent to other players as a
// number, so the order must not change (add new ones at the end).
enum class Activity : int {
  kNone = 0,
  kSit,      // on any seat
  kDrive,    // in a racing sim seat, hands on the wheel
  kPiano,    // at the piano bench, hands on the keys
  kRhythm,   // standing at the 4K MANIA cabinet, tapping
  kDarts,    // at the oche, aiming and throwing
  kBartend,  // behind the bar: shaking, pouring
  kDrink,    // sipping a drink
  kEmote,    // one of the emotes below
};

// Emotes from the emote menu (also sent as numbers; append only).
enum class Emote : int {
  kNone = 0, kWave, kDance, kClap, kCheer, kBow, kPoint, kShrug, kLaugh, kSalute, kThumbsUp, kFacepalm, kCount
};
const char* emoteName(Emote e);
// How long an emote plays (seconds; dancing goes on until you move).
float emoteLength(Emote e);

// What drives the avatar's animation this frame.
struct AvatarMotion {
  float pitch = 0;       // where you look (radians, + up)
  float headYaw = 0;     // head turned left/right of the body (radians, + = to the right)
  float walkPhase = 0;   // position in the walk cycle (radians; one cycle = 2 steps)
  float walkAmount = 0;  // 0 = standing, 1 = full walking speed
  float time = 0;        // seconds, for idle breathing and sway
  Activity activity = Activity::kNone;
  Emote emote = Emote::kNone;
  float actionTime = 0;  // seconds since the activity or emote started
  float steer = 0;       // driving: -1 left .. 1 right
  float handL = 0, handR = 0;  // piano: where each hand is along the keys (-1 .. 1)
  float lowSeat = 0;     // 0 = a stool or chair, 1 = a low sofa or beanbag
};

// Advances the walk cycle by how far you moved, so steps match your speed
// and the feet don't skate. `forwardDistance` is signed (backward < 0).
void advanceWalk(AvatarMotion& motion, float forwardDistance, float sideDistance, float dt, float fullSpeed);

enum class HairStyle { kCurly, kShort, kLong, kBun, kNone };

// One avatar look you can pick in the menu.
struct AvatarLook {
  const char* name;
  engine::Vec3 skin, top, pants, shoes, hair, iris;
  HairStyle hairStyle;
  bool longSleeves;
  bool stubble;
};
const std::vector<AvatarLook>& avatarLooks();

class Avatar {
 public:
  void build(const AvatarLook& look);
  const engine::Skeleton& skeleton() const { return skeleton_; }
  const engine::SkinnedMeshData& mesh() const { return mesh_; }

  // Bone matrices for this frame: idle breathing/sway blended with the walk
  // cycle, plus the head following where you look.
  void pose(const AvatarMotion& motion, std::vector<engine::Mat4>& boneMatrices) const;

  // First-person version of a pose: the head and neck shrink to a point at
  // the base of the neck, so they don't block your view while you can
  // still look down at your own body.
  void hideHead(const std::vector<engine::Mat4>& boneMatrices, std::vector<engine::Mat4>& out) const;

 private:
  engine::Skeleton skeleton_;
  engine::SkinnedMeshData mesh_;
};

// Where the avatar's feet are and which way it faces, from the camera. The
// eyes line up with the camera (also when your head tilts), so in first
// person you see your body from where your eyes would be.
engine::Mat4 avatarTransform(engine::Vec3 eyePosition, float yaw, float pitch);

// Sitting: the body with its hip joint at `hip`, facing `bodyYaw` (camera
// yaw convention), and where its eyes are then.
engine::Mat4 seatedTransform(engine::Vec3 hip, float bodyYaw);
engine::Vec3 seatedEye(engine::Vec3 hip, float bodyYaw, float lowSeat);

// Texture repeats per meter for the avatar's clothes.
constexpr float kAvatarTextureScale = 3.0f;

}  // namespace app
