#include "app/renderer.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstring>

#include "engine/file.h"
#include "engine/log.h"
#include "engine/texture.h"

using engine::Mat4;
using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

// WebGPU requires dynamic uniform offsets to be multiples of 256 bytes.
constexpr uint64_t kObjectStride = 256;
constexpr wgpu::Color kClearColor{0.02, 0.015, 0.015, 1.0};
constexpr wgpu::Color kAvatarBackdrop{0.10, 0.10, 0.13, 1.0};

wgpu::ShaderModule loadShader(const wgpu::Device& device, const char* path) {
  auto source = engine::readTextFile(path);
  if (!source) return nullptr;
  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source->c_str();
  wgpu::ShaderModuleDescriptor desc;
  desc.nextInChain = &wgsl;
  desc.label = path;
  return device.CreateShaderModule(&desc);
}

wgpu::Buffer makeUniformBuffer(const wgpu::Device& device, uint64_t size, const char* label) {
  wgpu::BufferDescriptor desc;
  desc.label = label;
  desc.size = size;
  desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  return device.CreateBuffer(&desc);
}

void setVec4(float out[4], Vec3 v, float w) {
  out[0] = v.x;
  out[1] = v.y;
  out[2] = v.z;
  out[3] = w;
}

}  // namespace

bool SceneRenderer::init(engine::Gpu& gpu, const BarScene& scene, const engine::SkinnedMeshData& avatar) {
  gpu_ = &gpu;
  const wgpu::Device& device = gpu.device;
  mirrorCount_ = scene.mirrors.size();
  if (!createScenePipelines() || !createGlassPipeline()) return false;

  createMaterials(colorMaps_, bumpMaps_, sampler_);
  for (size_t i = 0; i < 1 + mirrorCount_; ++i) {
    View v;
    v.buffer = makeUniformBuffer(device, sizeof(FrameUniforms), "frame uniforms");
    wgpu::BindGroupEntry entries[4];
    entries[0].binding = 0;
    entries[0].buffer = v.buffer;
    entries[0].size = sizeof(FrameUniforms);
    entries[1].binding = 1;
    entries[1].textureView = colorMaps_;
    entries[2].binding = 2;
    entries[2].textureView = bumpMaps_;
    entries[3].binding = 3;
    entries[3].sampler = sampler_;
    wgpu::BindGroupDescriptor desc;
    desc.layout = frameLayout_;
    desc.entryCount = 4;
    desc.entries = entries;
    v.bindGroup = device.CreateBindGroup(&desc);
    views_.push_back(v);
  }

  // Object uniforms: the scene objects (written once) followed by the avatar
  // (its position is rewritten every frame as you move).
  const auto& objects = scene.objects;
  const size_t total = objects.size() + 1 + kMaxModels;
  objectBuffer_ = makeUniformBuffer(device, total * kObjectStride, "object uniforms");
  {
    wgpu::BindGroupEntry entry;
    entry.binding = 0;
    entry.buffer = objectBuffer_;
    entry.size = sizeof(ObjectUniforms);
    wgpu::BindGroupDescriptor desc;
    desc.layout = objectLayout_;
    desc.entryCount = 1;
    desc.entries = &entry;
    objectBindGroup_ = device.CreateBindGroup(&desc);
  }
  std::vector<uint8_t> objectData(total * kObjectStride);
  for (size_t i = 0; i < objects.size(); ++i) {
    const SceneObject& o = objects[i];
    ObjectUniforms u = {};
    u.model = Mat4::transform(o.position, o.rotation);
    const engine::Material& m = material(o.material);
    setVec4(u.color, o.color, m.metallic);
    u.material[0] = o.emissive;
    u.material[1] = m.roughness;
    u.material[2] = static_cast<float>(m.texture);
    u.material[3] = o.textureScale;
    std::memcpy(objectData.data() + i * kObjectStride, &u, sizeof(u));
    objectUniforms_.push_back(u);
    sceneDraws_.push_back({engine::Mesh::upload(device, o.mesh), static_cast<uint32_t>(i * kObjectStride)});
  }
  {
    ObjectUniforms u = {};
    setVec4(u.color, {1, 1, 1}, 0);  // colors come from the mesh itself
    u.material[3] = kAvatarTextureScale;
    avatarOffset_ = static_cast<uint32_t>(objects.size() * kObjectStride);
    std::memcpy(objectData.data() + avatarOffset_, &u, sizeof(u));
    avatarMesh_ = engine::SkinnedMesh::upload(device, avatar);
  }
  loadModels(scene, objectData);
  gpu.queue.WriteBuffer(objectBuffer_, 0, objectData.data(), objectData.size());

  bonesBindGroup_ = makeBonesBindGroup(bonesBuffer_, "avatar bones");
  firstPersonBonesBindGroup_ = makeBonesBindGroup(firstPersonBonesBuffer_, "avatar bones (first person)");
  LOG_INFO("Avatar: %zu vertices.", avatar.vertices.size());

  // Lights and atmosphere don't change per frame.
  const auto lightCount = std::min<size_t>(scene.lights.size(), kMaxLights);
  if (scene.lights.size() > kMaxLights) LOG_ERROR("Too many lights; only %u used", kMaxLights);
  frame_.lightCount[0] = static_cast<uint32_t>(lightCount);
  for (size_t i = 0; i < lightCount; ++i) {
    const PointLight& l = scene.lights[i];
    setVec4(frame_.lights[i].position, l.position, l.range);
    setVec4(frame_.lights[i].color, l.color, l.intensity);
    frame_.lights[i].params[0] = l.radius;
  }
  setVec4(frame_.ambient, scene.ambient, 0);
  setVec4(frame_.fog, scene.fogColor, scene.fogDensity);

  mirrorTargets_.resize(mirrorCount_);
  for (MirrorTarget& m : mirrorTargets_) m.glassBuffer = makeUniformBuffer(device, sizeof(GlassUniforms), "glass");
  LOG_INFO("Scene: %zu objects, %zu lights, %zu mirrors.", objects.size(), lightCount, mirrorCount_);
  return true;
}

