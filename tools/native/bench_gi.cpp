// sources: looped/radiosity.cpp looped/bvh.cpp looped/scene_geometry.cpp looped/gltf.cpp bar/bar_scene.cpp bar/scene_names.cpp bar/world_files.cpp bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp looped/world/loop.cpp looped/world/kind.cpp looped/world/world.cpp looped/world/catalog.cpp looped/json.cpp
// Bounced light benchmark (tools/bench.mjs): the bar's geometry, simplified
// as the game does, the ray tree, the CPU build of the bounced light and the
// CPU bounces. Prints "name value unit" lines.
#define LIGHT_TOOL_NO_MAIN
#include "tools/native/light_tool.cpp"

int main() {
  Loaded L;
  double t0 = nowMs();
  load("bar", L);
  const double loadMs = nowMs() - t0;
  const size_t full = L.geo.triangleCount();
  t0 = nowMs();
  L.geo.simplify(0.05f);
  const double simplifyMs = nowMs() - t0;
  t0 = nowMs();
  Bvh bvh;
  bvh.build(L.geo.positions, L.geo.indices);
  const double bvhMs = nowMs() - t0;
  GiBuild b;
  b.begin(L.geo, lightsOf(L.scene), settingsFor(L));
  t0 = nowMs();
  while (!b.step(1000)) {}
  const double buildMs = nowMs() - t0;
  t0 = nowMs();
  const std::vector<Vec3> sol = giSolve(b, lampsOf(L.scene), 8.0f, 4);
  const double solveMs = nowMs() - t0;
  size_t usable = 0;
  for (uint32_t m : b.probeMask) usable += m >> 31;
  std::printf("gi_triangles_full %zu tris\n", full);
  std::printf("gi_triangles %zu tris\n", L.geo.triangleCount());
  std::printf("gi_load_ms %.0f ms\n", loadMs);
  std::printf("gi_simplify_ms %.0f ms\n", simplifyMs);
  std::printf("gi_bvh_ms %.0f ms\n", bvhMs);
  std::printf("gi_patches %zu\n", b.surfels.size());
  std::printf("gi_probes_usable %zu\n", usable);
  std::printf("gi_build_ms %.0f ms\n", buildMs);
  std::printf("gi_rays_per_s %.2f M/s\n", b.rays / 1e3 / std::max(buildMs, 1.0));
  std::printf("gi_solve4_ms %.1f ms\n", solveMs);
  return sol.empty();
}
