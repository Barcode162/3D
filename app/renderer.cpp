#include "app/renderer.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

#include "engine/file.h"
#include "engine/log.h"
#include "engine/pipeline_cache.h"
#include "engine/profiler.h"
#include "engine/texture.h"

using engine::Mat4;
using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

// General 4x4 inverse (Gauss-Jordan in double precision).
Mat4 inverse(const Mat4& m) {
  double a[4][8];
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      a[r][c] = m.at(c, r);
      a[r][c + 4] = r == c ? 1.0 : 0.0;
    }
  }
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    for (int r = col + 1; r < 4; ++r) {
      if (std::fabs(a[r][col]) > std::fabs(a[pivot][col])) pivot = r;
    }
    for (int c = 0; c < 8; ++c) std::swap(a[col][c], a[pivot][c]);
    const double p = a[col][col];
    if (std::fabs(p) < 1e-12) return Mat4{};
    for (int c = 0; c < 8; ++c) a[col][c] /= p;
    for (int r = 0; r < 4; ++r) {
      if (r == col) continue;
      const double f = a[r][col];
      for (int c = 0; c < 8; ++c) a[r][c] -= f * a[col][c];
    }
  }
  Mat4 out;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) out.at(c, r) = static_cast<float>(a[r][c + 4]);
  }
  return out;
}

// Tilts the near plane of world->clip matrix `m` (depth 0..1) onto `plane`
// (world space, a·x + b·y + c·z + d >= 0 is kept), so the GPU clips away
// everything on the other side (Lengyel's oblique near plane). Used for
// mirrors instead of a discard in the shader.
Mat4 obliqueNearPlane(const Mat4& m, const float plane[4]) {
  const Mat4 inv = inverse(m);
  // The plane in clip space (its x and y signs pick the far corner below).
  float clip[4];
  for (int j = 0; j < 4; ++j) {
    clip[j] = 0;
    for (int i = 0; i < 4; ++i) clip[j] += plane[i] * inv.at(j, i);
  }
  // The far corner of the view on the kept side, back in world space.
  const float corner[4] = {clip[0] >= 0 ? 1.0f : -1.0f, clip[1] >= 0 ? 1.0f : -1.0f, 1.0f, 1.0f};
  float q[4];
  for (int r = 0; r < 4; ++r) {
    q[r] = 0;
    for (int c = 0; c < 4; ++c) q[r] += inv.at(c, r) * corner[c];
  }
  const float s = plane[0] * q[0] + plane[1] * q[1] + plane[2] * q[2] + plane[3] * q[3];
  if (std::fabs(s) < 1e-12f) return m;
  Mat4 out = m;
  for (int c = 0; c < 4; ++c) out.at(c, 2) = plane[c] / s;  // the depth row
  return out;
}

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

Mat4 modelMatrix(Vec3 position, engine::Quat rotation, float scale) {
  Mat4 m = Mat4::transform(position, rotation);
  for (int c = 0; c < 3; ++c) {
    for (int r = 0; r < 3; ++r) m.at(c, r) *= scale;
  }
  return m;
}

void setVec4(float out[4], Vec3 v, float w) {
  out[0] = v.x;
  out[1] = v.y;
  out[2] = v.z;
  out[3] = w;
}

}  // namespace