wgpu::BindGroup SceneRenderer::makeBonesBindGroup(wgpu::Buffer& buffer, const char* label) {
  buffer = makeUniformBuffer(gpu_->device, kMaxBones * sizeof(Mat4), label);
  wgpu::BindGroupEntry entry;
  entry.binding = 0;
  entry.buffer = buffer;
  entry.size = kMaxBones * sizeof(Mat4);
  wgpu::BindGroupDescriptor desc;
  desc.layout = bonesLayout_;
  desc.entryCount = 1;
  desc.entries = &entry;
  return gpu_->device.CreateBindGroup(&desc);
}

bool SceneRenderer::createScenePipelines() {
  const wgpu::Device& device = gpu_->device;
  wgpu::ShaderModule module = loadShader(device, "/shaders/scene.wgsl");
  if (!module) return false;

  // Group 0: frame uniforms + material textures. Group 1: per-object uniforms.
  wgpu::BindGroupLayoutEntry frameEntries[4];
  frameEntries[0].binding = 0;
  frameEntries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  frameEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;
  frameEntries[0].buffer.minBindingSize = sizeof(FrameUniforms);
  for (uint32_t i = 1; i <= 2; ++i) {
    frameEntries[i].binding = i;
    frameEntries[i].visibility = wgpu::ShaderStage::Fragment;
    frameEntries[i].texture.sampleType = wgpu::TextureSampleType::Float;
    frameEntries[i].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;
  }
  frameEntries[3].binding = 3;
  frameEntries[3].visibility = wgpu::ShaderStage::Fragment;
  frameEntries[3].sampler.type = wgpu::SamplerBindingType::Filtering;
  wgpu::BindGroupLayoutDescriptor frameDesc;
  frameDesc.entryCount = 4;
  frameDesc.entries = frameEntries;
  frameLayout_ = device.CreateBindGroupLayout(&frameDesc);

  wgpu::BindGroupLayoutEntry objectEntry;
  objectEntry.binding = 0;
  objectEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  objectEntry.buffer.type = wgpu::BufferBindingType::Uniform;
  objectEntry.buffer.hasDynamicOffset = true;
  objectEntry.buffer.minBindingSize = sizeof(ObjectUniforms);
  wgpu::BindGroupLayoutDescriptor objectDesc;
  objectDesc.entryCount = 1;
  objectDesc.entries = &objectEntry;
  objectLayout_ = device.CreateBindGroupLayout(&objectDesc);

  wgpu::BindGroupLayoutEntry bonesEntry;
  bonesEntry.binding = 0;
  bonesEntry.visibility = wgpu::ShaderStage::Vertex;
  bonesEntry.buffer.type = wgpu::BufferBindingType::Uniform;
  bonesEntry.buffer.minBindingSize = kMaxBones * sizeof(Mat4);
  wgpu::BindGroupLayoutDescriptor bonesDesc;
  bonesDesc.entryCount = 1;
  bonesDesc.entries = &bonesEntry;
  bonesLayout_ = device.CreateBindGroupLayout(&bonesDesc);

  // Group 2 (bones) is only used by the skinned pipeline.
  wgpu::BindGroupLayout layouts[] = {frameLayout_, objectLayout_, bonesLayout_};
  wgpu::PipelineLayoutDescriptor layoutDesc;
  layoutDesc.bindGroupLayoutCount = 2;
  layoutDesc.bindGroupLayouts = layouts;
  wgpu::PipelineLayout layout = device.CreatePipelineLayout(&layoutDesc);
  layoutDesc.bindGroupLayoutCount = 3;
  wgpu::PipelineLayout skinnedLayout = device.CreatePipelineLayout(&layoutDesc);

  wgpu::ColorTargetState colorTarget;
  colorTarget.format = gpu_->surfaceFormat;
  wgpu::FragmentState fragment;
  fragment.module = module;
  fragment.entryPoint = "fs_main";
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;
  wgpu::DepthStencilState depth;
  depth.format = engine::Gpu::kDepthFormat;
  depth.depthWriteEnabled = true;
  depth.depthCompare = wgpu::CompareFunction::Less;
  const wgpu::VertexBufferLayout vertexLayout = engine::Mesh::vertexLayout();

  wgpu::RenderPipelineDescriptor desc;
  desc.label = "scene";
  desc.layout = layout;
  desc.vertex.module = module;
  desc.vertex.entryPoint = "vs_main";
  desc.vertex.bufferCount = 1;
  desc.vertex.buffers = &vertexLayout;
  desc.fragment = &fragment;
  desc.depthStencil = &depth;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.primitive.frontFace = wgpu::FrontFace::CCW;
  desc.primitive.cullMode = wgpu::CullMode::Back;
  pipeline_ = device.CreateRenderPipeline(&desc);
  // A mirror flips the world inside out, so front faces wind the other way.
  desc.label = "scene (mirrored)";
  desc.primitive.frontFace = wgpu::FrontFace::CW;
  mirroredPipeline_ = device.CreateRenderPipeline(&desc);

  // The avatar: skinned vertices, in mirrors and in first person.
  const wgpu::VertexBufferLayout skinnedVertexLayout = engine::SkinnedMesh::vertexLayout();
  desc.label = "skinned (mirrored)";
  desc.layout = skinnedLayout;
  desc.vertex.entryPoint = "vs_skinned";
  desc.vertex.buffers = &skinnedVertexLayout;
  fragment.entryPoint = "fs_skinned";
  skinnedMirroredPipeline_ = device.CreateRenderPipeline(&desc);
  desc.label = "skinned";
  desc.primitive.frontFace = wgpu::FrontFace::CCW;
  skinnedPipeline_ = device.CreateRenderPipeline(&desc);

  // Loaded .glb models: textured with UVs; group 2 holds the material.
  modelLayout_ = engine::GpuModel::createLayout(device);
  wgpu::BindGroupLayout modelLayouts[] = {frameLayout_, objectLayout_, modelLayout_};
  layoutDesc.bindGroupLayouts = modelLayouts;
  const wgpu::VertexBufferLayout modelVertexLayout = engine::GpuModel::vertexLayout();
  desc.label = "model";
  desc.layout = device.CreatePipelineLayout(&layoutDesc);
  desc.vertex.entryPoint = "vs_model";
  desc.vertex.buffers = &modelVertexLayout;
  fragment.entryPoint = "fs_model";
  modelPipeline_ = device.CreateRenderPipeline(&desc);
  desc.label = "model (mirrored)";
  desc.primitive.frontFace = wgpu::FrontFace::CW;
  modelMirroredPipeline_ = device.CreateRenderPipeline(&desc);

  wgpu::SamplerDescriptor samplerDesc;
  samplerDesc.addressModeU = samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
  samplerDesc.magFilter = samplerDesc.minFilter = wgpu::FilterMode::Linear;
  samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
  samplerDesc.maxAnisotropy = 8;
  modelSampler_ = device.CreateSampler(&samplerDesc);
  return true;
}

