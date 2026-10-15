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

using engine::Mat4;
using engine::Vec3;

namespace {

constexpr uint32_t kMaxLights = 8;
// WebGPU requires dynamic uniform offsets to be multiples of 256 bytes.
constexpr uint64_t kObjectStride = 256;
constexpr float kWalkSpeed = 2.2f;           // meters per second
constexpr float kLookSensitivity = 0.0055f;  // radians per CSS pixel
constexpr float kBodyRadius = 0.3f;          // meters

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
  FrameUniforms frameUniforms = {};

  wgpu::RenderPipeline pipeline;
  wgpu::Buffer frameBuffer;
  wgpu::Buffer objectBuffer;
  wgpu::BindGroup frameBindGroup;
  wgpu::BindGroup objectBindGroup;
  std::vector<DrawItem> draws;

  double lastFrameMs = 0;
  double fpsWindowStartMs = 0;
  int fpsFrames = 0;
};

App g_app;

wgpu::BindGroupLayout makeUniformLayout(bool dynamicOffset, uint64_t size) {
  wgpu::BindGroupLayoutEntry entry;
  entry.binding = 0;
  entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entry.buffer.type = wgpu::BufferBindingType::Uniform;
  entry.buffer.hasDynamicOffset = dynamicOffset;
  entry.buffer.minBindingSize = size;
  wgpu::BindGroupLayoutDescriptor desc;
  desc.entryCount = 1;
  desc.entries = &entry;
  return g_app.gpu.device.CreateBindGroupLayout(&desc);
}

wgpu::BindGroup makeBindGroup(const wgpu::BindGroupLayout& layout, const wgpu::Buffer& buffer,
                              uint64_t size) {
  wgpu::BindGroupEntry entry;
  entry.binding = 0;
  entry.buffer = buffer;
  entry.size = size;
  wgpu::BindGroupDescriptor desc;
  desc.layout = layout;
  desc.entryCount = 1;
  desc.entries = &entry;
  return g_app.gpu.device.CreateBindGroup(&desc);
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

  wgpu::BindGroupLayout frameLayout = makeUniformLayout(false, sizeof(FrameUniforms));
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
  g_app.frameBindGroup = makeBindGroup(frameLayout, g_app.frameBuffer, sizeof(FrameUniforms));

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
  if (!createRenderer()) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  g_app.camera.position = g_app.scene.spawnPosition;
  g_app.camera.yaw = g_app.scene.spawnYaw;
  g_app.input.attach("#canvas");
  g_app.lastFrameMs = g_app.fpsWindowStartMs = emscripten_get_now();
  LOG_INFO("Running. Left thumb: walk. Right thumb: look around.");
  emscripten_set_main_loop(frame, 0, false);
}

}  // namespace

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  g_app.gpu.init("#canvas", onGpuReady);
  return 0;
}
