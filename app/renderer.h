#pragma once
// Draws the bar: the scene objects, your avatar (whole in mirrors, headless
// in first person so you can see your body), the mirrors themselves, and
// the text on top.
//
// Mirrors: before the main view, each visible mirror gets its own pass that
// renders the room from a camera reflected across the glass into a
// screen-sized texture (only the part of the screen the mirror covers is
// drawn, to save GPU time). The glass then shows that texture.
#include <webgpu/webgpu_cpp.h>

#include <vector>

#include "app/avatar.h"
#include "app/bar_scene.h"
#include "engine/gpu.h"
#include "engine/math.h"
#include "engine/mesh.h"
#include "engine/skinned_mesh.h"
#include "engine/text.h"

namespace app {

class SceneRenderer {
 public:
  bool init(engine::Gpu& gpu, const BarScene& scene, const engine::SkinnedMeshData& avatar);

  // Extra glow on an interactable's parts (0 = normal).
  void setGlow(const Interactable& it, float glow);

  // Renders one frame to `target`. `avatar` places the avatar in the world;
  // `avatarBones` are its skinning matrices (see Avatar::pose) for mirrors,
  // `firstPersonBones` the same pose without the head (Avatar::hideHead).
  void render(const wgpu::TextureView& target, const engine::Mat4& viewProj, engine::Vec3 eye,
              const engine::Mat4& avatar, const std::vector<engine::Mat4>& avatarBones,
              const std::vector<engine::Mat4>& firstPersonBones, const std::vector<Mirror>& mirrors,
              const engine::TextRenderer& text);

  size_t objectCount() const { return sceneDraws_.size(); }

 private:
  static constexpr uint32_t kMaxLights = 8;
  static constexpr uint32_t kMaxBones = 32;  // matches shaders/scene.wgsl
  struct GpuLight {
    float position[4];  // xyz, range
    float color[4];     // rgb, intensity
  };
  // Mirrors the WGSL structs in shaders/scene.wgsl byte for byte.
  struct FrameUniforms {
    engine::Mat4 viewProj;
    float cameraPos[4];
    float ambient[4];
    float fog[4];
    uint32_t lightCount[4];
    GpuLight lights[kMaxLights];
    float clipPlane[4];
  };
  struct ObjectUniforms {
    engine::Mat4 model;
    float color[4];
    float material[4];  // emissive, shininess, texture layer, texture scale
  };
  // Mirrors `struct Mirror` in shaders/mirror.wgsl.
  struct GlassUniforms {
    engine::Mat4 viewProj;
    float center[4];
    float right[4];
    float up[4];
    float params[4];
  };
  struct DrawItem {
    engine::Mesh mesh;
    uint32_t uniformOffset;
  };
  // One camera's frame uniforms (the main view, or a mirror's view).
  struct View {
    wgpu::Buffer buffer;
    wgpu::BindGroup bindGroup;
  };
  struct MirrorTarget {
    wgpu::Texture texture;
    wgpu::TextureView view;
    wgpu::Buffer glassBuffer;
    wgpu::BindGroup glassBindGroup;
  };

  bool createScenePipelines();
  bool createGlassPipeline();
  void createMaterials(wgpu::TextureView& colorMaps, wgpu::TextureView& bumpMaps, wgpu::Sampler& sampler);
  void resizeMirrorTargets();
  void drawItems(const wgpu::RenderPassEncoder& pass, const std::vector<DrawItem>& items) const;
  void drawAvatar(const wgpu::RenderPassEncoder& pass, const wgpu::BindGroup& bones) const;
  wgpu::BindGroup makeBonesBindGroup(wgpu::Buffer& buffer, const char* label);
  void renderMirror(const wgpu::CommandEncoder& encoder, size_t index, const Mirror& mirror,
                    const engine::Mat4& viewProj, engine::Vec3 eye);

  engine::Gpu* gpu_ = nullptr;
  size_t mirrorCount_ = 0;
  wgpu::RenderPipeline pipeline_;          // normal view
  wgpu::RenderPipeline mirroredPipeline_;  // reflected views (triangle winding flips)
  wgpu::RenderPipeline skinnedPipeline_;          // the avatar, first person
  wgpu::RenderPipeline skinnedMirroredPipeline_;  // the avatar, in mirrors
  wgpu::RenderPipeline glassPipeline_;
  wgpu::BindGroupLayout frameLayout_, objectLayout_, bonesLayout_;
  wgpu::TextureView colorMaps_, bumpMaps_;
  wgpu::Sampler sampler_;
  std::vector<View> views_;  // [0] = main, [1 + i] = mirror i
  wgpu::Buffer objectBuffer_;
  wgpu::BindGroup objectBindGroup_;
  std::vector<DrawItem> sceneDraws_;
  std::vector<ObjectUniforms> objectUniforms_;  // CPU copy of the scene objects
  engine::SkinnedMesh avatarMesh_;
  uint32_t avatarOffset_ = 0;  // the avatar's slot in objectBuffer_
  wgpu::Buffer bonesBuffer_, firstPersonBonesBuffer_;
  wgpu::BindGroup bonesBindGroup_, firstPersonBonesBindGroup_;
  FrameUniforms frame_ = {};
  std::vector<MirrorTarget> mirrorTargets_;
  uint32_t targetW_ = 0, targetH_ = 0;
};

}  // namespace app