bool SceneRenderer::createGlassPipeline() {
  const wgpu::Device& device = gpu_->device;
  wgpu::ShaderModule module = loadShader(device, "/shaders/mirror.wgsl");
  if (!module) return false;
  wgpu::ColorTargetState colorTarget;
  colorTarget.format = gpu_->surfaceFormat;
  wgpu::FragmentState fragment;
  fragment.module = module;
  fragment.entryPoint = "fs_main";
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;
  wgpu::DepthStencilState depth;
  depth.format = engine::Gpu::kDepthFormat;
  depth.depthWriteEnabled = true;
  depth.depthCompare = wgpu::CompareFunction::Less;
  wgpu::RenderPipelineDescriptor desc;
  desc.label = "mirror glass";
  desc.vertex.module = module;
  desc.vertex.entryPoint = "vs_main";
  desc.fragment = &fragment;
  desc.depthStencil = &depth;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.primitive.cullMode = wgpu::CullMode::None;
  glassPipeline_ = device.CreateRenderPipeline(&desc);
  return true;
}

// Generates the bar's surface textures and uploads them as two texture
// arrays (colors, bumps).
void SceneRenderer::createMaterials(wgpu::TextureView& colorMaps, wgpu::TextureView& bumpMaps,
                                    wgpu::Sampler& sampler) {
  const double start = emscripten_get_now();
  std::vector<engine::Image> colors, bumps;
  for (engine::MaterialImages& m : buildBarTextures()) {
    colors.push_back(std::move(m.color));
    bumps.push_back(std::move(m.bump));
  }
  wgpu::TextureViewDescriptor viewDesc;
  viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
  colorMaps = engine::createTextureArray(gpu_->device, colors, true, "material colors").CreateView(&viewDesc);
  bumpMaps = engine::createTextureArray(gpu_->device, bumps, false, "material bumps").CreateView(&viewDesc);

  wgpu::SamplerDescriptor samplerDesc;
  samplerDesc.addressModeU = samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
  samplerDesc.magFilter = samplerDesc.minFilter = wgpu::FilterMode::Linear;
  samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
  samplerDesc.maxAnisotropy = 4;  // keeps the floor sharp at shallow angles
  sampler = gpu_->device.CreateSampler(&samplerDesc);
  LOG_INFO("Textures: %zu materials generated in %.0f ms.", colors.size(), emscripten_get_now() - start);
}

