#include "bar/avatar.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "bar/bar_scene.h"

using engine::Paint;
using engine::Ring;
using engine::Skin;
using engine::Vec3;

namespace app {
namespace {

// Sizes in meters for a person about 1.75 m tall, standing at the origin,
// facing -Z. +X is the avatar's right side.
// Look 0 is modeled on the user (from a photo): fair, warm skin; dark
// brown curly hair with volume on top, over the ears; short stubble;
// brown eyes and heavy brows; an athletic build with broad shoulders.
// The other looks change colors, sleeves and hair.
const Paint kEyeWhite{{0.95f, 0.94f, 0.92f}, 0.8f, -1};
const Paint kLips{{0.74f, 0.47f, 0.43f}, 0.5f, -1};
const Paint kSock{{0.12f, 0.12f, 0.14f}, 0.2f, kTexFabric};

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

const std::vector<AvatarLook>& avatarLooks() {
  // name, skin, top, pants, shoes, hair color, iris, hair style, long sleeves, stubble
  static const std::vector<AvatarLook> looks = {
      {"Josh", {0.90f, 0.69f, 0.57f}, {0.13f, 0.13f, 0.14f}, {0.20f, 0.24f, 0.38f}, {0.88f, 0.87f, 0.85f},
       {0.13f, 0.085f, 0.06f}, {0.24f, 0.14f, 0.07f}, HairStyle::kCurly, false, true},
      {"Bartender", {0.78f, 0.56f, 0.42f}, {0.92f, 0.92f, 0.9f}, {0.06f, 0.06f, 0.07f}, {0.05f, 0.05f, 0.05f},
       {0.08f, 0.06f, 0.05f}, {0.2f, 0.12f, 0.06f}, HairStyle::kShort, true, false},
      {"Rocker", {0.93f, 0.78f, 0.7f}, {0.05f, 0.05f, 0.05f}, {0.08f, 0.08f, 0.1f}, {0.1f, 0.06f, 0.04f},
       {0.04f, 0.035f, 0.03f}, {0.25f, 0.4f, 0.5f}, HairStyle::kLong, true, true},
      {"Sporty", {0.95f, 0.76f, 0.64f}, {0.75f, 0.1f, 0.1f}, {0.4f, 0.4f, 0.42f}, {0.92f, 0.92f, 0.92f},
       {0.85f, 0.68f, 0.35f}, {0.2f, 0.35f, 0.6f}, HairStyle::kShort, false, false},
      {"Classy", {0.42f, 0.27f, 0.18f}, {0.08f, 0.1f, 0.22f}, {0.08f, 0.1f, 0.22f}, {0.03f, 0.03f, 0.03f},
       {0.05f, 0.04f, 0.03f}, {0.12f, 0.07f, 0.03f}, HairStyle::kBun, true, false},
      {"Neon", {0.85f, 0.62f, 0.48f}, {1.0f, 0.25f, 0.65f}, {0.05f, 0.05f, 0.06f}, {0.95f, 0.95f, 0.95f},
       {0.1f, 0.85f, 0.95f}, {0.3f, 0.2f, 0.1f}, HairStyle::kCurly, false, false},
      {"Cozy", {0.88f, 0.66f, 0.52f}, {0.18f, 0.38f, 0.22f}, {0.72f, 0.64f, 0.5f}, {0.35f, 0.2f, 0.1f},
       {0.45f, 0.18f, 0.08f}, {0.3f, 0.45f, 0.25f}, HairStyle::kLong, true, false},
      {"Robot", {0.62f, 0.64f, 0.68f}, {0.35f, 0.37f, 0.42f}, {0.25f, 0.27f, 0.3f}, {0.15f, 0.15f, 0.17f},
       {0.62f, 0.64f, 0.68f}, {0.1f, 0.9f, 1.0f}, HairStyle::kNone, true, false},
  };
  return looks;
}

void Avatar::build(const AvatarLook& look) {
  const bool fem = look.body == 1;
  const TopStyle topStyle = look.topStyle != TopStyle::kAuto ? look.topStyle
                            : look.longSleeves ? TopStyle::kLongSleeve : TopStyle::kTee;
  const int facial = fem ? 0 : look.facialHair >= 0 ? look.facialHair : (look.stubble ? 1 : 0);
  const Paint kSkinPaint{look.skin, 0.35f, -1};
  const Paint kShirt{look.top, 0.25f, kTexFabric};
  const Paint kJeans{look.pants, 0.25f, look.bottomStyle == BottomStyle::kJoggers ? kTexFabric : kTexDenim};
  const Paint kShoes{look.shoes, 0.45f, -1};
  const Paint kSole{look.shoes * 0.7f, 0.3f, -1};
  const Paint kHair{look.hair, 0.55f, kTexHair};
  const Paint kHair2{look.highlight != Highlight::kNone ? look.hair2 : look.hair, 0.6f, kTexHair};
  const Paint kStubble{look.skin * 0.55f + look.hair * 0.15f, 0.15f, kTexHair};
  const Paint kBeard{look.hair * 0.9f, 0.3f, kTexHair};
  const Paint kIris{look.iris, 0.95f, -1};
  const Paint kBrow{look.hair * 0.85f, 0.2f, kTexHair};
  const Paint kLash{{0.03f, 0.02f, 0.02f}, 0.3f, -1};
  const Paint kLipsPaint = fem ? Paint{{0.72f, 0.32f, 0.36f}, 0.6f, -1} : kLips;
  // Clothes colour at a height: the patterns.
  const auto topAt = [&](float y, bool sleeve) {
    Paint p = kShirt;
    switch (look.topPattern) {
      case Pattern::kStripes:
        if (static_cast<int>(std::floor((y - 1.0f) / 0.055f)) % 2 != 0) p.color = look.top2;
        break;
      case Pattern::kGradient: p.color = engine::lerp(look.top, look.top2, engine::clampf((1.47f - y) / 0.47f, 0, 1)); break;
      case Pattern::kTwoTone: if (sleeve) p.color = look.top2; break;
      default: break;
    }
    return p;
  };
  const auto pantsAt = [&](float y) {
    Paint p = kJeans;
    if (look.bottomPattern == Pattern::kGradient) p.color = engine::lerp(look.pants, look.pants2, engine::clampf((0.98f - y) / 0.88f, 0, 1));
    if (look.bottomPattern == Pattern::kStripes && static_cast<int>(std::floor(y / 0.08f)) % 2 != 0) p.color = look.pants2;
    if (look.bottomPattern == Pattern::kTwoTone) p.color = look.pants2;
    return p;
  };
  engine::Skeleton& sk = skeleton_;
  Bones b{};
  b.root = sk.add("root", -1, {0, 0, 0});
  b.hips = sk.add("hips", b.root, {0, 0.95f, 0});
  b.spine = sk.add("spine", b.hips, {0, 1.08f, 0});
  b.chest = sk.add("chest", b.spine, {0, 1.26f, 0});
  b.neck = sk.add("neck", b.chest, {0, 1.45f, 0});
  b.head = sk.add("head", b.neck, {0, 1.54f, 0});
  const char* sideName[2] = {".L", ".R"};
  const float sh = fem ? 0.92f : 1.0f;  // shoulder width
  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;
    const std::string n = sideName[i];
    // (The same joints for every body: the poses share one skeleton. A
    // feminine body's narrower shoulders are in the mesh only.)
    b.upperArm[i] = sk.add("upperArm" + n, b.chest, {s * 0.2f, 1.41f, 0});
    b.forearm[i] = sk.add("forearm" + n, b.upperArm[i], {s * 0.215f, 1.13f, 0});
    b.hand[i] = sk.add("hand" + n, b.forearm[i], {s * 0.225f, 0.87f, 0});
    b.thigh[i] = sk.add("thigh" + n, b.hips, {s * 0.095f, 0.93f, 0});
    b.shin[i] = sk.add("shin" + n, b.thigh[i], {s * 0.1f, 0.5f, 0});
    b.foot[i] = sk.add("foot" + n, b.shin[i], {s * 0.1f, 0.085f, 0});
  }

  engine::SkinnedMeshData& m = mesh_;
  const Vec3 side{1, 0, 0};

