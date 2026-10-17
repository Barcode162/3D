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

#include <memory>
#include <vector>

#include "app/avatar.h"
#include "app/bar_scene.h"
#include "engine/billboard.h"
#include "engine/gpu.h"
#include "engine/math.h"
#include "engine/mesh.h"
#include "engine/model.h"
#include "engine/skinned_mesh.h"
#include "engine/text.h"

namespace app {

class SceneRenderer {
 public:
  // `avatars` = one mesh per avatar look (players pick one).
  bool init(engine::Gpu& gpu, const BarScene& scene, const std::vector<engine::SkinnedMeshData>& avatars);

  // Simple shapes that change every frame (piano key lights, darts, drinks,
  // cars...): boxes with a color and glow. At most kMaxBoxes are drawn.
  struct Box {
    engine::Vec3 center;
    engine::Vec3 size{0.1f, 0.1f, 0.1f};
    engine::Quat rotation;
    engine::Vec3 color{0.8f, 0.8f, 0.8f};
    float glow = 0;
    float roughness = 0.6f;
    float metallic = 0;
  };
  static constexpr size_t kMaxBoxes = 320;
  void setBoxes(const std::vector<Box>& boxes);
  // Turns one of the scene's lights up or down (1 = as built, 0 = off).
  void setLightScale(size_t light, float scale);
  // Which avatar look you (and each other player, see setOthers) wear.
  void setSelfAvatar(size_t look) { selfLook_ = look < avatarMeshes_.size() ? look : 0; }

  // Extra glow on an interactable's parts (0 = normal).
  void setGlow(const Interactable& it, float glow);
  // A model's own glow, kept under any highlight (e.g. a lit button).
  void setModelBaseGlow(size_t model, float glow);

  // Renders one frame to `target`. `avatar` places the avatar in the world;
  // `avatarBones` are its skinning matrices (see Avatar::pose) for mirrors,
  // `firstPersonBones` the same pose without the head (Avatar::hideHead).
  // Only the text layer on a plain background: for when the bar is hidden
  // (loading screen, 4K Mania), so nothing invisible gets drawn.
  void renderOverlay(const wgpu::TextureView& target, const engine::TextRenderer& text);
  // All of the bar's pipelines have compiled (they compile in the background).
  bool ready() const {
    return pipeline_ && mirroredPipeline_ && skinnedPipeline_ && skinnedMirroredPipeline_ && modelPipeline_ &&
           modelMirroredPipeline_ && modelBlendPipeline_ && modelBlendMirroredPipeline_ && glassPipeline_;
  }
  void render(const wgpu::TextureView& target, const engine::Mat4& viewProj, engine::Vec3 eye,
              const engine::Mat4& avatar, const std::vector<engine::Mat4>& avatarBones,
              const std::vector<engine::Mat4>& firstPersonBones, const std::vector<Mirror>& mirrors,
              const engine::TextRenderer& text, const engine::PictureBillboards& pictures);

  // Other players' avatars this frame (multiplayer): where each one stands
  // (avatarTransform) and its full pose. At most kMaxOthers are drawn.
  static constexpr size_t kMaxOthers = 7;
  void setOthers(const std::vector<engine::Mat4>& transforms, const std::vector<std::vector<engine::Mat4>>& bones,
                 const std::vector<int>& looks);

  size_t objectCount() const { return sceneDraws_.size() + modelsLoaded_; }

  // Model loading, for the loading screen: files finished (loaded or
  // failed), files in total, and overall progress 0..1 (downloads included).
  size_t modelFilesDone() const { return filesDone_; }
  size_t modelFiles() const { return fileProgress_.size(); }
  float loadProgress() const;

  // Moves placed model `index` (BarScene::models order), e.g. a glass that
  // physics is moving.
  void setModelTransform(size_t index, engine::Vec3 position, engine::Quat rotation);

