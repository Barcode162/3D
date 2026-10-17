#pragma once
// One race: the cars (you, maybe another player, bots), the fixed-rate
// physics loop, collisions (car vs car, car vs barrier), the bots' driving,
// the start (grid, lights), laps, positions and results. Everything runs on
// a tick counter (240 per second), never on the wall clock, so a race comes
// out the same at any frame rate.
#include <cstdint>
#include <string>
#include <vector>

#include "app/race/track.h"
#include "engine/vehicle.h"

namespace app::race {

struct Racer {
  engine::Vehicle car;
  engine::CarInput input;
  engine::CarAssists assists;
  engine::Vec3 paint{0.7f, 0.05f, 0.04f};
  std::string name;
  bool bot = true;
  bool remote = false;      // another player's car (moved by the network)
  // Track progress.
  float s = 0, lateral = 0;
  int hint = -1;
  int lap = 0;              // laps completed
  bool halfway = false;     // passed the middle of the lap (no cutting back)
  float distance = 0;       // total race distance (for positions)
  int place = 0;
  // Times on the race clock (ms from the start of the race, counted in
  // physics steps, so they don't depend on the frame rate).
  double lapStartMs = 0, lastLapMs = 0, bestLapMs = 0, finishMs = 0;
  bool finished = false;
  // The pose one physics step ago (drawing goes between it and now).
  engine::Vec3 prevPosition;
  engine::Quat prevRotation;
  // Bot state.
  float offset = 0;         // lateral offset from the racing line (overtaking)
  float skill = 1;          // 1 = at the limit
  float stuckTime = 0;
  float flipTime = 0;      // seconds upside down / on the side
  // Resets: the car passes through others for a moment after one (ghost),
  // and you can't ask for another right away.
  uint32_t ghostUntil = 0, lastResetTick = 0;
  bool ghost(uint32_t tick) const { return tick < ghostUntil; }
};

class World {
 public:
  void init();  // builds the track and the bots' racing line
  const Track& track() const { return track_; }

  static constexpr int kTicksPerSecond = 240;
  // A new race: `you` = your grid slot, `bots` = how many bots. `seed`
  // picks how long the lights stay on (the same seed gives the same race).
  void start(int yourSlot, int bots, int laps, uint32_t seed);
  // Advances the race by a frame of `dt` seconds (fixed 240 Hz steps inside).
  void update(float dt);
  // The race clock: ticks since start(), and the same in ms.
  uint32_t tick() const { return tick_; }
  double clockMs() const { return ticksToMs(tick_); }
  static double ticksToMs(uint32_t t) { return t * (1000.0 / kTicksPerSecond); }
  // How far between the last two physics steps this frame is (0..1), and a
  // racer's car posed there (for drawing and the camera).
  float alpha() const { return alpha_; }
  engine::Vehicle shownCar(const Racer& r) const;
  // Puts a racer back on the track, facing the right way, stopped, on a
  // spot away from other cars, as a ghost for 2 s.
  void resetCar(int index);
  // The reset button: refused within 4 s of the last reset. True if done.
  bool requestReset(int index);

  std::vector<Racer>& racers() { return racers_; }
  const std::vector<Racer>& racers() const { return racers_; }
  int you() const { return you_; }
  int laps() const { return laps_; }
  // Start lights: 0..5 lit, then out at go; true once racing.
  int lightsOn() const;
  bool started() const { return tick_ >= goTick_; }
  double goMs() const { return ticksToMs(goTick_); }
  bool allFinished() const;

  // Racing line (lateral offset per sample) and its target speed.
  const std::vector<float>& line() const { return line_; }
  const std::vector<float>& lineSpeed() const { return lineSpeed_; }

 private:
  void stepPhysics(float dt);
  void driveBot(Racer& r, int index);
  void collideWalls(Racer& r);
  void collideCars(Racer& a, Racer& b);
  void updateProgress(Racer& r);
  void buildRacingLine();

  Track track_;
  std::vector<Racer> racers_;
  std::vector<float> line_, lineSpeed_;
  int you_ = 0, laps_ = 3;
  uint32_t tick_ = 0, goTick_ = 0, lightsTick_ = 0;
  double accumulator_ = 0;  // seconds of frame time not yet simulated
  float alpha_ = 0;
};

engine::CarSpec sportsCar();

}  // namespace app::race