bool SceneRenderer::init(engine::Gpu& gpu, const BarScene& scene, const std::vector<engine::SkinnedMeshData>& avatars) {
  gpu_ = &gpu;
  const wgpu::Device& device = gpu.device;
  mirrorCount_ = scene.mirrors.size();
  if (!createScenePipelines() || !createGlassPipeline()) return false;
  engine::profiler().stage("pipelines");

  // Object uniforms: the scene objects (written once) followed by the avatar
  // (its position is rewritten every frame as you move), the other players'
  // avatars and the models.
  const auto& objects = scene.objects;
  const size_t total = objects.size() + 1 + kMaxOthers + kMaxModels + kMaxBoxes;
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
    for (const engine::SkinnedMeshData& a : avatars) avatarMeshes_.push_back(engine::SkinnedMesh::upload(device, a));
    for (size_t i = 0; i < kMaxOthers; ++i) {
      OtherSlot slot;
      slot.offset = static_cast<uint32_t>((objects.size() + 1 + i) * kObjectStride);
      std::memcpy(objectData.data() + slot.offset, &u, sizeof(u));
      others_.push_back(slot);
    }
  }
  loadModels(scene, objectData);
  // The model downloads are on their way; make the avatar textures meanwhile.
  createMaterials(colorMaps_, bumpMaps_, sampler_);
  engine::profiler().stage("textures");
  buildLighting(scene);
  for (size_t i = 0; i < 1 + mirrorCount_; ++i) {
    View v;
    v.buffer = makeUniformBuffer(device, sizeof(FrameUniforms), "frame uniforms");
    wgpu::BindGroupEntry entries[5];
    entries[0].binding = 0;
    entries[0].buffer = v.buffer;
    entries[0].size = sizeof(FrameUniforms);
    entries[1].binding = 1;
    entries[1].textureView = colorMaps_;
    entries[2].binding = 2;
    entries[2].textureView = bumpMaps_;
    entries[3].binding = 3;
    entries[3].sampler = sampler_;
    entries[4].binding = 4;
    entries[4].buffer = lightingBuffer_;
    entries[4].size = sizeof(LightingUniforms);
    wgpu::BindGroupDescriptor desc;
    desc.layout = frameLayout_;
    desc.entryCount = 5;
    desc.entries = entries;
    v.bindGroup = device.CreateBindGroup(&desc);
    views_.push_back(v);
  }

  boxOffset_ = static_cast<uint32_t>((objects.size() + 1 + kMaxOthers + kMaxModels) * kObjectStride);
  boxMesh_ = engine::Mesh::upload(device, engine::makeBox({1, 1, 1}));
  gpu.queue.WriteBuffer(objectBuffer_, 0, objectData.data(), objectData.size());

  bonesBindGroup_ = makeBonesBindGroup(bonesBuffer_, "avatar bones");
  firstPersonBonesBindGroup_ = makeBonesBindGroup(firstPersonBonesBuffer_, "avatar bones (first person)");
  for (OtherSlot& slot : others_) slot.bonesBindGroup = makeBonesBindGroup(slot.bonesBuffer, "other player bones");
  LOG_INFO("Avatars: %zu looks, %zu vertices each.", avatars.size(), avatars.empty() ? 0 : avatars[0].vertices.size());

  // Atmosphere doesn't change per frame.
  const size_t lightCount = std::min<size_t>(scene.lights.size(), kMaxLights);
  setVec4(frame_.ambient, scene.ambient, 0);
  setVec4(frame_.fog, scene.fogColor, scene.fogDensity);

  mirrorTargets_.resize(mirrorCount_);
  for (MirrorTarget& m : mirrorTargets_) m.glassBuffer = makeUniformBuffer(device, sizeof(GlassUniforms), "glass");
  LOG_INFO("Scene: %zu objects, %zu lights, %zu mirrors.", objects.size(), lightCount, mirrorCount_);
  return true;
}

