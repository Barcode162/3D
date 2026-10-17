#include "engine/model.h"

#include <emscripten/emscripten.h>

#include <cstddef>

#include "engine/fetch.h"
#include "engine/gpu_upload.h"
#include "engine/log.h"
#include "engine/profiler.h"
#include "engine/texture.h"

namespace engine {
namespace {

wgpu::Buffer makeBuffer(const wgpu::Device& device, const void* data, size_t size, wgpu::BufferUsage usage) {
  wgpu::BufferDescriptor desc;
  desc.size = (size + 3) & ~size_t{3};
  desc.usage = usage | wgpu::BufferUsage::CopyDst;
  wgpu::Buffer buffer = device.CreateBuffer(&desc);
  device.GetQueue().WriteBuffer(buffer, 0, data, size);
  return buffer;
}

wgpu::TextureView solidTexture(const wgpu::Device& device, uint8_t r, uint8_t g, uint8_t b, bool srgb) {
  const uint8_t pixel[4] = {r, g, b, 255};
  return createTexture2D(device, 1, 1, pixel, srgb, "solid").CreateView();
}

// Shared state while one model loads.
struct Loading {
  wgpu::Device device;
  wgpu::BindGroupLayout layout;
  wgpu::Sampler sampler;
  std::string url;
  ModelData data;
  // Per image: its texture as color (sRGB) and as data (linear), made
  // only for the uses the materials have.
  std::vector<wgpu::Texture> srgbTextures, linearTextures;
  size_t pendingImages = 0;
  double startMs = 0;
  std::function<void(std::shared_ptr<GpuModel>)> done;
};

}  // namespace

wgpu::BindGroupLayout GpuModel::createLayout(const wgpu::Device& device) {
  wgpu::BindGroupLayoutEntry e[7];
  e[0].binding = 1;
  e[0].visibility = wgpu::ShaderStage::Fragment;
  e[0].buffer.type = wgpu::BufferBindingType::Uniform;
  e[0].buffer.minBindingSize = sizeof(MaterialUniforms);
  for (uint32_t i = 1; i <= 4; ++i) {
    e[i].binding = i + 1;
    e[i].visibility = wgpu::ShaderStage::Fragment;
    e[i].texture.sampleType = wgpu::TextureSampleType::Float;
    e[i].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  }
  e[5].binding = 6;
  e[5].visibility = wgpu::ShaderStage::Fragment;
  e[5].sampler.type = wgpu::SamplerBindingType::Filtering;
  e[6].binding = 7;
  e[6].visibility = wgpu::ShaderStage::Fragment;
  e[6].texture.sampleType = wgpu::TextureSampleType::Float;
  e[6].texture.viewDimension = wgpu::TextureViewDimension::e2D;
  wgpu::BindGroupLayoutDescriptor desc;
  desc.entryCount = 7;
  desc.entries = e;
  return device.CreateBindGroupLayout(&desc);
}

wgpu::VertexBufferLayout GpuModel::vertexLayout() {
  static const wgpu::VertexAttribute attributes[] = {
      {.format = wgpu::VertexFormat::Float32x3, .offset = offsetof(ModelVertex, position), .shaderLocation = 0},
      {.format = wgpu::VertexFormat::Float32x3, .offset = offsetof(ModelVertex, normal), .shaderLocation = 1},
      {.format = wgpu::VertexFormat::Float32x2, .offset = offsetof(ModelVertex, uv), .shaderLocation = 2},
      {.format = wgpu::VertexFormat::Float32x4, .offset = offsetof(ModelVertex, tangent), .shaderLocation = 3},
  };
  wgpu::VertexBufferLayout layout;
  layout.arrayStride = sizeof(ModelVertex);
  layout.attributeCount = 4;
  layout.attributes = attributes;
  return layout;
}

namespace {

void finish(const std::shared_ptr<Loading>& l) {
  const wgpu::Device& device = l->device;
  auto model = std::make_shared<GpuModel>();
  model->boundsMin = l->data.boundsMin;
  model->boundsMax = l->data.boundsMax;

  // Textures, created once per image and shared by materials. Color images
  // are sRGB; data images (roughness, normals) are linear.
  std::vector<wgpu::TextureView> srgbViews(l->srgbTextures.size()), linearViews(l->linearTextures.size());
  const auto view = [&](int image, bool srgb, const wgpu::TextureView& fallback) {
    auto& textures = srgb ? l->srgbTextures : l->linearTextures;
    auto& views = srgb ? srgbViews : linearViews;
    if (image < 0 || static_cast<size_t>(image) >= textures.size() || !textures[static_cast<size_t>(image)]) {
      return fallback;
    }
    auto& slot = views[static_cast<size_t>(image)];
    if (!slot) slot = textures[static_cast<size_t>(image)].CreateView();
    return slot;
  };
  const wgpu::TextureView white = solidTexture(device, 255, 255, 255, true);
  const wgpu::TextureView whiteLinear = solidTexture(device, 255, 255, 255, false);
  const wgpu::TextureView flatNormal = solidTexture(device, 128, 128, 255, false);

  std::vector<ModelMaterial> materials = l->data.materials;
  materials.emplace_back();  // the default material, for parts without one
  for (const ModelMaterial& m : materials) {
    GpuModel::MaterialUniforms u = {};
    for (int i = 0; i < 4; ++i) u.baseColor[i] = m.baseColor[i];
    u.params[0] = m.metallic;
    u.params[1] = m.roughness;
    u.params[2] = m.normalScale;
    u.params[3] = m.occlusionStrength;
    u.emissive[0] = m.emissive.x;
    u.emissive[1] = m.emissive.y;
    u.emissive[2] = m.emissive.z;
    wgpu::BindGroupEntry e[7];
    e[0].binding = 1;
    e[0].buffer = makeBuffer(device, &u, sizeof(u), wgpu::BufferUsage::Uniform);
    e[0].size = sizeof(u);
    e[1].binding = 2;
    e[1].textureView = view(m.baseColorImage, true, white);
    e[2].binding = 3;
    e[2].textureView = view(m.metalRoughImage, false, whiteLinear);
    e[3].binding = 4;
    e[3].textureView = view(m.normalImage, false, flatNormal);
    e[4].binding = 5;
    e[4].textureView = view(m.occlusionImage, false, whiteLinear);
    e[5].binding = 6;
    e[5].sampler = l->sampler;
    e[6].binding = 7;
    e[6].textureView = view(m.emissiveImage, true, white);
    wgpu::BindGroupDescriptor desc;
    desc.layout = l->layout;
    desc.entryCount = 7;
    desc.entries = e;
    model->bindGroups.push_back(device.CreateBindGroup(&desc));
  }

  size_t triangles = 0;
  for (const ModelPrimitive& p : l->data.primitives) {
    GpuModel::Part part;
    part.vertexBuffer = makeBuffer(device, p.vertices.data(), p.vertices.size() * sizeof(ModelVertex),
                                   wgpu::BufferUsage::Vertex);
    part.indexBuffer = makeBuffer(device, p.indices.data(), p.indices.size() * sizeof(uint32_t),
                                  wgpu::BufferUsage::Index);
    part.indexCount = static_cast<uint32_t>(p.indices.size());
    const bool hasMaterial = p.material >= 0 && static_cast<size_t>(p.material) < l->data.materials.size();
    part.material = hasMaterial ? static_cast<size_t>(p.material) : l->data.materials.size();
    part.blend = hasMaterial && l->data.materials[static_cast<size_t>(p.material)].blend;
    model->hasBlend = model->hasBlend || part.blend;
    triangles += p.indices.size() / 3;
    model->parts.push_back(part);
  }
  LOG_INFO("Model %s: %zu parts, %zu triangles, %zu textures, %.0f ms.", l->url.c_str(), model->parts.size(),
           triangles, l->data.images.size(), emscripten_get_now() - l->startMs);
  {  // "assets/models/piano.glb.gz" -> "model:piano"
    std::string name = l->url.substr(l->url.find_last_of('/') + 1);
    name = name.substr(0, name.find('.'));
    profiler().stage("model:" + name, false);  // already logged above
  }
  l->done(model);
}

}  // namespace

void GpuModel::load(const wgpu::Device& device, const wgpu::BindGroupLayout& layout, const wgpu::Sampler& sampler,
                    const std::string& url, std::function<void(std::shared_ptr<GpuModel>)> done,
                    std::function<void(float)> progress) {
  auto l = std::make_shared<Loading>();
  l->device = device;
  l->layout = layout;
  l->sampler = sampler;
  l->url = url;
  l->done = std::move(done);
  l->startMs = emscripten_get_now();
  fetchFile(url, [l](bool ok, std::vector<uint8_t> bytes) {
    if (!ok) {
      LOG_ERROR("Model %s: download failed.", l->url.c_str());
      l->done(nullptr);
      return;
    }
    std::string error;
    if (!parseGlb(bytes, l->data, error)) {
      LOG_ERROR("Model %s: %s", l->url.c_str(), error.c_str());
      l->done(nullptr);
      return;
    }
    // Which images are colors and which are data (both is possible).
    const size_t count = l->data.images.size();
    std::vector<bool> asSrgb(count, false), asLinear(count, false);
    const auto use = [&](int image, bool srgb) {
      if (image >= 0 && static_cast<size_t>(image) < count) (srgb ? asSrgb : asLinear)[static_cast<size_t>(image)] = true;
    };
    for (const ModelMaterial& m : l->data.materials) {
      use(m.baseColorImage, true);
      use(m.emissiveImage, true);
      use(m.metalRoughImage, false);
      use(m.normalImage, false);
      use(m.occlusionImage, false);
    }
    l->srgbTextures.resize(count);
    l->linearTextures.resize(count);
    l->pendingImages = 0;
    for (size_t i = 0; i < count; ++i) l->pendingImages += asSrgb[i] || asLinear[i] ? 1 : 0;
    if (l->pendingImages == 0) {
      queueJob([l] { finish(l); });
      return;
    }
    for (size_t i = 0; i < count; ++i) {
      if (!asSrgb[i] && !asLinear[i]) continue;
      std::vector<wgpu::TextureFormat> formats;
      if (asSrgb[i]) formats.push_back(wgpu::TextureFormat::RGBA8UnormSrgb);
      if (asLinear[i]) formats.push_back(wgpu::TextureFormat::RGBA8Unorm);
      ModelImage& img = l->data.images[i];
      loadImageTexture(l->device, img.bytes, img.mimeType, formats, l->url.c_str(),
                       [l, i, srgb = static_cast<bool>(asSrgb[i])](std::vector<wgpu::Texture> textures) {
        if (textures.empty()) LOG_ERROR("Model %s: texture %zu could not be loaded.", l->url.c_str(), i);
        size_t k = 0;
        if (srgb && k < textures.size()) l->srgbTextures[i] = textures[k++];
        if (k < textures.size()) l->linearTextures[i] = textures[k];
        if (--l->pendingImages == 0) queueJob([l] { finish(l); });
      });
    }
    for (ModelImage& m : l->data.images) std::vector<uint8_t>().swap(m.bytes);  // the browser has its own copy
  }, std::move(progress));
}

}  // namespace engine
