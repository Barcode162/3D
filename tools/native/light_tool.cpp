// The light tool (docs/AUTOMATION.md): the bar's real geometry, natively.
//   build/native/light_tool stats [place]        triangles, materials, the ray tree
//   build/native/light_tool gi [place]           builds the bounced light, times it, solves it on the CPU
//   build/native/light_tool map [place] [y] [out.png]   a top-down map of the bounced light at height y
//   build/native/light_tool fit [place]          fits each lamp to the glowing surface beside it (L1.4)
// Built by tools/native/run.sh (as light_tool_test with "test") and by bench_gi.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "bar/bar_scene.h"
#include "looped/bvh.h"
#include "looped/gltf.h"
#include "looped/light_units.h"
#include "looped/radiosity.h"
#include "looped/scene_geometry.h"

using namespace engine;

namespace {

std::vector<uint8_t> readBytes(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

double nowMs() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Mat4 placement(Vec3 position, Quat rotation, float scale) {
  Mat4 m = Mat4::transform(position, rotation);
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) m.at(c, r) *= scale;
  return m;
}

struct Loaded {
  app::BarScene scene;
  AlbedoTable table;
  SceneGeometry geo;
  Aabb volume;
  int models = 0, skipped = 0;
};

// The place's still models, as the game has them (props left out).
void load(const std::string& place, Loaded& L) {
  L.scene = app::buildBarScene(place);
  const std::vector<uint8_t> t = readBytes("assets/models/albedo.json");
  L.table.parse(std::string(t.begin(), t.end()));
  std::map<std::string, ModelData> cache;
  for (size_t i = 0; i < L.scene.models.size(); ++i) {
    const app::ModelPlacement& p = L.scene.models[i];
    if (p.prop != app::Prop::kNone) {
      ++L.skipped;
      continue;
    }
    auto it = cache.find(p.file);
    if (it == cache.end()) {
      ModelData d;
      std::string error;
      if (!parseGlb(readBytes("assets/models/" + p.file), d, error)) {
        std::printf("can't read %s: %s\n", p.file.c_str(), error.c_str());
        continue;
      }
      it = cache.emplace(p.file, std::move(d)).first;
    }
    L.geo.add(it->second, placement(p.position, p.rotation, p.scale), p.file, L.table, static_cast<uint32_t>(i));
    ++L.models;
  }
  if (const char* c = std::getenv("SIMPLIFY")) {
    const size_t before = L.geo.triangleCount();
    L.geo.simplify(static_cast<float>(std::atof(c)));
    std::printf("simplified (%.3f m): %zu → %zu triangles\n", std::atof(c), before, L.geo.triangleCount());
  }
  L.volume = Aabb::empty();
  for (const Aabb& r : L.scene.rooms) {
    L.volume.min = minComponents(L.volume.min, r.min);
    L.volume.max = maxComponents(L.volume.max, r.max);
  }
}

std::vector<GiLight> lightsOf(const app::BarScene& s) {
  std::vector<GiLight> out;
  for (const PointLight& l : s.lights) out.push_back({l.position, l.radius});
  return out;
}

std::vector<GiLamp> lampsOf(const app::BarScene& s) {
  std::vector<GiLamp> out;
  for (const PointLight& l : s.lights) {
    Vec3 c = l.color;
    if (l.kelvin > 0) c = mulComponents(light::kelvinColor(l.kelvin), l.color);
    out.push_back({light::unitLuminance(c), light::candela(l.lumens > 0 ? l.lumens : light::lumensFromClassic(l.intensity))});
  }
  return out;
}

// A tiny PNG writer (stored, not compressed: tools only).
uint32_t crc32(const uint8_t* d, size_t n, uint32_t c = 0xffffffffu) {
  for (size_t i = 0; i < n; ++i) {
    c ^= d[i];
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
  }
  return c;
}
void writePng(const std::string& path, int w, int h, const std::vector<uint8_t>& rgb) {
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; ++y) {
    raw.push_back(0);
    raw.insert(raw.end(), rgb.begin() + y * w * 3, rgb.begin() + (y + 1) * w * 3);
  }
  std::vector<uint8_t> z{0x78, 0x01};
  for (size_t at = 0; at < raw.size(); at += 65535) {
    const size_t n = std::min<size_t>(65535, raw.size() - at);
    z.push_back(at + n == raw.size() ? 1 : 0);
    z.push_back(n & 0xff), z.push_back(n >> 8), z.push_back(~n & 0xff), z.push_back((~n >> 8) & 0xff);
    z.insert(z.end(), raw.begin() + at, raw.begin() + at + n);
  }
  uint32_t a = 1, b = 0;
  for (uint8_t v : raw) a = (a + v) % 65521, b = (b + a) % 65521;
  const uint32_t adler = (b << 16) | a;
  for (int s = 24; s >= 0; s -= 8) z.push_back((adler >> s) & 0xff);
  std::ofstream f(path, std::ios::binary);
  auto be = [&](uint32_t v) { for (int s = 24; s >= 0; s -= 8) f.put(static_cast<char>((v >> s) & 0xff)); };
  auto chunk = [&](const char* type, const std::vector<uint8_t>& data) {
    be(static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    f.write(reinterpret_cast<const char*>(td.data()), static_cast<std::streamsize>(td.size()));
    be(crc32(td.data(), td.size()) ^ 0xffffffffu);
  };
  f.write("\x89PNG\r\n\x1a\n", 8);
  std::vector<uint8_t> ihdr(13);
  for (int i = 0; i < 4; ++i) ihdr[i] = (w >> (24 - 8 * i)) & 0xff, ihdr[4 + i] = (h >> (24 - 8 * i)) & 0xff;
  ihdr[8] = 8, ihdr[9] = 2;
  chunk("IHDR", ihdr);
  chunk("IDAT", z);
  chunk("IEND", {});
}

int raybench(Loaded& L) {
  Bvh bvh;
  bvh.build(L.geo.positions, L.geo.indices);
  const Vec3 c = (L.volume.min + L.volume.max) * 0.5f;
  uint32_t seed = 1;
  auto rnd = [&] { seed = seed * 1664525u + 1013904223u; return static_cast<float>(seed >> 8) / 16777216.0f; };
  int hits = 0;
  const int n = 300000;
  double t0 = nowMs();
  for (int i = 0; i < n; ++i) {
    const Vec3 o = c + Vec3{(rnd() - 0.5f) * 16, rnd() * 2 - 0.8f, (rnd() - 0.5f) * 16};
    const Vec3 d = normalize(Vec3{rnd() - 0.5f, rnd() - 0.5f, rnd() - 0.5f});
    Bvh::Hit h;
    hits += bvh.intersect(o, d, 0, 30, &h);
  }
  const double a = nowMs() - t0;
  t0 = nowMs();
  int occ = 0;
  for (int i = 0; i < n; ++i) {
    const Vec3 o = c + Vec3{(rnd() - 0.5f) * 16, rnd() * 2 - 0.8f, (rnd() - 0.5f) * 16};
    const Vec3 d = normalize(Vec3{rnd() - 0.5f, rnd() - 0.5f, rnd() - 0.5f});
    occ += bvh.occluded(o, d, 0, 5);
  }
  const double b = nowMs() - t0;
  std::printf("nearest: %.2f M rays/s (%d%% hit); shadow: %.2f M rays/s\n", n / a / 1e3, hits * 100 / n, n / b / 1e3);
  return 0;
}

int stats(Loaded& L) {
  std::printf("place: %zu models placed, %d still ones used, %d props left out\n", L.scene.models.size(), L.models,
              L.skipped);
  std::printf("geometry: %zu triangles, %zu materials\n", L.geo.triangleCount(), L.geo.materials.size());
  float area = 0;
  for (uint32_t t = 0; t < L.geo.triangleCount(); ++t) area += L.geo.area(t);
  std::printf("surface: %.0f m²; volume %.1f x %.1f x %.1f m\n", area, L.volume.max.x - L.volume.min.x,
              L.volume.max.y - L.volume.min.y, L.volume.max.z - L.volume.min.z);
  const double t0 = nowMs();
  Bvh bvh;
  bvh.build(L.geo.positions, L.geo.indices);
  std::printf("ray tree: %zu nodes in %.0f ms\n", bvh.nodeCount(), nowMs() - t0);
  return 0;
}

GiSettings settingsFor(const Loaded& L) {
  GiSettings s;
  s.volume = L.volume;
  return s;
}

int gi(Loaded& L, bool print, GiBuild* out = nullptr) {
  GiBuild local;
  GiBuild& b = out ? *out : local;
  b.begin(L.geo, lightsOf(L.scene), settingsFor(L));
  const double t0 = nowMs();
  std::string last = b.phaseName();
  double phaseStart = t0;
  while (!b.step(20)) {
    if (last != b.phaseName()) {
      if (print) std::printf("  %-26s %6.0f ms\n", last.c_str(), nowMs() - phaseStart);
      last = b.phaseName();
      phaseStart = nowMs();
    }
  }
  if (print) std::printf("  %-26s %6.0f ms\n", last.c_str(), nowMs() - phaseStart);
  const double ms = nowMs() - t0;
  size_t usable = 0;
  for (uint32_t m : b.probeMask) usable += (m >> 31) & 1u;
  size_t missing = 0;
  for (uint32_t h : b.surfelHits) missing += h == GiBuild::kMiss;
  if (print) {
    std::printf("patches: %zu (%.2f m apart), %zu rays each, %.1f%% see nothing\n", b.surfels.size(),
                b.settings.surfelSpacing, static_cast<size_t>(b.settings.surfelRays),
                100.0 * missing / std::max<size_t>(1, b.surfelHits.size()));
    std::printf("probes: %d x %d x %d = %zu, %zu usable (%.0f%%)\n", b.nx, b.ny, b.nz, b.probeCount(), usable,
                100.0 * usable / std::max<size_t>(1, b.probeCount()));
    std::printf("lamp links: %zu (%.1f per patch)\n", b.direct.size(), static_cast<double>(b.direct.size()) / std::max<size_t>(1, b.surfels.size()));
    std::printf("built in %.0f ms: %.2f M rays, %.2f M rays/s\n", ms, b.rays / 1e6, b.rays / 1e3 / std::max(ms, 1.0));
  }
  return 0;
}

int map(Loaded& L, float y, const std::string& out) {
  GiBuild b;
  gi(L, true, &b);
  const double t0 = nowMs();
  const std::vector<Vec3> L4 = giSolve(b, lampsOf(L.scene), 8.0f, 4);
  std::printf("solved 4 bounces on the CPU in %.0f ms\n", nowMs() - t0);
  const int py = std::clamp(static_cast<int>(std::round((y - b.probeOrigin.y) / b.settings.probeSpacing)), 0, b.ny - 1);
  const int scale = 8, w = b.nx * scale, h = b.nz * scale;
  std::vector<uint8_t> rgb(static_cast<size_t>(w) * h * 3);
  for (int z = 0; z < b.nz; ++z) {
    for (int x = 0; x < b.nx; ++x) {
      const size_t i = static_cast<size_t>(x + b.nx * (py + b.ny * z));
      Vec3 c{0.15f, 0, 0.15f};  // unusable probes: purple
      if (b.probeMask[i] >> 31) {
        const Vec3 v = giProbeAverage(b, L4, i);
        const auto tm = [](float x) { return std::pow(x / (1.0f + x), 1 / 2.2f); };
        c = {tm(v.x * 0.8f), tm(v.y * 0.8f), tm(v.z * 0.8f)};
      }
      for (int dy = 0; dy < scale; ++dy)
        for (int dx = 0; dx < scale; ++dx) {
          const size_t o = (static_cast<size_t>(z * scale + dy) * w + x * scale + dx) * 3;
          rgb[o] = static_cast<uint8_t>(std::clamp(c.x, 0.0f, 1.0f) * 255);
          rgb[o + 1] = static_cast<uint8_t>(std::clamp(c.y, 0.0f, 1.0f) * 255);
          rgb[o + 2] = static_cast<uint8_t>(std::clamp(c.z, 0.0f, 1.0f) * 255);
        }
    }
  }
  writePng(out, w, h, rgb);
  std::printf("map at y = %.2f m: %s (%d x %d)\n", b.probeOrigin.y + py * b.settings.probeSpacing, out.c_str(), w, h);
  return 0;
}

}  // namespace

#ifndef LIGHT_TOOL_NO_MAIN
int main(int argc, char** argv) {
  const std::string cmd = argc > 1 ? argv[1] : "stats";
  const std::string place = argc > 2 ? argv[2] : "bar";
  Loaded L;
  load(place, L);
  if (cmd == "stats") return stats(L);
  if (cmd == "gi") return gi(L, true);
  if (cmd == "rays") return raybench(L);
  if (cmd == "map") return map(L, argc > 3 ? static_cast<float>(std::atof(argv[3])) : 1.2f, argc > 4 ? argv[4] : "build/gi_map.png");
  std::printf("usage: light_tool stats|gi|map|fit [place] ...\n");
  return 2;
}
#endif
