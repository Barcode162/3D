#pragma once
// Your body in the bar: a skeleton (18 bones) plus one smooth "skinned" mesh
// that bends with it. It stands where you stand, turns with you, and tilts
// its head as you look up and down. Seen in the mirrors.
#include <vector>

#include "engine/math.h"
#include "engine/skeleton.h"
#include "engine/skinned_mesh.h"

namespace app {

class Avatar {
 public:
  void build();
  const engine::Skeleton& skeleton() const { return skeleton_; }
  const engine::SkinnedMeshData& mesh() const { return mesh_; }

  // Bone matrices for this frame. `pitch` = where you look (radians, + up).
  void pose(float pitch, std::vector<engine::Mat4>& boneMatrices) const;

 private:
  engine::Skeleton skeleton_;
  engine::SkinnedMeshData mesh_;
};

// Where the avatar's feet are and which way it faces, from the camera.
engine::Mat4 avatarTransform(engine::Vec3 eyePosition, float yaw);

// Texture repeats per meter for the avatar's clothes.
constexpr float kAvatarTextureScale = 3.0f;

}  // namespace app
