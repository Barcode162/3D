#include "app/avatar.h"

#include <algorithm>
#include <cmath>

#include "app/bar_scene.h"

using engine::Paint;
using engine::Ring;
using engine::Skin;
using engine::Vec3;

namespace app {
namespace {

// Sizes in meters for a person about 1.75 m tall, standing at the origin,
// facing -Z. +X is the avatar's right side.
const Paint kSkinPaint{{0.86f, 0.64f, 0.52f}, 0.35f, -1};
const Paint kShirt{{0.16f, 0.50f, 0.56f}, 0.25f, kTexFabric};
const Paint kJeans{{0.30f, 0.38f, 0.62f}, 0.25f, kTexDenim};
const Paint kBelt{{0.16f, 0.10f, 0.06f}, 0.7f, -1};
const Paint kShoes{{0.10f, 0.08f, 0.07f}, 0.75f, -1};
const Paint kSock{{0.12f, 0.12f, 0.14f}, 0.2f, kTexFabric};
const Paint kHair{{0.20f, 0.12f, 0.07f}, 0.45f, -1};
const Paint kEyeWhite{{0.95f, 0.94f, 0.92f}, 0.8f, -1};
const Paint kIris{{0.22f, 0.13f, 0.07f}, 0.95f, -1};
const Paint kBrow{{0.18f, 0.11f, 0.07f}, 0.2f, -1};
const Paint kLips{{0.70f, 0.38f, 0.36f}, 0.5f, -1};

float smoothstep(float e0, float e1, float x) {
  const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return t * t * (3 - 2 * t);
}

// Bone indices (filled in by build()).
struct Bones {
  int root, hips, spine, chest, neck, head;
  int upperArm[2], forearm[2], hand[2], thigh[2], shin[2], foot[2];
};

// x position along a limb that runs through joints (y descending).
float limbX(float y, const std::vector<Vec3>& joints) {
  for (size_t i = 0; i + 1 < joints.size(); ++i) {
    const Vec3 a = joints[i], b = joints[i + 1];
    if (y >= b.y || i + 2 == joints.size()) {
      const float t = std::clamp((a.y - y) / (a.y - b.y), 0.0f, 1.0f);
      return a.x + (b.x - a.x) * t;
    }
  }
  return joints.back().x;
}

}  // namespace

void Avatar::build() {
  engine::Skeleton& sk = skeleton_;
  Bones b{};
  b.root = sk.add("root", -1, {0, 0, 0});
  b.hips = sk.add("hips", b.root, {0, 0.95f, 0});
  b.spine = sk.add("spine", b.hips, {0, 1.08f, 0});
  b.chest = sk.add("chest", b.spine, {0, 1.26f, 0});
  b.neck = sk.add("neck", b.chest, {0, 1.45f, 0});
  b.head = sk.add("head", b.neck, {0, 1.54f, 0});
  const char* sideName[2] = {".L", ".R"};
  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;
    const std::string n = sideName[i];
    b.upperArm[i] = sk.add("upperArm" + n, b.chest, {s * 0.2f, 1.41f, 0});
    b.forearm[i] = sk.add("forearm" + n, b.upperArm[i], {s * 0.215f, 1.13f, 0});
    b.hand[i] = sk.add("hand" + n, b.forearm[i], {s * 0.225f, 0.87f, 0});
    b.thigh[i] = sk.add("thigh" + n, b.hips, {s * 0.095f, 0.93f, 0});
    b.shin[i] = sk.add("shin" + n, b.thigh[i], {s * 0.1f, 0.5f, 0});
    b.foot[i] = sk.add("foot" + n, b.shin[i], {s * 0.1f, 0.085f, 0});
  }

  engine::SkinnedMeshData& m = mesh_;
  const Vec3 side{1, 0, 0};

  // --- Torso: elliptical cross-sections from the hips to the shoulders.
  auto torsoSkin = [&](float y) {
    return y < 1.13f ? Skin{b.hips, b.spine, smoothstep(0.98f, 1.12f, y)}
                     : Skin{b.spine, b.chest, smoothstep(1.16f, 1.30f, y)};
  };
  struct Section {
    float y, rx, rz;
    const Paint* paint;
  };
  const Section torso[] = {
      {0.80f, 0.10f, 0.08f, &kJeans},   {0.86f, 0.160f, 0.105f, &kJeans}, {0.95f, 0.170f, 0.110f, &kJeans},
      {0.975f, 0.163f, 0.104f, &kJeans}, {0.975f, 0.166f, 0.107f, &kBelt}, {1.015f, 0.158f, 0.102f, &kBelt},
      {1.015f, 0.156f, 0.100f, &kShirt}, {1.08f, 0.148f, 0.098f, &kShirt}, {1.18f, 0.155f, 0.105f, &kShirt},
      {1.27f, 0.165f, 0.115f, &kShirt},  {1.35f, 0.172f, 0.108f, &kShirt}, {1.41f, 0.168f, 0.095f, &kShirt},
      {1.45f, 0.120f, 0.075f, &kShirt},  {1.47f, 0.060f, 0.055f, &kShirt},
  };
  std::vector<Ring> rings;
  for (const Section& s : torso) rings.push_back({{0, s.y, 0.005f}, s.rx, s.rz, torsoSkin(s.y), *s.paint});
  engine::addTube(m, rings, side, 20);

