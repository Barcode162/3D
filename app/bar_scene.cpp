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
// The arcade behind doorway A (tools/assets/arcade.py, layout.py ARCADE_*).
constexpr float kArcX0 = 0.4f, kArcX1 = 8.4f, kArcZ1 = -7.7f, kArcZ0 = kArcZ1 - 8.0f, kArcH = 3.4f;
constexpr float kStageX1 = -5.4f, kStageZ0 = 3.8f, kStageH = 0.3f, kStepW = 0.35f, kStepH = 0.15f;
constexpr float kChimneyZ0 = -0.2f, kChimneyZ1 = 1.8f, kChimneyD = 0.45f;
// The bar counter's customer edge bows along a big circle from x = kBarXA
// to kBarXB, then the counter turns and runs back to the back wall.
constexpr float kBarXA = -7.6f, kBarXB = 1.1f, kBarZEnds = -4.75f, kBarZMid = -4.0f;
constexpr float kBarInset = 0.36f, kBarTurnR = 0.55f;
constexpr float kBarTopY = 1.08f;  // where glasses stand on the counter

// Light and glow colors.
constexpr Vec3 kWarmBulb{1.0f, 0.68f, 0.38f};
constexpr Vec3 kNeonPink{1.0f, 0.18f, 0.55f};
constexpr Vec3 kFire{1.0f, 0.45f, 0.15f};

struct Builder {
  BarScene& scene;
  int room = -1;  // models placed now belong to this room (streamed in when near)

  // Places a mesh made of material `m`.
  SceneObject& add(engine::MeshData mesh, Vec3 pos, MaterialId m, Quat rot = {}) {
    const engine::Material& mat = material(m);
    scene.objects.push_back({std::move(mesh), pos, rot, m, mat.color, mat.emissive, mat.textureScale});
    return scene.objects.back();
  }

