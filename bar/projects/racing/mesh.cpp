#include "bar/projects/racing/mesh.h"

#include <cmath>

using engine::Vec3;

namespace app::race {

void MeshBuilder::flatQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 color, uint32_t m) {
  const Vec3 n = engine::normalize(engine::cross(b - a, c - a));
  quad(vertex(a, n, color, m), vertex(b, n, color, m), vertex(c, n, color, m), vertex(d, n, color, m));
}

void MeshBuilder::box(Vec3 center, Vec3 half, Vec3 color, uint32_t m, engine::Quat rot) {
  const auto P = [&](float x, float y, float z) { return center + rot.rotate({x * half.x, y * half.y, z * half.z}); };
  flatQuad(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), color, m);      // +z
  flatQuad(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), color, m);  // -z
  flatQuad(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), color, m);      // +x
  flatQuad(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), color, m);  // -x
  flatQuad(P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), P(-1, 1, -1), color, m);      // +y
  flatQuad(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), color, m);  // -y
}

void MeshBuilder::cylinderX(Vec3 c, float r, float hw, int segments, Vec3 color, uint32_t m, bool caps) {
  const uint32_t base = static_cast<uint32_t>(vertices.size());
  for (int i = 0; i <= segments; ++i) {
    const float a = 2 * engine::kPi * static_cast<float>(i) / static_cast<float>(segments);
    const Vec3 n{0, std::cos(a), std::sin(a)};
    vertex(c + Vec3{-hw, n.y * r, n.z * r}, n, color, m);
    vertex(c + Vec3{hw, n.y * r, n.z * r}, n, color, m);
  }
  for (int i = 0; i < segments; ++i) {
    const uint32_t a = base + 2 * static_cast<uint32_t>(i);
    quad(a, a + 2, a + 3, a + 1);
  }
  if (!caps) return;
  for (float s : {-1.0f, 1.0f}) {
    const Vec3 n{s, 0, 0};
    const uint32_t center = vertex(c + Vec3{s * hw, 0, 0}, n, color, m);
    const uint32_t first = static_cast<uint32_t>(vertices.size());
    for (int i = 0; i <= segments; ++i) {
      const float a = 2 * engine::kPi * static_cast<float>(i) / static_cast<float>(segments);
      vertex(c + Vec3{s * hw, std::cos(a) * r, std::sin(a) * r}, n, color, m);
    }
    for (int i = 0; i < segments; ++i) {
      const uint32_t a = first + static_cast<uint32_t>(i);
      if (s > 0) triangle(center, a, a + 1);
      else triangle(center, a + 1, a);
    }
  }
}

void MeshBuilder::cone(Vec3 base, float r, float h, int segments, Vec3 color, uint32_t m) {
  const Vec3 tip = base + Vec3{0, h, 0};
  const float slope = r / h;
  for (int i = 0; i < segments; ++i) {
    const float a0 = 2 * engine::kPi * static_cast<float>(i) / static_cast<float>(segments);
    const float a1 = 2 * engine::kPi * static_cast<float>(i + 1) / static_cast<float>(segments);
    const Vec3 p0 = base + Vec3{std::cos(a0) * r, 0, std::sin(a0) * r};
    const Vec3 p1 = base + Vec3{std::cos(a1) * r, 0, std::sin(a1) * r};
    const Vec3 n0 = engine::normalize({std::cos(a0), slope, std::sin(a0)});
    const Vec3 n1 = engine::normalize({std::cos(a1), slope, std::sin(a1)});
    const Vec3 nt = engine::normalize(n0 + n1);
    triangle(vertex(p0, n0, color, m), vertex(tip, nt, color, m), vertex(p1, n1, color, m));
  }
}

void MeshBuilder::append(const MeshBuilder& o) {
  const uint32_t base = static_cast<uint32_t>(vertices.size());
  vertices.insert(vertices.end(), o.vertices.begin(), o.vertices.end());
  for (uint32_t i : o.indices) indices.push_back(base + i);
}

}  // namespace app::race