void SceneRenderer::setGlow(const Interactable& it, float glow) {
  for (size_t i = it.firstObject; i < it.firstObject + it.objectCount; ++i) {
    ObjectUniforms u = objectUniforms_[i];
    u.material[0] += glow;
    gpu_->queue.WriteBuffer(objectBuffer_, i * kObjectStride, &u, sizeof(u));
  }
}

// Reflection textures match the screen, so the glass can read them 1:1.
void SceneRenderer::resizeMirrorTargets() {
  if (targetW_ == gpu_->width && targetH_ == gpu_->height) return;
  targetW_ = gpu_->width;
  targetH_ = gpu_->height;
  for (MirrorTarget& m : mirrorTargets_) {
    wgpu::TextureDescriptor desc;
    desc.label = "mirror reflection";
    desc.size = {targetW_, targetH_, 1};
    desc.format = gpu_->surfaceFormat;
    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m.texture = gpu_->device.CreateTexture(&desc);
    m.view = m.texture.CreateView();
    wgpu::BindGroupEntry entries[2];
    entries[0].binding = 0;
    entries[0].buffer = m.glassBuffer;
    entries[0].size = sizeof(GlassUniforms);
    entries[1].binding = 1;
    entries[1].textureView = m.view;
    wgpu::BindGroupDescriptor bg;
    bg.layout = glassPipeline_.GetBindGroupLayout(0);
    bg.entryCount = 2;
    bg.entries = entries;
    m.glassBindGroup = gpu_->device.CreateBindGroup(&bg);
  }
}

void SceneRenderer::drawItems(const wgpu::RenderPassEncoder& pass, const std::vector<DrawItem>& items) const {
  for (const DrawItem& d : items) {
    pass.SetBindGroup(1, objectBindGroup_, 1, &d.uniformOffset);
    pass.SetVertexBuffer(0, d.mesh.vertexBuffer);
    pass.SetIndexBuffer(d.mesh.indexBuffer, wgpu::IndexFormat::Uint32);
    pass.DrawIndexed(d.mesh.indexCount);
  }
}

