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
// The look is modeled on the user (from a photo): fair, warm skin; dark
// brown curly hair with volume on top, over the ears; short stubble;
// brown eyes and heavy brows; an athletic build with broad shoulders.
const Paint kSkinPaint{{0.90f, 0.69f, 0.57f}, 0.35f, -1};
const Paint kShirt{{0.13f, 0.13f, 0.14f}, 0.25f, kTexFabric};  // charcoal tee
const Paint kJeans{{0.20f, 0.24f, 0.38f}, 0.25f, kTexDenim};   // dark wash
const Paint kBelt{{0.12f, 0.08f, 0.05f}, 0.7f, -1};
const Paint kShoes{{0.88f, 0.87f, 0.85f}, 0.45f, -1};  // white sneakers
const Paint kSole{{0.62f, 0.60f, 0.57f}, 0.3f, -1};
const Paint kSock{{0.12f, 0.12f, 0.14f}, 0.2f, kTexFabric};
const Paint kHair{{0.13f, 0.085f, 0.06f}, 0.55f, kTexHair};
const Paint kStubble{{0.50f, 0.36f, 0.29f}, 0.15f, kTexHair};
const Paint kEyeWhite{{0.95f, 0.94f, 0.92f}, 0.8f, -1};
const Paint kIris{{0.24f, 0.14f, 0.07f}, 0.95f, -1};
const Paint kBrow{{0.11f, 0.075f, 0.05f}, 0.2f, kTexHair};
const Paint kLips{{0.74f, 0.47f, 0.43f}, 0.5f, -1};

// Small deterministic random numbers (the same hair every time).
float hash01(uint32_t n) {
  n = (n ^ 61u) ^ (n >> 16);
  n *= 9u;
  n ^= n >> 4;
  n *= 0x27d4eb2du;
  n ^= n >> 15;
  return static_cast<float>(n & 0xffffffu) / static_cast<float>(0xffffffu);
}