// Fills the light list and the light grid (see LightingUniforms). A light
// reaches a cell if its range touches the cell's square; when more than 8
// do, the 8 brightest there win (logged, so the layout can be tuned).
void SceneRenderer::buildLighting(const BarScene& scene) {
  auto l = std::make_unique<LightingUniforms>();
  std::memset(l.get(), 0, sizeof(LightingUniforms));
  const size_t count = std::min<size_t>(scene.lights.size(), kMaxLights);
  if (scene.lights.size() > kMaxLights) LOG_ERROR("Too many lights; only %u used", kMaxLights);
  l->lightCount[0] = static_cast<uint32_t>(count);
  for (size_t i = 0; i < count; ++i) {
    const PointLight& p = scene.lights[i];
    setVec4(l->lights[i].position, p.position, p.range);
    setVec4(l->lights[i].color, p.color, p.intensity);
    l->lights[i].params[0] = p.radius;
    lightIntensity_.push_back(p.intensity);
  }
  // Cells cover the walkable area (blockers included) with at most 16 x 16.
  float x0 = scene.lightGridMin.x, z0 = scene.lightGridMin.z;
  const float spanX = scene.lightGridMax.x - x0, spanZ = scene.lightGridMax.z - z0;
  const float cell = std::max({spanX, spanZ, 1.0f}) / 16.0f;
  const auto nx = static_cast<uint32_t>(std::ceil(spanX / cell)), nz = static_cast<uint32_t>(std::ceil(spanZ / cell));
  l->grid[0] = x0;
  l->grid[1] = z0;
  l->grid[2] = 1.0f / cell;
  l->gridSize[0] = nx;
  l->gridSize[1] = nz;
  int crowded = 0;
  for (uint32_t cz = 0; cz < nz; ++cz) {
    for (uint32_t cx = 0; cx < nx; ++cx) {
      const float minX = x0 + cx * cell, minZ = z0 + cz * cell;
      std::vector<std::pair<float, uint32_t>> reach;  // (strength here, light)
      for (size_t i = 0; i < count; ++i) {
        const PointLight& p = scene.lights[i];
        const float dx = p.position.x - std::clamp(p.position.x, minX, minX + cell);
        const float dz = p.position.z - std::clamp(p.position.z, minZ, minZ + cell);
        const float d2 = dx * dx + dz * dz;
        if (d2 >= p.range * p.range) continue;
        reach.push_back({p.intensity / (d2 + 1.0f), static_cast<uint32_t>(i)});
      }
      std::sort(reach.begin(), reach.end(), [](auto a, auto b) { return a.first > b.first; });
      if (reach.size() > kLightsPerCell) ++crowded;
      const size_t n = std::min<size_t>(reach.size(), kLightsPerCell);
      uint32_t* c = l->cells[cz * nx + cx];
      for (size_t k = 0; k < n; ++k) c[k / 4] |= reach[k].second << (8 * (k % 4));
      c[2] = static_cast<uint32_t>(n);
    }
  }
  lightingBuffer_ = makeUniformBuffer(gpu_->device, sizeof(LightingUniforms), "lighting");
  gpu_->queue.WriteBuffer(lightingBuffer_, 0, l.get(), sizeof(LightingUniforms));
  LOG_INFO("Lights: %zu in a %ux%u grid of %.2f m cells%s.", count, nx, nz, cell,
           crowded ? " (some cells have more than 8; the faintest are left out)" : "");
  if (crowded) LOG_INFO("Crowded light cells: %d.", crowded);
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
  wgpu::BindGroupLayoutEntry frameEntries[5];
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
  frameEntries[4].binding = 4;
  frameEntries[4].visibility = wgpu::ShaderStage::Fragment;
  frameEntries[4].buffer.type = wgpu::BufferBindingType::Uniform;
  frameEntries[4].buffer.minBindingSize = sizeof(LightingUniforms);
  wgpu::BindGroupLayoutDescriptor frameDesc;
  frameDesc.entryCount = 5;
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
  engine::pipelines::create(device, desc, &pipeline_);
  // A mirror flips the world inside out, so front faces wind the other way.
  desc.label = "scene (mirrored)";
  desc.primitive.frontFace = wgpu::FrontFace::CW;
  engine::pipelines::create(device, desc, &mirroredPipeline_);

  // The avatar: skinned vertices, in mirrors and in first person.
  const wgpu::VertexBufferLayout skinnedVertexLayout = engine::SkinnedMesh::vertexLayout();
  desc.label = "skinned (mirrored)";
  desc.layout = skinnedLayout;
  desc.vertex.entryPoint = "vs_skinned";
  desc.vertex.buffers = &skinnedVertexLayout;
  fragment.entryPoint = "fs_skinned";
  engine::pipelines::create(device, desc, &skinnedMirroredPipeline_);
  desc.label = "skinned";
  desc.primitive.frontFace = wgpu::FrontFace::CCW;
  engine::pipelines::create(device, desc, &skinnedPipeline_);

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
  engine::pipelines::create(device, desc, &modelPipeline_);
  desc.label = "model (mirrored)";
  desc.primitive.frontFace = wgpu::FrontFace::CW;
  engine::pipelines::create(device, desc, &modelMirroredPipeline_);

  // See-through parts (glass): premultiplied alpha blending, tested against
  // the depth buffer but not written to it, drawn back to front.
  wgpu::BlendState blend;
  blend.color = {wgpu::BlendOperation::Add, wgpu::BlendFactor::One, wgpu::BlendFactor::OneMinusSrcAlpha};
  blend.alpha = {wgpu::BlendOperation::Add, wgpu::BlendFactor::One, wgpu::BlendFactor::OneMinusSrcAlpha};
  colorTarget.blend = &blend;
  depth.depthWriteEnabled = false;
  fragment.entryPoint = "fs_model_blend";
  desc.label = "model see-through (mirrored)";
  engine::pipelines::create(device, desc, &modelBlendMirroredPipeline_);
  desc.label = "model see-through";
  desc.primitive.frontFace = wgpu::FrontFace::CCW;
  engine::pipelines::create(device, desc, &modelBlendPipeline_);

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
  {  // uniform + reflection texture + a filtering sampler (the texture is half size)
    wgpu::BindGroupLayoutEntry e[3];
    e[0].binding = 0;
    e[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    e[0].buffer.type = wgpu::BufferBindingType::Uniform;
    e[1].binding = 1;
    e[1].visibility = wgpu::ShaderStage::Fragment;
    e[1].texture.sampleType = wgpu::TextureSampleType::Float;
    e[2].binding = 2;
    e[2].visibility = wgpu::ShaderStage::Fragment;
    e[2].sampler.type = wgpu::SamplerBindingType::Filtering;
    wgpu::BindGroupLayoutDescriptor ld;
    ld.entryCount = 3;
    ld.entries = e;
    glassLayout_ = device.CreateBindGroupLayout(&ld);
    wgpu::SamplerDescriptor sd;
    sd.magFilter = sd.minFilter = wgpu::FilterMode::Linear;
    mirrorSampler_ = device.CreateSampler(&sd);
  }
  desc.layout = engine::pipelines::pipelineLayout(device, glassLayout_);
  desc.vertex.module = module;
  desc.vertex.entryPoint = "vs_main";
  desc.fragment = &fragment;
  desc.depthStencil = &depth;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.primitive.cullMode = wgpu::CullMode::None;
  engine::pipelines::create(device, desc, &glassPipeline_);
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
  for (size_t i = it.firstModel; i < it.firstModel + it.modelCount && i < models_.size(); ++i) {
    modelHighlight_[i] = glow;
    writeModelGlow(i);
  }
}

void SceneRenderer::setLightScale(size_t light, float scale) {
  if (light >= lightIntensity_.size()) return;
  const float intensity = lightIntensity_[light] * scale;
  gpu_->queue.WriteBuffer(lightingBuffer_,
                          offsetof(LightingUniforms, lights) + light * sizeof(GpuLight) + offsetof(GpuLight, color) +
                              3 * sizeof(float),
                          &intensity, sizeof(intensity));
}

void SceneRenderer::setModelBaseGlow(size_t model, float glow) {
  if (model >= models_.size()) return;
  modelBaseGlow_[model] = glow;
  writeModelGlow(model);
}

// Models: only the glow (material.x) changes; their other values are zero.
void SceneRenderer::writeModelGlow(size_t i) {
  const float material[4] = {modelBaseGlow_[i] + modelHighlight_[i], 0, 0, 0};
  gpu_->queue.WriteBuffer(objectBuffer_, models_[i].uniformOffset + offsetof(ObjectUniforms, material), material,
                          sizeof(material));
}

// Reflection textures match the screen, so the glass can read them 1:1.
// Reflections render at half the screen's resolution (a quarter of the
// pixels); the glass shows them filtered.
void SceneRenderer::resizeMirrorTargets() {
  if (targetW_ == gpu_->width && targetH_ == gpu_->height) return;
  targetW_ = gpu_->width;
  targetH_ = gpu_->height;
  mirrorW_ = std::max(1u, (targetW_ + 1) / 2);
  mirrorH_ = std::max(1u, (targetH_ + 1) / 2);
  {
    wgpu::TextureDescriptor dd;
    dd.label = "mirror depth";
    dd.size = {mirrorW_, mirrorH_, 1};
    dd.format = engine::Gpu::kDepthFormat;
    dd.usage = wgpu::TextureUsage::RenderAttachment;
    mirrorDepth_ = gpu_->device.CreateTexture(&dd);
    mirrorDepthView_ = mirrorDepth_.CreateView();
  }
  for (MirrorTarget& m : mirrorTargets_) {
    wgpu::TextureDescriptor desc;
    desc.label = "mirror reflection";
    desc.size = {mirrorW_, mirrorH_, 1};
    desc.format = gpu_->surfaceFormat;
    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    m.texture = gpu_->device.CreateTexture(&desc);
    m.view = m.texture.CreateView();
    wgpu::BindGroupEntry entries[3];
    entries[0].binding = 0;
    entries[0].buffer = m.glassBuffer;
    entries[0].size = sizeof(GlassUniforms);
    entries[1].binding = 1;
    entries[1].textureView = m.view;
    entries[2].binding = 2;
    entries[2].sampler = mirrorSampler_;
    wgpu::BindGroupDescriptor bg;
    bg.layout = glassLayout_;
    bg.entryCount = 3;
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
    engine::prof::draw(d.mesh.indexCount);
  }
}

// Each placement gets an object slot (its position/rotation/scale) right
// away; the model itself streams in and is drawn once it's ready. Each file
// is downloaded once, however many times it's placed.
void SceneRenderer::loadModels(const BarScene& scene, std::vector<uint8_t>& objectData) {
  const size_t firstSlot = scene.objects.size() + 1 + kMaxOthers;  // after the objects and the avatars
  std::vector<std::string> files;
  std::vector<std::vector<size_t>> users;
  for (size_t i = 0; i < scene.models.size() && i < kMaxModels; ++i) {
    const ModelPlacement& p = scene.models[i];
    const auto offset = static_cast<uint32_t>((firstSlot + i) * kObjectStride);
    models_.push_back({nullptr, offset, p.position, p.scale});
    ObjectUniforms u = {};
    u.model = modelMatrix(p.position, p.rotation, p.scale);
    setVec4(u.color, {1, 1, 1}, 0);
    std::memcpy(objectData.data() + offset, &u, sizeof(u));
    const auto it = std::find(files.begin(), files.end(), p.file);
    if (it == files.end()) {
      files.push_back(p.file);
      users.push_back({i});
    } else {
      users[static_cast<size_t>(it - files.begin())].push_back(i);
    }
  }
  if (scene.models.size() > kMaxModels) LOG_ERROR("Too many models; only %zu used", kMaxModels);
  modelBaseGlow_.assign(models_.size(), 0.0f);
  modelHighlight_.assign(models_.size(), 0.0f);
  // Files are requested in placement order, so the room shell (placed
  // first) arrives first.
  fileProgress_.assign(files.size(), 0.0f);
  fileNames_ = files;
  for (size_t f = 0; f < files.size(); ++f) {
    engine::GpuModel::load(
        gpu_->device, modelLayout_, modelSampler_, "assets/models/" + files[f] + ".gz",
        [this, f, placements = users[f]](std::shared_ptr<engine::GpuModel> model) {
          fileProgress_[f] = 1.0f;
          ++filesDone_;
          if (!model) {
            ++filesFailed_;
            // The room shell: without it there's no bar to enter.
            if (fileNames_[f].find("hall") != std::string::npos) criticalFailed_ = true;
            return;
          }
          for (size_t i : placements) models_[i].gpu = model;
          modelsLoaded_ += placements.size();
        },
        // Downloading is most of the wait; decoding textures is the rest.
        [this, f](float fraction) { fileProgress_[f] = std::max(fileProgress_[f], 0.85f * fraction); });
  }
}

float SceneRenderer::loadProgress() const {
  if (fileProgress_.empty()) return 1.0f;
  float sum = 0;
  for (float p : fileProgress_) sum += p;
  return sum / static_cast<float>(fileProgress_.size());
}

void SceneRenderer::setModelTransform(size_t index, Vec3 position, engine::Quat rotation) {
  if (index >= models_.size()) return;
  ModelDraw& m = models_[index];
  m.position = position;
  const Mat4 model = modelMatrix(position, rotation, m.scale);
  // Only the matrix (the start of ObjectUniforms) changes.
  gpu_->queue.WriteBuffer(objectBuffer_, m.uniformOffset, &model, sizeof(model));
}

// Solid parts in any order; see-through parts sorted farthest first so
// nearer glass blends over farther glass.
void SceneRenderer::drawModels(const wgpu::RenderPassEncoder& pass, bool blend, Vec3 eye) const {
  std::vector<const ModelDraw*> order;
  for (const ModelDraw& m : models_) {
    if (m.gpu && (!blend || m.gpu->hasBlend)) order.push_back(&m);
  }
  if (blend) {
    std::sort(order.begin(), order.end(), [eye](const ModelDraw* a, const ModelDraw* b) {
      const Vec3 da = a->position - eye, db = b->position - eye;
      return engine::dot(da, da) > engine::dot(db, db);
    });
  }
  for (const ModelDraw* m : order) {
    pass.SetBindGroup(1, objectBindGroup_, 1, &m->uniformOffset);
    for (const engine::GpuModel::Part& part : m->gpu->parts) {
      if (part.blend != blend) continue;
      pass.SetBindGroup(2, m->gpu->bindGroups[part.material]);
      pass.SetVertexBuffer(0, part.vertexBuffer);
      pass.SetIndexBuffer(part.indexBuffer, wgpu::IndexFormat::Uint32);
      pass.DrawIndexed(part.indexCount);
      engine::prof::draw(part.indexCount);
    }
  }
}

void SceneRenderer::drawSkinned(const wgpu::RenderPassEncoder& pass, const engine::SkinnedMesh& mesh) const {
  pass.SetVertexBuffer(0, mesh.vertexBuffer);
  pass.SetIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32);
  pass.DrawIndexed(mesh.indexCount);
  engine::prof::draw(mesh.indexCount);
}

