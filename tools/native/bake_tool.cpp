// The baker (J116.3; the plan: docs/plan/jobs/J116.md → The light): the
// room's light worked out once per light channel and stored over its
// surfaces, then squeezed to a few shared layers, and checked against the
// reference renderer.
//   build/native/bake_tool unwrap [place]               every still surface laid out flat: pieces, sizes, the atlas
//   build/native/bake_tool bake [place] [out] [spot...]  bake, squeeze, and compare with the reference at the spots
//       (the reference's own pictures: <out>/ref/<spot>.pfm, made first if missing). SAMPLES (paths a texel),
//       BOUNCES, LAYERS (the shared layers kept), REF_SAMPLES.
// Built and run by tools/bake.mjs. Uses the light tool's loading
// (tools/native/light_tool.cpp, without its main).
#define LIGHT_TOOL_NO_MAIN
#include "tools/native/light_tool.cpp"

#include <array>
#include <atomic>
#include <thread>

#include "looped/bake/unwrap.h"

namespace {

using namespace engine;

constexpr float kTexel = 0.05f;   // metres a texel
constexpr float kMinArea = 0.3f;  // m²: smaller things are lit from the probe grid
constexpr int kAtlasWidth = 2048;
constexpr int kPadding = 2;

std::vector<bool> seeThrough(const Loaded& L, const std::string& file, const ModelData& m) {
  std::vector<bool> skip(m.primitives.size(), false);
  for (size_t i = 0; i < m.primitives.size(); ++i) {
    const int mat = m.primitives[i].material;
    if (mat >= 0 && static_cast<size_t>(mat) < m.materials.size()) skip[i] = L.table.lookup(file, m.materials[static_cast<size_t>(mat)]).seeThrough;
  }
  return skip;
}

float scaleOf(const Mat4& m) { return length(Vec3{m.at(0, 0), m.at(0, 1), m.at(0, 2)}); }

// Which placed things get their own light in the atlas, and at what detail.
struct Plan {
  std::vector<size_t> uses;  // indices into Loaded::placed
  std::map<std::string, bake::Pieces> cuts;
  std::map<std::string, bake::Unwrap> layouts;
  // Every kept piece of every use, where it sits in the atlas.
  struct Spot {
    int use;
    uint32_t piece;
    int x, y;
  };
  std::vector<Spot> spots;
  std::map<std::pair<int, uint32_t>, size_t> spotOf;
  int height = 0;
  double texels = 0;
  size_t probeLit = 0, fiddly = 0, small = 0;
  size_t pieces = 0, hidden = 0, tiny = 0, coarse = 0;
};

// Can anyone see this piece? A few points on it look out over its side: if
// most of their looks escape the building (an outside wall, the roof) or hit
// the back of something (pressed against a wall, inside a leg), no.
bool seen(const Bvh& bvh, const ModelData& m, const bake::Pieces::Piece& pc, const Mat4& world, bake::Rng& rng) {
  const ModelPrimitive& P = m.primitives[static_cast<size_t>(pc.prim)];
  if (pc.area <= 0) return false;
  for (int k = 0; k < 6; ++k) {
    float pick = rng.uniform() * pc.area;
    uint32_t tri = pc.tris.back();
    for (uint32_t t : pc.tris) {
      const Vec3 a = P.vertices[P.indices[3 * t]].position, b = P.vertices[P.indices[3 * t + 1]].position,
                 c = P.vertices[P.indices[3 * t + 2]].position;
      pick -= 0.5f * length(cross(b - a, c - a));
      if (pick <= 0) {
        tri = t;
        break;
      }
    }
    float u = rng.uniform(), v = rng.uniform();
    if (u + v > 1) u = 1 - u, v = 1 - v;
    const Vec3 a = P.vertices[P.indices[3 * tri]].position, b = P.vertices[P.indices[3 * tri + 1]].position,
               c = P.vertices[P.indices[3 * tri + 2]].position;
    const Vec4 wp = transformPoint(world, a + (b - a) * u + (c - a) * v);
    const Vec4 wa = transformPoint(world, a), wb = transformPoint(world, b), wc = transformPoint(world, c);
    const Vec3 nw = normalize(cross(Vec3{wb.x - wa.x, wb.y - wa.y, wb.z - wa.z}, Vec3{wc.x - wa.x, wc.y - wa.y, wc.z - wa.z}));
    if (!isFinite(nw)) continue;
    const Vec3 p = Vec3{wp.x, wp.y, wp.z} + nw * 0.002f;
    int out = 0, behind = 0;
    const int looks = 12;
    for (int r = 0; r < looks; ++r) {
      const float s1 = rng.uniform(), s2 = rng.uniform();
      const float rr = std::sqrt(s1), phi = 2 * kPi * s2;
      const Vec3 t0 = std::fabs(nw.y) < 0.9f ? normalize(cross(nw, {0, 1, 0})) : normalize(cross(nw, {1, 0, 0}));
      const Vec3 b0 = cross(nw, t0);
      const Vec3 d = normalize(t0 * (rr * std::cos(phi)) + b0 * (rr * std::sin(phi)) + nw * std::sqrt(std::max(0.0f, 1 - s1)));
      Bvh::Hit h;
      if (!bvh.intersect(p, d, 1e-4f, 80.0f, &h)) ++out;
      else if (h.back) ++behind;
    }
    if (out * 2 < looks && behind * 2 < looks) return true;
  }
  return false;
}

Plan plan(Loaded& L, const Bvh& bvh) {
  Plan P;
  bake::UnwrapSettings us;
  us.texel = kTexel;
  std::map<std::string, std::vector<uint8_t>> visible;
  std::map<std::string, std::vector<size_t>> placedOf;
  for (size_t i = 0; i < L.placed.size(); ++i) placedOf[L.placed[i].file].push_back(i);
  for (const auto& [file, list] : placedOf) {
    const ModelData& m = L.files.at(file);
    const std::vector<bool> skip = seeThrough(L, file, m);
    bake::Pieces cut = bake::cutModel(m, us, &skip);
    // Too fiddly for a lightmap (many tiny pieces: a stool's curves, a
    // plant): lit from the probe grid like the things that move.
    const double texelsNeeded = cut.area / (kTexel * kTexel);
    const bool fiddly = cut.pieces.empty() || texelsNeeded / static_cast<double>(cut.pieces.size()) < 12.0;
    std::vector<uint8_t> vis(cut.pieces.size(), 0);
    bake::Rng rng(world::hash32(file));
    for (size_t i : list) {
      const float s = scaleOf(L.placed[i].world);
      if (fiddly || cut.area * s * s < kMinArea || std::fabs(s - 1) > 0.1f) {
        ++P.probeLit;
        P.fiddly += fiddly;
        P.small += !fiddly;
        continue;
      }
      P.uses.push_back(i);
      for (size_t k = 0; k < cut.pieces.size(); ++k)
        if (!vis[k] && seen(bvh, m, cut.pieces[k], L.placed[i].world, rng)) vis[k] = 1;
    }
    visible[file] = std::move(vis);
    P.cuts[file] = std::move(cut);
  }
  // Each used model's layout: hidden pieces left out, and pieces too small
  // to hold a few texels (a trim, a bolt: lit from probes); high ones (the
  // ceiling, ducts, beams) at half the detail.
  for (size_t u : P.uses) {
    const std::string& file = L.placed[u].file;
    if (P.layouts.count(file)) continue;
    const bake::Pieces& cut = P.cuts[file];
    const std::vector<uint8_t>& vis = visible[file];
    std::vector<float> texel(cut.pieces.size(), 0);
    for (size_t k = 0; k < cut.pieces.size(); ++k) {
      ++P.pieces;
      if (!vis[k]) {
        ++P.hidden;
        continue;
      }
      const Vec4 mid = transformPoint(L.placed[u].world, cut.pieces[k].middle);
      const bool high = mid.y - L.volume.min.y > 2.8f && L.volume.max.y - L.volume.min.y > 3.6f;
      const float t = high ? 2 * kTexel : kTexel;
      if (cut.pieces[k].area < 4 * t * t) {
        ++P.tiny;
        continue;
      }
      texel[k] = t;
      P.coarse += high;
    }
    P.layouts[file] = bake::layOut(L.files.at(file), cut, texel, kPadding);
  }
  // Every kept piece of every use into the atlas, tallest first.
  std::vector<std::pair<int, int>> sizes;
  for (size_t ui = 0; ui < P.uses.size(); ++ui) {
    const bake::Unwrap& w = P.layouts[L.placed[P.uses[ui]].file];
    for (size_t k = 0; k < w.size.size(); ++k) {
      if (w.size[k].first == 0) continue;
      P.spotOf[{static_cast<int>(ui), static_cast<uint32_t>(k)}] = P.spots.size();
      P.spots.push_back({static_cast<int>(ui), static_cast<uint32_t>(k), 0, 0});
      sizes.push_back(w.size[k]);
      P.texels += static_cast<double>(w.size[k].first) * w.size[k].second;
    }
  }
  std::vector<std::pair<int, int>> at;
  P.height = bake::packShelves(sizes, kAtlasWidth, at);
  for (size_t i = 0; i < P.spots.size(); ++i) P.spots[i].x = at[i].first, P.spots[i].y = at[i].second;
  return P;
}

// ---- The atlas: where every texel is in the room ----

struct Atlas {
  int width = kAtlasWidth, height = 0;
  std::vector<Vec3> pos, nrm;
  std::vector<uint8_t> state;  // 0 empty, 1 on a surface, 2 dilated (copied from a neighbour)
  // Per triangle of the room's geometry: its corners in the atlas (texels), if it has a place there.
  std::vector<std::array<Vec2, 3>> triUv;
  std::vector<uint8_t> mapped;
  size_t at(int x, int y) const { return static_cast<size_t>(y) * width + x; }
};

Atlas layAtlas(const Loaded& L, const Plan& P) {
  Atlas A;
  A.height = std::max(1, P.height);
  const size_t n = static_cast<size_t>(A.width) * A.height;
  A.pos.assign(n, {});
  A.nrm.assign(n, {});
  A.state.assign(n, 0);
  A.triUv.assign(L.geo.triangleCount(), {});
  A.mapped.assign(L.geo.triangleCount(), 0);
  for (size_t ui = 0; ui < P.uses.size(); ++ui) {
    const Loaded::Placed& pl = L.placed[P.uses[ui]];
    const ModelData& m = L.files.at(pl.file);
    const bake::Unwrap& w = P.layouts.at(pl.file);
    const std::vector<bool> skip = seeThrough(L, pl.file, m);
    size_t base = pl.firstTri;  // the primitive's first triangle in the room's geometry
    for (size_t pi = 0; pi < m.primitives.size(); ++pi) {
      if (skip[pi]) continue;
      const ModelPrimitive& prim = m.primitives[pi];
      const bake::Unwrap::Prim& up = w.prims[pi];
      for (size_t k = 0; k < up.fromTri.size(); ++k) {
        const uint32_t v0 = up.indices[3 * k];
        const uint32_t piece = up.piece[v0];
        if (piece == bake::Unwrap::kNone) continue;  // lit from probes
        const auto spot = P.spotOf.find({static_cast<int>(ui), piece});
        if (spot == P.spotOf.end()) continue;
        const Plan::Spot& s = P.spots[spot->second];
        const size_t tri = base + up.fromTri[k];
        std::array<Vec2, 3> uv;
        Vec3 wp[3], wn[3];
        for (int c = 0; c < 3; ++c) {
          const uint32_t nv = up.indices[3 * k + c];
          uv[c] = {static_cast<float>(s.x) + up.uv[nv].x, static_cast<float>(s.y) + up.uv[nv].y};
          const ModelVertex& mv = prim.vertices[up.source[nv]];
          const Vec4 p4 = transformPoint(pl.world, mv.position);
          wp[c] = {p4.x, p4.y, p4.z};
          const Vec4 q4 = transformPoint(pl.world, mv.position + mv.normal);
          wn[c] = normalize(Vec3{q4.x, q4.y, q4.z} - wp[c]);
        }
        A.triUv[tri] = uv;
        A.mapped[tri] = 1;
        // The triangle's texels: each texel whose middle is inside it.
        const Vec3 flat = L.geo.normal(static_cast<uint32_t>(tri));
        const float x0 = std::floor(std::min({uv[0].x, uv[1].x, uv[2].x})), x1 = std::ceil(std::max({uv[0].x, uv[1].x, uv[2].x}));
        const float y0 = std::floor(std::min({uv[0].y, uv[1].y, uv[2].y})), y1 = std::ceil(std::max({uv[0].y, uv[1].y, uv[2].y}));
        const float area = (uv[1].x - uv[0].x) * (uv[2].y - uv[0].y) - (uv[2].x - uv[0].x) * (uv[1].y - uv[0].y);
        if (std::fabs(area) < 1e-9f) continue;
        for (int y = static_cast<int>(y0); y < static_cast<int>(y1); ++y)
          for (int x = static_cast<int>(x0); x < static_cast<int>(x1); ++x) {
            if (x < 0 || y < 0 || x >= A.width || y >= A.height) continue;
            const float px = x + 0.5f, py = y + 0.5f;
            const float b1 = ((px - uv[0].x) * (uv[2].y - uv[0].y) - (uv[2].x - uv[0].x) * (py - uv[0].y)) / area;
            const float b2 = ((uv[1].x - uv[0].x) * (py - uv[0].y) - (px - uv[0].x) * (uv[1].y - uv[0].y)) / area;
            const float b0 = 1 - b1 - b2;
            if (b0 < -1e-4f || b1 < -1e-4f || b2 < -1e-4f) continue;
            const size_t i = A.at(x, y);
            A.pos[i] = wp[0] * b0 + wp[1] * b1 + wp[2] * b2;
            Vec3 nn = normalize(wn[0] * b0 + wn[1] * b1 + wn[2] * b2);
            if (!isFinite(nn) || dot(nn, flat) < 0.2f) nn = flat;  // a smooth normal, unless it's broken
            A.nrm[i] = nn;
            A.state[i] = 1;
          }
      }
      base += prim.indices.size() / 3;
    }
  }
  return A;
}

// Empty texels next to filled ones take their value (twice): bilinear
// reading at a piece's edge then never mixes in an empty texel.
template <class T>
void dilate(const Atlas& A, std::vector<T>& v, std::vector<uint8_t>& state, int stride) {
  for (int pass = 0; pass < 2; ++pass) {
    std::vector<uint8_t> next = state;
    for (int y = 0; y < A.height; ++y)
      for (int x = 0; x < A.width; ++x) {
        const size_t i = A.at(x, y);
        if (state[i]) continue;
        for (int k = 0; k < 4; ++k) {
          const int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
          if (nx < 0 || ny < 0 || nx >= A.width || ny >= A.height) continue;
          const size_t j = A.at(nx, ny);
          if (!state[j]) continue;
          for (int c = 0; c < stride; ++c) v[i * stride + c] = v[j * stride + c];
          next[i] = 2;
          break;
        }
      }
    state = std::move(next);
  }
}

// ---- The squeeze: every channel's light in a few shared layers ----
//
// Each texel's light is a list of numbers (red, green, blue for each
// channel). Across the room those lists are made of a few shapes: the
// eigenvectors of their average outer product (principal components, by
// Jacobi rotations). Keeping the K strongest, a texel stores K numbers and
// the frame rebuilds every channel's light as their sum (looped/light,
// J116.4: w = B · s, then sum over k of u_k(x) w_k).
struct Squeeze {
  int dims = 0, layers = 0;
  std::vector<double> basis;    // layers x dims (row k: layer k's weights per channel colour)
  std::vector<double> strength;  // per layer: its eigenvalue
};

Squeeze squeeze(const std::vector<float>& data, const std::vector<uint8_t>& state, int dims, int layers) {
  std::vector<double> m(static_cast<size_t>(dims) * dims, 0.0);
  size_t count = 0;
  for (size_t i = 0; i < state.size(); ++i) {
    if (state[i] != 1) continue;
    const float* r = &data[i * dims];
    for (int a = 0; a < dims; ++a)
      for (int b = a; b < dims; ++b) m[a * dims + b] += static_cast<double>(r[a]) * r[b];
    ++count;
  }
  for (int a = 0; a < dims; ++a)
    for (int b = 0; b < a; ++b) m[a * dims + b] = m[b * dims + a];
  for (double& v : m) v /= std::max<size_t>(1, count);
  // Jacobi: rotate until the matrix is diagonal; the rotations are the eigenvectors.
  std::vector<double> V(static_cast<size_t>(dims) * dims, 0.0);
  for (int i = 0; i < dims; ++i) V[i * dims + i] = 1;
  for (int sweep = 0; sweep < 60; ++sweep) {
    double off = 0;
    for (int a = 0; a < dims; ++a)
      for (int b = a + 1; b < dims; ++b) off += m[a * dims + b] * m[a * dims + b];
    if (off < 1e-24) break;
    for (int p = 0; p < dims; ++p)
      for (int q = p + 1; q < dims; ++q) {
        const double apq = m[p * dims + q];
        if (std::fabs(apq) < 1e-30) continue;
        const double theta = (m[q * dims + q] - m[p * dims + p]) / (2 * apq);
        const double t = (theta >= 0 ? 1 : -1) / (std::fabs(theta) + std::sqrt(theta * theta + 1));
        const double c = 1 / std::sqrt(t * t + 1), s = t * c;
        for (int k = 0; k < dims; ++k) {
          const double akp = m[k * dims + p], akq = m[k * dims + q];
          m[k * dims + p] = c * akp - s * akq;
          m[k * dims + q] = s * akp + c * akq;
        }
        for (int k = 0; k < dims; ++k) {
          const double apk = m[p * dims + k], aqk = m[q * dims + k];
          m[p * dims + k] = c * apk - s * aqk;
          m[q * dims + k] = s * apk + c * aqk;
        }
        for (int k = 0; k < dims; ++k) {
          const double vkp = V[k * dims + p], vkq = V[k * dims + q];
          V[k * dims + p] = c * vkp - s * vkq;
          V[k * dims + q] = s * vkp + c * vkq;
        }
      }
  }
  std::vector<int> order(dims);
  for (int i = 0; i < dims; ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](int a, int b) { return m[a * dims + a] > m[b * dims + b]; });
  Squeeze S;
  S.dims = dims;
  S.layers = std::min(layers, dims);
  for (int k = 0; k < S.layers; ++k) {
    S.strength.push_back(m[order[k] * dims + order[k]]);
    for (int d = 0; d < dims; ++d) S.basis.push_back(V[d * dims + order[k]]);
  }
  return S;
}