// Each placement gets an object slot (its position/rotation/scale) right
// away; the model itself streams in and is drawn once it's ready.
void SceneRenderer::loadModels(const BarScene& scene, std::vector<uint8_t>& objectData) {
  const size_t firstSlot = scene.objects.size() + 1;  // after the objects and the avatar
  for (size_t i = 0; i < scene.models.size() && i < kMaxModels; ++i) {
    const ModelPlacement& p = scene.models[i];
    ObjectUniforms u = {};
    u.model = Mat4::transform(p.position, p.rotation);
    for (int c = 0; c < 3; ++c) {
      for (int r = 0; r < 3; ++r) u.model.at(c, r) *= p.scale;
    }
    setVec4(u.color, {1, 1, 1}, 0);
    const auto offset = static_cast<uint32_t>((firstSlot + i) * kObjectStride);
    std::memcpy(objectData.data() + offset, &u, sizeof(u));
    models_.push_back({nullptr, offset});
    engine::GpuModel::load(gpu_->device, modelLayout_, modelSampler_, "assets/models/" + p.file,
                           [this, i](std::shared_ptr<engine::GpuModel> model) {
                             models_[i].gpu = std::move(model);
                             if (models_[i].gpu) ++modelsLoaded_;
                           });
  }
  if (scene.models.size() > kMaxModels) LOG_ERROR("Too many models; only %zu used", kMaxModels);
}

void SceneRenderer::drawModels(const wgpu::RenderPassEncoder& pass) const {
  for (const ModelDraw& m : models_) {
    if (!m.gpu) continue;
    pass.SetBindGroup(1, objectBindGroup_, 1, &m.uniformOffset);
    for (const engine::GpuModel::Part& part : m.gpu->parts) {
      pass.SetBindGroup(2, m.gpu->bindGroups[part.material]);
      pass.SetVertexBuffer(0, part.vertexBuffer);
      pass.SetIndexBuffer(part.indexBuffer, wgpu::IndexFormat::Uint32);
      pass.DrawIndexed(part.indexCount);
    }
  }
}

void SceneRenderer::drawAvatar(const wgpu::RenderPassEncoder& pass, const wgpu::BindGroup& bones) const {
  pass.SetBindGroup(1, objectBindGroup_, 1, &avatarOffset_);
  pass.SetBindGroup(2, bones);
  pass.SetVertexBuffer(0, avatarMesh_.vertexBuffer);
  pass.SetIndexBuffer(avatarMesh_.indexBuffer, wgpu::IndexFormat::Uint32);
  pass.DrawIndexed(avatarMesh_.indexCount);
}

void SceneRenderer::renderMirror(const wgpu::CommandEncoder& encoder, size_t index, const Mirror& mirror,
                                 const Mat4& viewProj, Vec3 eye) {
  // Only when you're in front of the glass and it's on screen.
  if (!mirror.on || engine::dot(mirror.normal, eye - mirror.center) <= 0.01f) return;
  const float W = static_cast<float>(targetW_), H = static_cast<float>(targetH_);
  float minX = W, minY = H, maxX = 0, maxY = 0;
  bool behindCamera = false;
  for (float sx : {-1.0f, 1.0f}) {
    for (float sy : {-1.0f, 1.0f}) {
      const Vec3 corner = mirror.center + mirror.right * (sx * mirror.halfWidth) + mirror.up * (sy * mirror.halfHeight);
      const Vec4 c = engine::transformPoint(viewProj, corner);
      if (c.w <= 0.01f) {
        behindCamera = true;
        continue;
      }
      const float px = (c.x / c.w * 0.5f + 0.5f) * W, py = (0.5f - c.y / c.w * 0.5f) * H;
      minX = std::min(minX, px);
      maxX = std::max(maxX, px);
      minY = std::min(minY, py);
      maxY = std::max(maxY, py);
    }
  }
  if (behindCamera) {  // partly behind you: just use the whole screen
    minX = minY = 0;
    maxX = W;
    maxY = H;
  }
  const auto x0 = static_cast<uint32_t>(std::clamp(std::floor(minX), 0.0f, W));
  const auto y0 = static_cast<uint32_t>(std::clamp(std::floor(minY), 0.0f, H));
  const auto x1 = static_cast<uint32_t>(std::clamp(std::ceil(maxX), 0.0f, W));
  const auto y1 = static_cast<uint32_t>(std::clamp(std::ceil(maxY), 0.0f, H));
  if (x1 <= x0 || y1 <= y0) return;  // off screen

  // The reflected camera: the world mirrored across the glass, viewed by you.
  FrameUniforms f = frame_;
  f.viewProj = viewProj * Mat4::reflection(mirror.normal, mirror.center);
  const Vec4 mirroredEye = engine::transformPoint(Mat4::reflection(mirror.normal, mirror.center), eye);
  setVec4(f.cameraPos, {mirroredEye.x, mirroredEye.y, mirroredEye.z}, 1);
  setVec4(f.clipPlane, mirror.normal, -engine::dot(mirror.normal, mirror.center));
  const bool full = mirror.mode == MirrorMode::kFull;
  if (!full) {
    // Like a photo studio: avatars get extra even light on the plain backdrop.
    for (int c = 0; c < 3; ++c) f.ambient[c] = f.ambient[c] * 3.0f + 0.12f;
  }
  const View& view = views_[1 + index];
  gpu_->queue.WriteBuffer(view.buffer, 0, &f, sizeof(f));

  wgpu::RenderPassColorAttachment color;
  color.view = mirrorTargets_[index].view;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = full ? kClearColor : kAvatarBackdrop;
  wgpu::RenderPassDepthStencilAttachment depth;
  depth.view = gpu_->depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;
  passDesc.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  pass.SetScissorRect(x0, y0, x1 - x0, y1 - y0);
  pass.SetPipeline(mirroredPipeline_);
  pass.SetBindGroup(0, view.bindGroup);
  if (full) {
    drawItems(pass, sceneDraws_);
    pass.SetPipeline(modelMirroredPipeline_);
    drawModels(pass);
  }
  pass.SetPipeline(skinnedMirroredPipeline_);
  drawAvatar(pass, bonesBindGroup_);
  pass.End();
}