  // --- Torso: elliptical cross-sections from the hips to the shoulders,
  // sampled finely so stripes and gradients follow it; hard edges where one
  // paint meets another (belt, stripes, the top's hem).
  auto torsoSkin = [&](float y) {
    return y < 1.13f ? Skin{b.hips, b.spine, engine::smoothstep(0.98f, 1.12f, y)}
                     : Skin{b.spine, b.chest, engine::smoothstep(1.16f, 1.30f, y)};
  };
  struct Section {
    float y, rx, rz;
  };
  const Section torso[] = {{0.80f, 0.10f, 0.08f},    {0.86f, 0.160f, 0.105f}, {0.95f, 0.170f, 0.110f},
                           {0.975f, 0.164f, 0.105f}, {1.015f, 0.156f, 0.101f}, {1.08f, 0.144f, 0.097f},
                           {1.18f, 0.154f, 0.106f},  {1.27f, 0.172f, 0.121f},  {1.35f, 0.186f, 0.114f},
                           {1.41f, 0.180f, 0.098f},  {1.45f, 0.128f, 0.077f},  {1.47f, 0.064f, 0.058f}};
  const auto chestBump = [&](float y) {
    return engine::smoothstep(1.15f, 1.27f, y) * engine::smoothstep(1.39f, 1.29f, y);
  };
  // The torso's middle moves forward with the bust (its back stays put).
  const auto centerZ = [&](float y) { return fem ? 0.005f - 0.03f * chestBump(y) : 0.005f; };
  const auto shape = [&](float y, float& rx, float& rz) {
    const int n = static_cast<int>(sizeof(torso) / sizeof(torso[0]));
    rx = torso[n - 1].rx;
    rz = torso[n - 1].rz;
    for (int i = 0; i + 1 < n; ++i) {
      if (y <= torso[i + 1].y) {
        const float t = engine::clampf((y - torso[i].y) / (torso[i + 1].y - torso[i].y), 0, 1);
        rx = torso[i].rx + (torso[i + 1].rx - torso[i].rx) * t;
        rz = torso[i].rz + (torso[i + 1].rz - torso[i].rz) * t;
        break;
      }
    }
    if (fem) {  // narrower shoulders and waist, wider hips
      rx *= y > 1.3f ? 0.9f : y > 1.03f ? 0.9f + 0.02f * engine::smoothstep(1.2f, 1.3f, y) : 1.0f + 0.07f * engine::smoothstep(1.03f, 0.92f, y);
    }
    if (fem) rz += 0.03f * chestBump(y);  // the bust, part of the torso's shape
    const float grow = topStyle == TopStyle::kHoodie && y > 1.02f ? 0.01f : topStyle == TopStyle::kJacket && y > 1.0f ? 0.007f : 0.0f;
    rx += grow;
    rz += grow;
  };
  const bool belt = look.bottomStyle == BottomStyle::kJeans || look.bottomStyle == BottomStyle::kShorts;
  const float hem = topStyle == TopStyle::kHoodie || topStyle == TopStyle::kJacket ? 0.97f : 1.015f;
  const auto paintAt = [&](float y) {
    if (y < 0.975f || (!belt && y < hem)) return pantsAt(y);
    if (y < hem) return Paint{{0.12f, 0.08f, 0.05f}, 0.7f, -1};
    return topAt(y, false);
  };
  std::vector<float> ys;
  for (float y = 0.80f; y < 1.47f; y += 0.02f) ys.push_back(y);
  ys.push_back(1.47f);
  // Edges: the waist, the hem, stripe edges.
  std::vector<float> edges = {0.975f, hem};
  if (look.topPattern == Pattern::kStripes) {
    for (float y = 1.0f + 0.055f; y < 1.45f; y += 0.055f) edges.push_back(y);
  }
  std::vector<Ring> rings;
  std::sort(ys.begin(), ys.end());
  size_t e = 0;
  std::sort(edges.begin(), edges.end());
  for (float y : ys) {
    while (e < edges.size() && edges[e] <= y) {
      const float ey = edges[e++];
      float rx, rz;
      shape(ey, rx, rz);
      rings.push_back({{0, ey, centerZ(ey)}, rx, rz, torsoSkin(ey), paintAt(ey - 0.0005f)});
      rings.push_back({{0, ey, centerZ(ey)}, rx, rz, torsoSkin(ey), paintAt(ey + 0.0005f)});
    }
    float rx, rz;
    shape(y, rx, rz);
    rings.push_back({{0, y, centerZ(y)}, rx, rz, torsoSkin(y), paintAt(y)});
  }
  engine::addTube(m, rings, side, 20);
  // Hoodie: a hood folded behind the neck, a front pocket, strings.
  if (topStyle == TopStyle::kHoodie) {
    engine::addEllipsoid(m, {0, 1.46f, 0.075f}, {0.12f, 0.075f, 0.065f}, Skin{b.chest, b.chest, 0}, topAt(1.46f, false), 16, 10);
    Paint pocket = topAt(1.09f, false);
    pocket.color = pocket.color * 0.85f;
    engine::addEllipsoid(m, {0, 1.09f, -0.1f}, {0.1f, 0.045f, 0.014f}, Skin{b.spine, b.spine, 0}, pocket, 14, 6);
    for (float s : {-1.0f, 1.0f}) {
      engine::addEllipsoid(m, {s * 0.03f, 1.36f, -0.124f}, {0.005f, 0.05f, 0.005f}, Skin{b.chest, b.chest, 0},
                           Paint{{0.92f, 0.92f, 0.9f}, 0.3f, -1}, 6, 6);
    }
  }
  // Jacket: open at the front over a shirt in colour 2, with lapels.
  if (topStyle == TopStyle::kJacket) {
    engine::addEllipsoid(m, {0, 1.25f, -0.098f}, {0.068f, 0.19f, 0.026f}, Skin{b.spine, b.chest, 0.5f},
                         Paint{look.top2, 0.25f, kTexFabric}, 12, 12);
    for (float s : {-1.0f, 1.0f}) {
      engine::addEllipsoid(m, {s * 0.07f, 1.35f, -0.1f}, {0.032f, 0.085f, 0.02f}, Skin{b.chest, b.chest, 0},
                           Paint{look.top * 0.8f, 0.3f, kTexFabric}, 8, 8);
    }
  }

  // --- Neck.
  rings.clear();
  for (float y : {1.42f, 1.48f, 1.53f, 1.58f}) {
    const Skin skin = y < 1.5f ? Skin{b.chest, b.neck, engine::smoothstep(1.43f, 1.48f, y)}
                               : Skin{b.neck, b.head, engine::smoothstep(1.51f, 1.56f, y)};
    rings.push_back({{0, y, 0.01f}, fem ? 0.05f : 0.057f, fem ? 0.048f : 0.054f, skin, kSkinPaint});
  }
  engine::addTube(m, rings, side, 14);

