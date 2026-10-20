#include "looped/post.h"

#include <algorithm>

#include "looped/file.h"
#include "looped/log.h"
#include "looped/pipeline_cache.h"
#include "looped/profiler.h"

namespace engine {

namespace {

constexpr wgpu::TextureFormat kGlowFormat = wgpu::TextureFormat::RGBA16Float;

wgpu::ShaderModule shader(const wgpu::Device& device) {
  auto source = readTextFile("/shaders/post.wgsl");
  if (!source) return nullptr;
  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source->c_str();
  wgpu::ShaderModuleDescriptor desc;
  desc.nextInChain = &wgsl;
  desc.label = "post.wgsl";
  return device.CreateShaderModule(&desc);
}

}  // namespace

bool PostEffects::init(Gpu& gpu) {
  gpu_ = &gpu;
  const wgpu::Device& device = gpu.device;
  wgpu::ShaderModule module = shader(device);
  if (!module) return false;
  {
    wgpu::BindGroupLayoutEntry e[4];
    e[0].binding = 0;
    e[0].visibility = wgpu::ShaderStage::Fragment;
    e[0].buffer.type = wgpu::BufferBindingType::Uniform;
    for (int i = 1; i <= 2; ++i) {
      e[i].binding = static_cast<uint32_t>(i);
      e[i].visibility = wgpu::ShaderStage::Fragment;
      e[i].texture.sampleType = wgpu::TextureSampleType::Float;
    }
    e[3].binding = 3;
    e[3].visibility = wgpu::ShaderStage::Fragment;
    e[3].sampler.type = wgpu::SamplerBindingType::Filtering;
    wgpu::BindGroupLayoutDescriptor ld;
    ld.label = "post";
    ld.entryCount = 4;
    ld.entries = e;
    layout_ = device.CreateBindGroupLayout(&ld);
  }
  {
    wgpu::SamplerDescriptor sd;
    sd.magFilter = sd.minFilter = wgpu::FilterMode::Linear;
    sd.addressModeU = sd.addressModeV = wgpu::AddressMode::ClampToEdge;
    sampler_ = device.CreateSampler(&sd);
  }
  for (wgpu::Buffer& b : uniforms_) {
    wgpu::BufferDescriptor bd;
    bd.label = "post uniforms";
    bd.size = sizeof(Uniforms);
    bd.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    b = device.CreateBuffer(&bd);
  }
  // Four pipelines over one full-screen triangle.
  auto make = [&](const char* entry, wgpu::TextureFormat format, bool add, bool withDepth, wgpu::RenderPipeline* slot) {
    wgpu::ColorTargetState target;
    target.format = format;
    wgpu::BlendState blend;
    if (add) {
      blend.color = {wgpu::BlendOperation::Add, wgpu::BlendFactor::One, wgpu::BlendFactor::One};
      blend.alpha = {wgpu::BlendOperation::Add, wgpu::BlendFactor::One, wgpu::BlendFactor::One};
      target.blend = &blend;
    }
    wgpu::FragmentState fragment;
    fragment.module = module;
    fragment.entryPoint = entry;
    fragment.targetCount = 1;
    fragment.targets = &target;
    wgpu::DepthStencilState depth;
    depth.format = Gpu::kDepthFormat;
    depth.depthWriteEnabled = false;
    depth.depthCompare = wgpu::CompareFunction::Always;
    wgpu::RenderPipelineDescriptor desc;
    desc.label = entry;
    desc.layout = pipelines::pipelineLayout(device, layout_);
    desc.vertex.module = module;
    desc.vertex.entryPoint = "vs_full";
    desc.fragment = &fragment;
    if (withDepth) desc.depthStencil = &depth;
    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelines::create(device, desc, slot);
  };
  make("fs_prefilter", kGlowFormat, false, false, &prefilter_);
  make("fs_down", kGlowFormat, false, false, &down_);
  make("fs_up", kGlowFormat, true, false, &up_);
  make("fs_composite", gpu.surfaceFormat, false, true, &composite_);
  return true;
}

wgpu::BindGroup PostEffects::bind(const wgpu::Buffer& u, const wgpu::TextureView& a, const wgpu::TextureView& b) {
  wgpu::BindGroupEntry e[4];
  e[0].binding = 0;
  e[0].buffer = u;
  e[0].size = sizeof(Uniforms);
  e[1].binding = 1;
  e[1].textureView = a;
  e[2].binding = 2;
  e[2].textureView = b;
  e[3].binding = 3;
  e[3].sampler = sampler_;
  wgpu::BindGroupDescriptor bg;
  bg.layout = layout_;
  bg.entryCount = 4;
  bg.entries = e;
  return gpu_->device.CreateBindGroup(&bg);
}

void PostEffects::resize() {
  if (width_ == gpu_->width && height_ == gpu_->height && scene_) return;
  width_ = std::max(1u, gpu_->width);
  height_ = std::max(1u, gpu_->height);
  const wgpu::Device& device = gpu_->device;
  {
    wgpu::TextureDescriptor d;
    d.label = "scene (before glow)";
    d.size = {width_, height_, 1};
    d.format = gpu_->surfaceFormat;
    d.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    scene_ = device.CreateTexture(&d);
    sceneView_ = scene_.CreateView();
  }
  for (int i = 0; i < kLevels; ++i) {
    const uint32_t div = 4u << i;
    levelW_[i] = std::max(1u, (width_ + div - 1) / div);
    levelH_[i] = std::max(1u, (height_ + div - 1) / div);
    wgpu::TextureDescriptor d;
    d.label = "glow";
    d.size = {levelW_[i], levelH_[i], 1};
    d.format = kGlowFormat;
    d.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    levels_[i] = device.CreateTexture(&d);
    levelViews_[i] = levels_[i].CreateView();
  }
  // prefilter: scene → 0; down: 0 → 1, 1 → 2; up: 2 → 1, 1 → 0; composite: scene + 0.
  groups_[0] = bind(uniforms_[0], sceneView_, sceneView_);
  groups_[1] = bind(uniforms_[1], levelViews_[0], levelViews_[0]);
  groups_[2] = bind(uniforms_[2], levelViews_[1], levelViews_[1]);
  groups_[3] = bind(uniforms_[3], levelViews_[2], levelViews_[2]);
  groups_[4] = bind(uniforms_[4], levelViews_[1], levelViews_[1]);
  groups_[5] = bind(uniforms_[5], sceneView_, levelViews_[0]);
}

const wgpu::TextureView& PostEffects::sceneTarget() {
  resize();
  return sceneView_;
}

void PostEffects::pass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target, bool load,
                       const wgpu::RenderPipeline& pipeline, const wgpu::BindGroup& group, const char* label) {
  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = load ? wgpu::LoadOp::Load : wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = {0, 0, 0, 1};
  wgpu::RenderPassDescriptor pd;
  pd.label = label;
  pd.colorAttachmentCount = 1;
  pd.colorAttachments = &color;
  wgpu::RenderPassEncoder p = encoder.BeginRenderPass(&pd);
  p.SetPipeline(pipeline);
  p.SetBindGroup(0, group);
  p.Draw(3);
  prof::draw(3);
  p.End();
}

