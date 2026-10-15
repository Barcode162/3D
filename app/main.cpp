// Stage 1: walk around a first rough version of the bar in first person.
// Left thumb = joystick to walk, right thumb = drag to look around.
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/bar_scene.h"
#include "engine/camera.h"
#include "engine/file.h"
#include "engine/gpu.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/mesh.h"
#include "engine/texture.h"
#include "engine/text.h"

using engine::Mat4;
using engine::Vec3;

namespace {

constexpr uint32_t kMaxLights = 8;
// WebGPU requires dynamic uniform offsets to be multiples of 256 bytes.
constexpr uint64_t kObjectStride = 256;
constexpr float kWalkSpeed = 2.2f;           // meters per second
constexpr float kLookSensitivity = 0.0055f;  // radians per CSS pixel
constexpr float kBodyRadius = 0.3f;          // meters
constexpr float kLabelShowDistance = 6.0f;   // name tags appear within this range
constexpr float kPromptDistance = 2.2f;      // ...and their prompt this close
constexpr double kIntroMaxMs = 12000;        // title + controls hint on start...
constexpr double kIntroAfterInputMs = 1500;  // ...until shortly after you first move

// These structs mirror the WGSL structs in shaders/scene.wgsl byte for byte.
struct GpuLight {
  float position[4];  // xyz, range
  float color[4];     // rgb, intensity
};
struct FrameUniforms {
  Mat4 viewProj;
  float cameraPos[4];
  float ambient[4];
  float fog[4];
  uint32_t lightCount[4];
  GpuLight lights[kMaxLights];
};
struct ObjectUniforms {
  Mat4 model;
  float color[4];
  float material[4];
};
static_assert(sizeof(FrameUniforms) == 384);
static_assert(sizeof(ObjectUniforms) <= kObjectStride);

struct DrawItem {
  engine::Mesh mesh;
  uint32_t uniformOffset;
};

struct App {
  engine::Gpu gpu;
  engine::Input input;
  engine::FirstPersonCamera camera;
  app::BarScene scene;
  engine::TextRenderer text;
  FrameUniforms frameUniforms = {};

  wgpu::RenderPipeline pipeline;
  wgpu::Buffer frameBuffer;
  wgpu::Buffer objectBuffer;
  wgpu::BindGroup frameBindGroup;
  wgpu::BindGroup objectBindGroup;
  std::vector<DrawItem> draws;

