#pragma once
// Geometry for the racing world: vertices with a color and a material id
// (the race shader adds the surface detail: asphalt grain, grass, paint
// gloss...), and helpers to build shapes in code.
#include <cstdint>
#include <vector>

#include "engine/math.h"

namespace app::race {

// Material ids, matching shaders/race.wgsl.
enum Material : uint32_t {
  kPlain = 0,    // matte paint, concrete, plastic
  kAsphalt = 1,
  kGrass = 2,
  kGravel = 3,
  kCurb = 4,
  kPaint = 5,    // road markings
  kCarPaint = 6, // glossy clear-coated paint
  kGlass = 7,
  kRubber = 8,
  kMetal = 9,
  kGlow = 10,    // lights (color = emitted light)
  kFoliage = 11,
  kDirt = 12,
};

struct Vertex {
  engine::Vec3 position;
  engine::Vec3 normal;
  engine::Vec3 color;  // linear RGB
  uint32_t material;
};

struct MeshBuilder {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  uint32_t vertex(engine::Vec3 p, engine::Vec3 n, engine::Vec3 c, uint32_t m) {
    vertices.push_back({p, n, c, m});
    return static_cast<uint32_t>(vertices.size() - 1);
  }
  void triangle(uint32_t a, uint32_t b, uint32_t c) { indices.insert(indices.end(), {a, b, c}); }
  // a, b, c, d counter-clockwise seen from the front.
  void quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    triangle(a, b, c);
    triangle(a, c, d);
  }
  // A flat quad with its own normal (corners counter-clockwise from the front).
  void flatQuad(engine::Vec3 a, engine::Vec3 b, engine::Vec3 c, engine::Vec3 d, engine::Vec3 color, uint32_t m);
  // An axis-aligned box rotated by `rot` around `center`.
  void box(engine::Vec3 center, engine::Vec3 half, engine::Vec3 color, uint32_t m, engine::Quat rot = {});
  // A cylinder along the local x axis (wheels), `segments` around.
  void cylinderX(engine::Vec3 center, float radius, float halfWidth, int segments, engine::Vec3 color, uint32_t m,
                 bool caps = true);
  // A cone (trees): base center, radius, height.
  void cone(engine::Vec3 base, float radius, float height, int segments, engine::Vec3 color, uint32_t m);
  void append(const MeshBuilder& other);
};

}  // namespace app::race