  // Places a .glb model (assets/models/<file>); returns its index.
  size_t model(const char* file, Vec3 pos, MaterialId m, Quat rot = {}, Prop prop = Prop::kNone) {
    scene.models.push_back({file, pos, rot, 1, m, prop, room});
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

  // A seat: hip point, facing (a direction on the floor), how low it is.
  Seat& seat(const char* name, Vec3 hip, Vec3 facing, float low, SeatKind kind = SeatKind::kStool, int model = -1) {
    Seat st;
    st.name = name;
    st.hip = hip;
    st.yaw = std::atan2(facing.x, -facing.z);
    st.lowSeat = low;
    st.kind = kind;
    st.model = model;
    st.boundsMin = {hip.x - 0.28f, 0, hip.z - 0.28f};
    st.boundsMax = {hip.x + 0.28f, hip.y + 0.12f, hip.z + 0.28f};
    scene.seats.push_back(st);
    return scene.seats.back();
  }

  // A small thing to use by looking at it or tapping it.
  Interactable& thing(const char* prompt, Vec3 center, Vec3 half, Action action, int value = 0) {
    Interactable it;
    it.prompt = prompt;
    it.tagPosition = center;
    it.boundsMin = center - half;
    it.boundsMax = center + half;
    it.action = action;
    it.value = value;
    it.direct = true;
    scene.interactables.push_back(it);
    return scene.interactables.back();
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
  // Walls block walking; doorway A leads to the arcade (buildArcade), B is
  // taped off (buildConstruction).
  b.blockRect(kX0 - 2, kArcZ1, kDoorAX0, kZ0);
  b.blockRect(kDoorAX1, kArcZ1, kX1 + 2, kZ0);
  b.blockRect(kX0 - 2, kZ0 - 2, kArcX0, kArcZ1);
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
    const size_t stool = b.model("bar_stool.glb", p, kLeather, turnY(0.4f * static_cast<float>(i)));
    const float x = kBarXA + 0.7f + static_cast<float>(i) * 1.05f;
    b.seat("Bar stool", p + Vec3{0, 0.9f, 0}, frontOfBar(x, -0.3f) - p, 0, SeatKind::kStool, static_cast<int>(stool));
    b.solid({p.x, 0.77f, p.z}, {0.2f, 0.05f, 0.2f}, kLeather);
    b.solid({p.x, 0.37f, p.z}, {0.03f, 0.33f, 0.03f}, kChrome);
    b.solid({p.x, 0.023f, p.z}, {0.2f, 0.023f, 0.2f}, kChrome);
  }

  // Drink spots on the counter (where the bartender serves), and the
  // bartender's station behind the bar.
  for (int i = 0; i < 8; ++i) {
    const float x = kBarXA + 0.7f + static_cast<float>(i) * 1.05f;
    b.scene.barTopFront[i] = frontOfBar(x, -0.12f) + Vec3{0, kBarTopY, 0};
    b.thing("Drink", b.scene.barTopFront[i] + Vec3{0, 0.06f, 0}, {0.09f, 0.09f, 0.09f}, Action::kDrinkShelf, i);
  }
  {
    const Vec3 inside = frontOfBar(-2.6f, -1.25f);  // behind the bar, clear of the tap tower
    b.scene.bartenderSpot = inside + Vec3{0, 1.65f, 0};
    Interactable& st = b.thing("Bartender station\nTap: start a shift", inside + Vec3{0, 1.0f, 0}, {0.6f, 0.5f, 0.4f},
                               Action::kBartender);
    st.name = "BARTENDER";
    st.tagPosition = inside + Vec3{0, 2.1f, 0};
    st.direct = false;
    b.thing("Service bell\nTap: ring it", frontOfBar(-2.2f, -0.25f) + Vec3{0, kBarTopY + 0.04f, 0}, {0.12f, 0.1f, 0.12f},
            Action::kBell);
    b.thing("Tip jar\nTap: leave a tip", frontOfBar(-1.0f, -0.3f) + Vec3{0, kBarTopY + 0.08f, 0}, {0.12f, 0.12f, 0.12f},
            Action::kTipJar);
    b.thing("Beer taps\nTap: pour a beer", frontOfBar(-4.6f, -0.55f) + Vec3{0, kBarTopY + 0.3f, 0},
            {0.35f, 0.3f, 0.2f}, Action::kTaps);
    b.thing("Coffee machine\nTap: make a coffee", Vec3{-1.2f, 1.25f, kZ0 + 0.35f}, {0.3f, 0.25f, 0.3f}, Action::kCoffee);
  }

  // Pendant lamps hang from the soffit over the counter.
  for (float x : {-6.6f, -4.3f, -2.0f, 0.2f}) {
    b.model("pendant_lamp.glb", {x, kSoffitY - 0.06f, -4.6f}, kBlackSteel);
    b.scene.roomLights.push_back(static_cast<int>(b.scene.lights.size()));
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
  b.blockRect(0.1f, kZ0 + 0.6f, 0.7f, kZ0 + 0.95f);  // the ladder's feet (parked at the right end)
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
      const size_t stool = b.model("table_stool.glb", p, kLeather, turnY(ang));
      b.seat("Stool", p + Vec3{0, 0.62f, 0}, c - p, 0.1f, SeatKind::kStool, static_cast<int>(stool));
      b.solid({p.x, 0.5f, p.z}, {0.18f, 0.04f, 0.18f}, kLeather);
      b.solid({p.x, 0.23f, p.z}, {0.13f, 0.22f, 0.13f}, kBlackSteel);
    }
  }
  // The decor (tools/assets/decor_walls.py): string lights, neon signs,
  // frames, the collector wall, the clock and TV, plants, speakers,
  // sconces, coats, crates and a keg, the bay window table.
  b.model("decor_walls.glb", {0, 0, 0}, kDarkWood);
  b.blockRect(4.7f, kZ0, 5.95f, kZ0 + 1.0f);  // crates and the keg
  b.block({1.95f, 0, kZ1 - 1.25f}, 0.3f, 0.3f);  // chalkboard A-frame
  for (Vec3 p : {Vec3{-1.85f, 0, kZ1 - 0.3f}, Vec3{5.35f, 0, kZ1 - 0.35f}, Vec3{4.6f, 0, kZ1 - 0.3f}, Vec3{8.55f, 0, 6.6f}}) b.block(p, 0.3f, 0.3f);
  // The string lights' warm glow over the middle of the hall.
  for (int k = 0; k < 3; ++k) b.scene.roomLights.push_back(static_cast<int>(b.scene.lights.size()) + k);
  b.light({-1.8f, 3.2f, 0.4f}, kWarmBulb, 1.7f, 6.5f, 0.8f);
  b.light({2.8f, 3.2f, 2.2f}, kWarmBulb, 1.6f, 6.5f, 0.8f);
  b.light({-3.0f, 3.2f, 4.6f}, kWarmBulb, 1.2f, 5.0f, 0.8f);
  // The window seat in the bay, three places looking into the room.
  for (float x : {-1.4f, -0.2f, 1.0f}) b.seat("Window seat", {x, 0.56f, kZ1 + 0.5f}, {0, 0, -1}, 0.6f, SeatKind::kChair);
  // The light switch by the street door, the door itself, the TV.
  b.thing("Light switch\nTap: dim the lights", {kEntryX0 - 0.25f, 1.3f, kZ1 - 0.05f}, {0.08f, 0.1f, 0.08f},
          Action::kLightSwitch);
  b.thing("Street door\nTap: knock", {(kEntryX0 + kEntryX1) / 2, 1.1f, kZ1 - 0.05f}, {0.7f, 1.1f, 0.12f}, Action::kDoor);
  // Street light through the bay window and the door.
  b.light({-0.2f, 1.8f, kZ1 + 0.6f}, {0.55f, 0.6f, 0.9f}, 0.6f, 4.0f, 1.0f);
}

