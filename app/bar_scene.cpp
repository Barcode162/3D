#include "app/bar_scene.h"

#include <algorithm>
#include <cmath>

using engine::Quat;
using engine::Vec3;

namespace app {
namespace {

// The floor plan (docs/BAR_REDESIGN.md; same numbers as
// tools/assets/layout.py). x to the right, -z toward the bar.
constexpr float kX0 = -9.0f, kX1 = 9.0f;  // left and right walls
constexpr float kZ0 = -7.4f, kZ1 = 7.0f;  // back wall (bar) and front wall (street)
constexpr float kH = 4.0f;                // hall ceiling
constexpr float kWall = 0.3f;
constexpr float kSoffitX1 = 2.6f, kSoffitZ1 = -3.5f, kSoffitY = 3.1f;
constexpr float kChamferAX = 6.0f, kChamferBZ = kZ0 + 3.0f;  // diagonal wall (6, z0) to (9, z0 + 3)
constexpr float kBayX0 = -2.2f, kBayX1 = 1.8f, kBayDepth = 1.0f;
constexpr float kEntryX0 = 2.5f, kEntryX1 = 3.9f;
constexpr float kDoorAX0 = 2.9f, kDoorAX1 = 4.5f;
constexpr float kDoorBZ0 = -3.3f, kDoorBZ1 = -1.7f;
constexpr float kStageX1 = -5.4f, kStageZ0 = 3.8f, kStageH = 0.3f, kStepW = 0.35f, kStepH = 0.15f;
constexpr float kChimneyZ0 = -0.2f, kChimneyZ1 = 1.8f, kChimneyD = 0.45f;
// The bar counter's customer edge bows along a big circle from x = kBarXA
// to kBarXB, then the counter turns and runs back to the back wall.
constexpr float kBarXA = -7.9f, kBarXB = 1.1f, kBarZEnds = -4.75f, kBarZMid = -4.0f;
constexpr float kBarInset = 0.36f, kBarTurnR = 0.55f;
constexpr float kBarTopY = 1.08f;  // where glasses stand on the counter

// Light and glow colors.
constexpr Vec3 kWarmBulb{1.0f, 0.68f, 0.38f};
constexpr Vec3 kNeonPink{1.0f, 0.18f, 0.55f};
constexpr Vec3 kFire{1.0f, 0.45f, 0.15f};

struct Builder {
  BarScene& scene;

  // Places a mesh made of material `m`.
  SceneObject& add(engine::MeshData mesh, Vec3 pos, MaterialId m, Quat rot = {}) {
    const engine::Material& mat = material(m);
    scene.objects.push_back({std::move(mesh), pos, rot, m, mat.color, mat.emissive, mat.textureScale});
    return scene.objects.back();
  }

  // Places a .glb model (assets/models/<file>); returns its index.
  size_t model(const char* file, Vec3 pos, MaterialId m, Quat rot = {}, Prop prop = Prop::kNone) {
    scene.models.push_back({file, pos, rot, 1, m, prop});
    return scene.models.size() - 1;
  }

  // An invisible solid box for physics (a model's shape).
  void solid(Vec3 center, Vec3 half, MaterialId m, Quat rot = {}) {
    scene.colliders.push_back({center, half, rot, m});
  }

  void block(Vec3 center, float halfX, float halfZ) {
    scene.blockers.push_back({center.x - halfX, center.z - halfZ, center.x + halfX, center.z + halfZ});
  }
  void blockRect(float x0, float z0, float x1, float z1) { scene.blockers.push_back({x0, z0, x1, z1}); }

  void floor(float x0, float z0, float x1, float z1, float h) { scene.floors.push_back({x0, z0, x1, z1, h}); }