// ---- Baking ----

struct Baked {
  int channels = 0;
  std::vector<float> light;  // per texel: channels x rgb (lux at brightness 1)
  double ms = 0;
};

Baked bakeAtlas(const bake::Room& room, Atlas& A, int channels, int samples, int bounces, int threads) {
  Baked B;
  B.channels = channels;
  const int dims = 3 * channels;
  B.light.assign(A.pos.size() * dims, 0.0f);
  std::vector<size_t> todo;
  for (size_t i = 0; i < A.state.size(); ++i)
    if (A.state[i] == 1) todo.push_back(i);
  std::atomic<size_t> next{0};
  const auto t0 = std::chrono::steady_clock::now();
  auto work = [&] {
    std::vector<Vec3> acc(channels);
    for (size_t k; (k = next.fetch_add(64)) < todo.size();) {
      for (size_t j = k; j < std::min(todo.size(), k + 64); ++j) {
        const size_t i = todo[j];
        const Vec3 n = A.nrm[i];
        const Vec3 p = A.pos[i] + n * 0.002f;
        bake::Rng rng(0x51ED27ull * (i + 1), i + 7);
        // A texel inside something (under a wall, in a crease): most short
        // looks hit the back of a surface. Left for its neighbours to fill.
        int behind = 0;
        for (int r = 0; r < 8; ++r) {
          const float s1 = rng.uniform(), s2 = rng.uniform();
          const Vec3 t0v = std::fabs(n.y) < 0.9f ? normalize(cross(n, {0, 1, 0})) : normalize(cross(n, {1, 0, 0}));
          const Vec3 b0 = cross(n, t0v);
          const float rr = std::sqrt(s1), phi = 2 * kPi * s2;
          const Vec3 d = normalize(t0v * (rr * std::cos(phi)) + b0 * (rr * std::sin(phi)) + n * std::sqrt(std::max(0.0f, 1 - s1)));
          Bvh::Hit h;
          if (room.bvh.intersect(p, d, 1e-4f, 0.25f, &h) && h.back) ++behind;
        }
        if (behind >= 4) {
          A.state[i] = 0;
          continue;
        }
        std::fill(acc.begin(), acc.end(), Vec3{});
        for (int s = 0; s < samples; ++s) bake::incomingByChannel(room, p, n, rng, bounces, 2, acc.data());
        float* out = &B.light[i * dims];
        for (int c = 0; c < channels; ++c) {
          out[3 * c] = acc[c].x / samples;
          out[3 * c + 1] = acc[c].y / samples;
          out[3 * c + 2] = acc[c].z / samples;
        }
      }
    }
  };
  std::vector<std::thread> pool;
  for (int t = 1; t < threads; ++t) pool.emplace_back(work);
  work();
  for (std::thread& t : pool) t.join();
  B.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  return B;
}