void buildLounge(Builder& b) {
  // Fire glow in front of the chimney breast (the fireplace model comes with
  // the lounge pass), and a reading lamp.
  // The lounge (tools/assets/lounge.py): fireplace, Chesterfield sofa, two
  // wingback armchairs, coffee table with a chess game, rug, bookshelf,
  // floor lamp. The flames are see-through glowing cards.
  const float fz = (kChimneyZ0 + kChimneyZ1) / 2;
  b.model("lounge.glb", {0, 0, 0}, kLeather);
  b.model("fire_flames.glb", {0, 0, 0}, kGlowGlass);
  b.blockRect(kX0, fz - 1.15f, -8.05f, fz + 1.15f);               // hearth
  // Walking blockers a bit smaller than the furniture (you stand with your
  // shins at the sofa's seat and the coffee table's top under your hands),
  // so you fit between the sofa and the coffee table (walk_test).
  b.blockRect(-5.95f, fz - 1.2f, -5.25f, fz + 1.2f);              // sofa
  b.solid({-5.75f, 0.27f, fz}, {0.45f, 0.27f, 1.2f}, kLeather);
  b.solid({-5.4f, 0.45f, fz}, {0.1f, 0.33f, 1.2f}, kLeather);
  b.blockRect(-7.25f, fz - 0.5f, -6.85f, fz + 0.5f);              // coffee table
  b.solid({-6.95f, 0.43f, fz}, {0.38f, 0.02f, 0.6f}, kDarkWood);
  for (float dz : {-1.3f, 1.3f}) {
    b.block({-7.25f, 0, fz + dz}, 0.45f, 0.45f);                   // armchairs
    b.solid({-7.25f, 0.3f, fz + dz}, {0.4f, 0.3f, 0.4f}, kLeather);
  }
  b.blockRect(kX0, kChimneyZ1 + 0.1f, kX0 + 0.36f, kStageZ0 - kStepW - 0.1f);  // bookshelf
  b.block({-6.3f, 0, fz + 1.95f}, 0.2f, 0.2f);                   // floor lamp
  b.block({-5.85f, 0, fz + 1.85f}, 0.26f, 0.26f);                // side table
  b.solid({-5.85f, 0.55f, fz + 1.85f}, {0.24f, 0.015f, 0.24f}, kDarkWood);
  for (float dz : {-0.7f, 0.0f, 0.7f}) b.seat("Sofa", {-5.85f, 0.55f, fz + dz}, {-1, 0, 0}, 1.0f, SeatKind::kSofa);
  b.seat("Armchair", {-7.25f, 0.55f, fz - 1.3f}, {0, 0, 1}, 0.8f, SeatKind::kChair);
  b.seat("Armchair", {-7.25f, 0.55f, fz + 1.3f}, {0, 0, -1}, 0.8f, SeatKind::kChair);
  b.thing("Fireplace\nTap: fire on/off", {kX0 + 0.5f, 0.6f, fz}, {0.4f, 0.5f, 0.8f}, Action::kFireplace);
  b.thing("Bookshelf\nTap: read a book", {kX0 + 0.2f, 1.2f, (kChimneyZ1 + kStageZ0 - kStepW) / 2}, {0.25f, 1.0f, 0.6f},
          Action::kBooks);
  b.thing("Chess\nTap: play a game", {-6.95f, 0.5f, fz}, {0.3f, 0.12f, 0.3f}, Action::kChess);
  // The chess pieces (tools/assets/chess.py), one model each, plus two spare
  // queens a side for promotions (under the table until needed). The board is
  // 0.44 m on the coffee table, rank 1 toward +z (tools/assets/lounge.py).
  {
    b.scene.chessCenter = {-6.95f, 0.45f + 0.0205f, fz};
    const char* names[7] = {"", "pawn", "knight", "bishop", "rook", "queen", "king"};
    const int backRow[8] = {4, 2, 3, 5, 6, 3, 2, 4};
    const auto add = [&](int kind, int color, int sq) {
      const Vec3 c = b.scene.chessCenter;
      const Vec3 p = sq < 0 ? c + Vec3{0, -0.6f, 0}
                            : c + Vec3{(static_cast<float>(sq % 8) - 3.5f) * 0.055f, 0, (3.5f - static_cast<float>(sq / 8)) * 0.055f};
      const std::string file = std::string("chess_") + names[kind] + (color ? "_b.glb" : "_w.glb");
      b.scene.chessPieces.push_back({b.model(file.c_str(), p, kDarkWood), static_cast<int8_t>(kind | (color ? 8 : 0))});
    };
    for (int f = 0; f < 8; ++f) {
      add(backRow[f], 0, f);
      add(1, 0, 8 + f);
      add(1, 1, 48 + f);
      add(backRow[f], 1, 56 + f);
    }
    for (int k = 0; k < 2; ++k) {
      add(5, 0, -1);
      add(5, 1, -1);
    }
  }
  b.scene.fireLight = static_cast<int>(b.scene.lights.size());
  b.light({kX0 + 0.9f, 0.6f, fz}, kFire, 1.8f, 5.0f, 0.4f);
  b.light({-6.3f, 1.5f, fz + 1.95f}, kWarmBulb, 1.1f, 4.0f, 0.2f);
  // The grand piano on the stage (tools/assets/piano.py), angled so its
  // open lid faces the room; the pianist faces the hall.
  const float pianoYaw = engine::kPi / 2 + 0.26f;
  const Vec3 piano{-8.0f, kStageH, 5.6f};
  const Quat pr = turnY(pianoYaw);
  b.model("piano.glb", piano, kLacquer, pr);
  b.scene.pianoPosition = piano;
  b.scene.pianoYaw = pianoYaw;
  b.seat("Piano bench", piano + pr.rotate({0, 0.6f, -0.62f}), pr.rotate({0, 0, 1}), 0.2f, SeatKind::kPianoBench);
  {
    Interactable& songs = b.thing("Sheet music\nTap: pick a song", piano + pr.rotate({0, 1.02f, 0.12f}),
                                  {0.25f, 0.15f, 0.25f}, Action::kPianoSongs);
    songs.name = "PIANO";
    songs.tagPosition = piano + Vec3{0, 1.9f, 0};
  }
  b.solid(piano + pr.rotate({0, 0.8f, 1.05f}), {0.8f, 0.18f, 1.1f}, kLacquer, pr);   // case
  b.solid(piano + pr.rotate({0, 0.5f, -0.62f}), {0.38f, 0.04f, 0.18f}, kLacquer, pr);  // bench
  b.blockRect(-8.25f, 4.25f, -5.75f, 6.55f);
  b.block(piano + pr.rotate({0, 0, -0.62f}), 0.25f, 0.4f);
  // Mic, guitar, amp, stool, the lighting bar (tools/assets/stage.py).
  b.model("stage.glb", {0, 0, 0}, kBlackSteel);
  b.block({-6.25f, 0, 6.3f}, 0.25f, 0.25f);   // mic stand
  b.block({-5.72f, 0, 6.62f}, 0.25f, 0.3f);   // guitar
  b.block({-6.95f, 0, kZ1 - 0.22f}, 0.32f, 0.2f);  // amp
  b.block({-6.8f, 0, 6.35f}, 0.22f, 0.22f);   // stool
  b.solid({-6.8f, kStageH + 0.74f, 6.35f}, {0.17f, 0.025f, 0.17f}, kLeather);
  b.seat("Stage stool", {-6.8f, kStageH + 0.84f, 6.35f}, {1, 0, -1}, 0, SeatKind::kStool);
  b.thing("Guitar\nTap: strum", {-5.72f, 0.9f, 6.62f}, {0.25f, 0.6f, 0.25f}, Action::kGuitar);
  b.thing("Microphone\nTap: speak to the whole bar", {-6.25f, 1.7f, 6.3f}, {0.12f, 0.2f, 0.12f}, Action::kMic);
  b.solid({-6.95f, kStageH + 0.24f, kZ1 - 0.22f}, {0.3f, 0.24f, 0.13f}, kLeather);
  // Stage light: a warm spot on the piano.
  b.light({-6.4f, 3.4f, 4.6f}, kWarmBulb, 3.2f, 5.5f, 0.15f);
  b.light({-7.8f, 3.2f, 6.4f}, {1.0f, 0.8f, 0.6f}, 1.6f, 4.0f, 0.15f);
}

