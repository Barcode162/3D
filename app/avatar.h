#pragma once
// Your body in the bar. For now a simple mannequin made of basic shapes; it
// stands where you stand and turns with you, so you can see yourself in the
// mirrors. (Next stage: a real skeleton with bones, then animation.)
#include <vector>

#include "engine/math.h"
#include "engine/mesh.h"

namespace app {

struct AvatarPart {
  engine::MeshData mesh;
  engine::Vec3 position;  // relative to the feet; the avatar faces -Z
  engine::Vec3 color;
  float shininess = 0.2f;
};

std::vector<AvatarPart> buildAvatar();

// Where the avatar's feet are and which way it faces, from the camera.
engine::Mat4 avatarTransform(engine::Vec3 eyePosition, float yaw);

}  // namespace app