void SceneRenderer::drawAvatar(const wgpu::RenderPassEncoder& pass, const wgpu::BindGroup& bones) const {
  if (avatarMeshes_.empty()) return;
  pass.SetBindGroup(1, objectBindGroup_, 1, &avatarOffset_);
  pass.SetBindGroup(2, bones);
  drawSkinned(pass, avatarMeshes_[selfLook_]);
}

void SceneRenderer::setOthers(const std::vector<Mat4>& transforms, const std::vector<std::vector<Mat4>>& bones,
                              const std::vector<int>& looks) {
  othersShown_ = std::min({transforms.size(), bones.size(), others_.size()});
  for (size_t i = 0; i < othersShown_; ++i) {
    gpu_->queue.WriteBuffer(objectBuffer_, others_[i].offset, &transforms[i], sizeof(Mat4));
    const size_t count = std::min<size_t>(bones[i].size(), kMaxBones);
    if (count) gpu_->queue.WriteBuffer(others_[i].bonesBuffer, 0, bones[i].data(), count * sizeof(Mat4));
    const int look = i < looks.size() ? looks[i] : 0;
    others_[i].look = look >= 0 && static_cast<size_t>(look) < avatarMeshes_.size() ? look : 0;
  }
}

void SceneRenderer::drawOthers(const wgpu::RenderPassEncoder& pass) const {
  for (size_t i = 0; i < othersShown_ && !avatarMeshes_.empty(); ++i) {
    pass.SetBindGroup(1, objectBindGroup_, 1, &others_[i].offset);
    pass.SetBindGroup(2, others_[i].bonesBindGroup);
    drawSkinned(pass, avatarMeshes_[static_cast<size_t>(others_[i].look)]);
  }
}

