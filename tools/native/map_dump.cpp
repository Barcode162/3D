// The bar's floor plan for the hub's map (hub/projects/bar3d-map.json):
// the rooms, the walls and furniture you can't walk through, and every
// thing you can use, in metres (x, z on the floor). Rebuild it after the
// layout changes:
//   g++ -std=c++20 -O1 -I. tools/native/map_dump.cpp bar/bar_scene.cpp bar/scene_names.cpp bar/world_files.cpp \
//       bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp looped/json.cpp looped/world/*.cpp -o build/map_dump && build/map_dump > hub/projects/bar3d-map.json
#include <cstdio>
#include <string>

#include "bar/bar_scene.h"

static std::string esc(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\';
    o += c;
  }
  return o;
}

int main() {
  const app::BarScene s = app::buildBarScene();
  std::printf("{\n  \"rooms\": [");
  for (size_t i = 0; i < s.rooms.size(); ++i) {
    const auto& r = s.rooms[i];
    std::printf("%s[%.2f, %.2f, %.2f, %.2f]", i ? ", " : "", r.min.x, r.min.z, r.max.x, r.max.z);
  }
  std::printf("],\n  \"blockers\": [");
  for (size_t i = 0; i < s.blockers.size(); ++i) {
    const auto& b = s.blockers[i];
    std::printf("%s[%.2f, %.2f, %.2f, %.2f]", i ? ", " : "", b.minX, b.minZ, b.maxX, b.maxZ);
  }
  std::printf("],\n  \"things\": [");
  for (size_t i = 0; i < s.interactables.size(); ++i) {
    const auto& t = s.interactables[i];
    std::printf("%s\n    {\"name\": \"%s\", \"action\": %d, \"x\": %.2f, \"z\": %.2f}", i ? "," : "", esc(t.name).c_str(),
                static_cast<int>(t.action), t.tagPosition.x, t.tagPosition.z);
  }
  std::printf("\n  ],\n  \"seats\": [");
  for (size_t i = 0; i < s.seats.size(); ++i) {
    const auto& t = s.seats[i];
    std::printf("%s[%.2f, %.2f, %d]", i ? ", " : "", t.hip.x, t.hip.z, static_cast<int>(t.kind));
  }
  std::printf("],\n  \"spawn\": [%.2f, %.2f],\n  \"piano\": [%.2f, %.2f],\n  \"darts\": [%.2f, %.2f],\n",
              s.spawnPosition.x, s.spawnPosition.z, s.pianoPosition.x, s.pianoPosition.z, s.dartOche.x, s.dartOche.z);
  std::printf("  \"sims\": [[%.2f, %.2f], [%.2f, %.2f]],\n  \"bartender\": [%.2f, %.2f],\n  \"chess\": [%.2f, %.2f],\n",
              s.simPosition[0].x, s.simPosition[0].z, s.simPosition[1].x, s.simPosition[1].z, s.bartenderSpot.x,
              s.bartenderSpot.z, s.chessCenter.x, s.chessCenter.z);
  std::printf("  \"arcade\": [%.2f, %.2f, %.2f, %.2f],\n  \"jukebox\": [%.2f, %.2f]\n}\n", s.arcadeMin.x, s.arcadeMin.z,
              s.arcadeMax.x, s.arcadeMax.z, s.jukeboxSpeaker.x, s.jukeboxSpeaker.z);
}