  double introEndMs = 0;
  double lastFrameMs = 0;
  double fpsWindowStartMs = 0;
  int fpsFrames = 0;
};

App g_app;

// Binding 0 is a uniform buffer. With `textures`, bindings 1-3 are the
// surface color maps, bump maps and their sampler.
wgpu::BindGroupLayout makeUniformLayout(bool dynamicOffset, uint64_t size, bool textures = false) {
  wgpu::BindGroupLayoutEntry entries[4];
  entries[0].binding = 0;
  entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
  entries[0].buffer.hasDynamicOffset = dynamicOffset;
  entries[0].buffer.minBindingSize = size;
  for (uint32_t i = 1; i <= 2; ++i) {
    entries[i].binding = i;
    entries[i].visibility = wgpu::ShaderStage::Fragment;
    entries[i].texture.sampleType = wgpu::TextureSampleType::Float;
    entries[i].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;
  }
  entries[3].binding = 3;
  entries[3].visibility = wgpu::ShaderStage::Fragment;
  entries[3].sampler.type = wgpu::SamplerBindingType::Filtering;
  wgpu::BindGroupLayoutDescriptor desc;
  desc.entryCount = textures ? 4 : 1;
  desc.entries = entries;
  return g_app.gpu.device.CreateBindGroupLayout(&desc);
}

wgpu::BindGroup makeBindGroup(const wgpu::BindGroupLayout& layout, const wgpu::Buffer& buffer,
                              uint64_t size, const wgpu::TextureView& colorMaps = nullptr,
                              const wgpu::TextureView& bumpMaps = nullptr, const wgpu::Sampler& sampler = nullptr) {
  wgpu::BindGroupEntry entries[4];
  entries[0].binding = 0;
  entries[0].buffer = buffer;
  entries[0].size = size;
  entries[1].binding = 1;
  entries[1].textureView = colorMaps;
  entries[2].binding = 2;
  entries[2].textureView = bumpMaps;
  entries[3].binding = 3;
  entries[3].sampler = sampler;
  wgpu::BindGroupDescriptor desc;
  desc.layout = layout;
  desc.entryCount = colorMaps ? 4 : 1;
  desc.entries = entries;
  return g_app.gpu.device.CreateBindGroup(&desc);
}

// Generates the bar's surface textures and uploads them as two texture
// arrays (colors, bumps).
void createMaterials(wgpu::TextureView& colorMaps, wgpu::TextureView& bumpMaps, wgpu::Sampler& sampler) {
  const double start = emscripten_get_now();
  std::vector<engine::Image> colors, bumps;
  for (engine::MaterialImages& m : app::buildBarMaterials()) {
    colors.push_back(std::move(m.color));
    bumps.push_back(std::move(m.bump));
  }
  wgpu::TextureViewDescriptor viewDesc;
  viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
  colorMaps = engine::createTextureArray(g_app.gpu.device, colors, true, "material colors").CreateView(&viewDesc);
  bumpMaps = engine::createTextureArray(g_app.gpu.device, bumps, false, "material bumps").CreateView(&viewDesc);

  wgpu::SamplerDescriptor samplerDesc;
  samplerDesc.addressModeU = samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
  samplerDesc.magFilter = samplerDesc.minFilter = wgpu::FilterMode::Linear;
  samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
  samplerDesc.maxAnisotropy = 4;  // keeps the floor sharp at shallow angles
  sampler = g_app.gpu.device.CreateSampler(&samplerDesc);
  LOG_INFO("Textures: %zu materials generated in %.0f ms.", colors.size(), emscripten_get_now() - start);
}

wgpu::Buffer makeUniformBuffer(uint64_t size, const char* label) {
  wgpu::BufferDescriptor desc;
  desc.label = label;
  desc.size = size;
  desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  return g_app.gpu.device.CreateBuffer(&desc);
}

bool createRenderer() {
  auto source = engine::readTextFile("/shaders/scene.wgsl");
  if (!source) return false;
  const wgpu::Device& device = g_app.gpu.device;

  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source->c_str();
  wgpu::ShaderModuleDescriptor moduleDesc;
  moduleDesc.nextInChain = &wgsl;
  moduleDesc.label = "scene";
  wgpu::ShaderModule module = device.CreateShaderModule(&moduleDesc);

  wgpu::BindGroupLayout frameLayout = makeUniformLayout(false, sizeof(FrameUniforms), true);
  wgpu::BindGroupLayout objectLayout = makeUniformLayout(true, sizeof(ObjectUniforms));
  wgpu::BindGroupLayout layouts[] = {frameLayout, objectLayout};
  wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
  pipelineLayoutDesc.bindGroupLayoutCount = 2;
  pipelineLayoutDesc.bindGroupLayouts = layouts;

  wgpu::ColorTargetState colorTarget;
  colorTarget.format = g_app.gpu.surfaceFormat;
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
  desc.layout = device.CreatePipelineLayout(&pipelineLayoutDesc);
  desc.vertex.module = module;
  desc.vertex.entryPoint = "vs_main";
  desc.vertex.bufferCount = 1;
  desc.vertex.buffers = &vertexLayout;
  desc.fragment = &fragment;
  desc.depthStencil = &depth;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.primitive.frontFace = wgpu::FrontFace::CCW;
  desc.primitive.cullMode = wgpu::CullMode::Back;
  g_app.pipeline = device.CreateRenderPipeline(&desc);

  g_app.frameBuffer = makeUniformBuffer(sizeof(FrameUniforms), "frame uniforms");
  wgpu::TextureView colorMaps, bumpMaps;
  wgpu::Sampler sampler;
  createMaterials(colorMaps, bumpMaps, sampler);
  g_app.frameBindGroup =
      makeBindGroup(frameLayout, g_app.frameBuffer, sizeof(FrameUniforms), colorMaps, bumpMaps, sampler);

  // Upload every object once; the bar doesn't move (yet), so its per-object
  // data is written a single time.
  const auto& objects = g_app.scene.objects;
  g_app.objectBuffer = makeUniformBuffer(objects.size() * kObjectStride, "object uniforms");
  g_app.objectBindGroup = makeBindGroup(objectLayout, g_app.objectBuffer, sizeof(ObjectUniforms));
  std::vector<uint8_t> objectData(objects.size() * kObjectStride);
  for (size_t i = 0; i < objects.size(); ++i) {
    const app::SceneObject& o = objects[i];
    ObjectUniforms u = {};
    u.model = Mat4::transform(o.position, o.rotation);
    u.color[0] = o.color.x;
    u.color[1] = o.color.y;
    u.color[2] = o.color.z;
    u.material[0] = o.emissive;
    u.material[1] = o.shininess;
    u.material[2] = static_cast<float>(o.texture);
    u.material[3] = o.textureScale;
    std::memcpy(objectData.data() + i * kObjectStride, &u, sizeof(u));
    g_app.draws.push_back({engine::Mesh::upload(device, o.mesh), static_cast<uint32_t>(i * kObjectStride)});
  }
  g_app.gpu.queue.WriteBuffer(g_app.objectBuffer, 0, objectData.data(), objectData.size());

  // Lights and atmosphere don't change per frame either.
  FrameUniforms& f = g_app.frameUniforms;
  const auto lightCount = std::min<size_t>(g_app.scene.lights.size(), kMaxLights);
  if (g_app.scene.lights.size() > kMaxLights) LOG_ERROR("Too many lights; only %u used", kMaxLights);
  f.lightCount[0] = static_cast<uint32_t>(lightCount);
  for (size_t i = 0; i < lightCount; ++i) {
    const app::PointLight& l = g_app.scene.lights[i];
    f.lights[i] = {{l.position.x, l.position.y, l.position.z, l.range},
                   {l.color.x, l.color.y, l.color.z, l.intensity}};
  }
  const Vec3& a = g_app.scene.ambient;
  const Vec3& fog = g_app.scene.fogColor;
  f.ambient[0] = a.x; f.ambient[1] = a.y; f.ambient[2] = a.z;
  f.fog[0] = fog.x; f.fog[1] = fog.y; f.fog[2] = fog.z; f.fog[3] = g_app.scene.fogDensity;

  LOG_INFO("Scene: %zu objects, %zu lights.", objects.size(), lightCount);
  return true;
}

void updateCamera(float dt) {
  const engine::Vec2 look = g_app.input.takeLookDelta();
  g_app.camera.look(look.x * kLookSensitivity, -look.y * kLookSensitivity);

  const engine::Vec2 move = g_app.input.move();
  if (look.x != 0 || look.y != 0 || move.x != 0 || move.y != 0) {
    g_app.introEndMs = std::min(g_app.introEndMs, emscripten_get_now() + kIntroAfterInputMs);
  }
  Vec3 p = g_app.camera.position;
  p += g_app.camera.flatForward() * (move.y * kWalkSpeed * dt);
  p += g_app.camera.flatRight() * (move.x * kWalkSpeed * dt);
  g_app.camera.position = app::resolveCollisions(g_app.scene, p, kBodyRadius);
}

void updateFps(double nowMs) {
  ++g_app.fpsFrames;
  const double elapsed = nowMs - g_app.fpsWindowStartMs;
  if (elapsed < 500) return;
  char text[96];
  std::snprintf(text, sizeof(text), "%.0f fps  |  %ux%u @%.1fx  |  %zu objects",
                g_app.fpsFrames * 1000.0 / elapsed, g_app.gpu.width, g_app.gpu.height, g_app.gpu.pixelRatio,
                g_app.draws.size());
  engine::setStatus(text);
  g_app.fpsFrames = 0;
  g_app.fpsWindowStartMs = nowMs;
}

// Queues this frame's text: name tags over things in the bar, the prompt
// for whatever you're standing at, and the intro title.
void queueText(double nowMs, const Mat4& viewProj) {
  engine::TextRenderer& text = g_app.text;
  const float W = static_cast<float>(g_app.gpu.width), H = static_cast<float>(g_app.gpu.height);
  const float px = g_app.gpu.pixelRatio;  // device pixels per CSS pixel
  text.begin(viewProj, g_app.gpu.width, g_app.gpu.height);

  const engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  const Vec3 up = engine::cross(right, forward);
  const app::Label* promptLabel = nullptr;
  float bestFacing = 0.8f;  // must be looking roughly toward it
  for (const app::Label& label : g_app.scene.labels) {
    const Vec3 toLabel = label.position - cam.position;
    const float dist = engine::length(toLabel);
    if (dist > kLabelShowDistance) continue;
    // Fade in as you approach; the tag always turns to face you.
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {1.0f, 0.86f, 0.62f, engine::clampf((kLabelShowDistance - dist) / 1.5f, 0, 1)};
    style.outline.w *= style.color.w;
    style.outlineWidth = 0.8f;
    text.addWorld(label.text, label.position, right, up, 0.1f, style);

    const Vec3 toFlat{toLabel.x, 0, toLabel.z};
    const float facing = engine::dot(engine::normalize(toFlat), cam.flatForward());
    if (engine::length(toFlat) < kPromptDistance && facing > bestFacing) {
      bestFacing = facing;
      promptLabel = &label;
    }
  }
  if (promptLabel) {
    engine::TextStyle style;
    style.align = 0.5f;
    style.outlineWidth = 0.8f;
    text.addScreen(promptLabel->prompt, W * 0.5f, H * 0.62f, 14 * px, style);
  }

  if (nowMs < g_app.introEndMs) {
    const float fade = engine::clampf(static_cast<float>((g_app.introEndMs - nowMs) / 1000.0), 0, 1);
    engine::TextStyle title;
    title.align = 0.5f;
    title.color = {1.0f, 0.35f, 0.7f, fade};
    title.outline.w *= fade;
    title.outlineWidth = 0.9f;
    text.addScreen("BAR3D", W * 0.5f, H * 0.2f, 34 * px, title);
    engine::TextStyle hint;
    hint.align = 0.5f;
    hint.color.w = fade;
    hint.outline.w *= fade;
    text.addScreen("Left thumb: walk\nRight thumb: look around", W * 0.5f, H * 0.2f + 52 * px, 11 * px, hint);
  }
  text.upload(g_app.gpu.queue);
}

void frame() {
  const double nowMs = emscripten_get_now();
  // Clamp the time step so a stall (e.g. switching apps) doesn't teleport you.
  const float dt = std::min(static_cast<float>((nowMs - g_app.lastFrameMs) / 1000.0), 0.1f);
  g_app.lastFrameMs = nowMs;

  g_app.gpu.resizeIfNeeded();
  updateFps(nowMs);
  updateCamera(dt);

  wgpu::TextureView target = g_app.gpu.beginFrame();
  if (!target) return;

  FrameUniforms& f = g_app.frameUniforms;
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  f.viewProj = g_app.camera.projection(aspect) * g_app.camera.view();
  f.cameraPos[0] = g_app.camera.position.x;
  f.cameraPos[1] = g_app.camera.position.y;
  f.cameraPos[2] = g_app.camera.position.z;
  g_app.gpu.queue.WriteBuffer(g_app.frameBuffer, 0, &f, sizeof(f));
  queueText(nowMs, f.viewProj);

  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = {0.02, 0.015, 0.015, 1.0};

  wgpu::RenderPassDepthStencilAttachment depth;
  depth.view = g_app.gpu.depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;

  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;
  passDesc.depthStencilAttachment = &depth;

  wgpu::CommandEncoder encoder = g_app.gpu.device.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  pass.SetPipeline(g_app.pipeline);
  pass.SetBindGroup(0, g_app.frameBindGroup);
  for (const DrawItem& d : g_app.draws) {
    pass.SetBindGroup(1, g_app.objectBindGroup, 1, &d.uniformOffset);
    pass.SetVertexBuffer(0, d.mesh.vertexBuffer);
    pass.SetIndexBuffer(d.mesh.indexBuffer, wgpu::IndexFormat::Uint32);
    pass.DrawIndexed(d.mesh.indexCount);
  }
  g_app.text.draw(pass);  // after the scene: text is see-through and on top
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  g_app.gpu.queue.Submit(1, &commands);
}

void onGpuReady(bool ok) {
  if (!ok) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  g_app.scene = app::buildBarScene();
  if (!createRenderer() ||
      !g_app.text.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat)) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  g_app.camera.position = g_app.scene.spawnPosition;
  g_app.camera.yaw = g_app.scene.spawnYaw;
  g_app.input.attach("#canvas");
  g_app.lastFrameMs = g_app.fpsWindowStartMs = emscripten_get_now();
  g_app.introEndMs = g_app.lastFrameMs + kIntroMaxMs;
  LOG_INFO("Running. Left thumb: walk. Right thumb: look around.");
  emscripten_set_main_loop(frame, 0, false);
}

}  // namespace

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  g_app.gpu.init("#canvas", onGpuReady);
  return 0;
}