  // --- Head and face (all follow the head bone).
  const Skin head{b.head, b.head, 0};
  const Vec3 skull{0, 1.645f, -0.005f}, skullR{0.083f, 0.108f, 0.098f};
  const Vec3 jaw{0, 1.60f, -0.02f}, jawR = fem ? Vec3{0.064f, 0.05f, 0.062f} : Vec3{0.071f, 0.056f, 0.067f};
  engine::addEllipsoid(m, skull, skullR, head, kSkinPaint, 24, 18);
  engine::addEllipsoid(m, jaw, jawR, head, kSkinPaint, 18, 12);
  engine::addEllipsoid(m, {0, 1.548f, -0.052f}, fem ? Vec3{0.025f, 0.016f, 0.021f} : Vec3{0.03f, 0.018f, 0.024f}, head, kSkinPaint, 12, 8);
  engine::addEllipsoid(m, {0, 1.634f, -0.098f}, fem ? Vec3{0.012f, 0.024f, 0.016f} : Vec3{0.015f, 0.027f, 0.018f}, head, kSkinPaint, 10, 8);
  engine::addEllipsoid(m, {0, 1.615f, -0.108f}, {0.011f, 0.009f, 0.01f}, head, kSkinPaint, 10, 8);  // nose tip
  engine::addEllipsoid(m, {0, 1.591f, -0.084f}, fem ? Vec3{0.022f, 0.0085f, 0.009f} : Vec3{0.023f, 0.0065f, 0.008f}, head, kLipsPaint, 10, 6);
  for (float s : {-1.0f, 1.0f}) {
    engine::addEllipsoid(m, {s * 0.032f, 1.66f, -0.086f}, {0.017f, 0.011f, 0.01f}, head, kEyeWhite, 12, 8);
    engine::addEllipsoid(m, {s * 0.032f, 1.659f, -0.094f}, {0.008f, 0.009f, 0.004f}, head, kIris, 10, 6);
    engine::addEllipsoid(m, {s * 0.032f, 1.667f, -0.087f}, {0.019f, 0.006f, 0.011f}, head, kSkinPaint, 12, 6);
    if (fem) {  // lashes, thinner and higher brows
      engine::addEllipsoid(m, {s * 0.034f, 1.6695f, -0.095f}, {0.018f, 0.0022f, 0.004f}, head, kLash, 10, 4);
      engine::addEllipsoid(m, {s * 0.032f, 1.686f, -0.088f}, {0.02f, 0.0035f, 0.006f}, head, kBrow, 10, 6);
    } else {
      engine::addEllipsoid(m, {s * 0.026f, 1.681f, -0.088f}, {0.016f, 0.0065f, 0.007f}, head, kBrow, 10, 6);
      engine::addEllipsoid(m, {s * 0.044f, 1.682f, -0.083f}, {0.014f, 0.0055f, 0.007f}, head, kBrow, 10, 6);
    }
    engine::addEllipsoid(m, {s * 0.083f, 1.645f, 0.0f}, {0.012f, 0.026f, 0.018f}, head, kSkinPaint, 10, 8);  // ear
  }

  // Facial hair: stubble (thin shells over the jaw, skipping the lips, and a
  // light mustache) or a full beard.
  auto stubble = [](Vec3 p) {
    if (std::abs(p.x) < 0.03f && std::abs(p.y - 1.591f) < 0.0075f) return false;  // lips
    if (std::abs(p.x) > 0.062f) return p.y < 1.665f && p.z > -0.05f && p.z < 0.02f;  // sideburns, jawline
    return p.y < 1.612f && p.z < 0.0f;
  };
  constexpr float kShell = 0.0022f;
  if (facial >= 1) {
    const float sh2 = facial == 2 ? 0.009f : kShell;
    const Paint fp = facial == 2 ? kBeard : kStubble;
    engine::addEllipsoid(m, skull, skullR + Vec3{sh2, sh2, sh2}, head, fp, 24, 18, stubble);
    engine::addEllipsoid(m, jaw, jawR + Vec3{sh2, sh2 * 1.6f, sh2}, head, fp, 18, 12, stubble);
    engine::addEllipsoid(m, {0, 1.548f, -0.052f}, {0.033f + sh2, 0.021f + sh2 * 1.5f, 0.027f + sh2}, head, fp, 12, 8,
                         [](Vec3 p) { return p.z < -0.04f || p.y < 1.545f; });
    engine::addEllipsoid(m, {0, 1.601f, -0.09f}, {0.025f, 0.0055f + (facial == 2 ? 0.003f : 0.0f), 0.007f}, head, fp, 10, 6);
  }

