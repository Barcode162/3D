#pragma once
// Glow and mood after the scene (looped/shaders/post.wgsl): the scene draws
// into an offscreen picture (sceneTarget), then apply() picks out the bright
// parts, blurs them into a soft glow at 1/4, 1/8 and 1/16 of the screen,
// adds it back and sets the mood (tint, saturation, contrast, vignette)
// while drawing onto the screen. About one extra full-screen read and write
// plus small blurs; off at Low quality (the scene then draws straight to
// the screen, as before).
#include <webgpu/webgpu_cpp.h>

#include <functional>

#include "looped/gpu.h"
#include "looped/math.h"

namespace engine {

// How a scene feels after it's drawn. The defaults change nothing but add
// a little glow.
struct Mood {
  float glow = 0.6f;            // how strongly bright things bleed light
  float glowThreshold = 0.55f;  // how bright (linear, 0..1) something must be to glow
  float glowKnee = 0.25f;       // how softly the glow fades in above the threshold
  Vec3 tint{1, 1, 1};           // e.g. warm amber (1.05, 1.0, 0.93)
  float saturation = 1;
  float contrast = 1;
  float lift = 0;               // raises the darkest parts a little (0..0.1)
  float vignette = 0.2f;        // darker corners (0 = none)
};

class PostEffects {
 public:
  bool init(Gpu& gpu);
  // Pipelines compiled (they compile in the background).
  bool ready() const { return prefilter_ && down_ && up_ && composite_; }
  void setMood(const Mood& mood) { mood_ = mood; }
  const Mood& mood() const { return mood_; }
  // Where the scene draws this frame (screen sized, the surface's format).
  const wgpu::TextureView& sceneTarget();
  // Glow + mood from the scene picture onto `target`. `overlay` draws on top
  // in the same pass (text, which shouldn't glow); the pass has the screen's
  // depth buffer, cleared.
  void apply(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target,
             const std::function<void(const wgpu::RenderPassEncoder&)>& overlay);

 private:
  struct Uniforms {
    float texel[4];
    float params[4];
    float grade[4];
    float grade2[4];
  };
  static constexpr int kLevels = 3;  // 1/4, 1/8, 1/16
  void resize();
  wgpu::BindGroup bind(const wgpu::Buffer& u, const wgpu::TextureView& a, const wgpu::TextureView& b);
  void pass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target, bool load,
            const wgpu::RenderPipeline& pipeline, const wgpu::BindGroup& group, const char* label);

  Gpu* gpu_ = nullptr;
  Mood mood_;
  wgpu::RenderPipeline prefilter_, down_, up_, composite_;
  wgpu::BindGroupLayout layout_;
  wgpu::Sampler sampler_;
  uint32_t width_ = 0, height_ = 0;
  wgpu::Texture scene_;
  wgpu::TextureView sceneView_;
  wgpu::Texture levels_[kLevels];
  wgpu::TextureView levelViews_[kLevels];
  uint32_t levelW_[kLevels] = {}, levelH_[kLevels] = {};
  // One uniform buffer and bind group per step: prefilter, down x2, up x2, composite.
  static constexpr int kSteps = 6;
  wgpu::Buffer uniforms_[kSteps];
  wgpu::BindGroup groups_[kSteps];
};

}  // namespace engine