// Smoothing the bake's grain (an edge-stopping "a-trous" filter, as light
// bakers do): each texel takes a weighted average of texels around it, 1,
// 2, then 4 texels apart, counting only those on the same surface (close in
// the room, facing the same way), so a shadow's edge on the floor stays
// where it is while the grain inside it evens out.
void smooth(const Atlas& A, std::vector<float>& v, int stride, int passes) {
  static const float kernel[3] = {3.0f / 8, 1.0f / 4, 1.0f / 16};
  std::vector<float> out(v.size());
  for (int pass = 0; pass < passes; ++pass) {
    const int step = 1 << pass;
    const float reach = kTexel * step * 2.5f;
    for (int y = 0; y < A.height; ++y)
      for (int x = 0; x < A.width; ++x) {
        const size_t i = A.at(x, y);
        if (A.state[i] != 1) {
          for (int c = 0; c < stride; ++c) out[i * stride + c] = v[i * stride + c];
          continue;
        }
        float wsum = 0;
        float acc[64] = {};
        for (int dy = -2; dy <= 2; ++dy)
          for (int dx = -2; dx <= 2; ++dx) {
            const int nx = x + dx * step, ny = y + dy * step;
            if (nx < 0 || ny < 0 || nx >= A.width || ny >= A.height) continue;
            const size_t j = A.at(nx, ny);
            if (A.state[j] != 1) continue;
            const float nd = dot(A.nrm[i], A.nrm[j]);
            if (nd < 0.9f) continue;
            const float d2 = lengthSq(A.pos[i] - A.pos[j]);
            if (d2 > reach * reach * 4) continue;
            const float w = kernel[std::abs(dx)] * kernel[std::abs(dy)] * std::exp(-d2 / (reach * reach)) * nd * nd * nd * nd;
            for (int c = 0; c < stride && c < 64; ++c) acc[c] += w * v[j * stride + c];
            wsum += w;
          }
        for (int c = 0; c < stride; ++c) out[i * stride + c] = wsum > 0 && c < 64 ? acc[c] / wsum : v[i * stride + c];
      }
    v.swap(out);
  }
}