void SceneRenderer::setBoxes(const std::vector<Box>& boxes) {
  boxCount_ = std::min(boxes.size(), kMaxBoxes);
  if (!boxCount_) return;
  static std::vector<uint8_t> data;
  data.assign(boxCount_ * kObjectStride, 0);
  for (size_t i = 0; i < boxCount_; ++i) {
    const Box& b = boxes[i];
    ObjectUniforms u = {};
    u.model = Mat4::transform(b.center, b.rotation);
    for (int r = 0; r < 3; ++r) {  // scale the unit cube to the box size
      u.model.at(0, r) *= b.size.x;
      u.model.at(1, r) *= b.size.y;
      u.model.at(2, r) *= b.size.z;
    }
    setVec4(u.color, b.color, b.metallic);
    u.material[0] = b.glow;
    u.material[1] = b.roughness;
    u.material[2] = -1;
    u.material[3] = 1;
    std::memcpy(data.data() + i * kObjectStride, &u, sizeof(u));
  }
  gpu_->queue.WriteBuffer(objectBuffer_, boxOffset_, data.data(), data.size());
}

void SceneRenderer::drawBoxes(const wgpu::RenderPassEncoder& pass) const {
  pass.SetVertexBuffer(0, boxMesh_.vertexBuffer);
  pass.SetIndexBuffer(boxMesh_.indexBuffer, wgpu::IndexFormat::Uint32);
  for (size_t i = 0; i < boxCount_; ++i) {
    const uint32_t offset = boxOffset_ + static_cast<uint32_t>(i * kObjectStride);
    pass.SetBindGroup(1, objectBindGroup_, 1, &offset);
    pass.DrawIndexed(boxMesh_.indexCount);
    engine::prof::draw(boxMesh_.indexCount);
  }
}

