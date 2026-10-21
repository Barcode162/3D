// One-time proof (docs/ENGINE.md E2): the bar made from its place file
// (assets/world/places/bar.json, bar/bar_scene.cpp) is the bar the old code
// made (tools/native/legacy/bar_layout.cpp), value by value. Expected
// differences, on purpose: round stools now face where their seats face
// (their model and box shapes turn with them), and the table by the darts
// got the two leg boxes every other round table has.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "bar/bar_scene.h"

namespace app {
BarScene legacyBarScene();
}

using engine::Quat;
using engine::Vec3;
using namespace app;

static int failed = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    std::printf("FAIL %s\n", what.c_str());
    ++failed;
  }
}
static bool near(float a, float b, float e = 2e-4f) { return std::fabs(a - b) <= e; }
static bool near(Vec3 a, Vec3 b, float e = 2e-4f) { return near(a.x, b.x, e) && near(a.y, b.y, e) && near(a.z, b.z, e); }
static bool sameTurn(Quat a, Quat b) { return std::fabs(engine::dot(a, b)) > 1 - 1e-6f; }
static bool sameAngle(float a, float b) { return std::fabs(std::remainder(a - b, 2 * engine::kPi)) < 2e-4f; }
static std::string str(Vec3 v) {
  char b[80];
  std::snprintf(b, sizeof b, "(%.4f %.4f %.4f)", v.x, v.y, v.z);
  return b;
}

