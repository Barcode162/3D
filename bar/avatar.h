#pragma once
// Your body in the bar: a skeleton (18 bones) plus one smooth "skinned" mesh
// that bends with it. It stands where you stand, turns with you, and tilts
// its head as you look up and down. Seen in the mirrors.
#include <string>
#include <vector>

#include "looped/math.h"
#include "looped/skeleton.h"
#include "looped/skinned_mesh.h"
#include "bar/activity_kinds.h"

namespace app {

const char* emoteName(Emote e);
// How long an emote plays (seconds; dancing goes on until you move).
float emoteLength(Emote e);

// What drives the avatar's animation this frame.
struct AvatarMotion {
  float pitch = 0;       // where you look (radians, + up)
  float headYaw = 0;     // head turned left/right of the body (radians, + = to the right)
  float walkPhase = 0;   // position in the walk cycle (radians; one cycle = 2 steps)
  float walkAmount = 0;  // 0 = standing, 1 = full walking speed
  float walkCycle = 1.3f;  // meters per walk cycle right now (shorter when slow)
  float walkDir = 0;     // the walk's direction from facing (radians, + = to the right; backward is mirrored)
  float time = 0;        // seconds, for idle breathing and sway
  Activity activity = Activity::kNone;
  Emote emote = Emote::kNone;
  float actionTime = 0;  // seconds since the activity or emote started
  // An upper-body pose on top of the activity (the pose layer): e.g. kDrink
  // while seated keeps the legs on the stool and lifts the glass.
  Activity pose = Activity::kNone;
  float poseTime = 0;
  float steer = 0;       // driving: -1 left .. 1 right
  float handL = 0, handR = 0;  // piano: where each hand is along the keys (-1 .. 1)
  float lowSeat = 0;     // 0 = a stool or chair, 1 = a low sofa or beanbag
};

// Advances the walk cycle by how far you moved, so steps match your speed
// and the feet don't skate. `forwardDistance` is signed (backward < 0).
void advanceWalk(AvatarMotion& motion, float forwardDistance, float sideDistance, float dt, float fullSpeed);

enum class HairStyle { kCurly, kShort, kLong, kBun, kNone, kPonytail, kBob, kMohawk, kAfro, kBuzz, kCount };
enum class TopStyle { kAuto = -1, kTee, kLongSleeve, kTank, kHoodie, kJacket, kCount };
enum class BottomStyle { kJeans, kShorts, kSkirt, kJoggers, kCount };
// Patterns: solid, stripes (tops), a gradient from colour 1 to colour 2 down
// the body, two-tone (tops: sleeves in colour 2).
enum class Pattern { kSolid, kStripes, kGradient, kTwoTone, kCount };
enum class Highlight { kNone, kTips, kStreaks, kRoots, kCount };

// One avatar look: the presets in the menu, or one made in the customizer
// (lookFromStyle). Fields after `stubble` have defaults, so the presets only
// fill the first ones.
struct AvatarLook {
  const char* name;
  engine::Vec3 skin, top, pants, shoes, hair, iris;
  HairStyle hairStyle;
  bool longSleeves;
  bool stubble;
  int body = 0;                       // 0 = masculine, 1 = feminine
  engine::Vec3 hair2{0, 0, 0};        // highlight colour
  Highlight highlight = Highlight::kNone;
  TopStyle topStyle = TopStyle::kAuto;  // kAuto = tee or long sleeve from longSleeves
  engine::Vec3 top2{0, 0, 0};
  Pattern topPattern = Pattern::kSolid;
  BottomStyle bottomStyle = BottomStyle::kJeans;
  engine::Vec3 pants2{0, 0, 0};
  Pattern bottomPattern = Pattern::kSolid;
  int facialHair = -1;                // -1 = from stubble, 0 none, 1 stubble, 2 beard
};
const std::vector<AvatarLook>& avatarLooks();

// The customizer's choices (bar/web/menu.js avatar panel): small numbers, each an
// index into a palette or a style list. Sent to the room as world key
// a<slot> (the same order), so everyone builds the same avatar.
// The look number sent to others for "my own style" (world key a<slot>).
constexpr int kCustomLook = 255;

struct AvatarStyle {
  static constexpr int kFields = 17;
  int v[kFields] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 1};
  // body, skin, hairStyle, hairColor, highlightColor, highlight, topStyle,
  // topColor, topColor2, topPattern, bottomStyle, bottomColor, bottomColor2,
  // bottomPattern, shoes, eyes, facialHair
  bool operator==(const AvatarStyle& o) const;
  void clamp();  // each field into its range
  std::string key() const;  // e.g. "0.3.1..." (to share built meshes)
};
// Palette sizes (and the menu's swatches use the same colours: avatarPalette).
constexpr int kSkinTones = 10, kHairColors = 14, kClothColors = 20, kEyeColors = 8;
AvatarLook lookFromStyle(const AvatarStyle& s);
// The colours as "#rrggbb" lists for the menu: "skin", "hair", "cloth", "eyes".
std::string avatarPaletteJson();

class Avatar;
// For tests: walks the avatar at full speed forward, sideways and backward
// and reports how far planted feet slide and how low the ankles go.
std::string walkReport(const Avatar& avatar);

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
