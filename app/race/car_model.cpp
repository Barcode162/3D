#include "app/race/car_model.h"

#include <algorithm>
#include <cmath>
#include <vector>

using engine::Vec3;

namespace app::race {
namespace {

// Side profile along the car (front = -z): half width, top of the body
// side (shoulder), and the hood/deck crown.
struct Section {
  float z, halfWidth, shoulder;
};
const Section kBody[] = {
    {-2.28f, 0.70f, -0.12f}, {-2.22f, 0.82f, -0.02f}, {-2.10f, 0.89f, 0.08f}, {-1.80f, 0.92f, 0.15f},
    {-1.30f, 0.93f, 0.20f},  {-0.80f, 0.93f, 0.24f},  {-0.40f, 0.93f, 0.27f}, {0.20f, 0.93f, 0.29f},
    {0.80f, 0.94f, 0.31f},   {1.40f, 0.94f, 0.33f},   {1.90f, 0.92f, 0.34f},  {2.12f, 0.88f, 0.32f},
    {2.24f, 0.80f, 0.24f},
};

Section bodyAt(float z) {
  const int n = sizeof(kBody) / sizeof(kBody[0]);
  if (z <= kBody[0].z) return kBody[0];
  for (int i = 1; i < n; ++i) {
    if (z <= kBody[i].z) {
      const float t = (z - kBody[i - 1].z) / (kBody[i].z - kBody[i - 1].z);
      const float s = t * t * (3 - 2 * t);
      return {z, kBody[i - 1].halfWidth + (kBody[i].halfWidth - kBody[i - 1].halfWidth) * s,
              kBody[i - 1].shoulder + (kBody[i].shoulder - kBody[i - 1].shoulder) * s};
    }
  }
  return kBody[n - 1];
}

}  // namespace

MeshBuilder buildCarBody(const CarStyle& style) {
  const engine::CarSpec spec = carModel(0).spec;
  MeshBuilder m;
  const float ground = -spec.cgHeight;
  const float floorY = ground + 0.12f;
  const float R = spec.tire.radius;
  const float axles[2] = {-spec.front.z, -spec.rear.z};
  const Vec3 paint{0.6f, 0.6f, 0.6f};
  const Vec3 dark{0.02f, 0.02f, 0.025f};

  // Lower body: a loft of rounded sections, with arches cut over the wheels.
  // Ring (right half, bottom to top), mirrored for the left.
  const int kSlices = 46;
  const int kRing = 7;
  std::vector<Vec3> grid;
  for (int si = 0; si <= kSlices; ++si) {
    const float z = kBody[0].z + (kBody[12].z - kBody[0].z) * static_cast<float>(si) / kSlices;
    const Section sec = bodyAt(z);
    float bottom = floorY;
    for (float az : axles) {
      const float dz = z - az, arch = R + 0.06f;
      if (std::fabs(dz) < arch) bottom = std::max(bottom, ground + R + std::sqrt(arch * arch - dz * dz) - 0.04f);
    }
    const float w = sec.halfWidth, top = sec.shoulder;
    const float crown = 0.05f;
    const Vec3 ring[kRing] = {
        {w * 0.80f, bottom, z}, {w * 0.97f, bottom + 0.05f, z}, {w, std::min(bottom + 0.16f, top - 0.1f), z},
        {w, top - 0.09f, z},    {w * 0.95f, top, z},           {w * 0.65f, top + crown * 0.7f, z},
        {0, top + crown, z}};
    for (const Vec3& p : ring) grid.push_back(p);
  }
  // Smooth normals per grid vertex (right half), then emit both halves.
  std::vector<Vec3> normals(grid.size());
  const auto G = [&](int si, int k) { return grid[static_cast<size_t>(si * kRing + k)]; };
  for (int si = 0; si < kSlices; ++si) {
    for (int k = 0; k + 1 < kRing; ++k) {
      const Vec3 a = G(si, k), b = G(si + 1, k), c = G(si + 1, k + 1);
      const Vec3 n = engine::normalize(engine::cross(c - a, b - a));
      for (int idx : {si * kRing + k, (si + 1) * kRing + k, (si + 1) * kRing + k + 1, si * kRing + k + 1}) {
        normals[static_cast<size_t>(idx)] += n;
      }
    }
  }
  for (float side : {1.0f, -1.0f}) {
    const uint32_t base = static_cast<uint32_t>(m.vertices.size());
    for (size_t i = 0; i < grid.size(); ++i) {
      Vec3 n = engine::normalize(normals[i]);
      const Vec3 p = grid[i];
      m.vertex({p.x * side, p.y, p.z}, {n.x * side, n.y, n.z}, paint, kCarPaint);
    }
    for (int si = 0; si < kSlices; ++si) {
      for (int k = 0; k + 1 < kRing; ++k) {
        const uint32_t a = base + static_cast<uint32_t>(si * kRing + k), b = a + kRing, c = b + 1, d = a + 1;
        if (side > 0) m.quad(a, d, c, b);
        else m.quad(a, b, c, d);
      }
    }
  }
  // Nose and tail caps (dark grille / diffuser), underbody.
  for (int end : {0, kSlices}) {
    const float sign = end == 0 ? -1.0f : 1.0f;
    for (int k = 0; k + 1 < kRing; ++k) {
      const Vec3 a = G(end, k), b = G(end, k + 1);
      const Vec3 mid{0, (a.y + b.y) * 0.5f, a.z};
      const Vec3 color = k < 3 ? dark : paint;
      const uint32_t mat = k < 3 ? kPlain : kCarPaint;
      if (sign < 0) {
        m.flatQuad({-a.x, a.y, a.z}, a, b, {-b.x, b.y, b.z}, color, mat);
      } else {
        m.flatQuad(a, {-a.x, a.y, a.z}, {-b.x, b.y, b.z}, b, color, mat);
      }
      (void)mid;
    }
  }
  m.box({0, floorY + 0.01f, 0}, {0.75f, 0.01f, 2.15f}, dark, kPlain);

  // Cabin: windscreen, side windows, roof, rear window (glass + painted pillars).
  const float roofY = ground + 1.27f;
  struct Glass {
    float z, halfWidth, y;
  };
  const Glass cabin[] = {{-0.70f, 0.84f, 0.25f}, {-0.05f, 0.66f, roofY}, {0.75f, 0.66f, roofY}, {1.55f, 0.84f, 0.32f}};
  for (int i = 0; i < 3; ++i) {
    const Glass a = cabin[i], b = cabin[i + 1];
    const Section sa = bodyAt(a.z), sb = bodyAt(b.z);
    const Vec3 la{-sa.halfWidth * 0.9f, sa.shoulder, a.z}, ra{sa.halfWidth * 0.9f, sa.shoulder, a.z};
    const Vec3 lb{-sb.halfWidth * 0.9f, sb.shoulder, b.z}, rb{sb.halfWidth * 0.9f, sb.shoulder, b.z};
    const Vec3 lta{-a.halfWidth * (i == 0 ? 0.8f : 1.0f), a.y, a.z}, rta{-lta.x, a.y, a.z};
    const Vec3 ltb{-b.halfWidth * (i == 2 ? 0.8f : 1.0f), b.y, b.z}, rtb{-ltb.x, b.y, b.z};
    // Top panel: glass for the windscreen and rear window, paint for the roof.
    if (i == 1) m.flatQuad(lta, rta, rtb, ltb, paint, kCarPaint);
    else m.flatQuad(i == 0 ? la : lta, i == 0 ? ra : rta, i == 0 ? rta : rb, i == 0 ? lta : lb, Vec3{0.02f, 0.02f, 0.03f}, kGlass);
    // Sides: window glass.
    if (i == 0) {
      m.flatQuad(ra, rb, rtb, rta, Vec3{0.02f, 0.02f, 0.03f}, kGlass);
      m.flatQuad(lb, la, lta, ltb, Vec3{0.02f, 0.02f, 0.03f}, kGlass);
    } else if (i == 1) {
      m.flatQuad(ra, rb, rtb, rta, Vec3{0.02f, 0.02f, 0.03f}, kGlass);
      m.flatQuad(lb, la, lta, ltb, Vec3{0.02f, 0.02f, 0.03f}, kGlass);
    } else {
      m.flatQuad(ra, rb, rta, rta, paint, kCarPaint);
      m.flatQuad(lb, la, lta, lta, paint, kCarPaint);
    }
  }
  // Mirrors, splitter, rear wing.
  for (float s : {-1.0f, 1.0f}) {
    m.box({s * 0.98f, 0.33f, -0.62f}, {0.08f, 0.05f, 0.07f}, paint, kCarPaint);
    m.box({s * 0.62f, 0.02f, -2.27f}, {0.2f, 0.06f, 0.02f}, {3.0f, 2.9f, 2.6f}, kGlow);  // headlights
    if (style.wing) m.box({s * 0.55f, 0.47f, 2.05f}, {0.03f, 0.1f, 0.05f}, dark, kPlain);  // wing posts
  }
  m.box({0, ground + 0.16f, -2.25f}, {0.85f, 0.015f, 0.12f}, dark, kPlain);
  if (style.wing) m.box({0, 0.58f, 2.08f}, {0.86f, 0.02f, 0.16f}, dark, kPlain);
  else m.box({0, 0.36f, 2.18f}, {0.8f, 0.015f, 0.06f}, paint, kCarPaint);  // a small lip spoiler
  // The model's size (normals stay close enough for these small scales).
  for (Vertex& v : m.vertices) {
    v.position.x *= style.width;
    v.position.z *= style.length;
  }
  return m;
}

MeshBuilder buildTailLights(const CarStyle& style) {
  MeshBuilder m;
  for (float s : {-1.0f, 1.0f}) {
    m.box({s * 0.62f * style.width, 0.2f, 2.245f * style.length}, {0.2f, 0.045f, 0.01f}, {1.0f, 0.03f, 0.02f}, kGlow);
  }
  return m;
}

MeshBuilder buildWheel(float r) {
  MeshBuilder m;
  const float hw = 0.125f;
  m.cylinderX({0, 0, 0}, r, hw, 20, {0.025f, 0.025f, 0.028f}, kRubber, true);
  // Rim face slightly outside the tire's sidewall, with five spokes.
  m.cylinderX({hw + 0.002f, 0, 0}, r * 0.66f, 0.004f, 20, {0.12f, 0.12f, 0.13f}, kMetal, true);
  for (int k = 0; k < 5; ++k) {
    const float a = 2 * engine::kPi * static_cast<float>(k) / 5;
    const engine::Quat q = engine::Quat::axisAngle({1, 0, 0}, a);
    m.box(q.rotate({hw + 0.012f, r * 0.33f, 0}), {0.008f, r * 0.3f, 0.03f}, {0.7f, 0.7f, 0.72f}, kMetal, q);
  }
  m.box({hw + 0.014f, 0, 0}, {0.01f, 0.05f, 0.05f}, {0.7f, 0.7f, 0.72f}, kMetal);
  return m;
}

}  // namespace app::race