int main() {
  const BarScene a = legacyBarScene();
  const BarScene b = buildBarScene();
  check(b.problems.empty(), "the files read without problems: " + b.problems);

  check(a.models.size() == b.models.size(), "models: " + std::to_string(a.models.size()) + " vs " + std::to_string(b.models.size()));
  for (size_t i = 0; i < std::min(a.models.size(), b.models.size()); ++i) {
    const ModelPlacement &x = a.models[i], &y = b.models[i];
    const std::string w = "model " + std::to_string(i) + " " + x.file;
    const bool stool = x.file == "bar_stool.glb" || x.file == "table_stool.glb";
    check(x.file == y.file, w + " file " + y.file);
    check(near(x.position, y.position), w + " at " + str(x.position) + " vs " + str(y.position));
    check(stool || sameTurn(x.rotation, y.rotation), w + " turn");
    check(x.scale == y.scale && x.material == y.material && x.prop == y.prop && x.room == y.room, w + " scale/material/prop/room");
  }

  // Box shapes: the same set (a square footprint may turn: stools, tables).
  std::vector<bool> used(b.colliders.size());
  for (const Collider& c : a.colliders) {
    bool found = false;
    for (size_t k = 0; k < b.colliders.size() && !found; ++k) {
      const Collider& d = b.colliders[k];
      if (used[k] || !near(c.center, d.center) || !near(c.half, d.half) || c.material != d.material) continue;
      const bool square = near(c.half.x, c.half.z, 1e-4f);
      if (!square && !sameTurn(c.rotation, d.rotation)) continue;
      used[k] = found = true;
    }
    check(found, "collider at " + str(c.center) + " half " + str(c.half));
  }
  int extra = 0;
  for (size_t k = 0; k < used.size(); ++k)
    if (!used[k]) {
      ++extra;
      std::printf("     new collider at %s half %s\n", str(b.colliders[k].center).c_str(), str(b.colliders[k].half).c_str());
    }
  check(extra == 2, "two new colliders (the darts table's legs), got " + std::to_string(extra));

  auto sameRects = [&](auto& xs, auto& ys, const char* what, auto eq) {
    std::vector<bool> u(ys.size());
    for (const auto& x : xs) {
      bool f = false;
      for (size_t k = 0; k < ys.size() && !f; ++k)
        if (!u[k] && eq(x, ys[k])) u[k] = f = true;
      check(f, std::string(what) + " missing");
    }
    check(xs.size() == ys.size(), std::string(what) + " count");
  };
  sameRects(a.blockers, b.blockers, "blocker", [](const Blocker& x, const Blocker& y) {
    return near(x.minX, y.minX) && near(x.minZ, y.minZ) && near(x.maxX, y.maxX) && near(x.maxZ, y.maxZ);
  });
  sameRects(a.floors, b.floors, "floor", [](const FloorArea& x, const FloorArea& y) {
    return near(x.minX, y.minX) && near(x.minZ, y.minZ) && near(x.maxX, y.maxX) && near(x.maxZ, y.maxZ) && near(x.height, y.height);
  });

  check(a.lights.size() == b.lights.size(), "lights count");
  for (size_t i = 0; i < std::min(a.lights.size(), b.lights.size()); ++i) {
    const auto &x = a.lights[i], &y = b.lights[i];
    check(near(x.position, y.position) && near(x.color, y.color) && near(x.intensity, y.intensity) && near(x.range, y.range) &&
              near(x.radius, y.radius),
          "light " + std::to_string(i));
  }
  check(a.roomLights == b.roomLights, "the lights the switch dims");
  check(a.fireLight == b.fireLight, "the fire's light");

  check(a.seats.size() == b.seats.size(), "seats count");
  for (size_t i = 0; i < std::min(a.seats.size(), b.seats.size()); ++i) {
    const Seat &x = a.seats[i], &y = b.seats[i];
    const std::string w = "seat " + std::to_string(i) + " " + x.name;
    check(x.name == y.name && x.kind == y.kind && x.model == y.model && x.machine == y.machine, w + " name/kind/model/machine");
    check(near(x.hip, y.hip) && sameAngle(x.yaw, y.yaw) && near(x.lowSeat, y.lowSeat), w + " hip/facing/low " + str(y.hip));
    check(near(x.boundsMin, y.boundsMin) && near(x.boundsMax, y.boundsMax), w + " bounds");
  }

  check(a.interactables.size() == b.interactables.size(), "uses count");
  std::vector<bool> u(b.interactables.size());
  for (const Interactable& x : a.interactables) {
    bool f = false;
    for (size_t k = 0; k < b.interactables.size() && !f; ++k) {
      const Interactable& y = b.interactables[k];
      if (u[k] || x.action != y.action || x.value != y.value || x.name != y.name || x.prompt != y.prompt) continue;
      if (!near(x.tagPosition, y.tagPosition) || !near(x.boundsMin, y.boundsMin) || !near(x.boundsMax, y.boundsMax)) continue;
      if (x.modelCount != y.modelCount || (x.modelCount && x.firstModel != y.firstModel)) continue;
      if (x.mirror != y.mirror || x.direct != y.direct || x.objectCount != y.objectCount) continue;
      u[k] = f = true;
    }
    check(f, "use \"" + (x.name.empty() ? x.prompt : x.name) + "\"");
  }

  check(a.mirrors.size() == b.mirrors.size(), "mirrors");
  for (size_t i = 0; i < std::min(a.mirrors.size(), b.mirrors.size()); ++i) {
    const Mirror &x = a.mirrors[i], &y = b.mirrors[i];
    check(near(x.center, y.center) && near(x.normal, y.normal) && near(x.right, y.right) && near(x.up, y.up) &&
              near(x.halfWidth, y.halfWidth) && near(x.halfHeight, y.halfHeight) && x.mode == y.mode &&
              x.buttonModels[0] == y.buttonModels[0] && x.buttonModels[1] == y.buttonModels[1] &&
              x.buttonModels[2] == y.buttonModels[2],
          "mirror");
  }

  check(near(a.spawnPosition, b.spawnPosition) && sameAngle(a.spawnYaw, b.spawnYaw), "spawn");
  check(near(a.ambient, b.ambient) && near(a.fogColor, b.fogColor) && near(a.fogDensity, b.fogDensity), "air");
  check(near(a.lightGridMin, b.lightGridMin) && near(a.lightGridMax, b.lightGridMax), "light grid");
  check(a.rooms.size() == b.rooms.size(), "rooms");
  for (size_t i = 0; i < std::min(a.rooms.size(), b.rooms.size()); ++i)
    check(near(a.rooms[i].min, b.rooms[i].min) && near(a.rooms[i].max, b.rooms[i].max), "room " + std::to_string(i));
  check(a.doorways.size() == b.doorways.size(), "doorways");
  for (size_t i = 0; i < std::min(a.doorways.size(), b.doorways.size()); ++i) {
    bool same = a.doorways[i].a == b.doorways[i].a && a.doorways[i].b == b.doorways[i].b;
    for (int k = 0; k < 4; ++k) same = same && near(a.doorways[i].corners[k], b.doorways[i].corners[k]);
    check(same, "doorway " + std::to_string(i));
  }
  check(near(a.jukeboxSpeaker, b.jukeboxSpeaker) && near(a.arcadeSpeaker, b.arcadeSpeaker), "speakers");
  check(near(a.arcadeMin, b.arcadeMin) && near(a.arcadeMax, b.arcadeMax) && near(a.arcadeDoor, b.arcadeDoor), "arcade");
  check(near(a.chessCenter, b.chessCenter) && near(a.chessSquare, b.chessSquare), "chessboard");
  check(a.chessPieces.size() == b.chessPieces.size(), "chess pieces");
  for (size_t i = 0; i < std::min(a.chessPieces.size(), b.chessPieces.size()); ++i)
    check(a.chessPieces[i].model == b.chessPieces[i].model && a.chessPieces[i].piece == b.chessPieces[i].piece, "chess piece " + std::to_string(i));
  check(a.coinModel == b.coinModel, "coin");
  check(near(a.pianoPosition, b.pianoPosition) && sameAngle(a.pianoYaw, b.pianoYaw), "piano");
  check(near(a.dartBoard, b.dartBoard) && near(a.dartOche, b.dartOche), "darts");
  for (int r = 0; r < 2; ++r) check(near(a.simPosition[r], b.simPosition[r]), "rig " + std::to_string(r));
  for (int k = 0; k < 4; ++k) check(near(a.rhythmScreen[k], b.rhythmScreen[k]), "4K Mania screen corner " + std::to_string(k));
  for (int k = 0; k < 8; ++k) check(near(a.barTopFront[k], b.barTopFront[k]), "drink spot " + std::to_string(k));
  check(near(a.bartenderSpot, b.bartenderSpot), "bartender's spot");

  std::printf(failed ? "%d FAILED\n" : "SCENE COMPARE PASSED: the place file makes the same bar\n", failed);
  return failed ? 1 : 0;
}
