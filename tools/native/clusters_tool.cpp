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
    for (const ModelPrimitive& p : m.primitives) {
      if (p.indices.empty()) continue;
      // The new frame shades each triangle by its own facing, so only
      // position and texture place make a corner: corners that differ only
      // in their normal are one (what's left are true texture seams).
      std::map<std::tuple<float, float, float, float, float>, uint32_t> weld;
      std::vector<uint32_t> remap(p.vertices.size());
      std::vector<Vec3> pos;
      // A plain-coloured material (no picture on it) doesn't need its texture places at all.
      const ModelMaterial* mat = p.material >= 0 && static_cast<size_t>(p.material) < m.materials.size() ? &m.materials[static_cast<size_t>(p.material)] : nullptr;
      const bool pictured = mat && (mat->baseColorImage >= 0 || mat->normalImage >= 0 || mat->emissiveImage >= 0 || mat->metalRoughImage >= 0 ||
                                    mat->occlusionImage >= 0);
      for (size_t i = 0; i < p.vertices.size(); ++i) {
        const ModelVertex& v = p.vertices[i];
        auto k = std::make_tuple(v.position.x, v.position.y, v.position.z, pictured ? v.uv.x : 0.0f, pictured ? v.uv.y : 0.0f);
        auto it = weld.find(k);
        if (it == weld.end()) it = weld.emplace(k, static_cast<uint32_t>(pos.size())).first, pos.push_back(v.position);
        remap[i] = it->second;
      }
      std::vector<uint32_t> idx(p.indices.size());
      for (size_t i = 0; i < idx.size(); ++i) idx[i] = remap[p.indices[i]];
      b.prims.push_back(draw::buildClusters(pos, idx));
      // The spec's numbers: seams, pieces, open edges.
      std::map<std::tuple<float, float, float>, uint32_t> place;
      std::vector<uint32_t> placeOf(pos.size());
      std::map<uint32_t, int> cornersAt;
      for (size_t i = 0; i < pos.size(); ++i) {
        auto k = std::make_tuple(pos[i].x, pos[i].y, pos[i].z);
        auto it = place.find(k);
        if (it == place.end()) it = place.emplace(k, static_cast<uint32_t>(place.size())).first;
        placeOf[i] = it->second;
        ++cornersAt[it->second];
      }
      uint32_t seam = 0;
      for (size_t i = 0; i < pos.size(); ++i) seam += cornersAt[placeOf[i]] > 1;
      std::vector<uint32_t> parent(place.size());
      for (uint32_t i = 0; i < parent.size(); ++i) parent[i] = i;
      auto find = [&](uint32_t x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
      };
      std::map<std::pair<uint32_t, uint32_t>, int> edgeUse;
      for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const uint32_t a = placeOf[idx[i]], bb = placeOf[idx[i + 1]], c = placeOf[idx[i + 2]];
        parent[find(bb)] = find(a), parent[find(c)] = find(a);
        for (auto [x, y] : {std::make_pair(a, bb), std::make_pair(bb, c), std::make_pair(c, a)}) ++edgeUse[{std::min(x, y), std::max(x, y)}];
      }
      std::set<uint32_t> roots;
      for (uint32_t i = 0; i < parent.size(); ++i) roots.insert(find(i));
      uint32_t open = 0;
      for (const auto& [e, n] : edgeUse) open += n == 1;
      b.measure.corners += static_cast<uint32_t>(pos.size());
      b.measure.seamShare += static_cast<float>(seam);  // summed now, a share below
      b.measure.parts += static_cast<uint32_t>(roots.size());
      b.measure.openShare += static_cast<float>(open);
      b.measure.size += static_cast<float>(edgeUse.size());  // edges, for now
      b.full += b.prims.back().fullTriangles;
      b.root += b.prims.back().rootTriangles;
      b.levels = std::max(b.levels, b.prims.back().levels);
    }
    draw::ModelMeasure& M = b.measure;
    M.triangles = b.full;
    M.seamShare = M.corners ? M.seamShare / static_cast<float>(M.corners) : 0;
    M.openShare = M.size > 0 ? M.openShare / M.size : 0;
    M.trianglesPerPart = M.parts ? static_cast<float>(b.full) / static_cast<float>(M.parts) : 0;
    M.simplestShare = b.full ? static_cast<float>(b.root) / static_cast<float>(b.full) : 0;
    M.materials = static_cast<uint32_t>(m.materials.size());
    const Vec3 ext = m.boundsMax - m.boundsMin;
    M.size = std::max({ext.x, ext.y, ext.z});
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