void SceneRenderer::renderMirror(const wgpu::CommandEncoder& encoder, size_t index, const Mirror& mirror,
                                 const Mat4& viewProj, Vec3 eye) {
  // Only when you're in front of the glass and it's on screen.
  if (!mirror.on || engine::dot(mirror.normal, eye - mirror.center) <= 0.01f) return;
  // The glass on screen: its corners clipped against the near plane (so a
  // mirror partly behind you still gets a tight box), in half-res pixels.
  const float W = static_cast<float>(mirrorW_), H = static_cast<float>(mirrorH_);
  Vec4 poly[8];
  int count = 0;
  {
    Vec4 quad[4];
    const float sx[4] = {-1, 1, 1, -1}, sy[4] = {-1, -1, 1, 1};
    for (int k = 0; k < 4; ++k) {
      quad[k] = engine::transformPoint(
          viewProj, mirror.center + mirror.right * (sx[k] * mirror.halfWidth) + mirror.up * (sy[k] * mirror.halfHeight));
    }
    constexpr float kNearW = 0.01f;
    for (int k = 0; k < 4; ++k) {
      const Vec4 a = quad[k], b = quad[(k + 1) % 4];
      const bool ina = a.w > kNearW, inb = b.w > kNearW;
      if (ina) poly[count++] = a;
      if (ina != inb) {
        const float t = (kNearW - a.w) / (b.w - a.w);
        poly[count++] = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, kNearW};
      }
    }
  }
  if (count == 0) return;  // entirely behind you
  float minX = W, minY = H, maxX = 0, maxY = 0;
  for (int k = 0; k < count; ++k) {
    const float px = (poly[k].x / poly[k].w * 0.5f + 0.5f) * W, py = (0.5f - poly[k].y / poly[k].w * 0.5f) * H;
    minX = std::min(minX, px);
    maxX = std::max(maxX, px);
    minY = std::min(minY, py);
    maxY = std::max(maxY, py);
  }
  const auto x0 = static_cast<uint32_t>(std::clamp(std::floor(minX), 0.0f, W));
  const auto y0 = static_cast<uint32_t>(std::clamp(std::floor(minY), 0.0f, H));
  const auto x1 = static_cast<uint32_t>(std::clamp(std::ceil(maxX), 0.0f, W));
  const auto y1 = static_cast<uint32_t>(std::clamp(std::ceil(maxY), 0.0f, H));
  if (x1 <= x0 || y1 <= y0) return;  // off screen

  // The reflected camera: the world mirrored across the glass, viewed by you.
  FrameUniforms f = frame_;
  // The near plane is tilted onto the glass, so the GPU itself cuts away
  // everything behind the mirror (no per-pixel test in the shader).
  const float glassPlane[4] = {mirror.normal.x, mirror.normal.y, mirror.normal.z,
                               -engine::dot(mirror.normal, mirror.center)};
  f.viewProj = obliqueNearPlane(viewProj * Mat4::reflection(mirror.normal, mirror.center), glassPlane);
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
  depth.view = mirrorDepthView_;  // half resolution, like the reflection
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;
  passDesc.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  pass.SetScissorRect(x0, y0, x1 - x0, y1 - y0);
  engine::prof::setPass(engine::prof::kMirror);
  pass.SetPipeline(mirroredPipeline_);
  pass.SetBindGroup(0, view.bindGroup);
  if (full) {
    drawItems(pass, sceneDraws_);
    drawBoxes(pass);
    pass.SetPipeline(modelMirroredPipeline_);
    drawModels(pass, false, {mirroredEye.x, mirroredEye.y, mirroredEye.z});
  }
  pass.SetPipeline(skinnedMirroredPipeline_);
  drawAvatar(pass, bonesBindGroup_);
  drawOthers(pass);
  if (full) {
    pass.SetPipeline(modelBlendMirroredPipeline_);
    drawModels(pass, true, {mirroredEye.x, mirroredEye.y, mirroredEye.z});
  }
  pass.End();
  engine::prof::setPass(engine::prof::kMain);
}