  void light(Vec3 pos, Vec3 color, float intensity, float range, float radius = 0.1f) {
    scene.lights.push_back({pos, color, intensity, range, radius});
  }
};

Quat turnY(float radians) { return Quat::axisAngle({0, 1, 0}, radians); }

// The circle the counter's customer edge follows: center (x, z) and radius.
Vec3 barArc() {
  const float a = (kBarXB - kBarXA) / 2, sag = kBarZMid - kBarZEnds;
  const float r = (a * a + sag * sag) / (2 * sag);
  return {(kBarXA + kBarXB) / 2, kBarZMid - r, r};
}

// A point `out` meters in front of the counter's customer edge (negative =
// over the counter top), at floor position x along the curve.
Vec3 frontOfBar(float x, float out) {
  const Vec3 arc = barArc();
  const float ang = std::atan2(std::sqrt(arc.z * arc.z - (x - arc.x) * (x - arc.x)), x - arc.x);
  return {arc.x + (arc.z + out) * std::cos(ang), 0, arc.y + (arc.z + out) * std::sin(ang)};
}

// The counter's centerline on the floor (x, z): along the curve, around
// the turn and back to the back wall. Customers are on the right-hand side
// (normal = (-dz, dx) of the walking direction). Matches
// layout.bar_centerline().
std::vector<engine::Vec2> barCenterline(float step) {
  const float a = (kBarXB - kBarXA) / 2, sag = kBarZEnds - kBarZMid;  // sag < 0 (bows toward +z)
  const float r = (a * a + sag * sag) / (2 * -sag);
  const float xc = (kBarXA + kBarXB) / 2, zc = kBarZMid - r, rc = r - kBarInset;
  const auto angle = [&](float x) { return std::atan2(std::sqrt(std::max(r * r - (x - xc) * (x - xc), 0.0f)), x - xc); };
  const float a0 = angle(kBarXA), a1 = angle(kBarXB);
  std::vector<engine::Vec2> pts;
  const int n = std::max(2, static_cast<int>(std::fabs(a0 - a1) * rc / step));
  for (int i = 0; i <= n; ++i) {
    const float t = a0 + (a1 - a0) * static_cast<float>(i) / static_cast<float>(n);
    pts.push_back({xc + rc * std::cos(t), zc + rc * std::sin(t)});
  }
  const engine::Vec2 p1 = pts[pts.size() - 2], p2 = pts.back();
  const float phi1 = std::atan2(p2.y - p1.y, p2.x - p1.x), phiEnd = -engine::kPi / 2;
  const float cx = p2.x + kBarTurnR * std::sin(phi1), cz = p2.y - kBarTurnR * std::cos(phi1);
  const int m = std::max(2, static_cast<int>(std::fabs(phi1 - phiEnd) * kBarTurnR / step));
  for (int i = 1; i <= m; ++i) {
    const float phi = phi1 + (phiEnd - phi1) * static_cast<float>(i) / static_cast<float>(m);
    pts.push_back({cx - kBarTurnR * std::sin(phi), cz + kBarTurnR * std::cos(phi)});
  }
  const engine::Vec2 end = pts.back();
  const int k = std::max(1, static_cast<int>((end.y - kZ0) / step));
  for (int i = 1; i <= k; ++i) pts.push_back({end.x, end.y + (kZ0 - end.y) * static_cast<float>(i) / static_cast<float>(k)});
  return pts;
}

void buildRoom(Builder& b) {
  // The shell (tools/assets/hall.py): parquet, brick and plaster walls, the
  // diagonal corner, the bar soffit, the open ceiling, the bay window, the
  // street door, the stage and the two unfinished rooms.
  b.model("hall.glb", {0, 0, 0}, kBrick);
  const float h = kH, t = kWall;
  b.solid({0, -0.05f, 0}, {kX1 + 1, 0.05f, kZ1 + 2}, kOakFloor);
  b.solid({0, h + 0.05f, 0}, {kX1 + 1, 0.05f, kZ1 + 2}, kPlaster);
  b.solid({0, h / 2, kZ0 - t / 2}, {kX1 + 1, h / 2, t / 2}, kBrick);                 // back wall
  b.solid({kX1 + t / 2, h / 2, 0}, {t / 2, h / 2, kZ1 + 1}, kBrick);                 // right wall
  b.solid({kX0 - t / 2, h / 2, 0}, {t / 2, h / 2, kZ1 + 1}, kPlaster);               // left wall
  b.solid({(kX0 - 1 + kBayX0) / 2, h / 2, kZ1 + t / 2}, {(kBayX0 - kX0 + 1) / 2, h / 2, t / 2}, kPlaster);  // front wall
  b.solid({(kBayX1 + kX1) / 2 + 0.5f, h / 2, kZ1 + t / 2}, {(kX1 - kBayX1) / 2 + 0.5f, h / 2, t / 2}, kPlaster);
  b.solid({(kBayX0 + kBayX1) / 2, h / 2, kZ1 + kBayDepth + 0.1f}, {(kBayX1 - kBayX0) / 2 + 0.2f, h / 2, 0.1f}, kPlaster);
  b.solid({(kBayX0 + kBayX1) / 2, 0.24f, kZ1 + 0.68f}, {(kBayX1 - kBayX0) / 2, 0.24f, 0.29f}, kPlaster);  // window seat
  {  // diagonal wall
    const float dx = kX1 - kChamferAX, dz = kChamferBZ - kZ0, len = std::sqrt(dx * dx + dz * dz);
    const float nx = -dz / len, nz = dx / len;
    b.solid({(kChamferAX + kX1) / 2 - nx * t / 2, h / 2, (kZ0 + kChamferBZ) / 2 - nz * t / 2}, {len / 2 + 0.3f, h / 2, t / 2},
            kBrick, turnY(-std::atan2(dz, dx)));
    // Walking: stair-step rectangles along the diagonal.
    for (int i = 0; i < 6; ++i) {
      const float f0 = static_cast<float>(i) / 6, f1 = static_cast<float>(i + 1) / 6;
      b.blockRect(kChamferAX + dx * f0, kZ0 - 1, kX1 + 1, kZ0 + dz * f1);
    }
  }
  b.solid({kX0 + kChimneyD / 2, h / 2, (kChimneyZ0 + kChimneyZ1) / 2}, {kChimneyD / 2, h / 2, (kChimneyZ1 - kChimneyZ0) / 2},
          kBrick);
  // Walls block walking; the doorways are taped off (see buildConstruction).
  b.blockRect(kX0 - 2, kZ0 - 2, kX1 + 2, kZ0);
  b.blockRect(kX0 - 2, kZ0 - 2, kX0, kZ1 + 2);
  b.blockRect(kX1, kZ0 - 2, kX1 + 2, kZ1 + 2);
  b.blockRect(kX0 - 2, kZ1, kBayX0, kZ1 + 2);
  b.blockRect(kBayX1, kZ1, kX1 + 2, kZ1 + 2);
  b.blockRect(kBayX0 - 1, kZ1 + 0.4f, kBayX1 + 1, kZ1 + 3);  // window seat
  b.blockRect(kX0 - 1, kChimneyZ0, kX0 + kChimneyD, kChimneyZ1);
  // The stage and its step are floors you can walk onto.
  b.floor(kX0, kStageZ0, kStageX1, kZ1, kStageH);
  b.floor(kX0, kStageZ0 - kStepW, kStageX1 + kStepW, kZ1, kStepH);
  b.solid({(kX0 + kStageX1) / 2, kStageH / 2, (kStageZ0 + kZ1) / 2}, {(kStageX1 - kX0) / 2, kStageH / 2, (kZ1 - kStageZ0) / 2},
          kOakFloor);
  b.solid({(kX0 + kStageX1 + kStepW) / 2, kStepH / 2, (kStageZ0 - kStepW + kZ1) / 2},
          {(kStageX1 + kStepW - kX0) / 2, kStepH / 2, (kZ1 - kStageZ0 + kStepW) / 2}, kOakFloor);
}

void buildBar(Builder& b) {
  // The curved counter with its underbar and everything on top
  // (tools/assets/bar_counter_l.py).
  b.model("bar_counter_l.glb", {0, 0, 0}, kVarnishedWood);
  // It follows barCenterline(): a box per short piece for physics
  // (top, drink-rail edge and body) and for walking (body plus foot rail on
  // the customer side).
  const std::vector<engine::Vec2> path = barCenterline(0.5f);
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    const engine::Vec2 a = path[i], c = path[i + 1];
    const float dx = c.x - a.x, dz = c.y - a.y, len = std::sqrt(dx * dx + dz * dz);
    if (len < 1e-4f) continue;
    const float nx = -dz / len, nz = dx / len;  // toward the customers
    const Quat rot = turnY(-std::atan2(dz, dx));
    const Vec3 mid{(a.x + c.x) / 2, 0, (a.y + c.y) / 2};
    b.solid(mid + Vec3{nx * 0.02f, kBarTopY - 0.03f, nz * 0.02f}, {len / 2 + 0.02f, 0.03f, 0.38f}, kVarnishedWood, rot);
    b.solid(mid + Vec3{nx * 0.37f, kBarTopY + 0.03f, nz * 0.37f}, {len / 2 + 0.02f, 0.03f, 0.045f}, kLeather, rot);
    b.solid(mid + Vec3{0, 0.5f, 0}, {len / 2 + 0.02f, 0.5f, 0.27f}, kDarkWood, rot);
    float x0 = 1e9f, z0 = 1e9f, x1 = -1e9f, z1 = -1e9f;
    for (engine::Vec2 p : {a, c}) {
      for (float s : {-0.3f, 0.45f}) {
        x0 = std::min(x0, p.x + nx * s);
        x1 = std::max(x1, p.x + nx * s);
        z0 = std::min(z0, p.y + nz * s);
        z1 = std::max(z1, p.y + nz * s);
      }
    }
    b.blockRect(x0, z0, x1, z1);
  }

