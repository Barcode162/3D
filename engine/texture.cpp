#include "engine/texture.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

#include "engine/log.h"

namespace engine {
namespace {

uint8_t toUnorm(float v) { return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f)); }

float linearToSrgb(float v) {
  v = std::clamp(v, 0.0f, 1.0f);
  return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
}

Image halve(const Image& src) {
  Image dst(src.size / 2);
  for (int y = 0; y < dst.size; ++y) {
    for (int x = 0; x < dst.size; ++x) {
      const Image::Pixel* p[4] = {&src.at(2 * x, 2 * y), &src.at(2 * x + 1, 2 * y), &src.at(2 * x, 2 * y + 1),
                                  &src.at(2 * x + 1, 2 * y + 1)};
      Image::Pixel& d = dst.at(x, y);
      d = {0, 0, 0, 0};
      for (const Image::Pixel* s : p) {
        d.r += s->r * 0.25f;
        d.g += s->g * 0.25f;
        d.b += s->b * 0.25f;
        d.a += s->a * 0.25f;
      }
    }
  }
  return dst;
}

}  // namespace

wgpu::Texture createTextureArray(const wgpu::Device& device, const std::vector<Image>& layers, bool srgb,
                                 const char* label) {
  if (layers.empty()) return nullptr;
  const int size = layers[0].size;
  if (!std::has_single_bit(static_cast<unsigned>(size))) LOG_ERROR("Texture size %d is not a power of two", size);
  const auto mipCount = static_cast<uint32_t>(std::bit_width(static_cast<unsigned>(size)));

  wgpu::TextureDescriptor desc;
  desc.label = label;
  desc.size = {static_cast<uint32_t>(size), static_cast<uint32_t>(size), static_cast<uint32_t>(layers.size())};
  desc.format = srgb ? wgpu::TextureFormat::RGBA8UnormSrgb : wgpu::TextureFormat::RGBA8Unorm;
  desc.mipLevelCount = mipCount;
  desc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
  wgpu::Texture texture = device.CreateTexture(&desc);
  wgpu::Queue queue = device.GetQueue();

  std::vector<uint8_t> bytes;
  for (size_t layer = 0; layer < layers.size(); ++layer) {
    Image level = layers[layer];
    for (uint32_t mip = 0; mip < mipCount; ++mip) {
      // Averaging happens in linear space; encoding to bytes happens last.
      bytes.resize(static_cast<size_t>(level.size) * level.size * 4);
      for (size_t i = 0; i < level.pixels.size(); ++i) {
        const Image::Pixel& p = level.pixels[i];
        bytes[i * 4 + 0] = toUnorm(srgb ? linearToSrgb(p.r) : p.r);
        bytes[i * 4 + 1] = toUnorm(srgb ? linearToSrgb(p.g) : p.g);
        bytes[i * 4 + 2] = toUnorm(srgb ? linearToSrgb(p.b) : p.b);
        bytes[i * 4 + 3] = toUnorm(p.a);
      }
      wgpu::TexelCopyTextureInfo dst;
      dst.texture = texture;
      dst.mipLevel = mip;
      dst.origin = {0, 0, static_cast<uint32_t>(layer)};
      wgpu::TexelCopyBufferLayout layout;
      layout.bytesPerRow = static_cast<uint32_t>(level.size) * 4;
      layout.rowsPerImage = static_cast<uint32_t>(level.size);
      const wgpu::Extent3D extent{static_cast<uint32_t>(level.size), static_cast<uint32_t>(level.size), 1};
      queue.WriteTexture(&dst, bytes.data(), bytes.size(), &layout, &extent);
      if (level.size > 1) level = halve(level);
    }
  }
  return texture;
}

}  // namespace engine