 private:
  static constexpr uint32_t kMaxLights = 32;       // matches shaders/scene.wgsl
  static constexpr uint32_t kLightsPerCell = 8;    // ...and so on
  static constexpr uint32_t kMaxGridCells = 256;
  static constexpr uint32_t kMaxBones = 32;
  static constexpr size_t kMaxModels = 512;
  struct GpuLight {
    float position[4];  // xyz, range
    float color[4];     // rgb, intensity
    float params[4];    // x = radius of the glowing part
  };
  // Mirrors the WGSL structs in shaders/scene.wgsl byte for byte.
  struct FrameUniforms {
    engine::Mat4 viewProj;
    float cameraPos[4];
    float ambient[4];
    float fog[4];
    float clipPlane[4];
  };
  // The lights and the light grid: the floor is cut into square cells, and
  // each cell lists the (at most 8) lights that reach it, so a pixel only
  // adds up the lights near it. Written once.
  struct LightingUniforms {
    uint32_t lightCount[4];
    float grid[4];  // x0, z0, 1 / cell size, cells along x
    uint32_t gridSize[4];  // cells along x, cells along z
    GpuLight lights[kMaxLights];
    uint32_t cells[kMaxGridCells][4];  // x, y = 8 light indices (bytes), z = count
  };
  struct ObjectUniforms {
    engine::Mat4 model;
    float color[4];     // rgb = base color, a = metallic
    float material[4];  // emissive, roughness, texture layer, texture scale
  };
  // Mirrors `struct Mirror` in shaders/mirror.wgsl.
  struct GlassUniforms {
    engine::Mat4 viewProj;
    float center[4];
    float right[4];
    float up[4];
    float params[4];
  };
  // A placed .glb model; `gpu` stays empty until it has finished loading.
  struct ModelDraw {
    std::shared_ptr<engine::GpuModel> gpu;
    uint32_t uniformOffset;
    engine::Vec3 position;  // for sorting see-through models back to front
    float scale = 1;
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
  void drawModels(const wgpu::RenderPassEncoder& pass, bool blend, engine::Vec3 eye) const;
  void loadModels(const BarScene& scene, std::vector<uint8_t>& objectData);
  void drawAvatar(const wgpu::RenderPassEncoder& pass, const wgpu::BindGroup& bones) const;
  void drawOthers(const wgpu::RenderPassEncoder& pass) const;
  wgpu::BindGroup makeBonesBindGroup(wgpu::Buffer& buffer, const char* label);
  void renderMirror(const wgpu::CommandEncoder& encoder, size_t index, const Mirror& mirror,
                    const engine::Mat4& viewProj, engine::Vec3 eye);

  engine::Gpu* gpu_ = nullptr;
  size_t mirrorCount_ = 0;
  wgpu::RenderPipeline pipeline_;          // normal view
  wgpu::RenderPipeline mirroredPipeline_;  // reflected views (triangle winding flips)
  wgpu::RenderPipeline skinnedPipeline_;          // the avatar, first person
  wgpu::RenderPipeline skinnedMirroredPipeline_;  // the avatar, in mirrors
  wgpu::RenderPipeline modelPipeline_, modelMirroredPipeline_;  // loaded .glb models
  wgpu::RenderPipeline modelBlendPipeline_, modelBlendMirroredPipeline_;  // their see-through parts
  wgpu::RenderPipeline glassPipeline_;
  wgpu::BindGroupLayout glassLayout_;
  wgpu::BindGroupLayout frameLayout_, objectLayout_, bonesLayout_, modelLayout_;
  wgpu::Sampler modelSampler_;
  std::vector<ModelDraw> models_;
  size_t modelsLoaded_ = 0;
  std::vector<float> fileProgress_;  // per model file, 0..1
  std::vector<float> modelBaseGlow_, modelHighlight_;  // per placed model
  size_t filesDone_ = 0;
  wgpu::TextureView colorMaps_, bumpMaps_;
  wgpu::Sampler sampler_;
  std::vector<View> views_;  // [0] = main, [1 + i] = mirror i
  wgpu::Buffer lightingBuffer_;
  void buildLighting(const BarScene& scene);
  std::vector<float> lightIntensity_;  // as built, per light
  void writeModelGlow(size_t model);
  wgpu::Buffer objectBuffer_;
  wgpu::BindGroup objectBindGroup_;
  std::vector<DrawItem> sceneDraws_;
  std::vector<ObjectUniforms> objectUniforms_;  // CPU copy of the scene objects
  std::vector<engine::SkinnedMesh> avatarMeshes_;
  size_t selfLook_ = 0;
  engine::Mesh boxMesh_;
  uint32_t boxOffset_ = 0;  // first box slot in objectBuffer_
  size_t boxCount_ = 0;
  void drawBoxes(const wgpu::RenderPassEncoder& pass) const;
  void drawSkinned(const wgpu::RenderPassEncoder& pass, const engine::SkinnedMesh& mesh) const;
  uint32_t avatarOffset_ = 0;  // the avatar's slot in objectBuffer_
  wgpu::Buffer bonesBuffer_, firstPersonBonesBuffer_;
  wgpu::BindGroup bonesBindGroup_, firstPersonBonesBindGroup_;
  struct OtherSlot {
    uint32_t offset;  // in objectBuffer_
    wgpu::Buffer bonesBuffer;
    wgpu::BindGroup bonesBindGroup;
    int look = 0;
  };
  std::vector<OtherSlot> others_;
  size_t othersShown_ = 0;
  FrameUniforms frame_ = {};
  std::vector<MirrorTarget> mirrorTargets_;
  uint32_t targetW_ = 0, targetH_ = 0;
};

}  // namespace app