// Where the hair grows, by direction from the head's center (unit vector,
// -Z = the face): over the top, down the back to the nape, and over the
// ears, with a hairline high on the forehead.
bool hairGrows(Vec3 d) {
  if (d.z < -0.5f) return d.y > 0.52f;                          // forehead
  if (d.z < -0.3f) return d.y > 0.25f || (std::abs(d.x) > 0.8f && d.y > 0.05f);  // temples
  if (d.z < 0.15f) return d.y > 0.2f || (std::abs(d.x) > 0.6f && d.y > -0.4f);  // over the ears
  return d.y > -0.62f;                                          // back of the head
}

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
      {1.015f, 0.154f, 0.100f, &kShirt}, {1.08f, 0.144f, 0.097f, &kShirt}, {1.18f, 0.154f, 0.106f, &kShirt},
      {1.27f, 0.172f, 0.121f, &kShirt},  {1.35f, 0.186f, 0.114f, &kShirt}, {1.41f, 0.180f, 0.098f, &kShirt},
      {1.45f, 0.128f, 0.077f, &kShirt},  {1.47f, 0.064f, 0.058f, &kShirt},
  };
  std::vector<Ring> rings;
  for (const Section& s : torso) rings.push_back({{0, s.y, 0.005f}, s.rx, s.rz, torsoSkin(s.y), *s.paint});
  engine::addTube(m, rings, side, 20);

  // --- Neck.
  rings.clear();
  for (float y : {1.42f, 1.48f, 1.53f, 1.58f}) {
    const Skin skin = y < 1.5f ? Skin{b.chest, b.neck, smoothstep(1.43f, 1.48f, y)}
                               : Skin{b.neck, b.head, smoothstep(1.51f, 1.56f, y)};
    rings.push_back({{0, y, 0.01f}, 0.057f, 0.054f, skin, kSkinPaint});
  }
  engine::addTube(m, rings, side, 14);

  // --- Head and face (all follow the head bone).
  const Skin head{b.head, b.head, 0};
  const Vec3 skull{0, 1.645f, -0.005f}, skullR{0.083f, 0.108f, 0.098f};
  const Vec3 jaw{0, 1.60f, -0.02f}, jawR{0.071f, 0.056f, 0.067f};
  engine::addEllipsoid(m, skull, skullR, head, kSkinPaint, 24, 18);
  engine::addEllipsoid(m, jaw, jawR, head, kSkinPaint, 18, 12);
  engine::addEllipsoid(m, {0, 1.548f, -0.052f}, {0.03f, 0.018f, 0.024f}, head, kSkinPaint, 12, 8);  // chin
  engine::addEllipsoid(m, {0, 1.634f, -0.098f}, {0.015f, 0.027f, 0.018f}, head, kSkinPaint, 10, 8);  // nose
  engine::addEllipsoid(m, {0, 1.615f, -0.108f}, {0.012f, 0.01f, 0.011f}, head, kSkinPaint, 10, 8);   // nose tip
  engine::addEllipsoid(m, {0, 1.591f, -0.084f}, {0.023f, 0.0065f, 0.008f}, head, kLips, 10, 6);     // mouth
  for (float s : {-1.0f, 1.0f}) {
    engine::addEllipsoid(m, {s * 0.032f, 1.66f, -0.086f}, {0.017f, 0.011f, 0.01f}, head, kEyeWhite, 12, 8);
    engine::addEllipsoid(m, {s * 0.032f, 1.659f, -0.094f}, {0.008f, 0.009f, 0.004f}, head, kIris, 10, 6);
    // Relaxed upper lids and low, thick brows.
    engine::addEllipsoid(m, {s * 0.032f, 1.667f, -0.087f}, {0.019f, 0.006f, 0.011f}, head, kSkinPaint, 12, 6);
    engine::addEllipsoid(m, {s * 0.026f, 1.681f, -0.088f}, {0.016f, 0.0065f, 0.007f}, head, kBrow, 10, 6);
    engine::addEllipsoid(m, {s * 0.044f, 1.682f, -0.083f}, {0.014f, 0.0055f, 0.007f}, head, kBrow, 10, 6);
    engine::addEllipsoid(m, {s * 0.083f, 1.645f, 0.0f}, {0.012f, 0.026f, 0.018f}, head, kSkinPaint, 10, 8);  // ear
  }

  // Stubble: thin shells over the jaw and lower face, skipping the lips,
  // plus a light mustache.
  auto stubble = [](Vec3 p) {
    if (std::abs(p.x) < 0.03f && std::abs(p.y - 1.591f) < 0.0075f) return false;  // lips
    if (std::abs(p.x) > 0.062f) return p.y < 1.665f && p.z > -0.05f && p.z < 0.02f;  // sideburns, jawline
    return p.y < 1.612f && p.z < 0.0f;
  };
  constexpr float kShell = 0.0022f;
  engine::addEllipsoid(m, skull, skullR + Vec3{kShell, kShell, kShell}, head, kStubble, 24, 18, stubble);
  engine::addEllipsoid(m, jaw, jawR + Vec3{kShell, kShell, kShell}, head, kStubble, 18, 12, stubble);
  engine::addEllipsoid(m, {0, 1.548f, -0.052f}, {0.033f, 0.021f, 0.027f}, head, kStubble, 12, 8,
                       [](Vec3 p) { return p.z < -0.04f || p.y < 1.545f; });
  engine::addEllipsoid(m, {0, 1.601f, -0.09f}, {0.025f, 0.0055f, 0.007f}, head, kStubble, 10, 6);

  // Hair: a cap where it grows, then a few hundred curls over it for the
  // curly volume (more lift on top). Curl shades vary a little for depth.
  const Vec3 hairCenter{0, 1.655f, 0.004f}, capR{0.092f, 0.116f, 0.106f};
  engine::addEllipsoid(m, hairCenter, capR, head, kHair, 24, 18, [&](Vec3 p) {
    const Vec3 d = engine::normalize(Vec3{(p.x - hairCenter.x) / capR.x, (p.y - hairCenter.y) / capR.y,
                                          (p.z - hairCenter.z) / capR.z});
    return hairGrows(d + Vec3{0, 0.08f, 0});  // the cap stops a bit inside the curls
  });
  constexpr int kCurlDirections = 600;
  for (int k = 0; k < kCurlDirections; ++k) {
    // Even directions over the sphere (Fibonacci spiral), jittered.
    const float y = 1.0f - 2.0f * (k + 0.5f) / kCurlDirections;
    const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
    const float a = k * 2.39996323f + hash01(k * 7u + 1u) * 0.3f;
    const Vec3 d{r * std::cos(a), y, r * std::sin(a)};
    if (!hairGrows(d) || hash01(k * 7u + 2u) < 0.35f) continue;
    const float lift = 1.0f + 0.12f * std::max(0.0f, d.y) + 0.05f * hash01(k * 7u + 3u);
    const float size = 0.019f + 0.012f * hash01(k * 7u + 4u);
    const Vec3 c = hairCenter + Vec3{d.x * capR.x, d.y * capR.y, d.z * capR.z} * lift;
    const float shade = 0.75f + 0.5f * hash01(k * 7u + 5u);
    Paint curl = kHair;
    curl.color = kHair.color * shade;
    engine::addEllipsoid(m, c, {size, size * (0.8f + 0.4f * hash01(k * 7u + 6u)), size}, head, curl, 8, 6);
  }
  // A few loose curls falling onto the forehead, more on the right side.
  const Vec3 fringe[] = {{0.035f, 1.735f, -0.088f}, {0.058f, 1.725f, -0.078f}, {0.015f, 1.742f, -0.09f},
                         {-0.03f, 1.745f, -0.088f}};
  for (const Vec3& c : fringe) engine::addEllipsoid(m, c, {0.02f, 0.024f, 0.017f}, head, kHair, 8, 6);

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
        {1.41f, 0.053f, 0.053f, &kShirt}, {1.39f, 0.06f, 0.06f, &kShirt}, {1.33f, 0.061f, 0.061f, &kShirt},
        {1.265f, 0.058f, 0.058f, &kShirt}, {1.26f, 0.049f, 0.051f, &kSkinPaint}, {1.20f, 0.047f, 0.05f, &kSkinPaint},
        {1.15f, 0.041f, 0.042f, &kSkinPaint}, {1.13f, 0.038f, 0.039f, &kSkinPaint}, {1.11f, 0.040f, 0.041f, &kSkinPaint},
        {1.05f, 0.044f, 0.043f, &kSkinPaint}, {0.95f, 0.035f, 0.034f, &kSkinPaint}, {0.89f, 0.028f, 0.03f, &kSkinPaint},
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
    engine::addEllipsoid(m, {s * 0.19f, 1.405f, 0.004f}, {0.068f, 0.058f, 0.064f},
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
    engine::addEllipsoid(m, {x, 0.012f, -0.05f}, {0.054f, 0.014f, 0.13f}, foot, kSole, 14, 6);
  }
}

