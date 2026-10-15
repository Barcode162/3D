#pragma once
// Procedural geometry (built in code, no model files) and GPU upload.
#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <vector>

#include "engine/math.h"

namespace engine {

struct Vertex {
  Vec3 position;
  Vec3 normal;
};

struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

// All shapes are centered on the origin.
MeshData makeBox(Vec3 size);
MeshData makeCylinder(float radius, float height, int segments = 24);
MeshData makeSphere(float radius, int segments = 24, int rings = 16);

struct Mesh {
  wgpu::Buffer vertexBuffer;
  wgpu::Buffer indexBuffer;
  uint32_t indexCount = 0;

  static Mesh upload(const wgpu::Device& device, const MeshData& data);
  static wgpu::VertexBufferLayout vertexLayout();
};

}  // namespace engine
