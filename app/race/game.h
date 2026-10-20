#pragma once
// The racing game as a whole: sitting in a rig dives you into it (the
// whole screen becomes the race). First the garage (pick a car, its paint
// and rims), then the race: against the other rig's player when they're
// ready too (the room runs the lobby and the start: cloud/games/race.js,
// world key rc), else against bots. Owns the race world, the 3D view, the
// controls, the cameras, the HUD and the engine sound.
#include <webgpu/webgpu_cpp.h>

#include <string>
#include <vector>

#include "app/race/controls.h"
#include "app/race/view.h"
#include "app/race/world.h"
#include "engine/audio.h"
#include "engine/gpu.h"
#include "engine/input.h"
#include "engine/text.h"

namespace app {
class OtherPlayers;
}

namespace app::race {

// What you picked in the garage.
struct CarChoice {
  int car = 0, paint = 0, rim = 0;
};

class Game {
 public:
  void init(engine::Gpu* gpu, engine::Sound engineHum, const app::OtherPlayers* players) {
    gpu_ = gpu;
    hum_ = engineHum;
    players_ = players;
    view_.preparePipelines(*gpu);  // compile them while the bar loads
  }
  bool open() const { return open_; }
  bool pipelinesReady() const { return view_.pipelinesReady(); }
  // Builds the track and its meshes in background jobs (call after you
  // enter the bar, so sitting in a rig later doesn't freeze the game).
  void prepare();
  bool built() const { return ready_; }
  // Sit down in rig 0 (left) or 1 (right): opens the garage (`straightIn`:
  // straight into a race with bots, for the ?sit= test option). Returns
  // false while the track is still being built: it then opens by itself
  // once it's ready (pending() until then; call tryPending each frame).
  bool start(int rig, double nowMs, bool straightIn = false, bool pad = false);
  bool pending() const { return pendingRig_ >= 0; }
  bool tryPending(double nowMs);
  void close();
  // Returns true when you chose to leave.
  bool update(double nowMs, float dt, const engine::InputFrame& input);
  // The room's racing lobby changed (world key rc).
  void onRoomRace(const std::vector<double>& values);
  // Draws the race full screen, with the HUD (queued into `text`, which
  // must be begun for this frame) on top.
  void render(const wgpu::TextureView& target, engine::TextRenderer& text, double nowMs);

  // The Car setup panel's numbers (web/menu.js "carSetup", JSON): applied at once.
  void setCarSetup(const std::string& json);
  // ?telemetry=1: the telemetry overlay on (also a switch in Car setup).
  void setTelemetry(bool on) { telemetryHook_ = on; }

  float steer() const;  // for your avatar's hands
  // What others need: x, z, heading, speed (m/s), steer.
  void stateFor(float out[5]) const;

 private:
  enum class Screen { kGarage, kRace };
  // The room's lobby (cloud/games/race.js values()).
  struct RoomRig {
    int slot = -1, car = 0, paint = 0, rim = 0, state = 0;
    double timeMs = 0;
  };
  struct RoomRace {
    int phase = 0, laps = 3, seed = 0;
    double goAt = 0;  // server seconds
    RoomRig rigs[2];
  };
  static constexpr int kGarage = 0, kReady = 1, kRacing = 2, kFinished = 3, kDnf = 4;

  bool online() const;
  void openGarage(double nowMs);
  void showChoice();  // your car on the garage floor, as picked
  void pickChanged();
  void sendRace(const char* op, bool solo = false);
  void updateNet(double nowMs);
  // Starts a race: the room's (with the other player, or you and bots) or a
  // local one with bots (offline, or while the room's race is someone else's).
  void beginRace(double nowMs, bool fromRoom);
  void updateRemote(double nowMs, float dt);
  bool updateGarage(double nowMs, float dt, const engine::InputFrame& input);
  void updateCamera(float dt);
  void garageCamera(double nowMs);
  void queueHud(engine::TextRenderer& text, float W, float H, float px, double nowMs);
  void queueGarage(engine::TextRenderer& text, float W, float H, double nowMs);
  void queueTelemetry(engine::TextRenderer& text, float W, float H, float u);
  void beginPractice(double nowMs, bool pad);
  // The garage's buttons this frame (pixels), for drawing and for taps.
  struct Hit {
    float x, y, w, h;
    int action, arg;
  };
  std::vector<Hit> garageButtons(float W, float H) const;
  std::string otherRigText() const;

  engine::Gpu* gpu_ = nullptr;
  const app::OtherPlayers* players_ = nullptr;
  engine::Sound hum_;
  int humVoice_ = 0;
  bool ready_ = false, open_ = false, prepared_ = false;
  int pendingRig_ = -1;
  bool pendingStraight_ = false, pendingPad_ = false;
  bool practice_ = false;     // the skid pad / Ring practice with the Ridgeback GS
  bool telemetryHook_ = false;
  engine::Vec3 gSmooth_;      // the g meter, lightly smoothed
  Screen screen_ = Screen::kGarage;
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
  double garageMs_ = 0;
  std::string build_;
  CarChoice choice_;
  bool wantReady_ = false;   // you pressed READY (waiting for the other rig)
  // Room side.
  RoomRace room_;
  bool haveRoom_ = false;
  bool joined_ = false;      // told the room we're in the garage (since we got the rig)
  int claimReq_ = 0;
  bool roomRace_ = false;    // the race running is the room's
  double raceGoAt_ = 0;      // its start (server s), so it starts once
  bool finishSent_ = false;
  int remoteRacer_ = -1;     // the other player's car in world_
  int remotePlayer_ = -1;    // ... and who they are (engine::net player)
  std::string remoteName_;
  bool remoteLive_ = false;  // their states arrive
  bool enterWas_ = false;
  std::vector<int> requests_;  // room answers still to read (and drop)
};

}  // namespace app::race