void advanceWalk(AvatarMotion& m, float forward, float sideways, float dt, float fullSpeed) {
  constexpr float kCycleLength = 1.5f;  // meters per full cycle (two steps)
  const float distance = std::sqrt(forward * forward + sideways * sideways);
  // Walking backward plays the cycle in reverse.
  m.walkPhase += (forward < -0.3f * distance ? -1.0f : 1.0f) * distance / kCycleLength * 2.0f * engine::kPi;
  m.walkPhase = std::fmod(m.walkPhase, 2.0f * engine::kPi);
  // Ease in and out of walking instead of snapping.
  const float speed = dt > 0 ? distance / dt : 0;
  const float target = engine::clampf(speed / fullSpeed, 0, 1);
  m.walkAmount += (target - m.walkAmount) * std::min(1.0f, dt * 8.0f);
}

void Avatar::pose(const AvatarMotion& m, std::vector<engine::Mat4>& out) const {
  using engine::Quat;
  using engine::radians;
  std::vector<Quat> rot(skeleton_.bones.size());
  const Vec3 xAxis{1, 0, 0}, yAxis{0, 1, 0}, zAxis{0, 0, 1};
  auto bone = [&](const std::string& name) -> Quat& { return rot[static_cast<size_t>(skeleton_.find(name))]; };

  const float w = m.walkAmount;
  const float phase = m.walkPhase;
  const float swing = std::sin(phase);  // +1: left leg forward, right leg back
  // Idle: slow breathing and a gentle sway, fading out while walking.
  const float idle = 1.0f - w;
  const float breath = std::sin(m.time * 2.0f * engine::kPi / 4.2f);
  const float sway = std::sin(m.time * 2.0f * engine::kPi / 7.0f);

  // Body: hips twist with the stride, chest twists back, slight forward lean.
  bone("hips") = Quat::axisAngle(yAxis, radians(6.0f) * swing * w) *
                 Quat::axisAngle(zAxis, radians(1.2f) * sway * idle);
  bone("spine") = Quat::axisAngle(xAxis, -radians(3.0f) * w);
  bone("chest") = Quat::axisAngle(yAxis, -radians(9.0f) * swing * w) *
                  Quat::axisAngle(xAxis, radians(1.5f) * breath * idle);

  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;  // -1 = left, +1 = right
    const std::string n = i == 0 ? ".L" : ".R";
    // This leg's swing: forward when positive. The legs are half a cycle apart.
    const float legSwing = i == 0 ? swing : -swing;
    const float legSpeed = i == 0 ? std::cos(phase) : -std::cos(phase);  // > 0 while swinging forward

    // Legs: the thigh swings, the knee bends while the leg travels forward,
    // and the foot stays roughly level with the floor.
    const float thigh = radians(26.0f) * legSwing * w;
    const float knee = -radians(5.0f) * w - radians(50.0f) * std::max(0.0f, legSpeed) * w;
    bone("thigh" + n) = Quat::axisAngle(xAxis, thigh);
    bone("shin" + n) = Quat::axisAngle(xAxis, knee);
    bone("foot" + n) = Quat::axisAngle(xAxis, -(thigh + knee) * 0.75f);

    // Arms swing opposite to the legs; elbows bend a little more going forward.
    const float armSwing = -legSwing;
    bone("upperArm" + n) = Quat::axisAngle(zAxis, s * radians(9.0f + 1.0f * breath * idle)) *
                           Quat::axisAngle(xAxis, radians(20.0f) * armSwing * w + radians(2.0f) * sway * idle * s);
    bone("forearm" + n) =
        Quat::axisAngle(xAxis, radians(12.0f) + radians(18.0f) * std::max(0.0f, armSwing) * w);
  }

  // Look up/down: the neck takes some of it, the head the rest.
  const float look = engine::clampf(m.pitch, radians(-60.0f), radians(60.0f));
  bone("neck") = Quat::axisAngle(xAxis, look * 0.35f);
  bone("head") = Quat::axisAngle(xAxis, look * 0.65f);
  skeleton_.skinMatrices(rot, out);

  // Bob: the body is lowest when the legs are spread, highest mid-step.
  const float bob = -0.03f * swing * swing * w + 0.004f * breath * idle;
  const engine::Mat4 lift = engine::Mat4::translation({0, bob, 0});
  for (engine::Mat4& mat : out) mat = lift * mat;
}

