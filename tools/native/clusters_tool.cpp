// The bar's models in clusters (J116.6, looped/draw/clusters.h):
//   build/native/clusters_tool [place] [limit px]
// builds every still model's clusters, then counts the triangles drawn from
// the place's standard spots (a phone held sideways: 600 pixels a metre at
// 1 m, at most `limit` pixels of error) against every triangle.
// Built and run by tools/clusters.mjs. Uses the light tool's loading.
#define LIGHT_TOOL_NO_MAIN
#include "tools/native/light_tool.cpp"

#include <set>
#include <tuple>

#include "looped/draw/clusters.h"
#include "looped/draw/model_spec.h"

namespace {
using namespace engine;

float scaleOf(const Mat4& m) { return length(Vec3{m.at(0, 0), m.at(0, 1), m.at(0, 2)}); }

struct Built {
  std::vector<draw::ClusterTree> prims;
  uint32_t full = 0, root = 0;
  int levels = 0;
  draw::ModelMeasure measure;
};
}  // namespace

int main(int argc, char** argv) {
  const std::string place = argc > 1 ? argv[1] : "bar";
  const float limit = argc > 2 ? static_cast<float>(std::atof(argv[2])) : 1.0f;
  Loaded L;
  load(place, L);
  std::map<std::string, Built> built;
  const double t0 = nowMs();
  for (const auto& [file, m] : L.files) {
    if (file.rfind("#box-", 0) == 0) continue;
    Built b;
    b.measure = draw::measureModel(m, &b.prims);
    b.full = b.measure.triangles;
    for (const draw::ClusterTree& t : b.prims) b.root += t.rootTriangles, b.levels = std::max(b.levels, t.levels);
    built[file] = std::move(b);
  }
  std::printf("clusters for %zu models in %.1f s\n", built.size(), (nowMs() - t0) / 1000);
  if (std::getenv("SPEC")) {  // the model spec (looped/draw/model_spec.h): every model that breaks a rule, and why
    int pass = 0, fail = 0;
    uint64_t failTris = 0, allTris = 0;
    std::map<std::string, int> byRule;
    std::vector<std::pair<uint32_t, std::string>> order;
    for (const auto& [f, b] : built) order.push_back({b.full, f});
    std::sort(order.rbegin(), order.rend());
    for (const auto& [tris, f] : order) {
      const auto problems = draw::checkModel(built[f].measure);
      allTris += tris;
      if (problems.empty()) {
        ++pass;
        continue;
      }
      ++fail;
      failTris += tris;
      std::printf("%-26s %6u triangles:", f.c_str(), tris);
      for (const auto& p : problems) std::printf("  %s %s;", p.rule->id, p.measured.c_str()), ++byRule[p.rule->id];
      std::printf("\n");
    }
    std::printf("\n%d models meet the spec, %d don't (%.0f%% of all triangles)\n", pass, fail, 100.0 * failTris / std::max<uint64_t>(1, allTris));
    for (const auto& r : draw::modelRules()) std::printf("  %-12s broken by %3d: %s\n", r.id, byRule[r.id], r.rule);
    return 0;
  }
  std::vector<std::pair<uint32_t, std::string>> big;
  for (const auto& [f, b] : built) big.push_back({b.full, f});
  std::sort(big.rbegin(), big.rend());
  std::printf("%-28s %9s %6s %9s\n", "model", "triangles", "levels", "simplest");
  for (size_t i = 0; i < big.size() && i < 10; ++i) {
    const Built& b = built[big[i].second];
    std::printf("%-28s %9u %6d %9u\n", big[i].second.c_str(), b.full, b.levels, b.root);
  }
  // From the standard spots.
  struct Spot {
    const char* name;
    Vec3 eye;
  } spots[] = {{"door", L.scene.spawnPosition}, {"middle", (L.volume.min + L.volume.max) * 0.5f + Vec3{0, -0.5f, 0}}};
  for (const Spot& s : spots) {
    uint64_t full = 0, drawn = 0;
    std::vector<uint32_t> picked;
    for (const Loaded::Placed& pl : L.placed) {
      auto it = built.find(pl.file);
      if (it == built.end()) continue;
      // The eye in the model's own space.
      const float sc = scaleOf(pl.world);
      const Vec3 t{pl.world.at(3, 0), pl.world.at(3, 1), pl.world.at(3, 2)};
      const Vec3 cx = Vec3{pl.world.at(0, 0), pl.world.at(0, 1), pl.world.at(0, 2)} * (1 / sc);
      const Vec3 cy = Vec3{pl.world.at(1, 0), pl.world.at(1, 1), pl.world.at(1, 2)} * (1 / sc);
      const Vec3 cz = Vec3{pl.world.at(2, 0), pl.world.at(2, 1), pl.world.at(2, 2)} * (1 / sc);
      const Vec3 d = s.eye - t;
      draw::DetailView v;
      v.eye = Vec3{dot(d, cx), dot(d, cy), dot(d, cz)} * (1 / sc);
      v.pixelsPerMetre = 600;
      v.limitPixels = limit;
      for (const draw::ClusterTree& tr : it->second.prims) {
        draw::pickClusters(tr, v, picked);
        for (uint32_t c : picked) drawn += tr.clusters[c].count;
        full += tr.fullTriangles;
      }
    }
    std::printf("from the %s (%.1f %.1f %.1f), %.0f px: %llu of %llu triangles drawn (%.0f%%)\n", s.name, s.eye.x, s.eye.y, s.eye.z, limit,
                static_cast<unsigned long long>(drawn), static_cast<unsigned long long>(full), 100.0 * drawn / std::max<uint64_t>(1, full));
  }
  return 0;
}
