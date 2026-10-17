#include "engine/gpu_upload.h"

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>

#include <algorithm>
#include <bit>
#include <deque>
#include <map>

#include "engine/log.h"
#include "engine/pipeline_cache.h"
#include "engine/profiler.h"

namespace engine {
namespace {


// --- Mipmaps on the GPU: draw each level from the one above with a
// filtered (bilinear) read of the middle of every 2x2 block.
const char* kMipShader = R"(
@group(0) @binding(0) var src: texture_2d<f32>;
@group(0) @binding(1) var smp: sampler;
struct VsOut { @builtin(position) pos: vec4f, @location(0) uv: vec2f };
@vertex fn vs(@builtin(vertex_index) i: u32) -> VsOut {
  let p = vec2f(f32((i << 1u) & 2u), f32(i & 2u));
  var o: VsOut;
  o.pos = vec4f(p * 2.0 - 1.0, 0.0, 1.0);
  o.uv = vec2f(p.x, 1.0 - p.y);
  return o;
}
@fragment fn fs(in: VsOut) -> @location(0) vec4f { return textureSample(src, smp, in.uv); }
)";

struct MipKit {
  wgpu::Device device;
  wgpu::ShaderModule module;
  wgpu::Sampler sampler;
  wgpu::BindGroupLayout layout;
  std::map<wgpu::TextureFormat, wgpu::RenderPipeline> pipelines;
};

MipKit& mipKit(const wgpu::Device& device) {
  static MipKit kit;
  if (kit.device.Get() == device.Get()) return kit;
  kit = {};
  kit.device = device;
  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = kMipShader;
  wgpu::ShaderModuleDescriptor md;
  md.nextInChain = &wgsl;
  md.label = "mipmaps";
  kit.module = device.CreateShaderModule(&md);
  wgpu::SamplerDescriptor sd;
  sd.minFilter = wgpu::FilterMode::Linear;
  sd.magFilter = wgpu::FilterMode::Linear;
  kit.sampler = device.CreateSampler(&sd);
  wgpu::BindGroupLayoutEntry e[2];
  e[0].binding = 0;
  e[0].visibility = wgpu::ShaderStage::Fragment;
  e[0].texture.sampleType = wgpu::TextureSampleType::Float;
  e[1].binding = 1;
  e[1].visibility = wgpu::ShaderStage::Fragment;
  e[1].sampler.type = wgpu::SamplerBindingType::Filtering;
  wgpu::BindGroupLayoutDescriptor ld;
  ld.entryCount = 2;
  ld.entries = e;
  kit.layout = device.CreateBindGroupLayout(&ld);
  return kit;
}

// Starts compiling the mip pipeline for a format (once); empty until ready.
wgpu::RenderPipeline mipPipeline(MipKit& kit, wgpu::TextureFormat format) {
  auto it = kit.pipelines.find(format);
  if (it != kit.pipelines.end()) return it->second;
  kit.pipelines[format] = nullptr;
  wgpu::PipelineLayoutDescriptor pl;
  pl.bindGroupLayoutCount = 1;
  pl.bindGroupLayouts = &kit.layout;
  wgpu::ColorTargetState target;
  target.format = format;
  wgpu::FragmentState fs;
  fs.module = kit.module;
  fs.entryPoint = "fs";
  fs.targetCount = 1;
  fs.targets = &target;
  wgpu::RenderPipelineDescriptor rp;
  rp.label = "mipmaps";
  rp.layout = kit.device.CreatePipelineLayout(&pl);
  rp.vertex.module = kit.module;
  rp.vertex.entryPoint = "vs";
  rp.fragment = &fs;
  pipelines::create(kit.device, rp, &kit.pipelines[format]);
  return nullptr;
}

// --- Image decoding in the browser, kept as ImageBitmaps until a job
// copies them into textures.
struct PendingImage {
  std::vector<wgpu::TextureFormat> formats;
  std::string label;
  wgpu::Device device;
  std::function<void(std::vector<wgpu::Texture>)> done;
};
std::map<int, PendingImage>& pendingImages() {
  static std::map<int, PendingImage> p;
  return p;
}

// At most kMaxDecodes images decode at once (each decoded bitmap holds its
// full pixels in memory until its job copies it); the rest wait here.
constexpr int kMaxDecodes = 4;
int g_decoding = 0;
struct WaitingDecode {
  int id;
  std::vector<uint8_t> bytes;
  std::string mime;
};
std::deque<WaitingDecode>& waitingDecodes() {
  static std::deque<WaitingDecode> w;
  return w;
}

EM_JS(void, js_decodeBitmap, (int id, const uint8_t* data, int size, const char* mime), {
  var bytes = HEAPU8.slice(data, data + size);
  var blob = new Blob([bytes], {type: UTF8ToString(mime)});
  var store = Module['bitmaps'] || (Module['bitmaps'] = {});
  createImageBitmap(blob, {'colorSpaceConversion': 'none', 'premultiplyAlpha': 'none'}).then(function(bitmap) {
    store[id] = bitmap;
    _engine_bitmapDecoded(id, bitmap['width'], bitmap['height']);
  }).catch(function(e) {
    err('Image decode failed: ' + e);
    _engine_bitmapDecoded(id, 0, 0);
  });
});

EM_JS_DEPS(gpu_upload, "$WebGPU");
// Copies bitmap `id` into mip 0 of the texture. Returns 0 on failure.
EM_JS(int, js_copyBitmap, (int id, WGPUDevice device, WGPUTexture texture), {
  var bitmap = Module['bitmaps'][id];
  try {
    // Quoted names: the closure compiler must not rename them.
    var dev = WebGPU.getJsObject(device);
    dev['queue']['copyExternalImageToTexture']({'source': bitmap}, {'texture': WebGPU.getJsObject(texture)},
                                               [bitmap['width'], bitmap['height']]);
    return 1;
  } catch (e) {
    err('Texture copy failed: ' + e);
    return 0;
  }
});

EM_JS(void, js_dropBitmap, (int id), {
  var store = Module['bitmaps'];
  if (store[id]) store[id]['close']();
  delete store[id];
});

}  // namespace

