#pragma once
// One race: the cars (you, maybe another player, bots), the fixed-rate
// physics loop, collisions (car vs car, car vs barrier), the bots' driving,
// the start (grid, lights), laps, positions and results.
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
  double lapStartMs = 0, lastLapMs = 0, bestLapMs = 0, finishMs = 0;
  bool finished = false;
  // Bot state.
  float offset = 0;         // lateral offset from the racing line (overtaking)
  float skill = 1;          // 1 = at the limit
  float stuckTime = 0;
  float flipTime = 0;      // seconds upside down / on the side
};

class World {
 public:
  void init();  // builds the track and the bots' racing line
  const Track& track() const { return track_; }

  // A new race: `you` = your grid slot, `bots` = how many bots.
  void start(int yourSlot, int bots, int laps, double nowMs);
  // Advances the race by a frame (fixed 240 Hz steps inside).
  void update(double nowMs, float dt);
  // Puts a racer back on the track, facing the right way, stopped.
  void resetCar(int index);

  std::vector<Racer>& racers() { return racers_; }
  const std::vector<Racer>& racers() const { return racers_; }
  int you() const { return you_; }
  int laps() const { return laps_; }
  // Start lights: 0..5 lit, then out at go; true once racing.
  int lightsOn(double nowMs) const;
  bool started(double nowMs) const { return nowMs >= goMs_; }
  double goMs() const { return goMs_; }
  bool allFinished() const;

  // Racing line (lateral offset per sample) and its target speed.
  const std::vector<float>& line() const { return line_; }
  const std::vector<float>& lineSpeed() const { return lineSpeed_; }

 private:
  void stepPhysics(float dt, double nowMs);
  void driveBot(Racer& r, int index);
  void collideWalls(Racer& r);
  void collideCars(Racer& a, Racer& b);
  void updateProgress(Racer& r, double nowMs);
  void buildRacingLine();

  Track track_;
  std::vector<Racer> racers_;
  std::vector<float> line_, lineSpeed_;
  int you_ = 0, laps_ = 3;
  double goMs_ = 0, lightsMs_ = 0;
  double accumulator_ = 0;
  double simMs_ = 0;  // race clock (advances in fixed steps)
};

engine::CarSpec sportsCar();

}  // namespace app::race