void SceneRenderer::render(const wgpu::TextureView& target, const Mat4& viewProj, Vec3 eye, const Mat4& avatar,
                           const std::vector<Mat4>& avatarBones, const std::vector<Mat4>& firstPersonBones,
                           const std::vector<Mirror>& mirrors, const engine::TextRenderer& text) {
  const wgpu::Queue& queue = gpu_->queue;
  resizeMirrorTargets();

  // The avatar follows you. Only its model matrix changes (it comes first in
  // ObjectUniforms), plus the bone matrices for its pose.
  queue.WriteBuffer(objectBuffer_, avatarOffset_, &avatar, sizeof(avatar));
  const size_t boneCount = std::min<size_t>(avatarBones.size(), kMaxBones);
  if (boneCount) queue.WriteBuffer(bonesBuffer_, 0, avatarBones.data(), boneCount * sizeof(Mat4));
  const size_t fpCount = std::min<size_t>(firstPersonBones.size(), kMaxBones);
  if (fpCount) queue.WriteBuffer(firstPersonBonesBuffer_, 0, firstPersonBones.data(), fpCount * sizeof(Mat4));

  FrameUniforms f = frame_;
  f.viewProj = viewProj;
  setVec4(f.cameraPos, eye, 1);
  setVec4(f.clipPlane, {0, 0, 0}, 1);  // keep everything
  queue.WriteBuffer(views_[0].buffer, 0, &f, sizeof(f));

  wgpu::CommandEncoder encoder = gpu_->device.CreateCommandEncoder();
  for (size_t i = 0; i < mirrorCount_ && i < mirrors.size(); ++i) renderMirror(encoder, i, mirrors[i], viewProj, eye);

  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = kClearColor;
  wgpu::RenderPassDepthStencilAttachment depth;
  depth.view = gpu_->depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;
  passDesc.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  pass.SetPipeline(pipeline_);
  pass.SetBindGroup(0, views_[0].bindGroup);
  drawItems(pass, sceneDraws_);
  pass.SetPipeline(modelPipeline_);
  drawModels(pass);
  pass.SetPipeline(skinnedPipeline_);
  drawAvatar(pass, firstPersonBonesBindGroup_);  // your body, minus the head

  // Mirror glass.
  pass.SetPipeline(glassPipeline_);
  for (size_t i = 0; i < mirrorCount_ && i < mirrors.size(); ++i) {
    const Mirror& m = mirrors[i];
    GlassUniforms g = {};
    g.viewProj = viewProj;
    setVec4(g.center, m.center, 1);
    setVec4(g.right, m.right * m.halfWidth, 0);
    setVec4(g.up, m.up * m.halfHeight, 0);
    g.params[0] = m.on ? 1.0f : 0.0f;
    queue.WriteBuffer(mirrorTargets_[i].glassBuffer, 0, &g, sizeof(g));
    pass.SetBindGroup(0, mirrorTargets_[i].glassBindGroup);
    pass.Draw(6);
  }

  text.draw(pass);  // after the scene: text is see-through and on top
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace app