void buildGames(Builder& b) {
  // Darts on the front wall, right of the street door: the board, the score
  // chalkboard, a spotlight.
  const float dartX = 6.0f;
  b.model("dartboard.glb", {dartX, 1.73f, kZ1}, kDarkWood, turnY(engine::kPi));
  b.solid({dartX, 1.73f, kZ1 - 0.04f}, {0.66f, 0.32f, 0.04f}, kDarkWood);
  b.model("chalkboard.glb", {dartX + 1.7f, 1.6f, kZ1}, kDarkWood, turnY(engine::kPi));
  // Darts in the board, the oche at 2.37 m, the mat, spotlights, neon
  // (tools/assets/darts_zone.py), and a high table for the players' drinks.
  b.model("darts_zone.glb", {0, 0, 0}, kDarkWood);
  b.scene.dartBoard = {dartX, 1.73f, kZ1 - 0.0402f};
  b.scene.dartOche = {dartX, 1.65f, kZ1 - 0.04f - 2.37f - 0.15f};
  {
    Interactable& darts = b.thing("Darts\nTap: play darts", b.scene.dartOche - Vec3{0, 0.9f, 0}, {0.6f, 0.9f, 0.3f},
                                  Action::kDarts);
    darts.name = "DARTS";
    darts.tagPosition = {dartX, 2.35f, kZ1 - 0.3f};
    darts.direct = false;
  }
  b.light({dartX, 3.0f, kZ1 - 1.4f}, kWarmBulb, 1.5f, 4.0f, 0.15f);
  const Vec3 dt{dartX - 1.35f, 0, kZ1 - 2.0f};
  b.model("round_table.glb", dt, kVarnishedWood);
  b.solid({dt.x, 0.7625f, dt.z}, {0.45f, 0.0225f, 0.45f}, kVarnishedWood);
  b.block(dt, 0.5f, 0.5f);
  b.model("pint_glass.glb", dt + Vec3{0.15f, 0.785f, 0.1f}, kBottleGlass, {}, Prop::kPintGlass);
  // Two racing sim rigs facing the right wall (tools/assets/sim_rig.py).
  for (float z : {-3.0f, -1.0f}) {
    const Vec3 c{7.75f, 0, z};
    const Quat r = turnY(-engine::kPi / 2);  // the driver faces +x
    b.model("sim_rig.glb", c, kBlackSteel, r);
    const int rig = z < -2.0f ? 0 : 1;
    b.scene.simPosition[rig] = c;
    Seat& st = b.seat("Racing sim", c + Vec3{-0.2f, 0.4f, 0}, {1, 0, 0}, 1.0f, SeatKind::kSim);
    st.machine = rig;
    b.solid(c + Vec3{-0.3f, 0.35f, 0}, {0.35f, 0.35f, 0.3f}, kLeather);           // seat
    b.solid(c + Vec3{0.45f, 0.42f, 0}, {0.12f, 0.42f, 0.32f}, kBlackSteel);       // wheel deck
    b.solid(c + Vec3{1.15f, 1.08f, 0}, {0.06f, 0.24f, 0.95f}, kBlackSteel);       // screens
    b.blockRect(c.x - 0.6f, z - 0.5f, kX1, z + 0.5f);
    b.blockRect(c.x + 0.9f, z - 0.98f, kX1, z + 0.98f);
  }
  b.light({8.3f, 1.3f, -2.0f}, {0.5f, 0.6f, 1.0f}, 0.9f, 3.5f, 0.5f);
  // The rhythm corner on the diagonal wall (tools/assets/rhythm.py): the
  // 4-key cabinet, neon, speaker stacks, a beanbag, a high table.
  {
    const Vec3 mid{(kChamferAX + kX1) / 2, 0, (kZ0 + kChamferBZ) / 2};
    const Quat r = turnY(-engine::kPi / 4);  // local +z points into the room
    const auto at = [&](float x, float z) { return mid + r.rotate({x, 0, z}); };
    const size_t cabinet = b.model("rhythm.glb", mid, kLacquer, r);
    Interactable game{"4K MANIA", "4K Mania\nTap to play", at(0, 0.9f) + Vec3{0, 2.75f, 0}};
    const Vec3 c = at(0, 0.45f);
    game.boundsMin = c + Vec3{-0.6f, 0, -0.6f};
    game.boundsMax = c + Vec3{0.6f, 2.5f, 0.6f};
    game.firstModel = cabinet;
    game.modelCount = 1;
    game.action = Action::kRhythm;
    b.scene.interactables.push_back(game);
    b.solid(at(0, 0.45f) + Vec3{0, 1.1f, 0}, {0.45f, 1.1f, 0.45f}, kLacquer, r);
    b.block(at(0, 0.5f), 0.62f, 0.62f);
    for (float x : {-1.45f, 1.45f}) {
      b.solid(at(x, 0.28f) + Vec3{0, 0.58f, 0}, {0.28f, 0.58f, 0.23f}, kLacquer, r);
      b.block(at(x, 0.28f), 0.38f, 0.38f);
    }
    b.block(at(1.15f, 1.35f), 0.4f, 0.4f);   // beanbag
    b.seat("Beanbag", at(1.15f, 1.35f) + Vec3{0, 0.35f, 0}, r.rotate({0, 0, 1}), 1.0f, SeatKind::kSofa);
    {
      // The cabinet's screen (tools/assets/rhythm.py): a quad on the upper
      // housing, which is raised 1.02 m, pushed 0.12 m forward and tilted back.
      const Quat tilt = Quat::axisAngle({1, 0, 0}, -0.12f);
      const float sw = 0.28f, y0 = 0.12f, y1 = 1.10f, zf = 0.425f;
      const Vec3 local[4] = {{-sw, y0, zf}, {sw, y0, zf}, {sw, y1, zf}, {-sw, y1, zf}};
      for (int k = 0; k < 4; ++k) b.scene.rhythmScreen[k] = mid + r.rotate(Vec3{0, 1.02f, 0.12f} + tilt.rotate(local[k]));
    }
    b.block(at(-1.2f, 1.1f), 0.28f, 0.28f);  // high table
    b.solid(at(-1.2f, 1.1f) + Vec3{0, 1.05f, 0}, {0.25f, 0.015f, 0.25f}, kVarnishedWood);
    b.light(at(0, 1.3f) + Vec3{0, 1.7f, 0}, {0.85f, 0.4f, 1.0f}, 1.2f, 4.0f, 0.5f);
    b.light(at(0, 0.6f) + Vec3{0, 3.0f, 0}, {1.0f, 0.3f, 0.7f}, 0.8f, 3.0f, 0.4f);
  }
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

// The two doorways to the rooms still being built: taped off, with
// barricades, cones, a sign, and a work light inside
// (tools/assets/construction.py).
void buildConstruction(Builder& b) {
  b.model("construction.glb", {0, 0, 0}, kDoorWood);
  const float ax = (kDoorAX0 + kDoorAX1) / 2, bz = (kDoorBZ0 + kDoorBZ1) / 2;
  (void)ax;  // (doorway A is the arcade: buildArcade)
  // Barricades in front of room 3 (stopping 0.4 m short of the door on the
  // bar side, so the gap at the counter's end stays open: the way behind the bar).
  b.blockRect(kX0 - 1, kDoorBZ0 - 0.15f, kX0 + 1.15f, kDoorBZ1 + 1.3f);
  b.solid({kX0 + 0.7f, 0.5f, bz - 0.1f}, {0.15f, 0.5f, 1.1f}, kDoorWood);
  // A work light inside the unfinished room.
  b.light({kX0 - 1.8f, 1.8f, bz - 0.6f}, {1.0f, 0.92f, 0.75f}, 1.6f, 4.0f, 0.1f);
}

// The arcade (upgrade plan items 3-4): an 8 x 8 m room behind doorway A,
// almost empty: the glowing carpet, neon, an LED ceiling, a bench, and the
// platformer machine against the back wall (tools/assets/arcade.py,
// arcade_cabinet.py). Its music: app/arcade_music (main.cpp).
void buildArcade(Builder& b) {
  // The shell loads at start: its doorway front (the arch, the ARCADE sign)
  // faces the bar. What's inside loads as you come near or see the doorway
  // (SceneRenderer::streamRooms).
  b.model("arcade.glb", {0, 0, 0}, kBlackSteel);
  b.room = 1;
  b.scene.arcadeMin = {kArcX0, 0, kArcZ0};
  b.scene.arcadeMax = {kArcX1, kArcH, kArcZ1};
  b.scene.arcadeDoor = {(kDoorAX0 + kDoorAX1) / 2, 1.2f, kZ0};
  const float cx = (kArcX0 + kArcX1) / 2, cz = (kArcZ0 + kArcZ1) / 2, t = 0.12f;
  // Physics: floor, ceiling, walls (the front one is the bar's back wall).
  b.solid({cx, -0.05f, cz}, {(kArcX1 - kArcX0) / 2 + 0.2f, 0.05f, (kArcZ1 - kArcZ0) / 2 + 0.2f}, kOakFloor);
  b.solid({cx, kArcH + 0.05f, cz}, {(kArcX1 - kArcX0) / 2 + 0.2f, 0.05f, (kArcZ1 - kArcZ0) / 2 + 0.2f}, kPlaster);
  b.solid({cx, kArcH / 2, kArcZ0 - t / 2}, {(kArcX1 - kArcX0) / 2 + t, kArcH / 2, t / 2}, kPlaster);
  b.solid({kArcX0 - t / 2, kArcH / 2, cz}, {t / 2, kArcH / 2, (kArcZ1 - kArcZ0) / 2}, kPlaster);
  b.solid({kArcX1 + t / 2, kArcH / 2, cz}, {t / 2, kArcH / 2, (kArcZ1 - kArcZ0) / 2}, kPlaster);
  // Walking: inside the room only (the front wall is blocked in buildRoom).
  b.blockRect(kArcX0 - 2, kArcZ0 - 2, kArcX1 + 2, kArcZ0);
  b.blockRect(kArcX0 - 2, kArcZ0, kArcX0, kArcZ1);
  b.blockRect(kArcX1, kArcZ0, kArcX1 + 2, kZ0);
  // The platformer machine, facing the door.
  {
    const Vec3 at{cx, 0, kArcZ0 + 0.5f};
    const size_t cabinet = b.model("arcade_cabinet.glb", at, kLacquer);
    b.solid(at + Vec3{0, 0.93f, 0}, {0.37f, 0.93f, 0.44f}, kLacquer);
    b.block(at, 0.42f, 0.5f);
    Interactable game{"PLATFORMER", "Platformer\nTap to play", at + Vec3{0, 2.15f, 0.2f}};
    game.boundsMin = at + Vec3{-0.45f, 0, -0.45f};
    game.boundsMax = at + Vec3{0.45f, 1.9f, 0.55f};
    game.firstModel = cabinet;
    game.modelCount = 1;
    game.action = Action::kArcade;
    game.value = 0;
    b.scene.interactables.push_back(game);
    b.scene.arcadeSpeaker = at + Vec3{0, 1.6f, 0.3f};
  }
  // The bench along the left wall.
  b.block({kArcX0 + 0.35f, 0, cz}, 0.3f, 1.05f);
  b.solid({kArcX0 + 0.35f, 0.23f, cz}, {0.25f, 0.23f, 1.0f}, kLeather);
  b.seat("Arcade bench", {kArcX0 + 0.38f, 0.48f, cz - 0.5f}, {1, 0, 0}, 1.0f, SeatKind::kSofa);
  b.seat("Arcade bench", {kArcX0 + 0.38f, 0.48f, cz + 0.5f}, {1, 0, 0}, 1.0f, SeatKind::kSofa);
  // Lights: coloured washes (the neon itself glows, it doesn't light much).
  b.light({cx - 2.4f, 3.0f, cz - 2.2f}, {1.0f, 0.2f, 0.7f}, 1.4f, 5.5f, 0.3f);
  b.light({cx + 2.4f, 3.0f, cz - 2.2f}, {0.2f, 0.8f, 1.0f}, 1.4f, 5.5f, 0.3f);
  b.light({cx - 2.4f, 3.0f, cz + 2.2f}, {0.25f, 0.85f, 1.0f}, 1.2f, 5.0f, 0.3f);
  b.light({cx + 2.4f, 3.0f, cz + 2.2f}, {0.8f, 0.3f, 1.0f}, 1.2f, 5.0f, 0.3f);
  b.light({cx, 2.2f, kArcZ0 + 1.6f}, {1.0f, 0.75f, 0.9f}, 1.0f, 3.5f, 0.3f);    // the machine
  b.light({cx, 3.1f, cz}, {0.6f, 0.35f, 1.0f}, 0.9f, 4.5f, 0.4f);               // the ring
  b.light({(kDoorAX0 + kDoorAX1) / 2, 3.1f, kZ0 + 0.5f}, {1.0f, 0.2f, 0.65f}, 0.7f, 2.5f, 0.2f);  // the sign, bar side
}

// The giant 2-in-1 mirror on the right wall, from just above the floor to
// near the ceiling (tools/assets/big_mirror.py). Tap the glass to step
// through full -> avatars only -> off, or press a button on its panel.
void buildMirror(Builder& b) {
  const float zc = 3.6f, halfW = 2.2f, y0 = 0.14f, y1 = 3.62f;
  const size_t frame = b.model("big_mirror.glb", {0, 0, 0}, kBlackSteel);
  b.solid({kX1 - 0.04f, (y0 + y1) / 2, zc}, {0.04f, (y1 - y0) / 2 + 0.08f, halfW + 0.08f}, kBlackSteel);
  Mirror m{{kX1 - 0.03f, (y0 + y1) / 2, zc}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, halfW, (y1 - y0) / 2,
           MirrorMode::kFull};
  const int index = static_cast<int>(b.scene.mirrors.size());
  // The panel's three buttons: full, avatars only, off.
  const float panelZ = zc - halfW - 0.42f, panelY = 1.3f;
  for (int i = 0; i < 3; ++i) {
    const Vec3 p{kX1 - 0.012f, panelY + 0.09f - 0.09f * static_cast<float>(i), panelZ - 0.09f};
    m.buttonModels[i] = static_cast<int>(b.model("mirror_button.glb", p, kBrass));
    Interactable it;
    it.tagPosition = p;
    it.boundsMin = p - Vec3{0.06f, 0.04f, 0.05f};
    it.boundsMax = p + Vec3{0.01f, 0.04f, 0.05f};
    it.firstModel = static_cast<size_t>(m.buttonModels[i]);
    it.modelCount = 1;
    it.action = Action::kMirrorMode;
    it.mirror = index;
    it.value = i;
    b.scene.interactables.push_back(it);
  }
  b.scene.mirrors.push_back(m);
  Interactable it;
  it.name = "MIRROR";
  it.tagPosition = {kX1 - 0.4f, 3.95f, zc};
  it.boundsMin = {kX1 - 0.15f, y0, zc - halfW};
  it.boundsMax = {kX1, y1, zc + halfW};
  it.firstModel = frame;
  it.modelCount = 1;
  it.action = Action::kMirror;
  it.mirror = index;
  b.scene.interactables.push_back(it);
  // The vanity bulbs light whoever stands in front.
  b.light({kX1 - 1.2f, 3.6f, zc}, kWarmBulb, 1.6f, 5.5f, 0.6f);
}

}  // namespace

