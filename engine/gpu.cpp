#include "engine/gpu.h"

#include <emscripten/html5.h>

#include <algorithm>
#include <cmath>
#include <string_view>

#include "engine/log.h"

namespace engine {
namespace {

// Rendering at the phone's full 3x density costs a lot of GPU time for
// little visible gain, so cap it.
constexpr double kMaxPixelRatio = 2.0;

std::string toString(wgpu::StringView s) {
  if (!s.data) return {};
  return std::string(std::string_view(s));
}

}  // namespace

void Gpu::init(const char* canvasSelector, std::function<void(bool)> onReady) {
  canvasSelector_ = canvasSelector;
  onReady_ = std::move(onReady);

  instance_ = wgpu::CreateInstance(nullptr);
  if (!instance_) {
    LOG_ERROR("WebGPU is not available in this browser.");
    onReady_(false);
    return;
  }

  wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasSource;
  canvasSource.selector = canvasSelector_.c_str();
  wgpu::SurfaceDescriptor surfaceDesc;
  surfaceDesc.nextInChain = &canvasSource;
  surface_ = instance_.CreateSurface(&surfaceDesc);

  wgpu::RequestAdapterOptions options;
  options.powerPreference = wgpu::PowerPreference::HighPerformance;
  instance_.RequestAdapter(
      &options, wgpu::CallbackMode::AllowSpontaneous,
      [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
        if (status != wgpu::RequestAdapterStatus::Success) {
          LOG_ERROR("No GPU adapter: %s", toString(message).c_str());
          onReady_(false);
          return;
        }
        onAdapter(std::move(adapter));
      });
}

void Gpu::onAdapter(wgpu::Adapter adapter) {
  adapter_ = std::move(adapter);

  wgpu::AdapterInfo info;
  adapter_.GetInfo(&info);
  LOG_INFO("GPU: %s %s", toString(info.vendor).c_str(), toString(info.architecture).c_str());

  wgpu::DeviceDescriptor desc;
  desc.label = "main device";
  desc.SetUncapturedErrorCallback(
      [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
        LOG_ERROR("GPU error (%d): %s", static_cast<int>(type), toString(message).c_str());
      });
  desc.SetDeviceLostCallback(
      wgpu::CallbackMode::AllowSpontaneous,
      [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
        if (reason == wgpu::DeviceLostReason::Destroyed) return;
        LOG_ERROR("GPU device lost (%d): %s", static_cast<int>(reason), toString(message).c_str());
      });

  adapter_.RequestDevice(
      &desc, wgpu::CallbackMode::AllowSpontaneous,
      [this](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
        if (status != wgpu::RequestDeviceStatus::Success) {
          LOG_ERROR("Could not create GPU device: %s", toString(message).c_str());
          onReady_(false);
          return;
        }
        onDevice(std::move(device));
      });
}

void Gpu::onDevice(wgpu::Device dev) {
  device = std::move(dev);
  queue = device.GetQueue();

  wgpu::SurfaceCapabilities caps;
  surface_.GetCapabilities(adapter_, &caps);
  surfaceFormat = caps.formatCount > 0 ? caps.formats[0] : wgpu::TextureFormat::BGRA8Unorm;

  resizeIfNeeded();
  LOG_INFO("WebGPU ready. Surface format %d.", static_cast<int>(surfaceFormat));
  onReady_(true);
}

void Gpu::resizeIfNeeded() {
  double cssW = 0, cssH = 0;
  emscripten_get_element_css_size(canvasSelector_.c_str(), &cssW, &cssH);
  const double ratio = std::min(emscripten_get_device_pixel_ratio(), kMaxPixelRatio);
  const auto w = static_cast<uint32_t>(std::max(1.0, std::round(cssW * ratio)));
  const auto h = static_cast<uint32_t>(std::max(1.0, std::round(cssH * ratio)));
  if (w == width && h == height) return;

  width = w;
  height = h;
  pixelRatio = static_cast<float>(ratio);
  emscripten_set_canvas_element_size(canvasSelector_.c_str(), static_cast<int>(w), static_cast<int>(h));
  configureSurface();
}

void Gpu::configureSurface() {
  wgpu::SurfaceConfiguration config;
  config.device = device;
  config.format = surfaceFormat;
  config.width = width;
  config.height = height;
  config.alphaMode = wgpu::CompositeAlphaMode::Opaque;
  config.presentMode = wgpu::PresentMode::Fifo;
  surface_.Configure(&config);

  wgpu::TextureDescriptor depthDesc;
  depthDesc.label = "depth";
  depthDesc.size = {width, height, 1};
  depthDesc.format = kDepthFormat;
  depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
  depthView = device.CreateTexture(&depthDesc).CreateView();
}

wgpu::TextureView Gpu::beginFrame() {
  wgpu::SurfaceTexture surfaceTexture;
  surface_.GetCurrentTexture(&surfaceTexture);
  if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
      surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
    return nullptr;
  }
  return surfaceTexture.texture.CreateView();
}

}  // namespace engine