namespace {
const wgpu::Device* g_mipDevice = nullptr;
}  // namespace

void prepareMipmaps(const wgpu::Device& device) {
  g_mipDevice = &device;
  MipKit& kit = mipKit(device);
  mipPipeline(kit, wgpu::TextureFormat::RGBA8Unorm);
  mipPipeline(kit, wgpu::TextureFormat::RGBA8UnormSrgb);
  // Texture jobs make mipmaps: jobs wait until both mip pipelines have
  // compiled (a few ms after start-up) rather than compile one mid-load.
  setJobGate([] {
    MipKit& k = mipKit(*g_mipDevice);
    return k.pipelines[wgpu::TextureFormat::RGBA8Unorm] && k.pipelines[wgpu::TextureFormat::RGBA8UnormSrgb];
  });
}

void encodeMipmaps(const wgpu::Device& device, const wgpu::CommandEncoder& encoder, const wgpu::Texture& texture) {
  const uint32_t levels = texture.GetMipLevelCount();
  if (levels < 2) return;
  MipKit& kit = mipKit(device);
  const wgpu::TextureFormat format = texture.GetFormat();
  wgpu::RenderPipeline pipeline = mipPipeline(kit, format);
  for (uint32_t level = 1; level < levels; ++level) {
    wgpu::TextureViewDescriptor src;
    src.baseMipLevel = level - 1;
    src.mipLevelCount = 1;
    wgpu::TextureViewDescriptor dst;
    dst.baseMipLevel = level;
    dst.mipLevelCount = 1;
    wgpu::BindGroupEntry be[2];
    be[0].binding = 0;
    be[0].textureView = texture.CreateView(&src);
    be[1].binding = 1;
    be[1].sampler = kit.sampler;
    wgpu::BindGroupDescriptor bd;
    bd.layout = kit.layout;
    bd.entryCount = 2;
    bd.entries = be;
    wgpu::BindGroup group = device.CreateBindGroup(&bd);
    wgpu::RenderPassColorAttachment color;
    color.view = texture.CreateView(&dst);
    color.loadOp = wgpu::LoadOp::Clear;
    color.storeOp = wgpu::StoreOp::Store;
    wgpu::RenderPassDescriptor pass;
    pass.colorAttachmentCount = 1;
    pass.colorAttachments = &color;
    wgpu::RenderPassEncoder p = encoder.BeginRenderPass(&pass);
    p.SetPipeline(pipeline);
    p.SetBindGroup(0, group);
    p.Draw(3);
    p.End();
  }
}

void generateMipmaps(const wgpu::Device& device, const wgpu::Texture& texture) {
  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  encodeMipmaps(device, encoder, texture);
  wgpu::CommandBuffer commands = encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}

void startDecodes() {
  auto& waiting = waitingDecodes();
  while (g_decoding < kMaxDecodes && !waiting.empty()) {
    WaitingDecode w = std::move(waiting.front());
    waiting.pop_front();
    ++g_decoding;
    js_decodeBitmap(w.id, w.bytes.data(), static_cast<int>(w.bytes.size()), w.mime.c_str());
  }
}

void loadImageTexture(const wgpu::Device& device, std::vector<uint8_t> bytes, const std::string& mimeType,
                      std::vector<wgpu::TextureFormat> formats, const char* label,
                      std::function<void(std::vector<wgpu::Texture>)> done) {
  static int nextId = 1;
  const int id = nextId++;
  pendingImages()[id] = {std::move(formats), label ? label : "", device, std::move(done)};
  waitingDecodes().push_back({id, std::move(bytes), mimeType});
  startDecodes();
}

}  // namespace engine

// Called from JavaScript when a bitmap is ready (width 0 = failed). The
// GPU work waits in the job queue so it's spread over frames.
extern "C" EMSCRIPTEN_KEEPALIVE void engine_bitmapDecoded(int id, int width, int height) {
  using namespace engine;
  --g_decoding;
  startDecodes();
  queueJob([id, width, height] {
    auto& pending = pendingImages();
    const auto it = pending.find(id);
    if (it == pending.end()) {
      js_dropBitmap(id);  // always free the bitmap, even if nobody waits for it
      return;
    }
    PendingImage p = std::move(it->second);
    pending.erase(it);
    std::vector<wgpu::Texture> textures;
    wgpu::CommandEncoder encoder = p.device.CreateCommandEncoder();  // one submit for all mip work
    if (width > 0 && height > 0) {
      for (wgpu::TextureFormat format : p.formats) {
        wgpu::TextureDescriptor desc;
        desc.label = p.label.c_str();
        desc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        desc.format = format;
        desc.mipLevelCount = static_cast<uint32_t>(std::bit_width(static_cast<unsigned>(std::max(width, height))));
        desc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst |
                     wgpu::TextureUsage::RenderAttachment;
        wgpu::Texture texture = p.device.CreateTexture(&desc);
        if (!js_copyBitmap(id, p.device.Get(), texture.Get())) {
          textures.clear();
          break;
        }
        encodeMipmaps(p.device, encoder, texture);
        textures.push_back(texture);
      }
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    p.device.GetQueue().Submit(1, &commands);
    js_dropBitmap(id);
    p.done(std::move(textures));
  }, JobLane::kNormal, "texture");
}