std::vector<engine::MaterialImages> buildBarTextures() {
  constexpr int kSize = 512;
  std::vector<engine::MaterialImages> m;
  m.push_back(engine::makeFabric(kSize, 61));      // kTexFabric
  m.push_back(engine::makeDenim(kSize, 67));       // kTexDenim
  m.push_back(engine::makeHair(kSize, 71));        // kTexHair
  return m;
}

BarScene buildBarScene() {
  BarScene scene;
  scene.ambient = {0.07f, 0.05f, 0.045f};
  scene.fogColor = {0.025f, 0.015f, 0.015f};
  scene.fogDensity = 0.012f;
  scene.spawnPosition = {0.4f, 1.65f, 5.2f};  // just inside the street door
  scene.spawnYaw = 0;
  scene.lightGridMin = {kX0 - kWall - 3.2f, 0, kArcZ0 - 0.3f};
  scene.lightGridMax = {kX1 + 0.5f, 0, kZ1 + kBayDepth + 0.5f};

  // Rooms and doorways for drawing (see BarScene::rooms): the bar, the
  // arcade behind doorway A, and the taped-off room behind doorway B.
  scene.rooms = {
      {{kX0 - kWall - 0.01f, -0.2f, kZ0 - 0.05f}, {kX1 + 0.45f, 4.2f, kZ1 + kBayDepth + 1.2f}},  // 0: the bar
      {{kArcX0 - 0.15f, -0.2f, kArcZ0 - 0.15f}, {kArcX1 + 0.15f, kArcH + 0.2f, kArcZ1 + 0.1f}},  // 1: the arcade
      {{kX0 - kWall - 3.3f, -0.2f, kDoorBZ0 - 0.7f}, {kX0 - kWall, 3.0f, kDoorBZ1 + 1.4f}},  // 2: room 3
  };
  {
    constexpr float kTop = 2.6f, kPad = 0.1f;
    const float za = kZ0 - kWall / 2;  // doorway A, in the back wall
    scene.doorways.push_back({0, 1, {{kDoorAX0 - kPad, 0, za}, {kDoorAX1 + kPad, 0, za},
                                     {kDoorAX1 + kPad, kTop, za}, {kDoorAX0 - kPad, kTop, za}}});
    const float xb = kX0 - kWall / 2;  // doorway B, in the left wall
    scene.doorways.push_back({0, 2, {{xb, 0, kDoorBZ0 - kPad}, {xb, 0, kDoorBZ1 + kPad},
                                     {xb, kTop, kDoorBZ1 + kPad}, {xb, kTop, kDoorBZ0 - kPad}}});
  }

  Builder b{scene};
  buildRoom(b);
  buildBar(b);
  buildHall(b);
  buildLounge(b);
  buildGames(b);
  buildMirror(b);
  buildConstruction(b);
  buildArcade(b);
  b.room = -1;
  return scene;
}