// The light at an atlas place, read like the GPU reads a texture (bilinear).
Vec3 sample(const Atlas& A, const std::vector<float>& v, int stride, int offset, Vec2 uv) {
  const float x = uv.x - 0.5f, y = uv.y - 0.5f;
  const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, A.width - 1), y0 = std::clamp(static_cast<int>(std::floor(y)), 0, A.height - 1);
  const int x1 = std::min(x0 + 1, A.width - 1), y1 = std::min(y0 + 1, A.height - 1);
  const float fx = std::clamp(x - static_cast<float>(x0), 0.0f, 1.0f), fy = std::clamp(y - static_cast<float>(y0), 0.0f, 1.0f);
  auto at = [&](int xx, int yy) {
    const float* p = &v[A.at(xx, yy) * stride + offset];
    return Vec3{p[0], p[1], p[2]};
  };
  return (at(x0, y0) * (1 - fx) + at(x1, y0) * fx) * (1 - fy) + (at(x0, y1) * (1 - fx) + at(x1, y1) * fx) * fy;
}

std::vector<Vec3> readPfm(const std::string& path, int& w, int& h) {
  std::ifstream f(path, std::ios::binary);
  std::string tag;
  float scale = -1;
  f >> tag >> w >> h >> scale;
  f.get();
  std::vector<Vec3> px(static_cast<size_t>(w) * h);
  if (tag != "PF") return {};
  for (int y = h - 1; y >= 0; --y) f.read(reinterpret_cast<char*>(&px[static_cast<size_t>(y) * w]), static_cast<std::streamsize>(sizeof(Vec3) * w));
  return px;
}

