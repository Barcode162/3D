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
  void init(engine::Gpu* gpu, engine::Sound engineHum) {
    gpu_ = gpu;
    hum_ = engineHum;
    view_.preparePipelines(*gpu);  // compile them while the bar loads
  }
  bool open() const { return open_; }
  bool pipelinesReady() const { return view_.pipelinesReady(); }
  // Builds the track and its meshes in background jobs (call after you
  // enter the bar, so sitting in a rig later doesn't freeze the game).
  void prepare();
  bool built() const { return ready_; }
  // Sit down in rig 0 (left) or 1 (right). Returns false while the track is
  // still being built: the race then starts by itself once it's ready
  // (pending() until then; call tryPending each frame).
  bool start(int rig, double nowMs);
  bool pending() const { return pendingRig_ >= 0; }
  bool tryPending(double nowMs);
  void close();
  // Returns true when you chose to leave.
  bool update(double nowMs, float dt, const engine::InputFrame& input);
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
  engine::Sound hum_;
  int humVoice_ = 0;
  bool ready_ = false, open_ = false, prepared_ = false;
  int pendingRig_ = -1;
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