  // Glasses on the counter to pick up and throw.
  for (float x : {-5.7f, -3.7f, -1.7f, 0.05f}) {
    b.model("pint_glass.glb", frontOfBar(x, -0.2f) + Vec3{0, kBarTopY, 0}, kBottleGlass, {}, Prop::kPintGlass);
  }
  for (float x : {-4.9f, -2.6f}) {
    b.model("rocks_glass.glb", frontOfBar(x, -0.25f) + Vec3{0, kBarTopY, 0}, kBottleGlass, {}, Prop::kRocksGlass);
  }

  // Bar stools along the curve.
  for (int i = 0; i < 8; ++i) {
    const Vec3 p = frontOfBar(kBarXA + 0.7f + static_cast<float>(i) * 1.05f, 0.5f);
    b.model("bar_stool.glb", p, kLeather, turnY(0.4f * static_cast<float>(i)));
    b.solid({p.x, 0.77f, p.z}, {0.2f, 0.05f, 0.2f}, kLeather);
    b.solid({p.x, 0.37f, p.z}, {0.03f, 0.33f, 0.03f}, kChrome);
    b.solid({p.x, 0.023f, p.z}, {0.2f, 0.023f, 0.2f}, kChrome);
  }

  // Pendant lamps hang from the soffit over the counter.
  for (float x : {-6.6f, -4.3f, -2.0f, 0.2f}) {
    b.model("pendant_lamp.glb", {x, kSoffitY - 0.06f, -4.6f}, kBlackSteel);
    b.light({x, kSoffitY - 0.85f, -4.6f}, kWarmBulb, 2.0f, 5.0f, 0.08f);
  }
  // The back bar, floor to soffit (tools/assets/back_bar_tall.py):
  // cabinets, lit shelves, the onyx bay, the ladder.
  const float bx0 = -8.8f, bx1 = 0.9f;
  b.model("back_bar_tall.glb", {0, 0, 0}, kDarkWood);
  b.solid({(bx0 + bx1) / 2, 0.495f, kZ0 + 0.29f}, {(bx1 - bx0) / 2 + 0.03f, 0.495f, 0.3f}, kDarkWood);
  for (float y : {1.32f, 1.68f, 2.04f, 2.40f, 2.76f}) {
    b.solid({(bx0 + bx1) / 2, y, kZ0 + 0.16f}, {(bx1 - bx0) / 2, 0.0175f, 0.15f}, kDarkWood);
  }
  b.blockRect(bx0 - 0.1f, kZ0 - 1, bx1 + 0.05f, kZ0 + 0.6f);
  b.blockRect(-6.45f, kZ0 + 0.6f, -5.85f, kZ0 + 1.2f);  // the ladder's feet
  // Glow of the back bar's shelves (two halves).
  b.light({-6.3f, 1.9f, kZ0 + 1.2f}, kWarmBulb, 0.8f, 3.8f, 0.6f);
  b.light({-1.8f, 1.9f, kZ0 + 1.2f}, kWarmBulb, 0.8f, 3.8f, 0.6f);
  // The onyx centerpiece glows amber.
  b.light({-3.95f, 2.0f, kZ0 + 0.6f}, {1.0f, 0.55f, 0.2f}, 0.9f, 3.5f, 0.8f);
}

void buildHall(Builder& b) {
  // High-top tables in the middle of the hall, each with a candle and a pair
  // of stools.
  const Vec3 tables[] = {{-3.0f, 0, -0.6f}, {-0.2f, 0, -1.4f}, {2.4f, 0, -0.4f}, {-1.6f, 0, 2.0f}, {1.4f, 0, 2.4f},
                         {4.2f, 0, 1.6f}};
  for (size_t i = 0; i < std::size(tables); ++i) {
    const Vec3 c = tables[i];
    const float topY = 0.785f;
    b.model("round_table.glb", c, kVarnishedWood, turnY(0.7f * static_cast<float>(i)));
    b.solid({c.x, topY - 0.0225f, c.z}, {0.45f, 0.0225f, 0.45f}, kVarnishedWood);
    b.solid({c.x, 0.37f, c.z}, {0.05f, 0.37f, 0.05f}, kBlackSteel);
    b.solid({c.x, 0.025f, c.z}, {0.25f, 0.025f, 0.25f}, kBlackSteel);
    b.block(c, 0.5f, 0.5f);
    b.model("candle.glb", {c.x, topY, c.z}, kBottleGlass);
    b.solid({c.x, topY + 0.04f, c.z}, {0.037f, 0.04f, 0.037f}, kBottleGlass);
    if (i % 2 == 0) b.model("rocks_glass.glb", {c.x + 0.22f, topY, c.z + 0.12f}, kBottleGlass, {}, Prop::kRocksGlass);
    for (float s : {-1.0f, 1.0f}) {
      const float ang = 0.9f * static_cast<float>(i) + (s > 0 ? 0 : engine::kPi);
      const Vec3 p{c.x + 0.78f * std::cos(ang), 0, c.z + 0.78f * std::sin(ang)};
      b.model("table_stool.glb", p, kLeather, turnY(ang));
      b.solid({p.x, 0.5f, p.z}, {0.18f, 0.04f, 0.18f}, kLeather);
      b.solid({p.x, 0.23f, p.z}, {0.13f, 0.22f, 0.13f}, kBlackSteel);
    }
  }
  // Warm light over the middle (string lights come with the decor pass).
  b.light({-1.5f, 3.3f, 0.2f}, kWarmBulb, 1.6f, 6.5f, 0.6f);
  b.light({2.6f, 3.3f, 1.6f}, kWarmBulb, 1.4f, 6.0f, 0.6f);
  // Street light through the bay window and the door.
  b.light({-0.2f, 1.8f, kZ1 + 0.6f}, {0.55f, 0.6f, 0.9f}, 0.6f, 4.0f, 1.0f);
}

void buildLounge(Builder& b) {
  // Fire glow in front of the chimney breast (the fireplace model comes with
  // the lounge pass), and a reading lamp.
  b.light({kX0 + 0.9f, 0.6f, (kChimneyZ0 + kChimneyZ1) / 2}, kFire, 1.6f, 5.0f, 0.4f);
  b.light({-6.0f, 1.6f, -1.2f}, kWarmBulb, 1.0f, 4.0f, 0.2f);
  // Stage: a warm spot on the piano.
  b.light({-7.0f, 3.5f, 5.2f}, kWarmBulb, 2.0f, 5.0f, 0.15f);
}

void buildGames(Builder& b) {
  // Darts on the front wall, right of the street door: the board, the score
  // chalkboard, a spotlight.
  const float dartX = 6.0f;
  b.model("dartboard.glb", {dartX, 1.73f, kZ1}, kDarkWood, turnY(engine::kPi));
  b.solid({dartX, 1.73f, kZ1 - 0.04f}, {0.66f, 0.32f, 0.04f}, kDarkWood);
  b.model("chalkboard.glb", {dartX + 1.7f, 1.6f, kZ1}, kDarkWood, turnY(engine::kPi));
  b.light({dartX, 3.0f, kZ1 - 1.4f}, kWarmBulb, 1.3f, 4.0f, 0.15f);
  // Racing sims (right wall) and the rhythm corner (diagonal wall): screen
  // glow until their models land.
  b.light({8.0f, 1.3f, -1.6f}, {0.4f, 0.6f, 1.0f}, 0.8f, 3.5f, 0.5f);
  b.light({7.0f, 1.9f, -5.4f}, {0.8f, 0.3f, 1.0f}, 1.0f, 4.0f, 0.5f);
  // Jukebox against the front wall, between the stage and the bay window.
  const Vec3 j{-3.8f, 0, kZ1 - 0.45f};
  const size_t jukebox = b.model("jukebox.glb", j, kLacquer, turnY(engine::kPi));
  b.solid({j.x, 0.73f, j.z + 0.015f}, {0.45f, 0.73f, 0.27f}, kLacquer);
  b.block(j, 0.5f, 0.35f);
  b.light({j.x, 1.2f, j.z - 0.7f}, {1.0f, 0.45f, 0.25f}, 0.9f, 3.0f, 0.3f);
  Interactable it{"JUKEBOX", "Jukebox", {j.x, 2.1f, j.z}, {j.x - 0.5f, 0, j.z - 0.35f}, {j.x + 0.5f, 1.9f, j.z + 0.35f}};
  it.firstModel = jukebox;
  it.modelCount = 1;
  it.action = Action::kJukebox;
  b.scene.interactables.push_back(it);
  b.scene.jukeboxSpeaker = {j.x, 0.6f, j.z - 0.3f};
}

// The giant 2-in-1 mirror on the right wall, from just above the floor to
// near the ceiling. Tap it to step through full -> avatars only -> off.
void buildMirror(Builder& b) {
  const float zc = 3.6f, halfW = 2.2f, y0 = 0.14f, y1 = 3.62f;
  Mirror m{{kX1 - 0.03f, (y0 + y1) / 2, zc}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, halfW, (y1 - y0) / 2,
           MirrorMode::kFull};
  b.scene.mirrors.push_back(m);
  Interactable it;
  it.name = "MIRROR";
  it.tagPosition = {kX1 - 0.4f, 2.4f, zc};
  it.boundsMin = {kX1 - 0.15f, y0, zc - halfW};
  it.boundsMax = {kX1, y1, zc + halfW};
  it.action = Action::kMirror;
  it.mirror = 0;
  b.scene.interactables.push_back(it);
  b.light({kX1 - 2.2f, 3.3f, zc}, kWarmBulb, 1.2f, 5.0f, 0.4f);
}

}  // namespace

