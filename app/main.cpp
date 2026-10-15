// Stage 1: walk around a first rough version of the bar in first person.
// Left thumb = joystick to walk, right thumb = drag to look around.
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/bar_scene.h"
#include "app/jukebox.h"
#include "engine/audio.h"
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
constexpr float kTapReach = 4.0f;            // how far away a tap can use things
constexpr float kHighlightGlow = 0.35f;      // extra glow on the thing you're at
constexpr double kBannerMs = 7000;           // "now playing" banner duration
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
  app::Jukebox jukebox;
  app::JukeboxMenu jukeboxMenu;
  engine::TextRenderer text;
  FrameUniforms frameUniforms = {};

  wgpu::RenderPipeline pipeline;
  wgpu::Buffer frameBuffer;
  wgpu::Buffer objectBuffer;
  wgpu::BindGroup frameBindGroup;
  wgpu::BindGroup objectBindGroup;
  std::vector<DrawItem> draws;
  std::vector<ObjectUniforms> objectUniforms;  // CPU copy, for highlighting

  int focused = -1;      // interactable you're standing at and facing
  int highlighted = -1;  // interactable currently drawn glowing
  int bannerSong = 0;
  double bannerEndMs = 0;

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
    g_app.objectUniforms.push_back(u);
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
  if (g_app.jukeboxMenu.isOpen()) return;  // stand still while choosing a song
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
  for (const app::Interactable& it : g_app.scene.interactables) {
    if (g_app.jukeboxMenu.isOpen()) break;  // keep the menu uncluttered
    const float dist = engine::length(it.tagPosition - cam.position);
    if (dist > kLabelShowDistance) continue;
    // Fade in as you approach; the tag always turns to face you.
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {1.0f, 0.86f, 0.62f, engine::clampf((kLabelShowDistance - dist) / 1.5f, 0, 1)};
    style.outline.w *= style.color.w;
    style.outlineWidth = 0.8f;
    text.addWorld(it.name, it.tagPosition, right, up, 0.1f, style);
  }
  if (g_app.focused >= 0 && !g_app.jukeboxMenu.isOpen()) {
    const app::Interactable& it = g_app.scene.interactables[static_cast<size_t>(g_app.focused)];
    engine::TextStyle style;
    style.align = 0.5f;
    style.outlineWidth = 0.8f;
    const std::string prompt = it.action == app::Action::kJukebox ? g_app.jukebox.prompt() : it.prompt;
    text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, style);
  }

  // "Now playing" banner with the song credit, whenever a new song starts.
  const app::Song* song = g_app.jukebox.currentSong();
  if (song && nowMs < g_app.bannerEndMs) {
    const float fade = engine::clampf(static_cast<float>((g_app.bannerEndMs - nowMs) / 1000.0), 0, 1);
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {1.0f, 0.86f, 0.62f, fade};
    style.outline.w *= fade;
    style.outlineWidth = 0.8f;
    text.addScreen("Now playing: " + song->title, W * 0.5f, H * 0.11f, 12 * px, style);
    style.color = {0.85f, 0.85f, 0.85f, fade};
    text.addScreen(song->builtIn ? app::kMusicCredit : "Your song (only on this phone)", W * 0.5f,
                   H * 0.11f + 22 * px, 9 * px, style);
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
    text.addScreen("Left thumb: walk\nRight thumb: look around\nTap things to use them", W * 0.5f, H * 0.2f + 52 * px, 11 * px, hint);
  }
  g_app.jukeboxMenu.draw(text, g_app.jukebox, W, H, px);  // last, so it's on top
  text.upload(g_app.gpu.queue);
}

// Slab test: distance along the ray to the box, or -1 if it misses.
float rayBoxDistance(Vec3 origin, Vec3 dir, Vec3 boxMin, Vec3 boxMax) {
  float tNear = 0, tFar = 1e9f;
  const float o[3] = {origin.x, origin.y, origin.z}, d[3] = {dir.x, dir.y, dir.z};
  const float lo[3] = {boxMin.x, boxMin.y, boxMin.z}, hi[3] = {boxMax.x, boxMax.y, boxMax.z};
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(d[i]) < 1e-8f) {
      if (o[i] < lo[i] || o[i] > hi[i]) return -1;
      continue;
    }
    float t0 = (lo[i] - o[i]) / d[i], t1 = (hi[i] - o[i]) / d[i];
    if (t0 > t1) std::swap(t0, t1);
    tNear = std::max(tNear, t0);
    tFar = std::min(tFar, t1);
    if (tNear > tFar) return -1;
  }
  return tNear;
}