void SceneRenderer::renderOverlay(const wgpu::TextureView& target, const engine::TextRenderer& text) {
  wgpu::CommandEncoder encoder = gpu_->device.CreateCommandEncoder();
  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = kClearColor;
  wgpu::RenderPassDepthStencilAttachment depth;  // the text pipeline has a depth state
  depth.view = gpu_->depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;
  passDesc.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  text.draw(pass);
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  gpu_->queue.Submit(1, &commands);
}

void SceneRenderer::render(const wgpu::TextureView& target, const Mat4& viewProj, Vec3 eye, const Mat4& avatar,
                           const std::vector<Mat4>& avatarBones, const std::vector<Mat4>& firstPersonBones,
                           const std::vector<Mirror>& mirrors, const engine::TextRenderer& text,
                           const engine::PictureBillboards& pictures) {
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
  drawBoxes(pass);
  pass.SetPipeline(modelPipeline_);
  drawModels(pass, false, eye);
  pass.SetPipeline(skinnedPipeline_);
  drawAvatar(pass, firstPersonBonesBindGroup_);  // your body, minus the head
  drawOthers(pass);
  pass.SetPipeline(modelBlendPipeline_);
  drawModels(pass, true, eye);  // glass, after everything solid

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
    g.params[1] = 1.0f / static_cast<float>(targetW_);
    g.params[2] = 1.0f / static_cast<float>(targetH_);
    queue.WriteBuffer(mirrorTargets_[i].glassBuffer, 0, &g, sizeof(g));
    pass.SetBindGroup(0, mirrorTargets_[i].glassBindGroup);
    pass.Draw(6);
    engine::prof::draw(6);
  }

  pictures.draw(pass);  // profile pictures over heads
  text.draw(pass);  // after the scene: text is see-through and on top
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace app