void PostEffects::apply(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& target,
                        const std::function<void(const wgpu::RenderPassEncoder&)>& overlay) {
  resize();
  const wgpu::Queue& queue = gpu_->queue;
  const Mood& m = mood_;
  auto write = [&](int step, uint32_t srcW, uint32_t srcH) {
    Uniforms u = {};
    u.texel[0] = 1.0f / static_cast<float>(srcW);
    u.texel[1] = 1.0f / static_cast<float>(srcH);
    u.params[0] = m.glowThreshold;
    u.params[1] = m.glowKnee;
    u.params[2] = m.glow;
    u.params[3] = m.vignette;
    u.grade[0] = m.tint.x;
    u.grade[1] = m.tint.y;
    u.grade[2] = m.tint.z;
    u.grade[3] = m.saturation;
    u.grade2[0] = m.contrast;
    u.grade2[1] = m.lift;
    queue.WriteBuffer(uniforms_[step], 0, &u, sizeof(u));
  };
  write(0, width_, height_);
  write(1, levelW_[0], levelH_[0]);
  write(2, levelW_[1], levelH_[1]);
  write(3, levelW_[2], levelH_[2]);
  write(4, levelW_[1], levelH_[1]);
  write(5, width_, height_);
  const bool glow = m.glow > 0;
  if (glow) {
    pass(encoder, levelViews_[0], false, prefilter_, groups_[0], "glow: bright parts");
    pass(encoder, levelViews_[1], false, down_, groups_[1], "glow: 1/8");
    pass(encoder, levelViews_[2], false, down_, groups_[2], "glow: 1/16");
    pass(encoder, levelViews_[1], true, up_, groups_[3], "glow: up to 1/8");
    pass(encoder, levelViews_[0], true, up_, groups_[4], "glow: up to 1/4");
  } else {
    // No glow: the composite still reads level 0, so make it black once.
    wgpu::RenderPassColorAttachment color;
    color.view = levelViews_[0];
    color.loadOp = wgpu::LoadOp::Clear;
    color.storeOp = wgpu::StoreOp::Store;
    color.clearValue = {0, 0, 0, 1};
    wgpu::RenderPassDescriptor pd;
    pd.colorAttachmentCount = 1;
    pd.colorAttachments = &color;
    encoder.BeginRenderPass(&pd).End();
  }
  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = {0, 0, 0, 1};
  wgpu::RenderPassDepthStencilAttachment depth;
  depth.view = gpu_->depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor pd;
  pd.label = "mood";
  pd.colorAttachmentCount = 1;
  pd.colorAttachments = &color;
  pd.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder p = encoder.BeginRenderPass(&pd);
  p.SetPipeline(composite_);
  p.SetBindGroup(0, groups_[5]);
  p.Draw(3);
  prof::draw(3);
  if (overlay) overlay(p);
  p.End();
}

}  // namespace engine