// The world-space ray through a point on the screen (0..1 fractions).
Vec3 screenRay(engine::Vec2 tap) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  const Vec3 up = engine::cross(right, forward);
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  const float tanHalf = std::tan(cam.fovY * 0.5f);
  const float x = (tap.x * 2 - 1) * tanHalf * aspect, y = (1 - tap.y * 2) * tanHalf;
  return engine::normalize(forward + right * x + up * y);
}

void use(const app::Interactable& it) {
  if (it.action == app::Action::kJukebox) g_app.jukeboxMenu.open();
}

// Makes one interactable's parts glow (or stop glowing).
void setGlow(int index, float glow) {
  if (index < 0) return;
  const app::Interactable& it = g_app.scene.interactables[static_cast<size_t>(index)];
  for (size_t i = it.firstObject; i < it.firstObject + it.objectCount; ++i) {
    ObjectUniforms u = g_app.objectUniforms[i];
    u.material[0] += glow;
    g_app.gpu.queue.WriteBuffer(g_app.objectBuffer, i * kObjectStride, &u, sizeof(u));
  }
}

// Works out what you're standing at, handles taps, and keeps the music and
// your "ears" in sync with the camera.
void updateInteraction(double nowMs) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const auto& items = g_app.scene.interactables;

  g_app.focused = -1;
  float bestFacing = 0.8f;  // must be looking roughly toward it
  for (size_t i = 0; i < items.size(); ++i) {
    const Vec3 center = (items[i].boundsMin + items[i].boundsMax) * 0.5f;
    const Vec3 toFlat{center.x - cam.position.x, 0, center.z - cam.position.z};
    const float facing = engine::dot(engine::normalize(toFlat), cam.flatForward());
    if (engine::length(toFlat) < kPromptDistance && facing > bestFacing) {
      bestFacing = facing;
      g_app.focused = static_cast<int>(i);
    }
  }
  if (g_app.focused != g_app.highlighted) {
    setGlow(g_app.highlighted, 0);
    setGlow(g_app.focused, kHighlightGlow);
    g_app.highlighted = g_app.focused;
  }

  // A tap uses what it lands on; tapping anywhere also works while you're
  // standing at something (easier on a small screen).
  for (const engine::Vec2 tap : g_app.input.takeTaps()) {
    g_app.introEndMs = std::min(g_app.introEndMs, nowMs + kIntroAfterInputMs);
    if (g_app.jukeboxMenu.isOpen()) {
      g_app.jukeboxMenu.tap(tap, g_app.jukebox);
      continue;
    }
    const Vec3 dir = screenRay(tap);
    int hit = -1;
    float nearest = kTapReach;
    for (size_t i = 0; i < items.size(); ++i) {
      const float t = rayBoxDistance(cam.position, dir, items[i].boundsMin, items[i].boundsMax);
      if (t >= 0 && t < nearest) {
        nearest = t;
        hit = static_cast<int>(i);
      }
    }
    if (hit < 0) hit = g_app.focused;
    if (hit >= 0) use(items[static_cast<size_t>(hit)]);
  }

  g_app.jukebox.update();
  if (g_app.jukebox.songsStarted() != g_app.bannerSong) {
    g_app.bannerSong = g_app.jukebox.songsStarted();
    g_app.bannerEndMs = nowMs + kBannerMs;
  }
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  engine::audio::setListener(cam.position, forward, engine::cross(right, forward));
}

void frame() {
  const double nowMs = emscripten_get_now();
  // Clamp the time step so a stall (e.g. switching apps) doesn't teleport you.
  const float dt = std::min(static_cast<float>((nowMs - g_app.lastFrameMs) / 1000.0), 0.1f);
  g_app.lastFrameMs = nowMs;

  g_app.gpu.resizeIfNeeded();
  updateFps(nowMs);
  updateCamera(dt);
  updateInteraction(nowMs);

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
  g_app.jukebox.init(g_app.scene.jukeboxSpeaker);
  g_app.lastFrameMs = g_app.fpsWindowStartMs = emscripten_get_now();
  g_app.introEndMs = g_app.lastFrameMs + kIntroMaxMs;
  LOG_INFO("Running. Left thumb: walk. Right thumb: look around. Tap to use things.");
  emscripten_set_main_loop(frame, 0, false);
}

}  // namespace

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  g_app.gpu.init("#canvas", onGpuReady);
  return 0;
}
