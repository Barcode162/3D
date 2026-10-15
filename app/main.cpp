// Stage 1, step 1: prove the pipeline C++ -> WebAssembly -> WebGPU -> screen.
// Draws a spinning triangle; tapping the screen changes its tint.
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <array>
#include <cstdio>

#include "engine/file.h"
#include "engine/gpu.h"
#include "engine/log.h"

namespace {

struct FrameUniforms {
  float time;
  float aspect;
  float pad[2];
  float tint[4];
};

constexpr std::array<std::array<float, 4>, 4> kTints = {{
    {1.0f, 0.75f, 0.4f, 1.0f},  // warm bar light
    {1.0f, 0.3f, 0.7f, 1.0f},   // neon pink
    {0.3f, 0.9f, 1.0f, 1.0f},   // cool teal
    {0.6f, 0.4f, 1.0f, 1.0f},   // violet
}};

struct App {
  engine::Gpu gpu;
  wgpu::RenderPipeline pipeline;
  wgpu::Buffer uniformBuffer;
  wgpu::BindGroup bindGroup;
  size_t tintIndex = 0;
  double startMs = 0;
  double fpsWindowStartMs = 0;
  int fpsFrames = 0;
};

App app;

bool createPipeline() {
  auto source = engine::readTextFile("/shaders/triangle.wgsl");
  if (!source) return false;

  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source->c_str();
  wgpu::ShaderModuleDescriptor moduleDesc;
  moduleDesc.nextInChain = &wgsl;
  moduleDesc.label = "triangle";
  wgpu::ShaderModule module = app.gpu.device.CreateShaderModule(&moduleDesc);

  wgpu::ColorTargetState colorTarget;
  colorTarget.format = app.gpu.surfaceFormat;

  wgpu::FragmentState fragment;
  fragment.module = module;
  fragment.entryPoint = "fs_main";
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  wgpu::RenderPipelineDescriptor desc;
  desc.label = "triangle";
  desc.vertex.module = module;
  desc.vertex.entryPoint = "vs_main";
  desc.fragment = &fragment;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  app.pipeline = app.gpu.device.CreateRenderPipeline(&desc);

  wgpu::BufferDescriptor bufferDesc;
  bufferDesc.label = "frame uniforms";
  bufferDesc.size = sizeof(FrameUniforms);
  bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  app.uniformBuffer = app.gpu.device.CreateBuffer(&bufferDesc);

  wgpu::BindGroupEntry entry;
  entry.binding = 0;
  entry.buffer = app.uniformBuffer;
  entry.size = sizeof(FrameUniforms);
  wgpu::BindGroupDescriptor bindDesc;
  bindDesc.layout = app.pipeline.GetBindGroupLayout(0);
  bindDesc.entryCount = 1;
  bindDesc.entries = &entry;
  app.bindGroup = app.gpu.device.CreateBindGroup(&bindDesc);
  return true;
}

void updateFps(double nowMs) {
  ++app.fpsFrames;
  const double elapsed = nowMs - app.fpsWindowStartMs;
  if (elapsed < 500) return;
  char text[96];
  std::snprintf(text, sizeof(text), "%.0f fps  |  %ux%u @%.1fx", app.fpsFrames * 1000.0 / elapsed,
                app.gpu.width, app.gpu.height, app.gpu.pixelRatio);
  engine::setStatus(text);
  app.fpsFrames = 0;
  app.fpsWindowStartMs = nowMs;
}

void frame() {
  const double nowMs = emscripten_get_now();
  app.gpu.resizeIfNeeded();
  updateFps(nowMs);

  wgpu::TextureView target = app.gpu.beginFrame();
  if (!target) return;

  const auto& tint = kTints[app.tintIndex];
  FrameUniforms uniforms = {};
  uniforms.time = static_cast<float>((nowMs - app.startMs) / 1000.0);
  uniforms.aspect = static_cast<float>(app.gpu.width) / static_cast<float>(app.gpu.height);
  for (int i = 0; i < 4; ++i) uniforms.tint[i] = tint[i];
  app.gpu.queue.WriteBuffer(app.uniformBuffer, 0, &uniforms, sizeof(uniforms));

  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = {tint[0] * 0.06, tint[1] * 0.04, tint[2] * 0.08, 1.0};

  wgpu::RenderPassDescriptor passDesc;
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &color;

  wgpu::CommandEncoder encoder = app.gpu.device.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
  pass.SetPipeline(app.pipeline);
  pass.SetBindGroup(0, app.bindGroup);
  pass.Draw(3);
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  app.gpu.queue.Submit(1, &commands);
}

bool onTouchStart(int, const EmscriptenTouchEvent*, void*) {
  app.tintIndex = (app.tintIndex + 1) % kTints.size();
  LOG_INFO("Tap -> tint %zu", app.tintIndex);
  return true;
}

bool onMouseDown(int, const EmscriptenMouseEvent*, void*) {
  return onTouchStart(0, nullptr, nullptr);
}

void onGpuReady(bool ok) {
  if (!ok || !createPipeline()) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  emscripten_set_touchstart_callback("#canvas", nullptr, true, onTouchStart);
  emscripten_set_mousedown_callback("#canvas", nullptr, true, onMouseDown);
  app.startMs = app.fpsWindowStartMs = emscripten_get_now();
  LOG_INFO("Running. Tap the screen to change color.");
  emscripten_set_main_loop(frame, 0, false);
}

}  // namespace

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  app.gpu.init("#canvas", onGpuReady);
  return 0;
}
