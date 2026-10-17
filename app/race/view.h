#pragma once
// Draws the racing world: sky, track chunks (culled to the view), cars
// (body, tail lights, four wheels each), start lights. Can draw into the
// screen or into any texture (the rigs' monitors).
#include <webgpu/webgpu_cpp.h>

#include <vector>

#include "app/race/track.h"
#include "engine/gpu.h"
#include "engine/math.h"
#include "engine/text.h"
#include "engine/vehicle.h"

namespace app::race {

struct Camera {
  engine::Vec3 eye, target, up{0, 1, 0};
  float fovY = 1.0f;  // radians
  float zNear = 0.1f, zFar = 2500;
};

struct CarDraw {
  engine::Mat4 body;
  engine::Mat4 wheels[4];
  engine::Vec3 paint;
  float brake = 0;  // 0..1: tail light glow
};

class View {
 public:
  bool preparePipelines(engine::Gpu& gpu);  // early, while the bar loads
  bool init(engine::Gpu& gpu, const Track& track, const engine::CarSpec& spec);
  bool pipelinesReady() const { return pipeline_ && skyPipeline_; }
  // Where a car's parts are this frame.
  static CarDraw carDraw(const engine::Vehicle& car, engine::Vec3 paint, float brake);
  // Draws one frame into `target` (with `depth`, both width x height). Lights:
  // how many start lights are on (0..5). `text` (optional) is drawn on top.
  void render(const wgpu::TextureView& target, const wgpu::TextureView& depth, uint32_t width, uint32_t height,
              const Camera& camera, const std::vector<CarDraw>& cars, int lights, float timeSec,
              const engine::TextRenderer* text);

 private:
  struct FrameUniforms {
    engine::Mat4 viewProj;
    float camPos[4], sunDir[4], sunColor[4], skyTop[4], skyHorizon[4], groundColor[4], fog[4];
    float camRight[4], camUp[4], camForward[4];
    float cars[16][4];
    uint32_t carCount[4];
  };
  struct ObjectUniforms {
    engine::Mat4 model;
    float tint[4];
    float params[4];
  };
  struct Part {
    wgpu::Buffer vertices, indices;
    uint32_t indexCount = 0;
    engine::Vec3 boundsMin, boundsMax;
  };
  static constexpr uint32_t kObjectStride = 256;
  static constexpr uint32_t kMaxObjects = 160;

  Part upload(const MeshBuilder& mesh);
  void draw(const wgpu::RenderPassEncoder& pass, const Part& part, uint32_t slot) const;

  engine::Gpu* gpu_ = nullptr;
  wgpu::RenderPipeline pipeline_, skyPipeline_;
  bool prepared_ = false;
  wgpu::BindGroupLayout frameLayout_, objectLayout_;
  wgpu::Buffer frameBuffer_, objectBuffer_;
  wgpu::BindGroup frameGroup_, objectGroup_;
  std::vector<Part> chunks_;
  Part body_, tailLights_, wheel_, lightPod_;
  std::vector<uint8_t> objectData_;
  engine::Vec3 lightsPos_, lightsRight_;
  float lightsHeading_ = 0;
};

}  // namespace app::race
