// Walk-through test on the real bar layout (app::buildBarScene): which floor
// you can reach on foot from the door, whether standing up from every seat
// lets you walk away (no traps), and whether every usable thing can be
// reached. Prints an overhead map (#: blocked, .: reachable, space: free but
// unreachable, S: a seat's stand-up spot, !: a trapped seat).
// Usage: built and run by tools/native/run.sh; MAP=1 prints the map.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <vector>

#include "bar/bar_scene.h"

using engine::Vec3;

int main() {
  const app::BarScene scene = app::buildBarScene();
  constexpr float kRadius = 0.3f, kCell = 0.1f;
  float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
  for (const app::Blocker& b : scene.blockers) {
    minX = std::min(minX, b.minX); maxX = std::max(maxX, b.maxX);
    minZ = std::min(minZ, b.minZ); maxZ = std::max(maxZ, b.maxZ);
  }
  const int W = static_cast<int>((maxX - minX) / kCell) + 1, D = static_cast<int>((maxZ - minZ) / kCell) + 1;
  auto at = [&](int i, int k) { return Vec3{minX + (static_cast<float>(i) + 0.5f) * kCell, 0, minZ + (static_cast<float>(k) + 0.5f) * kCell}; };
  std::vector<char> freeCell(static_cast<size_t>(W * D)), reach(static_cast<size_t>(W * D));
  for (int k = 0; k < D; ++k) {
    for (int i = 0; i < W; ++i) {
      const Vec3 p = at(i, k);
      const Vec3 q = app::resolveCollisions(scene, p, kRadius);
      freeCell[static_cast<size_t>(k * W + i)] = std::fabs(q.x - p.x) + std::fabs(q.z - p.z) < 1e-3f;
    }
  }
  auto cellOf = [&](Vec3 p, int& i, int& k) {
    i = static_cast<int>((p.x - minX) / kCell);
    k = static_cast<int>((p.z - minZ) / kCell);
    return i >= 0 && k >= 0 && i < W && k < D;
  };
  // Nearest free cell to a point (within 0.3 m).
  auto nearestFree = [&](Vec3 p, int& bi, int& bk) {
    int ci, ck;
    if (!cellOf(p, ci, ck)) return false;
    float best = 1e9f;
    bool found = false;
    for (int dk = -3; dk <= 3; ++dk) {
      for (int di = -3; di <= 3; ++di) {
        const int i = ci + di, k = ck + dk;
        if (i < 0 || k < 0 || i >= W || k >= D || !freeCell[static_cast<size_t>(k * W + i)]) continue;
        const float d = static_cast<float>(di * di + dk * dk);
        if (d < best) { best = d; bi = i; bk = k; found = true; }
      }
    }
    return found;
  };
  int si, sk;
  if (!nearestFree(scene.spawnPosition, si, sk)) { std::printf("FAIL the spawn point is inside something\n"); return 1; }
  std::queue<std::pair<int, int>> q;
  q.push({si, sk});
  reach[static_cast<size_t>(sk * W + si)] = 1;
  while (!q.empty()) {
    auto [i, k] = q.front();
    q.pop();
    const int di[4] = {1, -1, 0, 0}, dk[4] = {0, 0, 1, -1};
    for (int n = 0; n < 4; ++n) {
      const int ni = i + di[n], nk = k + dk[n];
      if (ni < 0 || nk < 0 || ni >= W || nk >= D) continue;
      const size_t c = static_cast<size_t>(nk * W + ni);
      if (!freeCell[c] || reach[c]) continue;
      reach[c] = 1;
      q.push({ni, nk});
    }
  }
  std::vector<std::string> map(static_cast<size_t>(D), std::string(static_cast<size_t>(W), ' '));
  for (int k = 0; k < D; ++k) {
    for (int i = 0; i < W; ++i) {
      const size_t c = static_cast<size_t>(k * W + i);
      map[static_cast<size_t>(k)][static_cast<size_t>(i)] = !freeCell[c] ? '#' : reach[c] ? '.' : ' ';
    }
  }
  int failed = 0;
  // Seats: standing up (app::standSpot, used by bar/main.cpp standUp) must
  // put you somewhere you can walk away from, close to the seat.
  for (size_t n = 0; n < scene.seats.size(); ++n) {
    const app::Seat& s = scene.seats[n];
    const Vec3 p = app::standSpot(scene, s);
    int i, k;
    const float away = std::hypot(p.x - s.hip.x, p.z - s.hip.z);
    const bool ok = nearestFree(p, i, k) && reach[static_cast<size_t>(k * W + i)] && app::reachableOnFoot(scene, p) && away < 2.0f;
    int mi, mk;
    if (cellOf(p, mi, mk)) map[static_cast<size_t>(mk)][static_cast<size_t>(mi)] = ok ? 'S' : '!';
    if (!ok) {
      std::printf("FAIL seat %zu (%s) at %.2f, %.2f: standing up puts you at %.2f, %.2f (%.1f m away)%s\n", n, s.name,
                  s.hip.x, s.hip.z, p.x, p.z, away, app::reachableOnFoot(scene, p) ? "" : ", cut off from the room");
      ++failed;
    } else if (std::getenv("SEATS")) {
      std::printf("seat %zu (%s): stand at %.2f, %.2f (%.1f m)\n", n, s.name, p.x, p.z, away);
    }
  }
  // Things to use: some reachable floor within 2.2 m of them (the prompt distance).
  for (const app::Interactable& it : scene.interactables) {
    const Vec3 c = (it.boundsMin + it.boundsMax) * 0.5f;
    bool ok = false;
    for (int k = 0; k < D && !ok; ++k) {
      for (int i = 0; i < W && !ok; ++i) {
        if (!reach[static_cast<size_t>(k * W + i)]) continue;
        const Vec3 p = at(i, k);
        ok = std::hypot(p.x - c.x, p.z - c.z) < 2.2f;
      }
    }
    if (!ok) {
      std::printf("FAIL %s at %.2f, %.2f can't be reached on foot\n", it.name.empty() ? it.prompt.c_str() : it.name.c_str(), c.x, c.z);
      ++failed;
    }
  }
  int reachable = 0, free = 0;
  for (size_t c = 0; c < reach.size(); ++c) { reachable += reach[c]; free += freeCell[c]; }
  std::printf("floor: %.1f m2 reachable of %.1f m2 free (%d seats, %zu things to use)\n", reachable * kCell * kCell,
              free * kCell * kCell, static_cast<int>(scene.seats.size()), scene.interactables.size());
  if (std::getenv("MAP")) {
    // Every second row and column (0.2 m), x to the right, z down.
    for (int k = 0; k < D; k += 2) {
      std::string row;
      for (int i = 0; i < W; i += 2) row += map[static_cast<size_t>(k)][static_cast<size_t>(i)];
      std::printf("%s\n", row.c_str());
    }
    std::printf("x %.1f..%.1f (left to right), z %.1f..%.1f (top to bottom)\n", minX, maxX, minZ, maxZ);
  }
  // Tight spots (TIGHT=1): floor you can reach, but only by squeezing (a
  // walker 0.9 m wide can't get there). Listed as clusters, for moving
  // furniture so walking around feels natural.
  if (std::getenv("TIGHT")) {
    constexpr float kWide = 0.45f;
    std::vector<char> wideFree(static_cast<size_t>(W * D)), wideReach(static_cast<size_t>(W * D));
    for (int k = 0; k < D; ++k) {
      for (int i = 0; i < W; ++i) {
        const Vec3 p = at(i, k);
        const Vec3 r = app::resolveCollisions(scene, p, kWide);
        wideFree[static_cast<size_t>(k * W + i)] = std::fabs(r.x - p.x) + std::fabs(r.z - p.z) < 1e-3f;
      }
    }
    std::queue<std::pair<int, int>> wq;
    int wi = si, wk = sk;
    if (wideFree[static_cast<size_t>(wk * W + wi)]) { wq.push({wi, wk}); wideReach[static_cast<size_t>(wk * W + wi)] = 1; }
    while (!wq.empty()) {
      auto [i, k] = wq.front();
      wq.pop();
      const int di[4] = {1, -1, 0, 0}, dk[4] = {0, 0, 1, -1};
      for (int d = 0; d < 4; ++d) {
        const int ni = i + di[d], nk = k + dk[d];
        if (ni < 0 || nk < 0 || ni >= W || nk >= D) continue;
        const size_t c = static_cast<size_t>(nk * W + ni);
        if (!wideFree[c] || wideReach[c]) continue;
        wideReach[c] = 1;
        wq.push({ni, nk});
      }
    }
    // Reachable at 0.3 m, free at 0.45 m, but not reachable at 0.45 m: behind a squeeze.
    std::vector<char> seen(static_cast<size_t>(W * D));
    for (int k = 0; k < D; ++k) {
      for (int i = 0; i < W; ++i) {
        const size_t c0 = static_cast<size_t>(k * W + i);
        if (!reach[c0] || !wideFree[c0] || wideReach[c0] || seen[c0]) continue;
        // Flood the cluster.
        std::queue<std::pair<int, int>> cq;
        cq.push({i, k});
        seen[c0] = 1;
        int n = 0;
        float sx = 0, sz = 0;
        while (!cq.empty()) {
          auto [a, b] = cq.front();
          cq.pop();
          ++n;
          const Vec3 p = at(a, b);
          sx += p.x;
          sz += p.z;
          const int di[4] = {1, -1, 0, 0}, dk[4] = {0, 0, 1, -1};
          for (int d = 0; d < 4; ++d) {
            const int ni = a + di[d], nk = b + dk[d];
            if (ni < 0 || nk < 0 || ni >= W || nk >= D) continue;
            const size_t c = static_cast<size_t>(nk * W + ni);
            if (seen[c] || !reach[c] || !wideFree[c] || wideReach[c]) continue;
            seen[c] = 1;
            cq.push({ni, nk});
          }
        }
        if (n >= 4) std::printf("tight: %.2f m2 around (%.2f, %.2f) only reachable by squeezing\n", n * kCell * kCell, sx / n, sz / n);
      }
    }
  }
  std::printf(failed ? "WALK TEST FAILED (%d)\n" : "WALK TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