namespace {
// Where you can walk: a 0.1 m grid over the floor, flood-filled from the
// door (cells where a 0.3 m walker isn't pushed by any blocker).
struct WalkMap {
  float minX = 0, minZ = 0, cell = 0.1f;
  int w = 0, d = 0;
  std::vector<uint8_t> reach;
  bool cellOf(Vec3 p, int& i, int& k) const {
    i = static_cast<int>(std::floor((p.x - minX) / cell));
    k = static_cast<int>(std::floor((p.z - minZ) / cell));
    return i >= 0 && k >= 0 && i < w && k < d;
  }
  Vec3 center(int i, int k) const {
    return {minX + (static_cast<float>(i) + 0.5f) * cell, 0, minZ + (static_cast<float>(k) + 0.5f) * cell};
  }
};

WalkMap buildWalkMap(const BarScene& scene) {
  constexpr float kRadius = 0.3f;
  WalkMap m;
  float maxX = -1e9f, maxZ = -1e9f;
  m.minX = m.minZ = 1e9f;
  for (const Blocker& b : scene.blockers) {
    m.minX = std::min(m.minX, b.minX);
    m.minZ = std::min(m.minZ, b.minZ);
    maxX = std::max(maxX, b.maxX);
    maxZ = std::max(maxZ, b.maxZ);
  }
  m.w = static_cast<int>((maxX - m.minX) / m.cell) + 1;
  m.d = static_cast<int>((maxZ - m.minZ) / m.cell) + 1;
  std::vector<uint8_t> open(static_cast<size_t>(m.w * m.d));
  for (int k = 0; k < m.d; ++k) {
    for (int i = 0; i < m.w; ++i) {
      const Vec3 p = m.center(i, k);
      const Vec3 q = resolveCollisions(scene, p, kRadius);
      open[static_cast<size_t>(k * m.w + i)] = std::fabs(q.x - p.x) + std::fabs(q.z - p.z) < 1e-3f;
    }
  }
  m.reach.assign(open.size(), 0);
  int si, sk;
  if (!m.cellOf(scene.spawnPosition, si, sk)) return m;
  std::vector<int> queue;
  // Start from the nearest open cell to the door.
  for (int r = 0; r < 10 && queue.empty(); ++r) {
    for (int dk = -r; dk <= r && queue.empty(); ++dk) {
      for (int di = -r; di <= r && queue.empty(); ++di) {
        const int i = si + di, k = sk + dk;
        if (i >= 0 && k >= 0 && i < m.w && k < m.d && open[static_cast<size_t>(k * m.w + i)]) queue.push_back(k * m.w + i);
      }
    }
  }
  for (int c : queue) m.reach[static_cast<size_t>(c)] = 1;
  for (size_t q = 0; q < queue.size(); ++q) {
    const int c = queue[q], i = c % m.w, k = c / m.w;
    const int ni[4] = {i + 1, i - 1, i, i}, nk[4] = {k, k, k + 1, k - 1};
    for (int n = 0; n < 4; ++n) {
      if (ni[n] < 0 || nk[n] < 0 || ni[n] >= m.w || nk[n] >= m.d) continue;
      const int nc = nk[n] * m.w + ni[n];
      if (!open[static_cast<size_t>(nc)] || m.reach[static_cast<size_t>(nc)]) continue;
      m.reach[static_cast<size_t>(nc)] = 1;
      queue.push_back(nc);
    }
  }
  return m;
}
}  // namespace

