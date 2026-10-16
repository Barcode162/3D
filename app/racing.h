#pragma once
// The racing game on the two sim rigs: a retro "pseudo-3D" road racer (like
// the old arcade racers): a winding, hilly loop, five AI cars, three laps,
// lap times. You see it full screen while you drive; everyone else sees the
// same road on the rig's middle monitor while your avatar steers.
// Touch: the two left buttons steer, the right ones are brake and gas.
// Keys: arrows (or WASD-style lanes D F J K: left, right, brake, gas).
#include <functional>
#include <string>
#include <vector>

#include "engine/audio.h"
#include "engine/input.h"
#include "engine/math.h"
#include "engine/text.h"

namespace app {

class Racing {
 public:
  void init(const engine::Sound* engineHum);
  bool open() const { return open_; }
  void start();
  void close();
  // Returns true when you tapped "stand up".
  bool update(double nowMs, float dt, const std::vector<engine::Input::RawPress>& presses, float W, float H);
  void draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;
  float steer() const { return steer_; }

  // What others need to draw your view: [track position, lateral position,
  // speed, lap, steering].
  void stateFor(float out[5]) const;

  // Draws a driver's view (from a state as above) into any 4-cornered area:
  // `corners` = bottom-left, bottom-right, top-right, top-left. Used for the
  // monitors in the bar (world quads) and the full-screen view.
  struct View {
    float position = 0, x = 0, speed = 0;
  };
  // emit(corners, color): a quad in 0..1 screen coordinates (u right,
  // v down), corners in order. `aspect` = width / height of the area.
  using Emit = std::function<void(const engine::Vec2* corners, engine::Vec4 color)>;
  static void render(const View& view, int segments, float aspect, const Emit& emit,
                     const std::vector<float>* cars = nullptr);

 private:
  bool open_ = false;
  float position_ = 0;  // distance along the track (game units)
  float x_ = 0;         // -1 .. 1 = road edges
  float speed_ = 0;
  float steer_ = 0;
  int lap_ = 1;
  double lapStartMs_ = 0, raceStartMs_ = 0;
  double lastLapMs_ = 0, bestLapMs_ = 0, finishMs_ = 0;
  bool finished_ = false;
  bool left_ = false, right_ = false, gas_ = false, brake_ = false;
  struct Held {
    int id, button;
  };
  std::vector<Held> held_;
  std::vector<float> cars_;  // AI cars: (position, x, speed) triples
  const engine::Sound* hum_ = nullptr;
  int humVoice_ = 0;
  int place_ = 1;
};

}  // namespace app
