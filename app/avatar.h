#pragma once
// Your body in the bar: a skeleton (18 bones) plus one smooth "skinned" mesh
// that bends with it. It stands where you stand, turns with you, and tilts
// its head as you look up and down. Seen in the mirrors.
#include <vector>

#include "engine/math.h"
#include "engine/skeleton.h"
#include "engine/skinned_mesh.h"

namespace app {

// What drives the avatar's animation this frame.
struct AvatarMotion {
  float pitch = 0;       // where you look (radians, + up)
  float walkPhase = 0;   // position in the walk cycle (radians; one cycle = 2 steps)
  float walkAmount = 0;  // 0 = standing, 1 = full walking speed
  float time = 0;        // seconds, for idle breathing and sway
};

// Advances the walk cycle by how far you moved, so steps match your speed
// and the feet don't skate. `forwardDistance` is signed (backward < 0).
void advanceWalk(AvatarMotion& motion, float forwardDistance, float sideDistance, float dt, float fullSpeed);

class Avatar {
 public:
  void build();
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

// Texture repeats per meter for the avatar's clothes.
constexpr float kAvatarTextureScale = 3.0f;

}  // namespace app