  // --- Neck.
  rings.clear();
  for (float y : {1.42f, 1.48f, 1.53f, 1.58f}) {
    const Skin skin = y < 1.5f ? Skin{b.chest, b.neck, smoothstep(1.43f, 1.48f, y)}
                               : Skin{b.neck, b.head, smoothstep(1.51f, 1.56f, y)};
    rings.push_back({{0, y, 0.01f}, 0.052f, 0.05f, skin, kSkinPaint});
  }
  engine::addTube(m, rings, side, 14);

  // --- Head and face (all follow the head bone).
  const Skin head{b.head, b.head, 0};
  engine::addEllipsoid(m, {0, 1.645f, -0.005f}, {0.083f, 0.108f, 0.098f}, head, kSkinPaint, 24, 18);
  engine::addEllipsoid(m, {0, 1.60f, -0.02f}, {0.068f, 0.055f, 0.065f}, head, kSkinPaint, 18, 12);  // jaw
  engine::addEllipsoid(m, {0, 1.632f, -0.098f}, {0.014f, 0.024f, 0.016f}, head, kSkinPaint, 10, 8);  // nose
  engine::addEllipsoid(m, {0, 1.591f, -0.083f}, {0.022f, 0.006f, 0.007f}, head, kLips, 10, 6);      // mouth
  for (float s : {-1.0f, 1.0f}) {
    engine::addEllipsoid(m, {s * 0.032f, 1.66f, -0.086f}, {0.017f, 0.012f, 0.01f}, head, kEyeWhite, 12, 8);
    engine::addEllipsoid(m, {s * 0.032f, 1.66f, -0.094f}, {0.008f, 0.009f, 0.004f}, head, kIris, 10, 6);
    engine::addEllipsoid(m, {s * 0.033f, 1.686f, -0.085f}, {0.021f, 0.005f, 0.006f}, head, kBrow, 10, 6);
    engine::addEllipsoid(m, {s * 0.083f, 1.645f, 0.0f}, {0.012f, 0.026f, 0.018f}, head, kSkinPaint, 10, 8);  // ear
  }
  // Hair: a slightly bigger shell, cut away over the face.
  engine::addEllipsoid(m, {0, 1.66f, 0.01f}, {0.094f, 0.118f, 0.108f}, head, kHair, 24, 18, [](Vec3 p) {
    return p.y > 1.70f || (p.z > -0.04f && p.y > 1.665f) || (p.z > 0.01f && p.y > 1.57f);
  });

  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;
    // --- Arm: one tube from the shoulder to the fingertips.
    const std::vector<Vec3> armJoints = {
        {s * 0.19f, 1.45f, 0}, {s * 0.2f, 1.41f, 0}, {s * 0.215f, 1.13f, 0}, {s * 0.225f, 0.87f, 0}, {s * 0.228f, 0.70f, 0}};
    struct ArmRing {
      float y, r1, r2;
      const Paint* paint;
    };
    const ArmRing arm[] = {
        {1.41f, 0.05f, 0.05f, &kShirt}, {1.39f, 0.056f, 0.056f, &kShirt}, {1.33f, 0.055f, 0.055f, &kShirt},
        {1.265f, 0.054f, 0.054f, &kShirt}, {1.26f, 0.045f, 0.045f, &kSkinPaint}, {1.20f, 0.044f, 0.044f, &kSkinPaint},
        {1.15f, 0.040f, 0.040f, &kSkinPaint}, {1.13f, 0.038f, 0.038f, &kSkinPaint}, {1.11f, 0.039f, 0.039f, &kSkinPaint},
        {1.05f, 0.040f, 0.040f, &kSkinPaint}, {0.95f, 0.034f, 0.034f, &kSkinPaint}, {0.89f, 0.028f, 0.03f, &kSkinPaint},
        {0.87f, 0.024f, 0.034f, &kSkinPaint}, {0.84f, 0.022f, 0.044f, &kSkinPaint}, {0.78f, 0.022f, 0.046f, &kSkinPaint},
        {0.74f, 0.018f, 0.036f, &kSkinPaint}, {0.715f, 0.012f, 0.02f, &kSkinPaint},
    };
    auto armSkin = [&](float y) {
      if (y > 1.3f) return Skin{b.chest, b.upperArm[i], smoothstep(1.47f, 1.38f, y)};
      if (y > 1.0f) return Skin{b.upperArm[i], b.forearm[i], smoothstep(1.17f, 1.09f, y)};
      return Skin{b.forearm[i], b.hand[i], smoothstep(0.90f, 0.85f, y)};
    };
    rings.clear();
    for (const ArmRing& r : arm) rings.push_back({{limbX(r.y, armJoints), r.y, 0}, r.r1, r.r2, armSkin(r.y), *r.paint});
    engine::addTube(m, rings, side, 14);
    // Rounded shoulder, bridging the torso and the top of the arm.
    engine::addEllipsoid(m, {s * 0.188f, 1.405f, 0.004f}, {0.062f, 0.055f, 0.06f},
                         Skin{b.chest, b.upperArm[i], 0.5f}, kShirt, 16, 10);

