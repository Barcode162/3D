#include "engine/gpu_upload.h"

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>

#include <algorithm>
#include <bit>
#include <deque>
#include <map>

#include "engine/log.h"
#include "engine/profiler.h"

namespace engine {
namespace {

std::deque<std::function<void()>>& jobs() {
  static std::deque<std::function<void()>> q;
  return q;
}

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

wgpu::RenderPipeline mipPipeline(MipKit& kit, wgpu::TextureFormat format) {
  auto it = kit.pipelines.find(format);
  if (it != kit.pipelines.end()) return it->second;
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
  wgpu::RenderPipeline pipeline = kit.device.CreateRenderPipeline(&rp);
  kit.pipelines[format] = pipeline;
  return pipeline;
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

void queueJob(std::function<void()> job) { jobs().push_back(std::move(job)); }

size_t pendingJobs() { return jobs().size(); }

void runJobs(double budgetMs) {
  const double start = emscripten_get_now();
  auto& q = jobs();
  int ran = 0;
  double maxJob = 0, before = start;
  while (!q.empty()) {
    std::function<void()> job = std::move(q.front());
    q.pop_front();
    job();
    ++ran;
    const double now = emscripten_get_now();
    maxJob = std::max(maxJob, now - before);
    before = now;
    if (now - start > budgetMs) break;
  }
  profiler().jobs(ran, q.size(), maxJob);
}

void generateMipmaps(const wgpu::Device& device, const wgpu::Texture& texture) {
  const uint32_t levels = texture.GetMipLevelCount();
  if (levels < 2) return;
  MipKit& kit = mipKit(device);
  const wgpu::TextureFormat format = texture.GetFormat();
  wgpu::RenderPipeline pipeline = mipPipeline(kit, format);
  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
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
  wgpu::CommandBuffer commands = encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}

void loadImageTexture(const wgpu::Device& device, const std::vector<uint8_t>& bytes, const std::string& mimeType,
                      std::vector<wgpu::TextureFormat> formats, const char* label,
                      std::function<void(std::vector<wgpu::Texture>)> done) {
  static int nextId = 1;
  const int id = nextId++;
  pendingImages()[id] = {std::move(formats), label ? label : "", device, std::move(done)};
  js_decodeBitmap(id, bytes.data(), static_cast<int>(bytes.size()), mimeType.c_str());
}

}  // namespace engine

// Called from JavaScript when a bitmap is ready (width 0 = failed). The
// GPU work waits in the job queue so it's spread over frames.
extern "C" EMSCRIPTEN_KEEPALIVE void engine_bitmapDecoded(int id, int width, int height) {
  using namespace engine;
  queueJob([id, width, height] {
    auto& pending = pendingImages();
    const auto it = pending.find(id);
    if (it == pending.end()) return;
    PendingImage p = std::move(it->second);
    pending.erase(it);
    std::vector<wgpu::Texture> textures;
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
        generateMipmaps(p.device, texture);
        textures.push_back(texture);
      }
    }
    js_dropBitmap(id);
    p.done(std::move(textures));
  });
}