std::vector<engine::MaterialImages> buildBarTextures() {
  constexpr int kSize = 512;
  std::vector<engine::MaterialImages> m;
  m.push_back(engine::makeWoodPlanks(kSize, 11));  // kTexWoodPlanks
  m.push_back(engine::makeWoodGrain(kSize, 23));   // kTexWoodGrain
  m.push_back(engine::makeBrick(kSize, 37));       // kTexBrick
  m.push_back(engine::makeLeather(kSize, 41));     // kTexLeather
  m.push_back(engine::makePlaster(kSize, 53));     // kTexPlaster
  m.push_back(engine::makeFabric(kSize, 61));      // kTexFabric
  m.push_back(engine::makeDenim(kSize, 67));       // kTexDenim
  return m;
}

BarScene buildBarScene() {
  BarScene scene;
  scene.ambient = {0.07f, 0.05f, 0.045f};
  scene.fogColor = {0.025f, 0.015f, 0.015f};
  scene.fogDensity = 0.012f;
  scene.spawnPosition = {0.4f, 1.65f, 5.2f};  // just inside the street door
  scene.spawnYaw = 0;
  scene.lightGridMin = {kX0 - kWall - 3.2f, 0, kZ0 - kWall - 3.2f};
  scene.lightGridMax = {kX1 + 0.5f, 0, kZ1 + kBayDepth + 0.5f};

  Builder b{scene};
  buildRoom(b);
  buildBar(b);
  buildHall(b);
  buildLounge(b);
  buildGames(b);
  buildMirror(b);
  return scene;
}

