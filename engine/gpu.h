#pragma once
// Owns the WebGPU connection: instance, adapter, device and the canvas
// surface. Setup is asynchronous in the browser, so init() takes a callback
// that runs once the device is ready.
#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <functional>
#include <string>

namespace engine {

class Gpu {
 public:
  // canvasSelector is a CSS selector, e.g. "#canvas".
  void init(const char* canvasSelector, std::function<void(bool ok)> onReady);

  // Matches the drawing buffer to the canvas's on-screen size. Call every
  // frame; it only reconfigures when the size actually changed.
  void resizeIfNeeded();

  // Returns the texture to draw into this frame, or nullptr if unavailable.
  wgpu::TextureView beginFrame();

  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
  static constexpr wgpu::TextureFormat kDepthFormat = wgpu::TextureFormat::Depth24Plus;
  // Depth buffer matching the screen size; recreated on resize.
  wgpu::TextureView depthView;
  uint32_t width = 0;
  uint32_t height = 0;
  float pixelRatio = 1.0f;
  double cssWidth = 0, cssHeight = 0;  // the canvas in CSS pixels (measured once per frame)

 private:
  void onAdapter(wgpu::Adapter adapter);
  void onDevice(wgpu::Device device);
  void configureSurface();

  std::string canvasSelector_;
  std::function<void(bool)> onReady_;
  wgpu::Instance instance_;
  wgpu::Adapter adapter_;
  wgpu::Surface surface_;
};

}  // namespace engine
