#pragma once
// A 3D model on the GPU, loaded from a .glb file: vertex/index buffers per
// part plus one bind group per material (its uniform values and textures).
// Loading is asynchronous: download → parse → decode textures → upload.
#include <webgpu/webgpu_cpp.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/gltf.h"

namespace engine {

class GpuModel {
 public:
  struct Part {
    wgpu::Buffer vertexBuffer, indexBuffer;
    uint32_t indexCount = 0;
    size_t material = 0;  // index into bindGroups
    bool blend = false;   // see-through: drawn after everything solid
  };

  // Matches `struct ModelMaterial` in shaders/scene.wgsl.
  struct MaterialUniforms {
    float baseColor[4];
    float params[4];    // metallic, roughness, normal scale, occlusion strength
    float emissive[4];  // rgb
  };

  // The bind group layout models are drawn with (group 2 of the model
  // pipeline): binding 1 = material uniforms, 2..5 = base color,
  // metal/roughness, normal, occlusion textures, 6 = sampler, 7 = emissive
  // texture.
  static wgpu::BindGroupLayout createLayout(const wgpu::Device& device);
  static wgpu::VertexBufferLayout vertexLayout();

  // Starts loading `url`. `done` gets the model, or nullptr on failure
  // (the reason is logged). `progress` (optional) hears the download
  // progress, 0..1.
  static void load(const wgpu::Device& device, const wgpu::BindGroupLayout& layout, const wgpu::Sampler& sampler,
                   const std::string& url, std::function<void(std::shared_ptr<GpuModel>)> done,
                   std::function<void(float)> progress = nullptr);

  std::vector<Part> parts;
  bool hasBlend = false;
  std::vector<wgpu::BindGroup> bindGroups;  // one per material
  Vec3 boundsMin, boundsMax;                // in model space
};

}  // namespace engine
