#pragma once
// Eyes, glow and mood after the scene (looped/shaders/post.wgsl). The scene
// draws into an offscreen picture (sceneTarget, linear light in
// rgba16float), then apply():
// - eyes (new lighting): meters the picture (64 x 64 → 8 x 8 → 1 x 1) and
//   eases the exposure toward it over about a second, like eyes adapting;
// - glow: picks out the bright parts, blurs them at 1/4, 1/8 and 1/16 of the
//   screen and adds them back (off at Low quality);
// - the curve (AgX) and the mood (tint, saturation, contrast, vignette)
//   while drawing onto the screen, with `overlay` (text, name pictures) on top.
// Classic lighting: the scene arrives already in display colours, as before L1.
#include <webgpu/webgpu_cpp.h>

#include <functional>

#include "looped/gpu.h"
#include "looped/mood.h"

namespace engine {

class PostEffects {
 public:
  bool init(Gpu& gpu);
  // Pipelines compiled (they compile in the background).
  bool ready() const { return prefilter_ && down_ && up_ && composite_ && meter_ && meterDown_ && adapt_ && fixed_; }
  void setMood(const Mood& mood) { mood_ = mood; }
  const Mood& mood() const { return mood_; }
  // Where the scene draws this frame (screen sized, Gpu::kSceneFormat).
  const wgpu::TextureView& sceneTarget();
  // Eyes, glow and mood from the scene picture onto `target`. `overlay` draws
  // on top in the same pass, which has the scene's depth (so name pictures
  // hide behind walls). `glow` off = no bloom (Low quality); `classic` = the
  // lighting before L1; `eyes` off = a fixed exposure (?eyes=0).
  void apply(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target,
             const std::function<void(const wgpu::RenderPassEncoder&)>& overlay, bool glow, bool classic, bool eyes);
  // The next frame's eyes jump straight to the scene (after a teleport or
  // loading) instead of easing.
  void resetEyes() { resetEyes_ = true; }

 private:
  struct Uniforms {
    float texel[4];
    float params[4];
    float grade[4];
    float grade2[4];
    float eye[4];
    float eye2[4];
    float white[4];  // rgb: white balance (multiplies the light)
  };
  static constexpr int kLevels = 3;  // 1/4, 1/8, 1/16
  void resize();
  wgpu::BindGroup bind(const wgpu::Buffer& u, const wgpu::TextureView& a, const wgpu::TextureView& b,
                       const wgpu::TextureView& eye);
  void pass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target, bool load,
            const wgpu::RenderPipeline& pipeline, const wgpu::BindGroup& group, const char* label);

  Gpu* gpu_ = nullptr;
  Mood mood_;
  wgpu::RenderPipeline prefilter_, down_, up_, composite_, meter_, meterDown_, adapt_, fixed_;
  wgpu::BindGroupLayout layout_;
  wgpu::Sampler sampler_;
  uint32_t width_ = 0, height_ = 0;
  wgpu::Texture scene_;
  wgpu::TextureView sceneView_;
  wgpu::Texture levels_[kLevels];
  wgpu::TextureView levelViews_[kLevels];
  uint32_t levelW_[kLevels] = {}, levelH_[kLevels] = {};
  // Metering: 64 x 64, 8 x 8, and the eye itself in two 1 x 1 textures that
  // take turns (this frame's is eased from last frame's).
  wgpu::Texture meterTex_, meterSmallTex_, eyeTex_[2];
  wgpu::TextureView meterView_, meterSmallView_, eyeView_[2];
  int eyeCurrent_ = 0;
  bool resetEyes_ = true;
  bool glowWasOn_ = false, glowCleared_ = false;
  double lastMs_ = -1;
  // One uniform buffer and bind group per step: prefilter, down x2, up x2,
  // composite, meter, meter down, adapt (x2, one per eye texture).
  static constexpr int kSteps = 10;
  wgpu::Buffer uniforms_[kSteps];
  wgpu::BindGroup groups_[kSteps];
  wgpu::BindGroup compositeGroups_[2], prefilterGroups_[2];  // per current eye texture
};

}  // namespace engine