    // --- Leg: hip to ankle, jeans with a hem, then a sock.
    const std::vector<Vec3> legJoints = {{s * 0.095f, 0.98f, 0}, {s * 0.1f, 0.5f, 0}, {s * 0.1f, 0.05f, 0}};
    struct LegRing {
      float y, r;
      const Paint* paint;
    };
    const LegRing leg[] = {
        {0.98f, 0.080f, &kJeans}, {0.93f, 0.089f, &kJeans}, {0.85f, 0.086f, &kJeans}, {0.70f, 0.075f, &kJeans},
        {0.56f, 0.061f, &kJeans}, {0.50f, 0.057f, &kJeans}, {0.44f, 0.059f, &kJeans}, {0.36f, 0.061f, &kJeans},
        {0.22f, 0.054f, &kJeans}, {0.12f, 0.052f, &kJeans}, {0.105f, 0.054f, &kJeans}, {0.105f, 0.037f, &kSock},
        {0.06f, 0.036f, &kSock},
    };
    auto legSkin = [&](float y) {
      if (y > 0.8f) return Skin{b.hips, b.thigh[i], smoothstep(1.0f, 0.88f, y)};
      if (y > 0.25f) return Skin{b.thigh[i], b.shin[i], smoothstep(0.56f, 0.44f, y)};
      return Skin{b.shin[i], b.foot[i], smoothstep(0.11f, 0.07f, y)};
    };
    rings.clear();
    for (const LegRing& r : leg) rings.push_back({{limbX(r.y, legJoints), r.y, 0}, r.r, r.r, legSkin(r.y), *r.paint});
    engine::addTube(m, rings, side, 16);

    // --- Shoe: a rounded tube from heel to toe.
    const Skin foot{b.foot[i], b.foot[i], 0};
    rings.clear();
    const float x = s * 0.1f;
    rings.push_back({{x, 0.05f, 0.075f}, 0.03f, 0.03f, foot, kShoes});
    rings.push_back({{x, 0.05f, 0.06f}, 0.046f, 0.048f, foot, kShoes});
    rings.push_back({{x, 0.05f, 0.0f}, 0.05f, 0.05f, foot, kShoes});
    rings.push_back({{x, 0.045f, -0.10f}, 0.052f, 0.042f, foot, kShoes});
    rings.push_back({{x, 0.038f, -0.15f}, 0.045f, 0.033f, foot, kShoes});
    rings.push_back({{x, 0.034f, -0.172f}, 0.025f, 0.02f, foot, kShoes});
    engine::addTube(m, rings, side, 14);
  }
}

void Avatar::pose(float pitch, std::vector<engine::Mat4>& out) const {
  using engine::Quat;
  std::vector<Quat> rot(skeleton_.bones.size());
  const Vec3 xAxis{1, 0, 0}, zAxis{0, 0, 1};
  // Relaxed stance: arms slightly away from the body, elbows a little bent.
  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;
    const char* n = i == 0 ? ".L" : ".R";
    rot[static_cast<size_t>(skeleton_.find(std::string("upperArm") + n))] =
        Quat::axisAngle(zAxis, s * engine::radians(9.0f));
    rot[static_cast<size_t>(skeleton_.find(std::string("forearm") + n))] =
        Quat::axisAngle(xAxis, engine::radians(12.0f));  // forearm forward
  }
  // Look up/down: the neck takes some of it, the head the rest.
  const float look = engine::clampf(pitch, engine::radians(-60.0f), engine::radians(60.0f));
  rot[static_cast<size_t>(skeleton_.find("neck"))] = Quat::axisAngle(xAxis, look * 0.35f);
  rot[static_cast<size_t>(skeleton_.find("head"))] = Quat::axisAngle(xAxis, look * 0.65f);
  skeleton_.skinMatrices(rot, out);
}

engine::Mat4 avatarTransform(Vec3 eye, float yaw) {
  // The camera looks along (sin yaw, 0, -cos yaw); the avatar is built
  // facing -Z, so turn it by -yaw. The eyes are about 1.65 m up.
  return engine::Mat4::transform({eye.x, eye.y - 1.65f, eye.z}, engine::Quat::axisAngle({0, 1, 0}, -yaw));
}

}  // namespace app