bool reachableOnFoot(const BarScene& scene, Vec3 p) {
  static const BarScene* built = nullptr;
  static WalkMap map;
  if (built != &scene) { map = buildWalkMap(scene); built = &scene; }
  int i, k;
  return map.cellOf(p, i, k) && map.reach[static_cast<size_t>(k * map.w + i)];
}

Vec3 standSpot(const BarScene& scene, const Seat& s) {
  constexpr float kRadius = 0.3f;
  // Sofas and armchairs: stand up in front. Stools, the piano bench and the
  // racing rigs face a counter, a piano or screens: step off behind you.
  const Vec3 forward{std::sin(s.yaw), 0, -std::cos(s.yaw)};
  const bool front = s.kind == SeatKind::kSofa || s.kind == SeatKind::kChair;
  Vec3 want = s.hip + forward * (front ? 0.65f : -0.6f);
  want.y = 0;
  const Vec3 p = resolveCollisions(scene, want, kRadius);
  if (reachableOnFoot(scene, p)) return p;
  // Boxed in there: the nearest floor you can walk away from (rings out to 3 m).
  Vec3 best = p;
  float bestD = 1e9f;
  for (float r = 0.1f; r <= 3.0f && bestD > 1e8f; r += 0.1f) {
    for (int a = 0; a < 48; ++a) {
      const float t = engine::kPi * 2 * static_cast<float>(a) / 48.0f;
      const Vec3 c = want + Vec3{std::cos(t) * r, 0, std::sin(t) * r};
      if (!reachableOnFoot(scene, c)) continue;
      const Vec3 q = resolveCollisions(scene, c, kRadius);
      const float d = engine::length(q - want);
      if (reachableOnFoot(scene, q) && d < bestD) { bestD = d; best = q; }
    }
  }
  return best;
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