float groundHeight(const BarScene& scene, Vec3 p) {
  float h = 0;
  for (const FloorArea& f : scene.floors) {
    if (p.x >= f.minX && p.x <= f.maxX && p.z >= f.minZ && p.z <= f.maxZ) h = std::max(h, f.height);
  }
  return h;
}

Vec3 resolveCollisions(const BarScene& scene, Vec3 p, float radius) {
  for (int pass = 0; pass < 2; ++pass) {
    for (const Blocker& bl : scene.blockers) {
      const float cx = std::clamp(p.x, bl.minX, bl.maxX);
      const float cz = std::clamp(p.z, bl.minZ, bl.maxZ);
      const float dx = p.x - cx, dz = p.z - cz;
      const float dist2 = dx * dx + dz * dz;
      if (dist2 >= radius * radius) continue;
      if (dist2 > 1e-8f) {
        const float dist = std::sqrt(dist2);
        p.x = cx + dx / dist * radius;
        p.z = cz + dz / dist * radius;
      } else {
        // Center is inside the blocker: push out along the shortest way.
        const float pushes[4] = {p.x - bl.minX, bl.maxX - p.x, p.z - bl.minZ, bl.maxZ - p.z};
        const int side = static_cast<int>(std::min_element(pushes, pushes + 4) - pushes);
        if (side == 0) p.x = bl.minX - radius;
        if (side == 1) p.x = bl.maxX + radius;
        if (side == 2) p.z = bl.minZ - radius;
        if (side == 3) p.z = bl.maxZ + radius;
      }
    }
  }
  return p;
}

}  // namespace app
