#include "app/avatar.h"

using engine::makeBox;
using engine::makeCylinder;
using engine::makeSphere;
using engine::Vec3;

namespace app {

std::vector<AvatarPart> buildAvatar() {
  constexpr Vec3 kSkin{0.78f, 0.56f, 0.43f};
  constexpr Vec3 kShirt{0.08f, 0.33f, 0.38f};
  constexpr Vec3 kPants{0.07f, 0.07f, 0.11f};
  constexpr Vec3 kShoes{0.04f, 0.035f, 0.03f};
  constexpr Vec3 kEyes{0.03f, 0.03f, 0.04f};
  std::vector<AvatarPart> p;
  p.push_back({makeSphere(0.11f), {0, 1.62f, 0}, kSkin, 0.3f});               // head
  p.push_back({makeSphere(0.018f, 10, 8), {-0.04f, 1.645f, -0.1f}, kEyes, 0.9f});
  p.push_back({makeSphere(0.018f, 10, 8), {0.04f, 1.645f, -0.1f}, kEyes, 0.9f});
  p.push_back({makeCylinder(0.05f, 0.1f, 12), {0, 1.48f, 0}, kSkin, 0.3f});   // neck
  p.push_back({makeBox({0.38f, 0.5f, 0.22f}), {0, 1.2f, 0}, kShirt, 0.15f});  // torso
  p.push_back({makeBox({0.34f, 0.16f, 0.2f}), {0, 0.9f, 0}, kPants, 0.15f});  // hips
  for (float side : {-1.0f, 1.0f}) {
    p.push_back({makeCylinder(0.075f, 0.8f, 14), {0.1f * side, 0.44f, 0}, kPants, 0.15f});       // leg
    p.push_back({makeBox({0.11f, 0.07f, 0.26f}), {0.1f * side, 0.035f, -0.04f}, kShoes, 0.6f});  // shoe
    p.push_back({makeCylinder(0.055f, 0.32f, 12), {0.25f * side, 1.27f, 0}, kShirt, 0.15f});     // upper arm
    p.push_back({makeCylinder(0.045f, 0.3f, 12), {0.25f * side, 0.96f, 0}, kSkin, 0.3f});        // forearm
    p.push_back({makeSphere(0.05f, 12, 8), {0.25f * side, 0.78f, 0}, kSkin, 0.3f});              // hand
  }
  return p;
}

engine::Mat4 avatarTransform(Vec3 eye, float yaw) {
  // The camera looks along (sin yaw, 0, -cos yaw); the avatar is built
  // facing -Z, so turn it by -yaw. The eyes are about 1.65 m up.
  return engine::Mat4::transform({eye.x, eye.y - 1.65f, eye.z}, engine::Quat::axisAngle({0, 1, 0}, -yaw));
}

}  // namespace app
