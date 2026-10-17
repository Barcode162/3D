#include "app/race/view.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

#include "app/race/car_model.h"
#include "engine/file.h"
#include "engine/log.h"
#include "engine/pipeline_cache.h"
#include "engine/profiler.h"

using engine::Mat4;
using engine::Quat;
using engine::Vec3;

namespace app::race {
namespace {

void set4(float out[4], Vec3 v, float w) {
  out[0] = v.x;
  out[1] = v.y;
  out[2] = v.z;
  out[3] = w;
}

// Is a box (world bounds) at least partly inside the view?
bool visible(const Mat4& viewProj, Vec3 lo, Vec3 hi) {
  int outside[6] = {0, 0, 0, 0, 0, 0};
  for (int k = 0; k < 8; ++k) {
    const Vec3 p{k & 1 ? hi.x : lo.x, k & 2 ? hi.y : lo.y, k & 4 ? hi.z : lo.z};
    const engine::Vec4 c = engine::transformPoint(viewProj, p);
    outside[0] += c.x < -c.w;
    outside[1] += c.x > c.w;
    outside[2] += c.y < -c.w;
    outside[3] += c.y > c.w;
    outside[4] += c.z < 0;
    outside[5] += c.z > c.w;
  }
  for (int i = 0; i < 6; ++i) {
    if (outside[i] == 8) return false;
  }
  return true;
}

}  // namespace

View::Part View::upload(const MeshBuilder& mesh) {
  Part p;
  const wgpu::Device& device = gpu_->device;
  wgpu::BufferDescriptor vd;
  vd.size = (mesh.vertices.size() * sizeof(Vertex) + 3) & ~size_t{3};
  vd.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
  p.vertices = device.CreateBuffer(&vd);
  gpu_->queue.WriteBuffer(p.vertices, 0, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
  wgpu::BufferDescriptor id;
  id.size = (mesh.indices.size() * sizeof(uint32_t) + 3) & ~size_t{3};
  id.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
  p.indices = device.CreateBuffer(&id);
  gpu_->queue.WriteBuffer(p.indices, 0, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
  p.indexCount = static_cast<uint32_t>(mesh.indices.size());
  p.boundsMin = {1e9f, 1e9f, 1e9f};
  p.boundsMax = {-1e9f, -1e9f, -1e9f};
  for (const Vertex& v : mesh.vertices) {
    p.boundsMin = {std::min(p.boundsMin.x, v.position.x), std::min(p.boundsMin.y, v.position.y), std::min(p.boundsMin.z, v.position.z)};
    p.boundsMax = {std::max(p.boundsMax.x, v.position.x), std::max(p.boundsMax.y, v.position.y), std::max(p.boundsMax.z, v.position.z)};
  }
  return p;
}

// Shaders, layouts and the two pipelines (compiled in the background while
// the bar loads, so the first sit doesn't wait for them).
bool View::preparePipelines(engine::Gpu& gpu) {
  if (prepared_) return true;
  gpu_ = &gpu;
  const wgpu::Device& device = gpu.device;
  auto source = engine::readTextFile("/shaders/race.wgsl");
  if (!source) {
    LOG_ERROR("Race shader missing.");
    return false;
  }
  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source->c_str();
  wgpu::ShaderModuleDescriptor md;
  md.nextInChain = &wgsl;
  md.label = "race";
  wgpu::ShaderModule module = device.CreateShaderModule(&md);

  wgpu::BindGroupLayoutEntry fe;
  fe.binding = 0;
  fe.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  fe.buffer.type = wgpu::BufferBindingType::Uniform;
  fe.buffer.minBindingSize = sizeof(FrameUniforms);
  wgpu::BindGroupLayoutDescriptor fd;
  fd.entryCount = 1;
  fd.entries = &fe;
  frameLayout_ = device.CreateBindGroupLayout(&fd);
  wgpu::BindGroupLayoutEntry oe;
  oe.binding = 0;
  oe.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  oe.buffer.type = wgpu::BufferBindingType::Uniform;
  oe.buffer.hasDynamicOffset = true;
  oe.buffer.minBindingSize = sizeof(ObjectUniforms);
  wgpu::BindGroupLayoutDescriptor od;
  od.entryCount = 1;
  od.entries = &oe;
  objectLayout_ = device.CreateBindGroupLayout(&od);
  wgpu::BindGroupLayout layouts[] = {frameLayout_, objectLayout_};
  wgpu::PipelineLayoutDescriptor pld;
  pld.bindGroupLayoutCount = 2;
  pld.bindGroupLayouts = layouts;
  wgpu::PipelineLayout layout = device.CreatePipelineLayout(&pld);

  static const wgpu::VertexAttribute attrs[] = {
      {.format = wgpu::VertexFormat::Float32x3, .offset = offsetof(Vertex, position), .shaderLocation = 0},
      {.format = wgpu::VertexFormat::Float32x3, .offset = offsetof(Vertex, normal), .shaderLocation = 1},
      {.format = wgpu::VertexFormat::Float32x3, .offset = offsetof(Vertex, color), .shaderLocation = 2},
      {.format = wgpu::VertexFormat::Uint32, .offset = offsetof(Vertex, material), .shaderLocation = 3},
  };
  wgpu::VertexBufferLayout vbl;
  vbl.arrayStride = sizeof(Vertex);
  vbl.attributeCount = 4;
  vbl.attributes = attrs;
  wgpu::ColorTargetState target;
  target.format = gpu.surfaceFormat;
  wgpu::FragmentState frag;
  frag.module = module;
  frag.entryPoint = "fs_main";
  frag.targetCount = 1;
  frag.targets = &target;
  wgpu::DepthStencilState depth;
  depth.format = engine::Gpu::kDepthFormat;
  depth.depthWriteEnabled = true;
  depth.depthCompare = wgpu::CompareFunction::Less;
  wgpu::RenderPipelineDescriptor pd;
  pd.label = "race";
  pd.layout = layout;
  pd.vertex.module = module;
  pd.vertex.entryPoint = "vs_main";
  pd.vertex.bufferCount = 1;
  pd.vertex.buffers = &vbl;
  pd.fragment = &frag;
  pd.depthStencil = &depth;
  pd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  pd.primitive.cullMode = wgpu::CullMode::None;  // the shader lights both sides
  engine::pipelines::create(device, pd, &pipeline_, false);

  wgpu::PipelineLayoutDescriptor skyLayoutDesc;
  skyLayoutDesc.bindGroupLayoutCount = 1;
  skyLayoutDesc.bindGroupLayouts = &frameLayout_;
  frag.entryPoint = "fs_sky";
  wgpu::DepthStencilState skyDepth;
  skyDepth.format = engine::Gpu::kDepthFormat;
  skyDepth.depthWriteEnabled = false;
  skyDepth.depthCompare = wgpu::CompareFunction::Always;
  wgpu::RenderPipelineDescriptor sd;
  sd.label = "race sky";
  sd.layout = device.CreatePipelineLayout(&skyLayoutDesc);
  sd.vertex.module = module;
  sd.vertex.entryPoint = "vs_sky";
  sd.fragment = &frag;
  sd.depthStencil = &skyDepth;
  engine::pipelines::create(device, sd, &skyPipeline_, false);
  prepared_ = true;
  return true;
}

bool View::init(engine::Gpu& gpu, const Track& track, const engine::CarSpec& spec) {
  if (!preparePipelines(gpu)) return false;
  const wgpu::Device& device = gpu.device;

  wgpu::BufferDescriptor fb;
  fb.size = sizeof(FrameUniforms);
  fb.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  frameBuffer_ = device.CreateBuffer(&fb);
  wgpu::BufferDescriptor ob;
  ob.size = kObjectStride * kMaxObjects;
  ob.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  objectBuffer_ = device.CreateBuffer(&ob);
  objectData_.assign(ob.size, 0);
  wgpu::BindGroupEntry be;
  be.binding = 0;
  be.buffer = frameBuffer_;
  be.size = sizeof(FrameUniforms);
  wgpu::BindGroupDescriptor bd;
  bd.layout = frameLayout_;
  bd.entryCount = 1;
  bd.entries = &be;
  frameGroup_ = device.CreateBindGroup(&bd);
  be.buffer = objectBuffer_;
  be.size = sizeof(ObjectUniforms);
  bd.layout = objectLayout_;
  objectGroup_ = device.CreateBindGroup(&bd);

  size_t triangles = 0;
  for (const Track::Chunk& c : track.buildMeshes()) {
    Part p = upload(c.mesh);
    p.boundsMin = c.boundsMin - Vec3{1, 1, 1};
    p.boundsMax = c.boundsMax + Vec3{1, 7, 1};
    chunks_.push_back(p);
    triangles += c.mesh.indices.size() / 3;
  }
  body_ = upload(buildCarBody(spec));
  tailLights_ = upload(buildTailLights());
  wheel_ = upload(buildWheel(spec.tire.radius));
  MeshBuilder pod;
  pod.box({0, 0, 0}, {0.22f, 0.22f, 0.06f}, {1.0f, 0.02f, 0.01f}, kGlow);
  lightPod_ = upload(pod);
  const auto start = track.at(4);
  lightsPos_ = start.pos + Vec3{0, 6.3f, 0} + start.tangent * -0.4f;
  lightsRight_ = start.side;
  lightsHeading_ = engine::Vehicle::headingOf(start.tangent);
  LOG_INFO("Race world: %zu chunks, %zu triangles.", chunks_.size(), triangles);
  return true;  // the pipelines may still be compiling: render() waits for them
}

CarDraw View::carDraw(const engine::Vehicle& car, Vec3 paint, float brake) {
  CarDraw d;
  d.body = Mat4::transform(car.position, car.rotation);
  d.paint = paint;
  d.brake = brake;
  const engine::CarSpec& spec = car.spec();
  for (int i = 0; i < 4; ++i) {
    const engine::Vehicle::Wheel& w = car.wheels[static_cast<size_t>(i)];
    const engine::AxleSpec& a = w.front ? spec.front : spec.rear;
    const Vec3 local = w.mount - Vec3{0, a.restLength - w.compression, 0};
    // Steer (positive = right = turning toward +x), then roll (forward =
    // negative about x), mirrored for the left side so rims face out.
    Quat q = car.rotation * Quat::axisAngle({0, 1, 0}, -w.steer) * Quat::axisAngle({1, 0, 0}, -w.angle);
    if (w.left) q = q * Quat::axisAngle({0, 1, 0}, engine::kPi);
    d.wheels[i] = Mat4::transform(car.worldPoint(local), q);
  }
  return d;
}

void View::draw(const wgpu::RenderPassEncoder& pass, const Part& part, uint32_t slot) const {
  const uint32_t offset = slot * kObjectStride;
  pass.SetBindGroup(1, objectGroup_, 1, &offset);
  pass.SetVertexBuffer(0, part.vertices);
  pass.SetIndexBuffer(part.indices, wgpu::IndexFormat::Uint32);
  pass.DrawIndexed(part.indexCount);
  engine::prof::draw(part.indexCount);
}

void View::render(const wgpu::TextureView& target, const wgpu::TextureView& depthView, uint32_t width,
                  uint32_t height, const Camera& cam, const std::vector<CarDraw>& cars, int lights, float timeSec,
                  const engine::TextRenderer* text) {
  const float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1u));
  const Mat4 proj = Mat4::perspective(cam.fovY, aspect, cam.zNear, cam.zFar);
  const Mat4 view = Mat4::lookAt(cam.eye, cam.target, cam.up);
  const Mat4 viewProj = proj * view;

  FrameUniforms f = {};
  f.viewProj = viewProj;
  set4(f.camPos, cam.eye, 1);
  const Vec3 sun = engine::normalize({-0.45f, 0.62f, -0.4f});
  set4(f.sunDir, sun, 3.2f);
  set4(f.sunColor, {1.0f, 0.93f, 0.82f}, 1);
  set4(f.skyTop, {0.16f, 0.32f, 0.68f}, 1);
  set4(f.skyHorizon, {0.62f, 0.72f, 0.85f}, 1);
  set4(f.groundColor, {0.16f, 0.17f, 0.1f}, 1);
  f.fog[0] = 0.0016f;
  f.fog[1] = 0.03f;
  f.fog[2] = timeSec;
  const Vec3 fwd = engine::normalize(cam.target - cam.eye);
  const Vec3 right = engine::normalize(engine::cross(fwd, cam.up));
  const Vec3 up = engine::cross(right, fwd);
  const float tanHalf = std::tan(cam.fovY * 0.5f);
  set4(f.camRight, right, tanHalf * aspect);
  set4(f.camUp, up, tanHalf);
  set4(f.camForward, fwd, 0);
  const size_t carCount = std::min<size_t>(cars.size(), 16);
  for (size_t i = 0; i < carCount; ++i) {
    const Mat4& m = cars[i].body;
    const Vec3 forward{-m.at(2, 0), -m.at(2, 1), -m.at(2, 2)};
    set4(f.cars[i], {m.at(3, 0), m.at(3, 1) - 0.45f, m.at(3, 2)}, engine::Vehicle::headingOf(forward));
  }
  f.carCount[0] = static_cast<uint32_t>(carCount);
  gpu_->queue.WriteBuffer(frameBuffer_, 0, &f, sizeof(f));

  // Object slots: chunks (identity), then per car body + lights + 4 wheels, then start lights.
  uint32_t slot = 0;
  const auto put = [&](const Mat4& model, Vec3 tint, float tintOn, float glow) {
    if (slot >= kMaxObjects) return slot;
    ObjectUniforms o = {};
    o.model = model;
    set4(o.tint, tint, tintOn);
    o.params[0] = glow;
    std::memcpy(objectData_.data() + slot * kObjectStride, &o, sizeof(o));
    return slot++;
  };
  const uint32_t identity = put(Mat4{}, {1, 1, 1}, 0, 0);
  std::vector<uint32_t> carSlots;
  for (size_t i = 0; i < cars.size() && slot + 6 <= kMaxObjects - 5; ++i) {
    const CarDraw& c = cars[i];
    carSlots.push_back(put(c.body, c.paint, 1, 1.0f));
    put(c.body, {1, 1, 1}, 0, 1.2f + 5.0f * c.brake);
    for (const Mat4& w : c.wheels) put(w, {1, 1, 1}, 0, 0);
  }
  const uint32_t lightSlot = slot;
  const Quat lightRot = Quat::axisAngle({0, 1, 0}, lightsHeading_);
  for (int k = 0; k < 5; ++k) {
    const Vec3 p = lightsPos_ + lightsRight_ * (static_cast<float>(k - 2) * 0.7f);
    put(Mat4::transform(p, lightRot), {1, 1, 1}, 0, k < lights ? 6.0f : 0.08f);
  }
  gpu_->queue.WriteBuffer(objectBuffer_, 0, objectData_.data(), slot * kObjectStride);

  wgpu::CommandEncoder encoder = gpu_->device.CreateCommandEncoder();
  wgpu::RenderPassColorAttachment color;
  color.view = target;
  color.loadOp = wgpu::LoadOp::Clear;
  color.storeOp = wgpu::StoreOp::Store;
  color.clearValue = {0.5, 0.6, 0.8, 1};
  wgpu::RenderPassDepthStencilAttachment depth;
  depth.view = depthView;
  depth.depthLoadOp = wgpu::LoadOp::Clear;
  depth.depthStoreOp = wgpu::StoreOp::Discard;
  depth.depthClearValue = 1.0f;
  wgpu::RenderPassDescriptor rp;
  rp.colorAttachmentCount = 1;
  rp.colorAttachments = &color;
  rp.depthStencilAttachment = &depth;
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rp);
  engine::prof::setPass(engine::prof::kRacePass);
  if (!pipelinesReady()) {  // still compiling (only possible right after loading): just clear
    if (text) text->draw(pass);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    gpu_->queue.Submit(1, &commands);
    return;
  }
  pass.SetPipeline(skyPipeline_);
  pass.SetBindGroup(0, frameGroup_);
  pass.Draw(3);
  engine::prof::draw(3);
  pass.SetPipeline(pipeline_);
  pass.SetBindGroup(0, frameGroup_);
  for (const Part& c : chunks_) {
    if (visible(viewProj, c.boundsMin, c.boundsMax)) draw(pass, c, identity);
  }
  for (size_t i = 0; i < carSlots.size(); ++i) {
    const Mat4& m = cars[i].body;
    const Vec3 p{m.at(3, 0), m.at(3, 1), m.at(3, 2)};
    if (!visible(viewProj, p - Vec3{2.6f, 1.2f, 2.6f}, p + Vec3{2.6f, 1.5f, 2.6f})) continue;
    const uint32_t s = carSlots[i];
    draw(pass, body_, s);
    draw(pass, tailLights_, s + 1);
    for (uint32_t w = 0; w < 4; ++w) draw(pass, wheel_, s + 2 + w);
  }
  for (uint32_t k = 0; k < 5; ++k) draw(pass, lightPod_, lightSlot + k);
  if (text) text->draw(pass);
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  gpu_->queue.Submit(1, &commands);
}

}  // namespace app::race