  // --- Hair. A cap where it grows, then the style's shape. Highlights:
  // tips (the ends lighter), streaks (some strands), roots (the reverse).
  const Vec3 hairCenter{0, 1.655f, 0.004f}, capR{0.092f, 0.116f, 0.106f};
  const auto strand = [&](float t, uint32_t seed) {  // t: 0 at the root .. 1 at the tip
    Paint p = kHair;
    switch (look.highlight) {
      case Highlight::kTips: p.color = engine::lerp(look.hair, look.hair2, engine::smoothstep(0.35f, 1.0f, t)); break;
      case Highlight::kRoots: p.color = engine::lerp(look.hair2, look.hair, engine::smoothstep(0.0f, 0.6f, t)); break;
      case Highlight::kStreaks: if (hash01(seed) < 0.28f) p.color = look.hair2; break;
      default: break;
    }
    return p;
  };
  const HairStyle hs = look.hairStyle;
  if (hs != HairStyle::kNone) {
    const float grow = hs == HairStyle::kCurly || hs == HairStyle::kAfro ? 0.08f : 0.0f;
    const Paint capPaint = hs == HairStyle::kBuzz ? Paint{look.hair * 0.7f + look.skin * 0.3f, 0.2f, kTexHair}
                           : look.highlight == Highlight::kRoots ? kHair2 : kHair;
    const Vec3 cap = hs == HairStyle::kBuzz || hs == HairStyle::kMohawk ? skullR + Vec3{0.004f, 0.004f, 0.004f} : capR;
    engine::addEllipsoid(m, hairCenter + (hs == HairStyle::kBuzz || hs == HairStyle::kMohawk ? Vec3{0, -0.01f, -0.009f} : Vec3{}),
                         cap, head, hs == HairStyle::kMohawk ? Paint{look.skin * 0.8f + look.hair * 0.2f, 0.2f, kTexHair} : capPaint,
                         24, 18, [&](Vec3 p) {
      const Vec3 d = engine::normalize(Vec3{(p.x - hairCenter.x) / capR.x, (p.y - hairCenter.y) / capR.y,
                                            (p.z - hairCenter.z) / capR.z});
      return hairGrows(d + Vec3{0, grow, 0});  // the curly cap stops a bit inside the curls
    });
  }
  if (hs == HairStyle::kCurly || hs == HairStyle::kAfro) {
    const bool afro = hs == HairStyle::kAfro;
    const int count = afro ? 900 : 600;
    for (int k = 0; k < count; ++k) {
      // Even directions over the sphere (Fibonacci spiral), jittered.
      const float y = 1.0f - 2.0f * (k + 0.5f) / static_cast<float>(count);
      const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
      const float a = static_cast<float>(k) * 2.39996323f + hash01(static_cast<uint32_t>(k) * 7u + 1u) * 0.3f;
      const Vec3 d{r * std::cos(a), y, r * std::sin(a)};
      if (!hairGrows(d) || hash01(static_cast<uint32_t>(k) * 7u + 2u) < 0.35f) continue;
      const float lift = afro ? 1.35f + 0.25f * std::max(0.0f, d.y) + 0.08f * hash01(k * 7u + 3u)
                              : 1.0f + 0.12f * std::max(0.0f, d.y) + 0.05f * hash01(k * 7u + 3u);
      const float size = (afro ? 0.026f : 0.019f) + 0.012f * hash01(k * 7u + 4u);
      const Vec3 c = hairCenter + Vec3{d.x * capR.x, d.y * capR.y, d.z * capR.z} * lift;
      const float shade = 0.75f + 0.5f * hash01(k * 7u + 5u);
      Paint curl = strand(engine::clampf((lift - 1.0f) / (afro ? 0.6f : 0.17f) * 0.7f + 0.3f * hash01(k * 7u + 8u), 0, 1), k * 7u + 9u);
      curl.color = curl.color * shade;
      engine::addEllipsoid(m, c, {size, size * (0.8f + 0.4f * hash01(k * 7u + 6u)), size}, head, curl, 8, 6);
    }
    if (!afro) {  // a few loose curls falling onto the forehead, more on the right side
      const Vec3 fringe[] = {{0.035f, 1.735f, -0.088f}, {0.058f, 1.725f, -0.078f}, {0.015f, 1.742f, -0.09f},
                             {-0.03f, 1.745f, -0.088f}};
      for (const Vec3& c : fringe) engine::addEllipsoid(m, c, {0.02f, 0.024f, 0.017f}, head, strand(0.8f, 3u), 8, 6);
    }
  } else if (hs == HairStyle::kLong) {
    // Falls from the crown to the shoulders behind the head (rings, so the
    // highlight can fade along it).
    engine::addEllipsoid(m, {0, 1.62f, 0.035f}, {0.1f, 0.13f, 0.085f}, head, strand(0.2f, 1u), 18, 12);
    rings.clear();
    const float ys2[] = {1.66f, 1.58f, 1.52f, 1.47f, 1.42f, 1.38f};
    for (int i = 0; i < 6; ++i) {
      const float t = static_cast<float>(i) / 5.0f;
      const Skin sk2 = ys2[i] > 1.52f ? head : Skin{b.neck, b.chest, engine::smoothstep(1.52f, 1.4f, ys2[i])};
      rings.push_back({{0, ys2[i], 0.06f + 0.01f * t}, 0.1f - 0.01f * t, 0.05f, sk2, strand(0.2f + 0.8f * t, 2u)});
    }
    engine::addTube(m, rings, side, 16);
    // Side locks framing the face.
    for (float sx : {-1.0f, 1.0f}) {
      rings.clear();
      for (int i = 0; i < 4; ++i) {
        const float t = static_cast<float>(i) / 3.0f;
        rings.push_back({{sx * 0.085f, 1.68f - 0.17f * t, -0.03f + 0.02f * t}, 0.022f, 0.03f, head, strand(t, 4u)});
      }
      engine::addTube(m, rings, side, 10);
    }
  } else if (hs == HairStyle::kBun) {
    engine::addEllipsoid(m, {0, 1.76f, 0.06f}, {0.045f, 0.04f, 0.045f}, head, strand(1.0f, 5u), 14, 10);
  } else if (hs == HairStyle::kPonytail) {
    engine::addEllipsoid(m, {0, 1.70f, 0.1f}, {0.025f, 0.022f, 0.022f}, head, Paint{{0.1f, 0.1f, 0.12f}, 0.4f, -1}, 8, 6);  // the tie
    rings.clear();
    for (int i = 0; i < 6; ++i) {
      const float t = static_cast<float>(i) / 5.0f;
      const Skin sk2 = t < 0.4f ? head : Skin{b.head, b.neck, engine::smoothstep(0.4f, 1.0f, t)};
      rings.push_back({{0, 1.70f - 0.24f * t, 0.11f + 0.03f * std::sin(t * 3.0f)}, 0.03f * (1 - 0.5f * t) + 0.006f,
                       0.028f * (1 - 0.5f * t) + 0.006f, sk2, strand(t, 6u)});
    }
    engine::addTube(m, rings, side, 12);
  } else if (hs == HairStyle::kBob) {
    // Chin length all round, a fringe at the front.
    rings.clear();
    const float ys2[] = {1.74f, 1.68f, 1.62f, 1.57f};
    for (int i = 0; i < 4; ++i) {
      const float t = static_cast<float>(i) / 3.0f;
      rings.push_back({{0, ys2[i], 0.012f}, 0.1f + 0.008f * t, 0.112f + 0.004f * t, head, strand(0.3f + 0.7f * t, 7u)});
    }
    engine::addTube(m, rings, side, 24);
    engine::addEllipsoid(m, {0, 1.715f, -0.085f}, {0.07f, 0.03f, 0.025f}, head, strand(0.6f, 8u), 14, 8);  // fringe
  } else if (hs == HairStyle::kMohawk) {
    for (int k = 0; k < 9; ++k) {
      const float t = static_cast<float>(k) / 8.0f;
      const float a = -1.1f + 2.4f * t;  // from the forehead over to the back
      const Vec3 c = hairCenter + Vec3{0, std::cos(a) * 0.118f, std::sin(a) * 0.112f};
      engine::addEllipsoid(m, c, {0.016f, 0.045f - 0.012f * std::abs(t - 0.4f), 0.026f}, head, strand(0.5f + 0.5f * t, 9u + k), 8, 8);
    }
  } else if (hs == HairStyle::kShort && look.highlight != Highlight::kNone) {
    // Highlights on short hair: a few lighter tufts on top.
    for (int k = 0; k < 14; ++k) {
      const float a = hash01(k * 3u + 1u) * 6.283f, rr = 0.35f + 0.5f * hash01(k * 3u + 2u);
      const Vec3 d = engine::normalize(Vec3{std::cos(a) * rr, 0.85f, std::sin(a) * rr - 0.15f});
      if (!hairGrows(d)) continue;
      engine::addEllipsoid(m, hairCenter + Vec3{d.x * capR.x, d.y * capR.y, d.z * capR.z} * 1.02f, {0.018f, 0.008f, 0.018f},
                           head, look.highlight == Highlight::kRoots ? kHair : kHair2, 8, 6);
    }
  }

  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;
    // --- Arm: one tube from the shoulder to the fingertips.
    const std::vector<Vec3> armJoints = {{s * 0.19f * sh, 1.45f, 0}, {s * 0.2f * sh, 1.41f, 0}, {s * 0.215f * sh, 1.13f, 0},
                                         {s * 0.225f * sh, 0.87f, 0}, {s * 0.228f * sh, 0.70f, 0}};
    struct ArmRing {
      float y, r1, r2;
      bool cloth;
    };
    const ArmRing arm[] = {
        {1.41f, 0.053f, 0.053f, true}, {1.39f, 0.06f, 0.06f, true}, {1.33f, 0.061f, 0.061f, true},
        {1.265f, 0.058f, 0.058f, true}, {1.26f, 0.049f, 0.051f, false}, {1.20f, 0.047f, 0.05f, false},
        {1.15f, 0.041f, 0.042f, false}, {1.13f, 0.038f, 0.039f, false}, {1.11f, 0.040f, 0.041f, false},
        {1.05f, 0.044f, 0.043f, false}, {0.95f, 0.035f, 0.034f, false}, {0.89f, 0.028f, 0.03f, false},
        {0.87f, 0.024f, 0.034f, false}, {0.84f, 0.022f, 0.044f, false}, {0.78f, 0.022f, 0.046f, false},
        {0.74f, 0.018f, 0.036f, false}, {0.715f, 0.012f, 0.02f, false},
    };
    auto armSkin = [&](float y) {
      if (y > 1.3f) return Skin{b.chest, b.upperArm[i], engine::smoothstep(1.47f, 1.38f, y)};
      if (y > 1.0f) return Skin{b.upperArm[i], b.forearm[i], engine::smoothstep(1.17f, 1.09f, y)};
      return Skin{b.forearm[i], b.hand[i], engine::smoothstep(0.90f, 0.85f, y)};
    };
    const bool longArm = topStyle == TopStyle::kLongSleeve || topStyle == TopStyle::kHoodie || topStyle == TopStyle::kJacket;
    const float thin = fem ? 0.9f : 1.0f;
    rings.clear();
    for (const ArmRing& r : arm) {
      // Sleeves: short (tee), to the wrist (long, hoodie, jacket: a little
      // wider than the arm), none (tank).
      const bool sleeve = topStyle != TopStyle::kTank && (r.cloth || (longArm && r.y > 0.9f));
      const float grow = sleeve && !r.cloth ? 0.006f + (topStyle == TopStyle::kHoodie ? 0.006f : 0.0f) : 0.0f;
      rings.push_back({{limbX(r.y, armJoints), r.y, 0}, r.r1 * thin + grow, r.r2 * thin + grow, armSkin(r.y),
                       sleeve ? topAt(r.y, true) : kSkinPaint});
    }
    engine::addTube(m, rings, side, 14);
    // Rounded shoulder, bridging the torso and the top of the arm.
    engine::addEllipsoid(m, {s * 0.19f * sh, 1.405f, 0.004f}, Vec3{0.068f, 0.058f, 0.064f} * thin,
                         Skin{b.chest, b.upperArm[i], 0.5f}, topStyle == TopStyle::kTank ? kSkinPaint : topAt(1.405f, true), 16, 10);
    if (topStyle == TopStyle::kTank) {  // the tank's strap over the shoulder
      engine::addEllipsoid(m, {s * 0.12f * sh, 1.445f, 0.0f}, {0.022f, 0.012f, 0.06f}, Skin{b.chest, b.chest, 0}, topAt(1.44f, false), 8, 6);
    }