void Avatar::hideHead(const std::vector<engine::Mat4>& bones, std::vector<engine::Mat4>& out) const {
  out = bones;
  const int neck = skeleton_.find("neck"), head = skeleton_.find("head");
  if (neck < 0 || head < 0 || static_cast<size_t>(head) >= out.size()) return;
  // The base of the neck stays put when you look down (only the neck and
  // head bend), so the head and neck fold into it there, out of your view.
  // A skinning matrix maps a rest-pose joint to where it is now.
  const auto n = static_cast<size_t>(neck);
  const engine::Vec4 base = engine::transformPoint(out[n], skeleton_.bones[n].restPosition);
  engine::Mat4 collapse = engine::Mat4::translation({base.x, base.y, base.z});
  for (int c = 0; c < 3; ++c) collapse.at(c, c) = 0;  // every point -> base
  out[n] = collapse;
  out[static_cast<size_t>(head)] = collapse;
}

engine::Mat4 avatarTransform(Vec3 eye, float yaw, float pitch) {
  // The camera looks along (sin yaw, 0, -cos yaw); the avatar is built
  // facing -Z, so turn it by -yaw. Its eyes sit 16 cm above and 9 cm in
  // front of the neck, which the head tilts around: looking down swings the
  // eyes forward over the chest (that's how you see your feet). Shift the
  // body back by that much to keep the eyes at the camera. The feet stay on
  // the floor, so the height is fixed.
  constexpr float kEyeHeight = 1.66f, kEyeAbove = 0.16f, kEyeForward = 0.09f;
  const float p = engine::clampf(pitch, engine::radians(-60.0f), engine::radians(60.0f));
  const float forward = kEyeForward * std::cos(p) - kEyeAbove * std::sin(p);
  const engine::Vec3 back{-std::sin(yaw) * forward, 0, std::cos(yaw) * forward};
  return engine::Mat4::transform({eye.x + back.x, eye.y - kEyeHeight, eye.z + back.z},
                                 engine::Quat::axisAngle({0, 1, 0}, -yaw));
}

}  // namespace app
