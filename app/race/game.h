#pragma once
// The racing game as a whole: sitting in a rig dives you into it (the
// whole screen becomes the race). Owns the race world, the 3D view, the
// controls, the cameras, the HUD and the engine sound.
#include <webgpu/webgpu_cpp.h>

#include <string>

#include "app/race/controls.h"
#include "app/race/view.h"
#include "app/race/world.h"
#include "engine/audio.h"
#include "engine/gpu.h"
#include "engine/input.h"
#include "engine/text.h"

namespace app::race {

class Game {
 public:
  void init(engine::Gpu* gpu, const engine::Sound* engineHum) {
    gpu_ = gpu;
    hum_ = engineHum;
    view_.preparePipelines(*gpu);  // compile them while the bar loads
  }
  bool open() const { return open_; }
  bool pipelinesReady() const { return view_.pipelinesReady(); }
  // Sit down in rig 0 (left) or 1 (right).
  void start(int rig, double nowMs);
  void close();
  // Returns true when you chose to leave.
  bool update(double nowMs, float dt, engine::Input& input);
  // Draws the race full screen, with the HUD (queued into `text`, which
  // must be begun for this frame) on top.
  void render(const wgpu::TextureView& target, engine::TextRenderer& text, double nowMs);

  float steer() const;  // for your avatar's hands
  // What others need: x, z, heading, speed (m/s), steer.
  void stateFor(float out[5]) const;

 private:
  void updateCamera(float dt);
  void queueHud(engine::TextRenderer& text, float W, float H, float px, double nowMs);

  engine::Gpu* gpu_ = nullptr;
  const engine::Sound* hum_ = nullptr;
  int humVoice_ = 0;
  bool ready_ = false, open_ = false;
  World world_;
  View view_;
  Controls controls_;
  Camera camera_;
  int cameraMode_ = 0;
  float camYaw_ = 0;
  engine::Vec3 camPos_;
  bool manualGears_ = false;
  int rig_ = 0;
  double startMs_ = 0;
  double resultsMs_ = 0;
  std::string build_;
};

}  // namespace app::race