int bakeCommand(Loaded& L, const std::string& out, std::vector<std::string> spots) {
  std::filesystem::create_directories(out);
  const int samples = envInt("SAMPLES", 128), bounces = envInt("BOUNCES", 4), layersWanted = envInt("LAYERS", 4);
  const int threads = envInt("THREADS", 4);
  double t0 = nowMs();
  bake::Room room;
  std::vector<bake::Lamp> lamps = bake::lampsOf(L.scene.lights);
  room.build(L.geo, lamps);
  const int channels = static_cast<int>(lamps.size());  // each lamp its own channel here
  std::printf("room: %zu triangles, %d lamps (each its own channel), ray tree in %.0f ms\n", L.geo.triangleCount(), channels, nowMs() - t0);
  t0 = nowMs();
  const Plan P = plan(L, room.bvh);
  Atlas A = layAtlas(L, P);
  size_t onSurface = 0;
  for (uint8_t s : A.state) onSurface += s == 1;
  std::printf("atlas %d x %d: %zu texels on surfaces (%zu pieces; %zu placements lit from probes) in %.0f ms\n", A.width, A.height,
              onSurface, P.spots.size(), P.probeLit, nowMs() - t0);
  Baked B = bakeAtlas(room, A, channels, samples, bounces, threads);
  size_t kept = 0;
  for (uint8_t s : A.state) kept += s == 1;
  std::printf("baked %zu texels x %d paths in %.1f s (%zu inside things, left to their neighbours)\n", kept, samples, B.ms / 1000,
              onSurface - kept);
  const int dims = 3 * channels;
  // The squeeze, and how much each number of layers keeps.
  const Squeeze S = squeeze(B.light, A.state, dims, dims);
  double all = 0;
  for (double v : S.strength) all += v;
  std::printf("layers (how much of the room's light each keeps):");
  double sum = 0;
  for (int k = 0; k < S.layers; ++k) {
    sum += S.strength[k];
    std::printf(" %d: %.2f%%", k + 1, 100 * sum / std::max(all, 1e-30));
    if (sum / all > 0.99999) break;
  }
  std::printf("\n");
  const int layers = std::min(layersWanted, S.layers);
  // Each texel's K numbers, and its light rebuilt from them (all channels at 1).
  std::vector<float> coeff(A.pos.size() * layers, 0.0f), rebuilt(A.pos.size() * 3, 0.0f), exact(A.pos.size() * 3, 0.0f);
  for (size_t i = 0; i < A.pos.size(); ++i) {
    if (A.state[i] != 1) continue;
    const float* r = &B.light[i * dims];
    for (int k = 0; k < layers; ++k) {
      double u = 0;
      for (int d = 0; d < dims; ++d) u += S.basis[k * dims + d] * r[d];
      coeff[i * layers + k] = static_cast<float>(u);
      for (int c = 0; c < channels; ++c)
        for (int e = 0; e < 3; ++e) rebuilt[i * 3 + e] += static_cast<float>(u * S.basis[k * dims + 3 * c + e]);
    }
    for (int c = 0; c < channels; ++c)
      for (int e = 0; e < 3; ++e) exact[i * 3 + e] += r[3 * c + e];
  }
  // The grain smoothed (SMOOTH passes, 0 = none): the layers' numbers (what
  // the frame reads), and the exact sums for comparison.
  const int passes = envInt("SMOOTH", 3);
  if (passes > 0) {
    smooth(A, coeff, layers, passes);
    smooth(A, exact, 3, passes);
    std::fill(rebuilt.begin(), rebuilt.end(), 0.0f);
    for (size_t i = 0; i < A.pos.size(); ++i) {
      if (A.state[i] != 1) continue;
      for (int k = 0; k < layers; ++k)
        for (int c = 0; c < channels; ++c)
          for (int e = 0; e < 3; ++e) rebuilt[i * 3 + e] += static_cast<float>(coeff[i * layers + k] * S.basis[k * dims + 3 * c + e]);
    }
  }
  std::vector<uint8_t> st1 = A.state, st2 = A.state;
  dilate(A, rebuilt, st1, 3);
  dilate(A, exact, st2, 3);
  // The atlas as a picture (all channels at 1), for looking at.
  {
    std::vector<Vec3> px(A.pos.size());
    for (size_t i = 0; i < px.size(); ++i) px[i] = Vec3{rebuilt[i * 3], rebuilt[i * 3 + 1], rebuilt[i * 3 + 2]} * (0.5f / kPi);
    writePng(out + "/atlas.png", A.width, A.height, bake::toDisplay(px, A.width, A.height, Mood{}));
  }
  // ---- Against the reference, at each spot ----
  if (spots.empty()) spots = {"front=0,1.65,2.9,0,-12", "corner=2.6,1.7,2.6,-45,-15", "table=-1.5,1.3,1.6,43,-25"};
  for (const std::string& arg : spots) {
    std::string name = arg, cam;
    if (const size_t eq = arg.find('='); eq != std::string::npos) name = arg.substr(0, eq), cam = arg.substr(eq + 1);
    float c[5];
    if (std::sscanf(cam.c_str(), "%f,%f,%f,%f,%f", &c[0], &c[1], &c[2], &c[3], &c[4]) != 5) {
      std::printf("give spots as name=x,y,z,yaw,pitch\n");
      return 2;
    }
    bake::View v;
    v.eye = {c[0], c[1], c[2]};
    v.yaw = radians(c[3]);
    v.pitch = radians(c[4]);
    v.width = envInt("WIDTH", 480);
    v.height = envInt("HEIGHT", 270);
    // The reference, made once and kept.
    const std::string refPath = out + "/ref/" + name + ".pfm";
    int rw = 0, rh = 0;
    std::vector<Vec3> ref = readPfm(refPath, rw, rh);
    if (ref.empty() || rw != v.width || rh != v.height) {
      std::filesystem::create_directories(out + "/ref");
      bake::RenderSettings rs;
      rs.samples = envInt("REF_SAMPLES", 512);
      rs.bounces = 6;
      rs.threads = threads;
      const bake::Picture pic = bake::render(room, v, rs);
      writePfm(refPath, v.width, v.height, pic.light);
      ref = pic.light;
      std::printf("  (the reference at %s: %d paths a pixel, %.0f s)\n", name.c_str(), rs.samples, pic.ms / 1000);
    }
    // What the baked light gives each pixel: the first surface it sees, read from the atlas.
    const Vec3 f{std::sin(v.yaw) * std::cos(v.pitch), std::sin(v.pitch), -std::cos(v.yaw) * std::cos(v.pitch)};
    const Vec3 right = normalize(cross(f, {0, 1, 0})), up = cross(right, f);
    const float th = std::tan(v.fovY * 0.5f), aspect = static_cast<float>(v.width) / v.height;
    std::vector<Vec3> baked(ref.size()), bakedExact(ref.size());
    std::vector<uint8_t> has(ref.size(), 0);
    double errSum = 0, errExactSum = 0, refSum = 0;
    size_t n = 0, probes = 0;
    std::vector<double> errs;
    for (int y = 0; y < v.height; ++y)
      for (int x = 0; x < v.width; ++x) {
        const size_t i = static_cast<size_t>(y) * v.width + x;
        const Vec3 dir = normalize(f + right * ((2 * (x + 0.5f) / v.width - 1) * th * aspect) + up * ((1 - 2 * (y + 0.5f) / v.height) * th));
        Bvh::Hit h;
        if (!room.bvh.intersect(v.eye, dir, 0.05f, 200, &h)) continue;
        if (!A.mapped[h.tri]) {
          ++probes;
          continue;
        }
        const std::array<Vec2, 3>& uv = A.triUv[h.tri];
        // The BVH's u, v are for the triangle's corners in the order it was given (as the geometry has them).
        const Vec2 at{uv[0].x * (1 - h.u - h.v) + uv[1].x * h.u + uv[2].x * h.v, uv[0].y * (1 - h.u - h.v) + uv[1].y * h.u + uv[2].y * h.v};
        baked[i] = sample(A, rebuilt, 3, 0, at);
        bakedExact[i] = sample(A, exact, 3, 0, at);
        has[i] = 1;
        const double r = light::luminance(ref[i]), b = light::luminance(baked[i]), e = light::luminance(bakedExact[i]);
        errSum += std::fabs(b - r);
        errExactSum += std::fabs(e - r);
        refSum += r;
        errs.push_back(std::fabs(b - r) / std::max(r, 1e-3));
        ++n;
      }
    std::sort(errs.begin(), errs.end());
    const double median = errs.empty() ? 0 : errs[errs.size() / 2], p90 = errs.empty() ? 0 : errs[errs.size() * 9 / 10];
    std::printf("%-8s %zu pixels on baked surfaces (%zu on probe-lit things): error %.1f%% of the light (%.1f%% with every channel kept); per pixel, half under %.1f%%, 90%% under %.1f%%\n",
                name.c_str(), n, probes, 100 * errSum / std::max(refSum, 1e-9), 100 * errExactSum / std::max(refSum, 1e-9), 100 * median, 100 * p90);
    // Pictures: the reference's light, the baked light, side by side (light alone, surfaces 50% grey).
    std::vector<Vec3> pair(static_cast<size_t>(v.width) * 2 * v.height);
    for (int y = 0; y < v.height; ++y)
      for (int x = 0; x < v.width; ++x) {
        const size_t i = static_cast<size_t>(y) * v.width + x;
        pair[static_cast<size_t>(y) * 2 * v.width + x] = ref[i] * (0.5f / kPi);
        pair[static_cast<size_t>(y) * 2 * v.width + v.width + x] = (has[i] ? baked[i] : Vec3{0.02f, 0, 0.02f}) * (0.5f / kPi);
      }
    writePng(out + "/" + name + ".png", 2 * v.width, v.height, bake::toDisplay(pair, 2 * v.width, v.height, Mood{}));
  }
  // The baked layers, for the frame (J116.4).
  {
    std::ofstream f(out + "/layers.txt");
    f << "atlas " << A.width << " " << A.height << "\nchannels " << channels << "\nlayers " << layers << "\n";
    for (int k = 0; k < layers; ++k) {
      f << "layer " << k;
      for (int d = 0; d < dims; ++d) f << " " << S.basis[k * dims + d];
      f << "\n";
    }
  }
  return 0;
}