    // --- Leg: hip to ankle. Jeans and joggers to the ankle (joggers with a
    // cuff in colour 2), shorts to above the knee, a skirt (legs bare under it).
    const std::vector<Vec3> legJoints = {{s * 0.095f, 0.98f, 0}, {s * 0.1f, 0.5f, 0}, {s * 0.1f, 0.05f, 0}};
    struct LegRing {
      float y, r;
    };
    const LegRing leg[] = {{0.98f, 0.080f}, {0.93f, 0.089f}, {0.85f, 0.086f}, {0.70f, 0.075f}, {0.62f, 0.066f},
                           {0.56f, 0.061f}, {0.50f, 0.057f}, {0.44f, 0.059f}, {0.36f, 0.061f}, {0.22f, 0.054f},
                           {0.12f, 0.052f}, {0.105f, 0.054f}, {0.105f, 0.037f}, {0.06f, 0.036f}};
    auto legSkin = [&](float y) {
      if (y > 0.8f) return Skin{b.hips, b.thigh[i], engine::smoothstep(1.0f, 0.88f, y)};
      if (y > 0.25f) return Skin{b.thigh[i], b.shin[i], engine::smoothstep(0.56f, 0.44f, y)};
      return Skin{b.shin[i], b.foot[i], engine::smoothstep(0.11f, 0.07f, y)};
    };
    const BottomStyle bs = look.bottomStyle;
    rings.clear();
    for (const LegRing& r : leg) {
      float rr = r.r * (fem && r.y > 0.6f ? 1.04f : 1.0f);
      Paint p = pantsAt(r.y);
      const bool sock = r.y < 0.105f || (&r != &leg[0] && r.y == 0.105f && rr < 0.04f);
      if (sock) p = kSock;
      if (bs == BottomStyle::kShorts && r.y < 0.62f && !sock) p = kSkinPaint, rr *= 0.94f;
      if (bs == BottomStyle::kSkirt && !sock) p = kSkinPaint, rr *= 0.94f;
      if (bs == BottomStyle::kJoggers && !sock && r.y < 0.2f) {
        p = Paint{look.pants2, 0.25f, kTexFabric};  // the cuff
        rr = r.y < 0.13f ? 0.046f : rr;
      }
      rings.push_back({{limbX(r.y, legJoints), r.y, 0}, rr, rr, legSkin(r.y), p});
      if (bs == BottomStyle::kShorts && r.y == 0.62f) {  // the hem, a hard edge
        rings.back().paint = pantsAt(0.62f);
        rings.push_back({{limbX(0.6f, legJoints), 0.6f, 0}, rr * 0.94f, rr * 0.94f, legSkin(0.6f), kSkinPaint});
      }
    }
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
  // The skirt: a cone from the waist to above the knees, moving with the hips.
  if (look.bottomStyle == BottomStyle::kSkirt) {
    rings.clear();
    const float ys2[] = {0.99f, 0.93f, 0.84f, 0.74f, 0.64f, 0.62f};
    for (int k = 0; k < 6; ++k) {
      const float t = static_cast<float>(k) / 5.0f;
      const float rx = (fem ? 0.18f : 0.17f) + 0.08f * t, rz = 0.115f + 0.05f * t;
      rings.push_back({{0, ys2[k], 0.005f}, rx, rz, Skin{b.hips, b.hips, 0}, k == 5 ? Paint{look.pants * 0.8f, 0.3f, kTexFabric} : pantsAt(ys2[k])});
    }
    engine::addTube(m, rings, side, 24);
  }
}

namespace {
// Meters per full walk cycle (two steps): shorter steps sideways.
// Slower walks take shorter steps.
float walkCycle(float dir, float speed) { return (1.3f - 0.5f * std::fabs(std::sin(dir))) * (0.4f + 0.6f * speed); }
// The turn taking direction a onto b (both length 1), the short way.
engine::Quat fromTo(Vec3 a, Vec3 b) {
  const Vec3 c = engine::cross(a, b);
  const float d = engine::dot(a, b);
  if (d < -0.9999f) return engine::Quat::axisAngle({1, 0, 0}, engine::kPi);
  return engine::normalized({c.x, c.y, c.z, 1.0f + d});
}
}  // namespace

void advanceWalk(AvatarMotion& m, float forward, float sideways, float dt, float fullSpeed) {
  const float distance = std::sqrt(forward * forward + sideways * sideways);
  // Walking backward plays the cycle in reverse (the feet then move the
  // other way along the same line).
  const bool back = forward < -0.3f * distance;
  if (distance > 1e-5f) {
    const float target = back ? std::atan2(-sideways, -forward) : std::atan2(sideways, forward);
    const float clamped = engine::clampf(target, -engine::kPi * 0.5f, engine::kPi * 0.5f);
    m.walkDir += (clamped - m.walkDir) * std::min(1.0f, dt * 10.0f);
  }
  // Ease in and out of walking instead of snapping.
  const float speed = dt > 0 ? distance / dt : 0;
  const float target = engine::clampf(speed / fullSpeed, 0, 1);
  m.walkAmount += (target - m.walkAmount) * std::min(1.0f, dt * 8.0f);
  m.walkCycle = walkCycle(m.walkDir, m.walkAmount);
  m.walkPhase += (back ? -1.0f : 1.0f) * distance / m.walkCycle * 2.0f * engine::kPi;
  m.walkPhase = std::fmod(m.walkPhase, 2.0f * engine::kPi);
}

const char* emoteName(Emote e) {
  static const char* kNames[] = {"", "Wave", "Dance", "Clap", "Cheer", "Bow", "Point", "Shrug", "Laugh", "Salute",
                                 "Thumbs up", "Facepalm"};
  const int i = static_cast<int>(e);
  return i >= 0 && i < static_cast<int>(Emote::kCount) ? kNames[i] : "";
}

float emoteLength(Emote e) {
  switch (e) {
    case Emote::kDance: return 1e9f;
    case Emote::kBow: case Emote::kShrug: return 2.0f;
    case Emote::kWave: case Emote::kSalute: case Emote::kThumbsUp: case Emote::kFacepalm: return 2.5f;
    default: return 3.0f;
  }
}

void Avatar::pose(const AvatarMotion& m, std::vector<engine::Mat4>& out) const {
  using engine::Quat;
  using engine::radians;
  std::vector<Quat> rot(skeleton_.bones.size());
  const Vec3 xAxis{1, 0, 0}, yAxis{0, 1, 0}, zAxis{0, 0, 1};
  auto bone = [&](const std::string& name) -> Quat& { return rot[static_cast<size_t>(skeleton_.find(name))]; };
  auto X = [&](float deg) { return Quat::axisAngle(xAxis, radians(deg)); };
  auto Y = [&](float deg) { return Quat::axisAngle(yAxis, radians(deg)); };
  auto Z = [&](float deg) { return Quat::axisAngle(zAxis, radians(deg)); };

  // ---- Standing and walking: the feet are placed on the floor and the legs
  // reach them (two-bone IK), so planted feet don't slide. ----
  const float w = engine::smoothstep(0.0f, 1.0f, m.walkAmount);
  const float wf = engine::smoothstep(0.0f, 0.2f, m.walkAmount);  // the feet: walking ones almost at once
  const float idle = 1.0f - w;
  const float breath = std::sin(m.time * 2.0f * engine::kPi / 4.2f);
  const float sway = std::sin(m.time * 2.0f * engine::kPi / 7.0f);
  // Idle weight shift: slowly from one leg to the other (and a little extra
  // wobble so it doesn't look like a metronome).
  const float shift = std::sin(m.time * 2.0f * engine::kPi / 9.0f) + 0.3f * std::sin(m.time * 2.0f * engine::kPi / 3.7f);
  // The walk: each leg's cycle from heel strike (0) through stance (to
  // kStance) and swing (back to 1). The left leg strikes at walkPhase pi/2.
  constexpr float kStance = 0.56f;
  const float stride = m.walkCycle * kStance * 0.5f;  // the foot's reach, front and back, along the walk
  float u[2];
  for (int i = 0; i < 2; ++i) {
    float v = (m.walkPhase - engine::kPi * 0.5f) / (2.0f * engine::kPi) + (i == 0 ? 0.0f : 0.5f);
    u[i] = v - std::floor(v);
  }
  const Vec3 dir{std::sin(m.walkDir), 0, -std::cos(m.walkDir)};  // the walk's direction (model space; -z forward)
  const Vec3 stepAxis{std::cos(m.walkDir), 0, std::sin(m.walkDir)};  // turning about this moves a foot along `dir`
  float footS[2], footLift[2], ankle[2], legFwd[2], swingAmt[2];
  for (int i = 0; i < 2; ++i) {
    if (u[i] < kStance) {
      const float t = u[i] / kStance;
      footS[i] = stride * (1.0f - 2.0f * t);
      footLift[i] = 0;
      // Toes come down after the heel strikes; the heel lifts before push-off.
      ankle[i] = 14.0f * (1.0f - engine::smoothstep(0.0f, 0.14f, t)) - 26.0f * engine::smoothstep(0.62f, 1.0f, t);
      swingAmt[i] = 0;
    } else {
      const float t = (u[i] - kStance) / (1.0f - kStance);
      const float e = t * t * (3.0f - 2.0f * t);
      footS[i] = stride * (-1.0f + 2.0f * (e + 0.06f * std::sin(t * engine::kPi)));
      footLift[i] = 0.11f * std::pow(std::sin(engine::kPi * std::pow(t, 0.75f)), 1.4f);
      ankle[i] = -26.0f + 30.0f * engine::smoothstep(0.0f, 0.45f, t) + 10.0f * engine::smoothstep(0.6f, 1.0f, t);
      swingAmt[i] = std::sin(engine::kPi * t);
    }
    legFwd[i] = footS[i] / stride;
  }
  // The body: lowest at each heel strike, highest over the standing leg, and
  // over that leg sideways.
  const float strikeDip = 0.5f + 0.5f * std::cos(4.0f * engine::kPi * u[0]);
  Vec3 offset{-0.022f * std::sin(2.0f * engine::kPi * u[0]) * w + 0.024f * shift * idle, 0, 0};
  float bob = (-0.045f * strikeDip - 0.01f) * w + (-0.012f + 0.003f * breath) * idle;
  // Pelvis: twists with the stride, drops on the swinging side; idle: the
  // hip over the standing leg rises.
  const float twist = -7.0f * legFwd[0] * w;
  const float drop = 4.0f * (swingAmt[0] - swingAmt[1]) * w + 2.6f * shift * idle;
  const Quat hipsRot = Y(twist) * Z(drop);
  bone("hips") = hipsRot;
  bone("spine") = Y(-twist) * Z(-0.8f * drop) * X(-4.0f * w);  // back upright, a slight lean into the walk
  bone("chest") = Y(-0.6f * twist) * X(1.5f * breath * idle + 1.0f * w);

  const engine::Skeleton& sk = skeleton_;
  const Vec3 hipsPivot = sk.bones[static_cast<size_t>(sk.find("hips"))].restPosition;
  for (int i = 0; i < 2; ++i) {
    const float s = i == 0 ? -1.0f : 1.0f;  // -1 = left, +1 = right
    const std::string n = i == 0 ? ".L" : ".R";
    const int thighB = sk.find("thigh" + n), shinB = sk.find("shin" + n), footB = sk.find("foot" + n);
    const Vec3 hipRest = sk.bones[static_cast<size_t>(thighB)].restPosition;
    const Vec3 kneeRest = sk.bones[static_cast<size_t>(shinB)].restPosition;
    const Vec3 ankleRest = sk.bones[static_cast<size_t>(footB)].restPosition;
    const float l1 = engine::length(kneeRest - hipRest), l2 = engine::length(ankleRest - kneeRest);
    // Where this ankle goes (on the floor, model space): idle at rest, a hair
    // wider; walking along `dir`, lifted while swinging, raised by the heel.
    const float heel = 0.12f * std::sin(radians(std::max(0.0f, -ankle[i])));
    Vec3 walkTarget = ankleRest + dir * footS[i] + Vec3{0, footLift[i] + heel, 0};
    walkTarget.x += s * 0.06f * std::fabs(dir.x);  // sideways: a wider stance
    walkTarget.x = s < 0 ? std::min(walkTarget.x, -0.035f) : std::max(walkTarget.x, 0.035f);  // feet don't cross
    const Vec3 idleTarget = ankleRest + Vec3{s * 0.012f, 0, 0};
    const Vec3 target = engine::lerp(idleTarget, walkTarget, wf) - offset - Vec3{0, bob, 0};
    // The leg, in the hips' frame.
    const Vec3 hip = hipsPivot + hipsRot.rotate(hipRest - hipsPivot);
    const Vec3 local = conjugate(hipsRot).rotate(target - hip);
    const float d = engine::clampf(engine::length(local), 0.3f, (l1 + l2) * 0.999f);
    const float knee = engine::kPi - std::acos(engine::clampf((l1 * l1 + l2 * l2 - d * d) / (2 * l1 * l2), -1, 1));
    const float beta = std::acos(engine::clampf((l1 * l1 + d * d - l2 * l2) / (2 * l1 * d), -1, 1));
    const Quat aim = fromTo(engine::normalize(ankleRest - hipRest), engine::normalize(local));
    const Quat thighRot = aim * Quat::axisAngle(xAxis, beta);
    const Quat shinRot = Quat::axisAngle(xAxis, -knee);
    bone("thigh" + n) = thighRot;
    bone("shin" + n) = shinRot;
    // The foot: flat on the floor (toes a little out), rolling heel to toe.
    const Quat footWorld = Quat::axisAngle(stepAxis, radians(ankle[i] * wf)) * Y(-s * 6.0f);
    bone("foot" + n) = conjugate(hipsRot * thighRot * shinRot) * footWorld;

    // Arms swing opposite to the legs (forward a bit more than back), the
    // elbows bend more on the way forward; idle: relaxed, breathing.
    const float armSwing = -legFwd[i] * w;
    const float armDeg = armSwing > 0 ? 24.0f * armSwing : 16.0f * armSwing;
    bone("upperArm" + n) = Z(s * (8.0f + 1.0f * breath * idle)) * X(armDeg + 3.0f * idle + 2.0f * sway * idle * s);
    bone("forearm" + n) = X(14.0f + 2.0f * breath * idle + 22.0f * std::max(0.0f, armSwing));
    bone("hand" + n) = X(-6.0f - 6.0f * std::max(0.0f, -armSwing)) * Z(s * 4.0f);
  }
  // Head steady while the chest twists; idle: glances around now and then.
  const float glance = std::sin(m.time * 0.37f) * std::sin(m.time * 0.23f + 1.0f);
  bone("neck") = Y(0.6f * twist + 8.0f * glance * idle);

  // ---- Activities (crude key poses, see docs/REMASTER.md) ----
  const float t = m.actionTime;
  const bool seated = m.activity == Activity::kSit || m.activity == Activity::kDrive || m.activity == Activity::kPiano;
  if (seated) {
    // Thighs forward, shins hanging; a low seat bends the knees more.
    for (const char* n : {".L", ".R"}) {
      bone(std::string("thigh") + n) = X(85 + 10 * m.lowSeat) * Z(n[1] == 'L' ? 4.0f : -4.0f);
      bone(std::string("shin") + n) = X(-85 - 15 * m.lowSeat);
      bone(std::string("foot") + n) = X(5);
      bone(std::string("upperArm") + n) = Z(n[1] == 'L' ? -8.0f : 8.0f) * X(18);
      bone(std::string("forearm") + n) = X(45);
    }
    bone("hips") = Quat{};
    bone("spine") = X(2 + 4 * breath);
    bone("chest") = X(1.0f * breath);
    bob = 0;
    offset = {};
  }
  if (m.activity == Activity::kDrive) {
    // Hands at ten and two on the wheel; the wheel turn swings the arms.
    const float st = engine::clampf(m.steer, -1, 1) * 25.0f;
    bone("upperArm.L") = Z(-12 + st * 0.6f) * X(52 - st * 0.4f);
    bone("upperArm.R") = Z(12 + st * 0.6f) * X(52 + st * 0.4f);
    bone("forearm.L") = X(38);
    bone("forearm.R") = X(38);
    bone("spine") = X(10);  // the bucket seat leans back
  }
  if (m.activity == Activity::kPiano) {
    // Forearms level over the keys; each hand slides to where it plays and
    // the fingers' work shows as a small bounce.
    const float hands[2] = {m.handL, m.handR};
    for (int i = 0; i < 2; ++i) {
      const float sgn = i == 0 ? -1.0f : 1.0f;
      const std::string n = i == 0 ? ".L" : ".R";
      const float reach = engine::clampf(hands[i], -1, 1) * 28.0f;  // degrees outward (+) or inward
      const float bounce = 4.0f * std::sin(t * 13.0f + static_cast<float>(i) * 1.7f);
      bone("upperArm" + n) = Z(sgn * (6 + reach * sgn)) * X(32 + bounce * 0.3f);
      bone("forearm" + n) = X(52 + bounce);
      bone("hand" + n) = X(-12);
    }
    bone("spine") = X(-4);
  }
  if (m.activity == Activity::kRhythm) {
    const float tapL = std::max(0.0f, std::sin(t * 9.0f)), tapR = std::max(0.0f, std::sin(t * 9.0f + 2.1f));
    bone("upperArm.L") = Z(-10) * X(38 + 8 * tapL);
    bone("upperArm.R") = Z(10) * X(38 + 8 * tapR);
    bone("forearm.L") = X(55 - 20 * tapL);
    bone("forearm.R") = X(55 - 20 * tapR);
    bone("spine") = X(-6);
  }
  if (m.activity == Activity::kDarts) {
    // Aim with the dart by the eye; a throw (first 0.35 s) snaps the forearm out.
    const float k = engine::clampf(t / 0.35f, 0, 1);
    const float flick = t < 0.35f ? std::sin(k * engine::kPi * 0.5f) : 0.0f;
    bone("upperArm.R") = Z(6) * X(95 - 10 * flick);
    bone("forearm.R") = X(115 * (1 - flick) + 5);
    bone("hand.R") = X(-20 + 40 * flick);
    bone("upperArm.L") = Z(-15) * X(10);
  }
  if (m.activity == Activity::kBartend) {
    // Shaking a cocktail over the shoulder.
    const float shake = 12.0f * std::sin(t * 22.0f);
    bone("upperArm.R") = Z(20) * X(110 + shake);
    bone("forearm.R") = X(70);
    bone("upperArm.L") = Z(-15) * X(95 + shake);
    bone("forearm.L") = X(80);
    bone("chest") = Y(shake * 0.3f);
  }
  if (m.activity == Activity::kDrink || m.pose == Activity::kDrink) {
    // Glass to the mouth for a sip every few seconds, otherwise held low.
    const float st = m.activity == Activity::kDrink ? t : m.poseTime;
    const float sip = std::pow(std::max(0.0f, std::sin(st * 1.4f)), 4.0f);
    bone("upperArm.R") = Z(10) * X(25 + 30 * sip);
    bone("forearm.R") = X(70 + 65 * sip);
  }
  if (m.activity == Activity::kEmote) {
    // Fade in over 0.25 s and out at the end, so it doesn't snap.
    const float len = emoteLength(m.emote);
    const float fade = engine::smoothstep(0.0f, 1.0f, std::min(t / 0.3f, (len - t) / 0.35f));
    auto set = [&](const char* name, Quat q) {
      Quat& b = bone(name);
      // Blend from the walking/idle pose toward the emote pose.
      const float d = b.x * q.x + b.y * q.y + b.z * q.z + b.w * q.w;
      const float sgn = d < 0 ? -1.0f : 1.0f;
      Quat r{b.x + (q.x * sgn - b.x) * fade, b.y + (q.y * sgn - b.y) * fade, b.z + (q.z * sgn - b.z) * fade,
             b.w + (q.w * sgn - b.w) * fade};
      const float len2 = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
      b = {r.x / len2, r.y / len2, r.z / len2, r.w / len2};
    };
    const float osc = std::sin(t * 8.0f);
    switch (m.emote) {
      case Emote::kWave:
        set("upperArm.R", Z(125));
        set("forearm.R", Z(25 * osc) * X(20));
        break;
      case Emote::kDance: {
        const float beat = std::sin(t * engine::kPi * 2.2f);
        set("hips", Y(14 * beat) * Z(6 * beat));
        set("chest", Y(-10 * beat));
        set("upperArm.L", Z(-60 - 50 * std::max(0.0f, beat)) * X(30));
        set("upperArm.R", Z(60 + 50 * std::max(0.0f, -beat)) * X(30));
        set("forearm.L", X(70));
        set("forearm.R", X(70));
        set("thigh.L", X(15 * std::max(0.0f, beat)));
        set("shin.L", X(-30 * std::max(0.0f, beat)));
        set("thigh.R", X(15 * std::max(0.0f, -beat)));
        set("shin.R", X(-30 * std::max(0.0f, -beat)));
        bob += -0.03f * std::fabs(beat) * fade;
        break;
      }
      case Emote::kClap: {
        const float c = std::fabs(std::sin(t * 7.0f));
        set("upperArm.L", Z(10 + 12 * c) * X(55));
        set("upperArm.R", Z(-10 - 12 * c) * X(55));
        set("forearm.L", X(60) * Y(-20));
        set("forearm.R", X(60) * Y(20));
        break;
      }
      case Emote::kCheer:
        set("upperArm.L", Z(-150 + 15 * osc));
        set("upperArm.R", Z(150 - 15 * osc));
        set("forearm.L", X(10));
        set("forearm.R", X(10));
        bob += 0.03f * std::max(0.0f, osc) * fade;
        break;
      case Emote::kBow:
        set("spine", X(-30));
        set("chest", X(-20));
        set("upperArm.R", Z(-10) * X(40));
        set("forearm.R", X(80));
        break;
      case Emote::kPoint:
        set("upperArm.R", X(85));
        set("forearm.R", X(5));
        break;
      case Emote::kShrug:
        set("upperArm.L", Z(-25) * X(10));
        set("upperArm.R", Z(25) * X(10));
        set("forearm.L", X(75) * Y(30));
        set("forearm.R", X(75) * Y(-30));
        set("head", Z(10));
        break;
      case Emote::kLaugh: {
        const float h = std::sin(t * 18.0f);
        set("chest", X(4 + 4 * h));
        set("spine", X(-8));
        set("upperArm.L", Z(-5) * X(20));
        set("upperArm.R", Z(5) * X(20));
        set("forearm.L", X(85));
        set("forearm.R", X(85));
        break;
      }
      case Emote::kSalute:
        set("upperArm.R", Z(70) * X(35));
        set("forearm.R", X(135));
        break;
      case Emote::kThumbsUp:
        set("upperArm.R", Z(10) * X(40));
        set("forearm.R", X(75));
        break;
      case Emote::kFacepalm:
        set("upperArm.R", Z(-5) * X(65));
        set("forearm.R", X(140));
        break;
      default: break;
    }
  }

  // Look up/down: the neck takes some of it, the head the rest; turning the
  // head (while seated, the body stays facing the seat) is split the same way.
  const float look = engine::clampf(m.pitch, radians(-60.0f), radians(60.0f));
  const float turn = engine::clampf(m.headYaw, radians(-75.0f), radians(75.0f));
  const Quat headExtra = bone("head");  // emotes may tilt the head
  bone("neck") = Quat::axisAngle(yAxis, -turn * 0.4f) * Quat::axisAngle(xAxis, look * 0.35f) * bone("neck");
  bone("head") = Quat::axisAngle(yAxis, -turn * 0.6f) * Quat::axisAngle(xAxis, look * 0.65f) * headExtra;
  skeleton_.skinMatrices(rot, out);

  // Bob: the body is lowest when the legs are spread, highest mid-step.
  const engine::Mat4 lift = engine::Mat4::translation({offset.x, bob, offset.z});
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

namespace app {

engine::Mat4 seatedTransform(Vec3 hip, float bodyYaw) {
  // The hip joint is 0.95 m above the feet in the rest pose.
  return engine::Mat4::transform({hip.x, hip.y - 0.95f, hip.z}, engine::Quat::axisAngle({0, 1, 0}, -bodyYaw));
}

Vec3 seatedEye(Vec3 hip, float bodyYaw, float lowSeat) {
  // Eyes 0.71 m above the hips, a bit forward (leaning back a little on
  // low seats).
  const float forward = 0.09f - 0.05f * lowSeat;
  return {hip.x + std::sin(bodyYaw) * forward, hip.y + 0.71f, hip.z - std::cos(bodyYaw) * forward};
}

}  // namespace app

// ---- The customizer (AvatarStyle) ----
namespace app {
namespace {

const Vec3 kSkinPalette[kSkinTones] = {{0.98f, 0.84f, 0.74f}, {0.95f, 0.76f, 0.64f}, {0.90f, 0.69f, 0.57f}, {0.85f, 0.62f, 0.48f},
                                       {0.78f, 0.56f, 0.42f}, {0.66f, 0.45f, 0.32f}, {0.55f, 0.36f, 0.25f}, {0.42f, 0.27f, 0.18f},
                                       {0.32f, 0.2f, 0.13f},  {0.62f, 0.64f, 0.68f}};
const Vec3 kHairPalette[kHairColors] = {{0.04f, 0.035f, 0.03f}, {0.13f, 0.085f, 0.06f}, {0.3f, 0.17f, 0.08f}, {0.55f, 0.32f, 0.14f},
                                        {0.85f, 0.68f, 0.35f}, {0.95f, 0.88f, 0.65f}, {0.62f, 0.15f, 0.06f}, {0.85f, 0.85f, 0.88f},
                                        {1.0f, 0.4f, 0.75f},   {0.55f, 0.25f, 0.95f}, {0.15f, 0.55f, 1.0f},  {0.1f, 0.85f, 0.75f},
                                        {0.4f, 0.9f, 0.25f},   {1.0f, 0.55f, 0.1f}};
const Vec3 kClothPalette[kClothColors] = {
    {0.06f, 0.06f, 0.07f}, {0.92f, 0.92f, 0.9f},  {0.45f, 0.46f, 0.5f},  {0.13f, 0.13f, 0.14f}, {0.75f, 0.1f, 0.1f},
    {1.0f, 0.42f, 0.1f},   {0.98f, 0.8f, 0.15f},  {0.25f, 0.65f, 0.2f},  {0.08f, 0.35f, 0.2f},  {0.1f, 0.75f, 0.85f},
    {0.2f, 0.24f, 0.38f},  {0.08f, 0.1f, 0.22f},  {0.25f, 0.45f, 0.95f}, {0.45f, 0.2f, 0.75f},  {1.0f, 0.25f, 0.65f},
    {0.98f, 0.7f, 0.78f},  {0.72f, 0.64f, 0.5f},  {0.45f, 0.28f, 0.15f}, {0.55f, 0.08f, 0.15f}, {0.7f, 0.9f, 1.0f}};
const Vec3 kEyePalette[kEyeColors] = {{0.24f, 0.14f, 0.07f}, {0.12f, 0.07f, 0.03f}, {0.25f, 0.4f, 0.55f}, {0.2f, 0.45f, 0.25f},
                                      {0.45f, 0.35f, 0.15f}, {0.35f, 0.38f, 0.42f}, {0.6f, 0.2f, 0.75f}, {0.1f, 0.9f, 1.0f}};
constexpr int kRanges[AvatarStyle::kFields] = {2, kSkinTones, static_cast<int>(HairStyle::kCount), kHairColors, kHairColors,
                                               static_cast<int>(Highlight::kCount), static_cast<int>(TopStyle::kCount),
                                               kClothColors, kClothColors, static_cast<int>(Pattern::kCount),
                                               static_cast<int>(BottomStyle::kCount), kClothColors, kClothColors,
                                               static_cast<int>(Pattern::kCount), kClothColors, kEyeColors, 3};

std::string hex(Vec3 c) {
  char b[8];
  std::snprintf(b, sizeof(b), "#%02x%02x%02x", static_cast<int>(engine::clampf(c.x, 0, 1) * 255),
                static_cast<int>(engine::clampf(c.y, 0, 1) * 255), static_cast<int>(engine::clampf(c.z, 0, 1) * 255));
  return b;
}

}  // namespace

bool AvatarStyle::operator==(const AvatarStyle& o) const { return std::equal(std::begin(v), std::end(v), std::begin(o.v)); }

void AvatarStyle::clamp() {
  for (int i = 0; i < kFields; ++i) v[i] = std::clamp(v[i], 0, kRanges[i] - 1);
}

std::string AvatarStyle::key() const {
  std::string k;
  for (int i = 0; i < kFields; ++i) k += std::to_string(v[i]) + ".";
  return k;
}

AvatarLook lookFromStyle(const AvatarStyle& in) {
  AvatarStyle s = in;
  s.clamp();
  AvatarLook l{"Custom", kSkinPalette[s.v[1]], kClothPalette[s.v[7]], kClothPalette[s.v[11]], kClothPalette[s.v[14]],
               kHairPalette[s.v[3]], kEyePalette[s.v[15]], static_cast<HairStyle>(s.v[2]), false, false};
  l.body = s.v[0];
  l.hair2 = kHairPalette[s.v[4]];
  l.highlight = static_cast<Highlight>(s.v[5]);
  l.topStyle = static_cast<TopStyle>(s.v[6]);
  l.top2 = kClothPalette[s.v[8]];
  l.topPattern = static_cast<Pattern>(s.v[9]);
  l.bottomStyle = static_cast<BottomStyle>(s.v[10]);
  l.pants2 = kClothPalette[s.v[12]];
  l.bottomPattern = static_cast<Pattern>(s.v[13]);
  l.facialHair = s.v[16];
  return l;
}

std::string avatarPaletteJson() {
  std::string j = "{";
  const auto list = [&](const char* name, const Vec3* c, int n) {
    j += std::string("\"") + name + "\":[";
    for (int i = 0; i < n; ++i) j += (i ? ",\"" : "\"") + hex(c[i]) + "\"";
    j += "]";
  };
  list("skin", kSkinPalette, kSkinTones);
  j += ",";
  list("hair", kHairPalette, kHairColors);
  j += ",";
  list("cloth", kClothPalette, kClothColors);
  j += ",";
  list("eyes", kEyePalette, kEyeColors);
  return j + "}";
}

std::string walkReport(const Avatar& avatar) {
  const engine::Skeleton& sk = avatar.skeleton();
  const int feet[2] = {sk.find("foot.L"), sk.find("foot.R")};
  std::string out = "Walk check:";
  const struct { const char* name; float fwd, side; } kCases[] = {{"forward", 1, 0}, {"sideways", 0, 1}, {"backward", -1, 0}, {"slow", 0.35f, 0}};
  for (const auto& c : kCases) {
    AvatarMotion m;
    std::vector<engine::Mat4> bones;
    const float dt = 1.0f / 60.0f, speed = 2.2f;
    Vec3 body{0, 0, 0}, last[2]{};
    float slide = 0, planted = 0, lowest = 1, travel = 0;
    for (int f = 0; f < 360; ++f) {
      const float fwd = c.fwd * speed * dt, side = c.side * speed * dt;
      advanceWalk(m, fwd, side, dt, speed);
      body += Vec3{side, 0, -fwd};
      m.time = static_cast<float>(f) * dt;
      avatar.pose(m, bones);
      for (int i = 0; i < 2; ++i) {
        const size_t b = static_cast<size_t>(feet[i]);
        const engine::Vec4 p = engine::transformPoint(bones[b], sk.bones[b].restPosition);
        const Vec3 world = Vec3{p.x, p.y, p.z} + body;
        lowest = std::min(lowest, p.y);
        if (f > 120 && p.y < 0.095f) {  // planted (after the walk settles)
          slide += engine::length(Vec3{world.x - last[i].x, 0, world.z - last[i].z});
          planted += dt;
          travel += speed * dt;
        }
        last[i] = world;
      }
    }
    char line[160];
    std::snprintf(line, sizeof(line), " %s: planted feet slide %.0f%% of the walk, ankles down to %.3f m;", c.name,
                  travel > 0 ? 100.0f * slide / (travel * std::max(0.05f, std::fabs(c.fwd) + std::fabs(c.side))) : 0.0f, lowest);
    out += line;
  }
  return out;
}

}  // namespace app