int unwrapReport(Loaded& L) {
  double t0 = nowMs();
  Bvh bvh;
  bvh.build(L.geo.positions, L.geo.indices);
  std::printf("ray tree in %.0f ms\n", nowMs() - t0);
  t0 = nowMs();
  const Plan P = plan(L, bvh);
  std::printf("cut %zu model files in %.0f ms: of the lightmapped ones' %zu pieces, %zu never seen, %zu too small (probes), %zu up high at half detail\n",
              P.cuts.size(), nowMs() - t0, P.pieces, P.hidden, P.tiny, P.coarse);
  std::vector<std::pair<double, std::string>> big;
  std::map<std::string, int> count;
  for (size_t u : P.uses) ++count[L.placed[u].file];
  for (const auto& [file, n] : count) big.push_back({P.layouts.at(file).texels * n, file});
  std::sort(big.rbegin(), big.rend());
  std::printf("%-28s %4s %7s %9s %8s\n", "model", "uses", "pieces", "texels", "m²");
  for (size_t i = 0; i < big.size() && i < 16; ++i) {
    const bake::Unwrap& w = P.layouts.at(big[i].second);
    std::printf("%-28s %4d %7d %9.0f %8.1f\n", big[i].second.c_str(), count[big[i].second], w.kept, w.texels, w.area);
  }
  std::printf("lit from the atlas: %zu placements, %zu pieces, %.2f M texels; lit from probes: %zu placements (%zu fiddly, %zu small or scaled)\n",
              P.uses.size(), P.spots.size(), P.texels / 1e6, P.probeLit, P.fiddly, P.small);
  std::printf("atlas: %d x %d (%.2f M texels, %.0f%% filled)\n", kAtlasWidth, P.height,
              kAtlasWidth * static_cast<double>(P.height) / 1e6, 100 * P.texels / (static_cast<double>(kAtlasWidth) * P.height));
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string cmd = argc > 1 ? argv[1] : "unwrap";
  const std::string place = argc > 2 ? argv[2] : "bar";
  Loaded L;
  load(place, L);
  if (cmd == "unwrap") return unwrapReport(L);
  if (cmd == "bake") return bakeCommand(L, argc > 3 ? argv[3] : "build/bake/" + place, std::vector<std::string>(argv + std::min(argc, 4), argv + argc));
  std::printf("usage: bake_tool unwrap|bake [place] ...\n");
  return 2;
}
