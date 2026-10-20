#include "bar/projects/racing/world.h"

#include <algorithm>
#include <cmath>

#include "bar/projects/racing/cars.h"

using engine::Vec3;

namespace app::race {
namespace {

constexpr float kStep = 1.0f / World::kTicksPerSecond;
constexpr int kMaxSteps = 30;  // per frame (0.125 s); beyond that the race slows down
constexpr uint32_t ticks(double ms) { return static_cast<uint32_t>(ms * World::kTicksPerSecond / 1000.0 + 0.5); }

const Vec3 kPaints[] = {{0.75f, 0.02f, 0.02f}, {0.02f, 0.15f, 0.7f}, {0.9f, 0.55f, 0.02f}, {0.04f, 0.4f, 0.1f},
                        {0.8f, 0.8f, 0.82f},   {0.02f, 0.02f, 0.02f}, {0.45f, 0.05f, 0.55f}, {0.0f, 0.5f, 0.6f},
                        {0.85f, 0.25f, 0.4f},  {0.5f, 0.45f, 0.1f}};
const char* kBotNames[] = {"Rossi", "Kimi", "Nina", "Ayrton", "Mika", "Lena", "Jules", "Sora", "Max", "Ines"};

}  // namespace

engine::CarSpec sportsCar() {
  engine::CarSpec s;
  s.front = {.z = 1.25f, .halfTrack = 0.79f, .spring = 62000, .bump = 3600, .rebound = 5600, .antiRoll = 34000,
             .travel = 0.12f, .restLength = 0.3f, .driven = false, .steered = true, .brakeShare = 0.64f,
             .downforce = 0.18f, .rollCenter = 0.08f};
  s.rear = {.z = -1.3f, .halfTrack = 0.78f, .spring = 58000, .bump = 3400, .rebound = 5200, .antiRoll = 10000,
            .travel = 0.12f, .restLength = 0.3f, .driven = true, .steered = false, .brakeShare = 0.36f,
            .downforce = 0.24f, .rollCenter = 0.12f, .grip = 1.2f};
  return s;
}

void World::init() {
  track_.build();
  road_.track = &track_;
  buildRacingLine();
}

bool World::Road::query(Vec3 p, engine::drive::RoadHit& hit, int& hint) const {
  Vec3 n;
  engine::Surface surf;
  const Vec3 g = track->groundAt(p, hint, &n, &surf);
  hit.height = g.y;
  hit.normal = n;
  hit.mu = surf.grip;
  hit.rolling = std::max(0.0f, surf.rolling - 0.012f);  // (asphalt's own is in the tire)
  hit.drag = surf.drag;
  return true;
}

float World::groundY(Vec3 p, int& hint) const { return track_.groundAt(p, hint).y; }

void World::attachDrive(Racer& r) {
  const CarModel& m = carModel(r.model);
  if (!m.newPhysics || r.bot || r.remote || setup_.oldPhysics) {
    r.drive.reset();
    return;
  }
  auto d = std::make_shared<engine::drive::Car>();
  d->setup(engine::drive::applySetup(engine::drive::ridgebackGS(), setup_));
  const Vec3 up = r.car.up();
  d->place(r.car.position - up * r.car.spec().cgHeight, engine::Vehicle::headingOf(r.car.forward()), up);
  const float v = r.car.speed();
  if (std::fabs(v) > 0.5f) d->setSpeed(v);
  r.drive = d;
  syncFromDrive(r);
}

void World::syncFromDrive(Racer& r) {
  const engine::drive::Car& d = *r.drive;
  engine::Vehicle& v = r.car;
  v.position = d.position;
  v.rotation = d.rotation;
  v.velocity = d.velocity;
  v.angularVelocity = d.angularVelocity;
  v.rpm = d.rpm;
  v.gear = d.gear;
  v.clutchEngage = d.clutchEngage;
  v.throttleUsed = d.throttleUsed;
  v.brakeUsed = d.brakeUsed;
  v.shiftTimer = d.shiftTimer;
  for (size_t i = 0; i < 4; ++i) {
    engine::Vehicle::Wheel& w = v.wheels[i];
    const engine::drive::Car::Wheel& dw = d.wheels[i];
    const engine::AxleSpec& a = i < 2 ? v.spec().front : v.spec().rear;
    // (The old drawing puts the wheel at mount - restLength + compression.)
    w.mount = dw.mount + Vec3{0, a.restLength, 0};
    w.compression = dw.travel;
    w.compressionSpeed = dw.travelSpeed;
    w.spin = dw.spin;
    w.angle = dw.angle;
    w.steer = -dw.steer;  // (old: + = right)
    w.slipRatio = dw.kappa;
    w.slipAngle = std::tan(dw.alpha);
    w.load = dw.load;
    w.fx = dw.fx;
    w.fy = dw.fy;
    w.slide = dw.slide;
    w.contact = dw.contact;
    w.contactPoint = dw.contactPoint;
    w.contactNormal = dw.contactNormal;
  }
}

void World::syncToDrive(Racer& r) {
  engine::drive::Car& d = *r.drive;
  d.position = r.car.position;
  d.rotation = r.car.rotation;
  d.velocity = r.car.velocity;
  d.angularVelocity = r.car.angularVelocity;
}

void World::setSetup(const engine::drive::Setup& setup) {
  const bool toggled = setup.oldPhysics != setup_.oldPhysics;
  setup_ = setup;
  for (Racer& r : racers_) {
    if (!carModel(r.model).newPhysics || r.bot || r.remote) continue;
    if (!toggled) {
      if (r.drive) r.drive->setSpec(engine::drive::applySetup(engine::drive::ridgebackGS(), setup_));
      continue;
    }
    // Switching physics: the same spot, heading and speed.
    const Vec3 p = r.car.position;
    const engine::Quat q = r.car.rotation;
    const float v = r.car.speed();
    if (setup_.oldPhysics) {
      r.drive.reset();
      r.car.setup(carModel(r.model).spec);
      r.car.place(p, 0);  // (resets the old car's insides; the pose is put back below)
      r.car.position = p;
      r.car.rotation = q;
      if (std::fabs(v) > 0.5f) r.car.setSpeed(v);
    } else {
      attachDrive(r);
    }
    r.prevPosition = r.car.position;
    r.prevRotation = r.car.rotation;
  }
}

void World::startPractice(const RacerSetup& me, bool pad) {
  racers_.clear();
  practice_ = true;
  practicePad_ = pad;
  laps_ = 1;
  you_ = 0;
  Racer r;
  r.model = me.model;
  r.car.setup(carModel(r.model).spec);
  Vec3 p;
  float heading;
  if (pad) track_.padStart(p, heading);
  else track_.gridSlot(0, p, heading);
  r.car.place(p, heading);
  r.bot = false;
  r.name = me.name;
  r.paint = me.paint;
  r.rim = me.rim;
  r.prevPosition = r.car.position;
  r.prevRotation = r.car.rotation;
  racers_.push_back(r);
  attachDrive(racers_.back());
  tick_ = goTick_ = lightsTick_ = 0;
  accumulator_ = 0;
  alpha_ = 0;
  Racer& you = racers_.back();
  track_.locate(you.car.position, you.s, you.lateral, you.hint);
}

void World::buildRacingLine() {
  // Start in the middle and repeatedly pull every point toward the average
  // of its neighbors (shortens and straightens the path), kept inside the
  // road: the result hugs the insides of corners like a real racing line.
  const auto& S = track_.samples();
  const int n = static_cast<int>(S.size());
  line_.assign(static_cast<size_t>(n), 0.0f);
  const auto pointOf = [&](int i, float lat) {
    const auto& c = S[static_cast<size_t>((i % n + n) % n)];
    return Vec3{c.pos.x + c.side.x * lat, 0, c.pos.z + c.side.z * lat};
  };
  for (int iter = 0; iter < 400; ++iter) {
    const int reach = iter < 200 ? 12 : 5;
    std::vector<float> next = line_;
    for (int i = 0; i < n; ++i) {
      const Vec3 a = pointOf(i - reach, line_[static_cast<size_t>((i - reach + n) % n)]);
      const Vec3 b = pointOf(i + reach, line_[static_cast<size_t>((i + reach) % n)]);
      const Vec3 mid = (a + b) * 0.5f;
      const auto& c = S[static_cast<size_t>(i)];
      const float target = engine::dot(mid - Vec3{c.pos.x, 0, c.pos.z}, c.side);
      const float limit = c.halfWidth - 1.4f;
      next[static_cast<size_t>(i)] = engine::clampf(line_[static_cast<size_t>(i)] + (target - line_[static_cast<size_t>(i)]) * 0.5f,
                                                    -limit, limit);
    }
    line_.swap(next);
  }
  // Target speeds from the line's curvature, then braking and acceleration
  // limits so bots slow down before corners, not in them.
  const float step = track_.length() / static_cast<float>(n);
  lineSpeed_.assign(static_cast<size_t>(n), 0.0f);
  for (int i = 0; i < n; ++i) {
    const Vec3 a = pointOf(i - 6, line_[static_cast<size_t>((i - 6 + n) % n)]);
    const Vec3 b = pointOf(i, line_[static_cast<size_t>(i)]);
    const Vec3 c = pointOf(i + 6, line_[static_cast<size_t>((i + 6) % n)]);
    const float ab = engine::length(b - a), bc = engine::length(c - b), ac = engine::length(c - a);
    const float area = std::fabs((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) * 0.5f;
    const float curvature = area > 1e-4f ? 4 * area / (ab * bc * ac) : 0;
    const float grip = 1.12f * 9.81f;
    lineSpeed_[static_cast<size_t>(i)] = std::min(78.0f, std::sqrt(grip / std::max(curvature, 1e-4f)));
  }
  for (int pass = 0; pass < 2; ++pass) {
    for (int k = 2 * n; k > 0; --k) {  // braking (backwards)
      const int i = k % n, j = (k + 1) % n;
      lineSpeed_[static_cast<size_t>(i)] = std::min(lineSpeed_[static_cast<size_t>(i)],
                                                    std::sqrt(lineSpeed_[static_cast<size_t>(j)] * lineSpeed_[static_cast<size_t>(j)] + 2 * 9.0f * step));
    }
    for (int k = 0; k < 2 * n; ++k) {  // accelerating (forwards)
      const int i = (k + 1) % n, j = k % n;
      lineSpeed_[static_cast<size_t>(i)] = std::min(lineSpeed_[static_cast<size_t>(i)],
                                                    std::sqrt(lineSpeed_[static_cast<size_t>(j)] * lineSpeed_[static_cast<size_t>(j)] + 2 * 6.0f * step));
    }
  }
}

void World::start(int yourSlot, int bots, int laps, uint32_t seed) {
  RacerSetup me;
  me.grid = std::clamp(yourSlot, 0, bots);
  start({me}, bots, laps, seed);
}

void World::start(const std::vector<RacerSetup>& people, int bots, int laps, uint32_t seed, double goInMs) {
  racers_.clear();
  practice_ = practicePad_ = false;
  laps_ = laps;
  const int total = bots + static_cast<int>(people.size());
  you_ = 0;
  for (int k = 0; k < total; ++k) {
    const RacerSetup* who = nullptr;
    for (const RacerSetup& p : people) {
      if (std::clamp(p.grid, 0, total - 1) == k) who = &p;
    }
    Racer r;
    r.model = who ? who->model : 0;
    r.car.setup(carModel(r.model).spec);
    Vec3 p;
    float heading;
    track_.gridSlot(k, p, heading);
    r.car.place(p, heading);
    r.bot = who == nullptr;
    r.remote = who && !who->you;
    if (who && who->you) you_ = k;
    r.name = r.bot ? kBotNames[k % 10] : who->name;
    r.paint = kPaints[k % 10];
    if (who) {
      r.paint = who->paint;
      r.rim = who->rim;
    } else if (k % 10 == 0) {
      r.paint = kPaints[(k + 3) % 10];
    }
    // Front-runners are a little quicker, like a real grid.
    r.skill = 0.9f + 0.08f * static_cast<float>(total - k) / static_cast<float>(total);
    r.car.wheels[0].groundHint = -1;
    r.prevPosition = r.car.position;
    r.prevRotation = r.car.rotation;
    racers_.push_back(r);
    attachDrive(racers_.back());
  }
  tick_ = 0;
  lightsTick_ = ticks(1500);
  goTick_ = lightsTick_ + ticks(5 * 900 + 400 + seed % 700);  // lights out after a random hold
  if (goInMs >= 0) {  // the room's start: the same moment on every phone
    goTick_ = ticks(goInMs);
    lightsTick_ = goTick_ > ticks(5 * 900 + 400 + seed % 700) ? goTick_ - ticks(5 * 900 + 400 + seed % 700) : 0;
  }
  accumulator_ = 0;
  alpha_ = 0;
  for (Racer& r : racers_) {
    track_.locate(r.car.position, r.s, r.lateral, r.hint);
    r.distance = r.s - track_.length();  // behind the line: negative
    r.lapStartMs = ticksToMs(goTick_);
  }
}

int World::lightsOn() const {
  if (tick_ >= goTick_ || tick_ < lightsTick_) return 0;
  return std::min(5, static_cast<int>((tick_ - lightsTick_) / ticks(900)) + 1);
}

engine::Vehicle World::shownCar(const Racer& r) const {
  engine::Vehicle v = r.car;
  v.position = engine::lerp(r.prevPosition, r.car.position, alpha_);
  v.rotation = engine::nlerp(r.prevRotation, r.car.rotation, alpha_);
  return v;
}

void World::setRemote(int index, float x, float z, float heading, float speed, float steer, float dt) {
  if (index < 0 || index >= static_cast<int>(racers_.size())) return;
  Racer& r = racers_[static_cast<size_t>(index)];
  engine::Vehicle& car = r.car;
  const float oldHeading = engine::Vehicle::headingOf(car.forward());
  int hint = r.hint;
  float s, lat;
  track_.locate({x, car.position.y, z}, s, lat, hint);
  Vec3 n;
  const Vec3 ground = track_.surfacePoint(s, lat, &n);
  // Sitting on the road there, facing `heading`, tilted with the surface.
  const engine::Quat yaw = engine::Quat::axisAngle({0, 1, 0}, heading);
  const Vec3 up = engine::normalize(n), axis = engine::cross(Vec3{0, 1, 0}, up);
  const float tiltAngle = std::asin(std::min(engine::length(axis), 1.0f));
  const engine::Quat tilt = tiltAngle > 1e-4f ? engine::Quat::axisAngle(engine::normalize(axis), tiltAngle) : engine::Quat{};
  car.rotation = tilt * yaw;
  car.position = Vec3{x, ground.y, z} + car.up() * car.spec().cgHeight;
  car.velocity = car.forward() * speed;
  car.angularVelocity = {0, dt > 0 ? engine::wrapAngle(heading - oldHeading) / dt : 0, 0};
  for (auto& w : car.wheels) {
    w.steer = w.front ? steer * car.spec().maxSteer * 0.85f : 0;
    w.spin = speed / car.spec().tire.radius;
    w.angle += w.spin * dt;
    w.compression = 0.04f;
  }
  r.prevPosition = car.position;
  r.prevRotation = car.rotation;
}

void World::setGoIn(double ms) {
  if (tick_ >= goTick_) return;  // already racing
  const uint32_t hold = goTick_ - lightsTick_;
  goTick_ = tick_ + ticks(std::max(ms, 0.0));
  lightsTick_ = goTick_ > hold ? goTick_ - hold : 0;
  for (Racer& r : racers_) r.lapStartMs = ticksToMs(goTick_);
}

bool World::allFinished() const {
  for (const Racer& r : racers_) {
    if (!r.finished && !r.remote && !r.hidden) return false;
  }
  return true;
}

bool World::requestReset(int index) {
  const Racer& r = racers_[static_cast<size_t>(index)];
  if (r.lastResetTick > 0 && tick_ - r.lastResetTick < ticks(4000)) return false;
  resetCar(index);
  return true;
}

void World::resetCar(int index) {
  Racer& r = racers_[static_cast<size_t>(index)];
  if (practice_ && practicePad_) {
    // On the pad: upright where you are (or back at the start, near the wall).
    Vec3 p = r.car.position;
    float heading = engine::Vehicle::headingOf(r.car.forward());
    const Vec3 c = track_.pad().centre;
    if (std::hypot(p.x - c.x, p.z - c.z) > track_.pad().asphalt - 4) track_.padStart(p, heading);
    p.y = c.y;
    r.car.place(p, heading);
    if (r.drive) {
      r.drive->place(p, heading);
      syncFromDrive(r);
    }
    r.prevPosition = r.car.position;
    r.prevRotation = r.car.rotation;
    r.stuckTime = r.flipTime = 0;
    r.lastResetTick = std::max(tick_, 1u);
    return;
  }
  // A free spot: here or a little further back, across the road, at least
  // 6 m from every other car (else here, and the ghost time sorts it out).
  const auto c = track_.at(r.s);
  float bestS = r.s, bestLat = engine::clampf(r.lateral, -c.halfWidth + 1.5f, c.halfWidth - 1.5f);
  bool found = false;
  for (float back : {0.0f, 8.0f, 16.0f}) {
    const float s = std::fmod(r.s - back + track_.length(), track_.length());
    const auto here = track_.at(s);
    const float limit = here.halfWidth - 1.5f;
    for (float lat : {r.lateral, 0.0f, -3.0f, 3.0f, -5.0f, 5.0f}) {
      lat = engine::clampf(lat, -limit, limit);
      const Vec3 p = track_.surfacePoint(s, lat);
      bool clear = true;
      for (size_t j = 0; j < racers_.size() && clear; ++j) {
        if (static_cast<int>(j) == index) continue;
        const Vec3 d = racers_[j].car.position - p;
        clear = d.x * d.x + d.z * d.z > 36;
      }
      if (clear) {
        bestS = s;
        bestLat = lat;
        found = true;
        break;
      }
    }
    if (found) break;
  }
  r.car.place(track_.surfacePoint(bestS, bestLat), engine::Vehicle::headingOf(track_.at(bestS).tangent));
  if (r.drive) {
    Vec3 n;
    const Vec3 g = track_.surfacePoint(bestS, bestLat, &n);
    r.drive->place(g, engine::Vehicle::headingOf(track_.at(bestS).tangent), n);
    syncFromDrive(r);
  }
  r.ghostUntil = tick_ + ticks(2000);
  r.lastResetTick = std::max(tick_, 1u);
  r.prevPosition = r.car.position;  // no streak in the drawing
  r.prevRotation = r.car.rotation;
  r.stuckTime = r.flipTime = 0;
}

void World::update(float dt) {
  accumulator_ += std::max(dt, 0.0f);
  int steps = 0;
  while (accumulator_ >= kStep && steps < kMaxSteps) {
    stepPhysics(kStep);
    accumulator_ -= kStep;
    ++steps;
  }
  if (steps == kMaxSteps) accumulator_ = 0;  // too slow: let the race run slower instead of spiraling
  alpha_ = static_cast<float>(std::clamp(accumulator_ / kStep, 0.0, 1.0));
  // Positions: by laps and distance.
  std::vector<Racer*> order;
  for (Racer& r : racers_) order.push_back(&r);
  std::stable_sort(order.begin(), order.end(), [](const Racer* a, const Racer* b) {
    if (a->hidden != b->hidden) return b->hidden;
    if (a->finished != b->finished) return a->finished;
    if (a->finished) return a->finishMs < b->finishMs;
    return a->distance > b->distance;
  });
  for (size_t i = 0; i < order.size(); ++i) order[i]->place = static_cast<int>(i) + 1;
}

void World::stepPhysics(float dt) {
  ++tick_;
  const bool go = tick_ >= goTick_;
  for (Racer& r : racers_) {
    r.prevPosition = r.car.position;
    r.prevRotation = r.car.rotation;
  }
  for (size_t i = 0; i < racers_.size(); ++i) {
    Racer& r = racers_[i];
    if (r.remote) continue;
    if (r.bot) driveBot(r, static_cast<int>(i));
    engine::CarInput in = r.input;
    in.hold = !go;  // held on the grid until the lights go out (you can rev)
    if (r.finished && r.bot) {
      in.throttle = std::min(in.throttle, 0.3f);
    }
    if (r.drive) {
      // The new physics: two 480 Hz ticks per race step.
      engine::drive::Input d;
      d.steer = r.wheel + (in.steer - r.wheel) * setup_.steerAssist;
      d.throttle = in.throttle;
      d.brake = in.brake;
      d.handbrake = in.handbrake;
      d.clutch = in.clutch;
      d.shift = in.shift;
      d.hold = in.hold;
      d.autoGears = r.assists.autoGears;
      d.autoClutch = r.assists.autoClutch;
      d.abs = setup_.abs;
      d.tc = setup_.tc;
      d.handStrength = setup_.handStrength;
      d.steerWeight = setup_.steerWeight;
      for (int k = 0; k < 2; ++k) {
        r.drive->step(d, road_);
        d.shift = 0;
      }
      syncFromDrive(r);
    } else {
      r.car.step(dt, in, r.assists, track_);
    }
    // On its side or roof: back on the wheels after a moment.
    r.flipTime = r.car.up().y < 0.3f ? r.flipTime + dt : 0;
    if (r.flipTime > 1.5f) resetCar(static_cast<int>(i));
  }
  for (size_t i = 0; i < racers_.size(); ++i) {
    collideWalls(racers_[i]);
    for (size_t j = i + 1; j < racers_.size(); ++j) collideCars(racers_[i], racers_[j]);
  }
  for (Racer& r : racers_) {
    if (r.drive) syncToDrive(r);  // (collisions moved the shared body)
    if (practice_) {
      if (!practicePad_) track_.locate(r.car.position, r.s, r.lateral, r.hint);
    } else {
      updateProgress(r);
    }
  }
}

void World::driveBot(Racer& r, int index) {
  const auto& S = track_.samples();
  const int n = static_cast<int>(S.size());
  const float step = track_.length() / static_cast<float>(n);
  engine::Vehicle& car = r.car;
  const float v = std::max(car.speed(), 0.0f);
  // Look ahead along the line (further when faster) and steer toward it
  // (pure pursuit).
  const float ahead = 7 + v * 0.55f;
  const float sa = r.s + ahead;
  const int ia = static_cast<int>(sa / step) % n;
  const auto c = track_.at(sa);
  const float limit = c.halfWidth - 1.1f;
  const float lat = engine::clampf(line_[static_cast<size_t>(ia)] + r.offset, -limit, limit);
  const Vec3 target = track_.surfacePoint(sa, lat);
  const Vec3 d = target - car.position;
  const float lx = engine::dot(d, car.right()), lz = engine::dot(d, car.forward());
  const float curvature = 2 * lx / std::max(lx * lx + lz * lz, 1.0f);
  const float wheelbase = car.spec().front.z - car.spec().rear.z;
  const float angle = std::atan(wheelbase * curvature);
  float steer = engine::clampf(angle / (car.spec().maxSteer * 0.85f), -1, 1);
  // Catch slides: steer into the direction of travel.
  const float slip = std::atan2(engine::dot(car.velocity, car.right()), std::max(v, 1.0f));
  steer = engine::clampf(steer + slip * 0.8f, -1, 1);

  // Speed: the line's plan (scaled by skill), looked up a little ahead.
  const int is = static_cast<int>((r.s + v * 0.35f + 4) / step) % n;
  float targetV = lineSpeed_[static_cast<size_t>(is)] * r.skill;
  // Traffic: slow for a car right ahead, or pull out to pass it.
  float wantOffset = 0;
  for (size_t j = 0; j < racers_.size(); ++j) {
    if (static_cast<int>(j) == index) continue;
    const Racer& o = racers_[j];
    float gap = o.s - r.s;
    if (gap < -track_.length() / 2) gap += track_.length();
    if (gap > track_.length() / 2) gap -= track_.length();
    if (gap < 1.0f || gap > 28) continue;
    const float lateralGap = o.lateral - r.lateral;
    if (std::fabs(lateralGap) > 2.6f) continue;
    const float closing = v - std::max(o.car.speed(), 0.0f);
    if (closing > -1) {
      // Pass on the side with more room.
      const auto here = track_.at(r.s);
      const float roomLeft = o.lateral + here.halfWidth, roomRight = here.halfWidth - o.lateral;
      wantOffset = roomRight > roomLeft ? 3.2f : -3.2f;
      if (gap < 9 && std::fabs(lateralGap) < 1.9f) targetV = std::min(targetV, std::max(o.car.speed() - 1.0f, 0.0f));
    }
  }
  r.offset += (wantOffset - r.offset) * 0.01f;
  const float err = targetV - v;
  float throttle = engine::clampf(err * 0.35f + 0.25f, 0, 1);
  float brake = engine::clampf(-err * 0.25f - 0.05f, 0, 1);
  if (brake > 0) throttle = 0;
  // Stay gentle on the throttle while sliding.
  throttle *= engine::clampf(1 - (std::fabs(slip) - 0.08f) * 4, 0.2f, 1);
  r.input.steer = steer;
  r.input.throttle = throttle;
  r.input.brake = brake;
  r.input.handbrake = 0;
  // Stuck (in a wall, spun in gravel): back on the track after a moment.
  r.stuckTime = v < 2 && tick_ > goTick_ + ticks(3000) ? r.stuckTime + kStep : 0;
  if (r.stuckTime > 3.5f) resetCar(index);
}

void World::collideWalls(Racer& r) {
  // The deepest corner of the body past a barrier gets one push and one
  // impulse per step (several corners at once would add up and explode).
  if (r.remote || r.hidden) return;  // (a remote car hits walls on its own phone)
  if (track_.onPad(r.car.position)) {
    collidePadWall(r);
    return;
  }
  engine::Vehicle& car = r.car;
  const Vec3 h = car.spec().halfExtents;
  const Vec3 o = car.spec().boxOffset;
  float deepest = 0;
  Vec3 hitCorner, normal;
  for (float x : {-1.0f, 1.0f}) {
    for (float z : {-1.0f, 1.0f}) {
      const Vec3 corner = car.worldPoint({o.x + x * h.x, o.y - h.y * 0.5f, o.z + z * h.z});
      float s, lat;
      int hint = r.hint;
      track_.locate(corner, s, lat, hint);
      const auto c = track_.at(s);
      const float pen = lat >= 0 ? lat - c.wallRight : -c.wallLeft - lat;
      if (pen > deepest) {
        deepest = pen;
        hitCorner = corner;
        normal = c.side * (lat >= 0 ? -1.0f : 1.0f);  // pointing back toward the track
      }
    }
  }
  if (deepest <= 0) return;
  // Fully through (a big crash at an odd angle): back to the track side.
  const auto here = track_.at(r.s);
  if ((r.lateral > here.wallRight + 0.5f || r.lateral < -here.wallLeft - 0.5f)) {
    const float lat = r.lateral > 0 ? here.wallRight - 2.0f : -here.wallLeft + 2.0f;
    const Vec3 p = track_.surfacePoint(r.s, lat);
    car.position = {p.x, std::max(car.position.y, p.y + car.spec().cgHeight), p.z};
    car.velocity = car.velocity - normal * std::min(0.0f, engine::dot(car.velocity, normal)) * 1.3f;
    return;
  }
  car.position += normal * std::min(deepest, 0.08f);
  // Tire walls push at about the car's middle height (so a hit doesn't
  // flip the car) and soak up a hard hit.
  hitCorner.y = car.position.y;
  const Vec3 vp = car.pointVelocity(hitCorner);
  const float vn = engine::dot(vp, normal);
  if (vn >= 0) return;
  const float k = car.inverseMassAt(hitCorner, normal);
  const float j = -(1 + 0.15f) * vn / k;
  car.applyImpulse(hitCorner, normal * j);
  if (vn < -8) car.angularVelocity = car.angularVelocity * 0.6f;
  // Scraping along the barrier: friction.
  Vec3 vt = car.pointVelocity(hitCorner);
  vt = vt - normal * engine::dot(vt, normal);
  vt.y = 0;
  const float speedT = engine::length(vt);
  if (speedT > 0.01f) {
    const Vec3 t = vt * (1 / speedT);
    const float jt = std::min(speedT / car.inverseMassAt(hitCorner, t), 0.4f * j);
    car.applyImpulse(hitCorner, t * -jt);
  }
}

// The pad's round tire wall: the deepest body corner past it gets a push
// back toward the middle and one impulse (as for the track's barriers).
bool World::collidePadWall(Racer& r) {
  engine::Vehicle& car = r.car;
  const Vec3 h = car.spec().halfExtents, o = car.spec().boxOffset;
  const Vec3 c = track_.pad().centre;
  const float wall = track_.pad().wall;
  float deepest = 0;
  Vec3 hit, normal;
  for (float x : {-1.0f, 1.0f}) {
    for (float z : {-1.0f, 1.0f}) {
      const Vec3 corner = car.worldPoint({o.x + x * h.x, o.y - h.y * 0.5f, o.z + z * h.z});
      const Vec3 d{corner.x - c.x, 0, corner.z - c.z};
      const float pen = engine::length(d) - wall;
      if (pen > deepest) {
        deepest = pen;
        hit = corner;
        normal = engine::normalize(d) * -1.0f;
      }
    }
  }
  if (deepest <= 0) return false;
  car.position += normal * std::min(deepest, 0.08f);
  hit.y = car.position.y;
  const float vn = engine::dot(car.pointVelocity(hit), normal);
  if (vn >= 0) return true;
  const float j = -(1 + 0.15f) * vn / car.inverseMassAt(hit, normal);
  car.applyImpulse(hit, normal * j);
  Vec3 vt = car.pointVelocity(hit);
  vt = vt - normal * engine::dot(vt, normal);
  vt.y = 0;
  const float st = engine::length(vt);
  if (st > 0.01f) {
    const Vec3 t = vt * (1 / st);
    car.applyImpulse(hit, t * -std::min(st / car.inverseMassAt(hit, t), 0.4f * j));
  }
  return true;
}

void World::collideCars(Racer& A, Racer& B) {
  if (A.ghost(tick_) || B.ghost(tick_)) return;  // just reset: passes through
  if (A.hidden || B.hidden) return;
  engine::Vehicle& a = A.car;
  engine::Vehicle& b = B.car;
  const Vec3 d = b.position - a.position;
  if (engine::dot(d, d) > 36 || std::fabs(d.y) > 1.5f) return;
  // Top-down boxes; separating axis test on the four edge directions.
  struct Box {
    Vec3 c, ax, az;
    float hx, hz;
  };
  const auto boxOf = [&](const engine::Vehicle& v) {
    Vec3 fx = v.right(), fz = v.forward();
    fx.y = fz.y = 0;
    fx = engine::normalize(fx);
    fz = engine::normalize(fz);
    const Vec3 c = v.worldPoint(v.spec().boxOffset);
    const Vec3 h = v.spec().halfExtents;  // each car its own size
    return Box{{c.x, 0, c.z}, fx, fz, h.x, h.z};
  };
  const Box ba = boxOf(a), bb = boxOf(b);
  const Vec3 axes[4] = {ba.ax, ba.az, bb.ax, bb.az};
  float best = 1e9f;
  Vec3 normal;
  for (const Vec3& ax : axes) {
    const auto radius = [&](const Box& bx) {
      return bx.hx * std::fabs(engine::dot(bx.ax, ax)) + bx.hz * std::fabs(engine::dot(bx.az, ax));
    };
    const float dist = engine::dot(bb.c - ba.c, ax);
    const float overlap = radius(ba) + radius(bb) - std::fabs(dist);
    if (overlap <= 0) return;
    if (overlap < best) {
      best = overlap;
      normal = dist >= 0 ? ax : -ax;  // from A to B
    }
  }
  // Contact point: the corner of one box deepest inside the other.
  const auto deepest = [&](const Box& from, const Box& into, Vec3 n) {
    Vec3 pick = from.c;
    float most = -1e9f;
    for (float x : {-1.0f, 1.0f}) {
      for (float z : {-1.0f, 1.0f}) {
        const Vec3 p = from.c + from.ax * (x * from.hx) + from.az * (z * from.hz);
        const float depth = engine::dot(p - into.c, n);
        if (depth > most) {
          most = depth;
          pick = p;
        }
      }
    }
    return pick;
  };
  const Vec3 pa = deepest(bb, ba, -normal);  // B's corner pushing into A
  const Vec3 pb = deepest(ba, bb, normal);
  Vec3 p = (pa + pb) * 0.5f;
  p.y = (a.position.y + b.position.y) * 0.5f - 0.05f;
  // Push apart. A remote car doesn't move here (it does on its own phone,
  // which sees the same contact): this side takes a bit more than half.
  const float shareA = B.remote ? 0.6f : A.remote ? 0.4f : 0.5f;
  a.position += normal * (-best * shareA);
  b.position += normal * (best * (1 - shareA));
  const Vec3 vrel = b.pointVelocity(p) - a.pointVelocity(p);
  const float vn = engine::dot(vrel, normal);
  if (vn >= 0) return;
  // Both masses count (a remote car is as heavy as it is), so a bump from
  // the other player shoves you about as much as it shoves them.
  const float ka = a.inverseMassAt(p, normal);
  const float kb = b.inverseMassAt(p, normal);
  if (ka + kb <= 0) return;
  const float j = -(1 + 0.3f) * vn / (ka + kb);
  if (!A.remote) a.applyImpulse(p, normal * -j);
  if (!B.remote) b.applyImpulse(p, normal * j);
  // Rubbing: some friction along the contact.
  Vec3 vt = vrel - normal * vn;
  vt.y = 0;
  const float st = engine::length(vt);
  if (st > 0.01f) {
    const Vec3 t = vt * (1 / st);
    const float kt = a.inverseMassAt(p, t) + b.inverseMassAt(p, t);
    const float jt = std::min(st / std::max(kt, 1e-6f), 0.35f * j);
    if (!A.remote) a.applyImpulse(p, t * jt);
    if (!B.remote) b.applyImpulse(p, t * -jt);
  }
}

void World::updateProgress(Racer& r) {
  const double nowMs = clockMs();
  const float before = r.s;
  track_.locate(r.car.position, r.s, r.lateral, r.hint);
  const float L = track_.length();
  if (r.s > L * 0.4f && r.s < L * 0.6f) r.halfway = true;
  const bool crossed = before > L - 60 && r.s < 60;
  const bool backwards = before < 60 && r.s > L - 60;
  if (crossed && tick_ >= goTick_) {
    // (Lap 1 is timed from go: lapStartMs is set in start(). The line is
    // just ahead of the grid, so the first crossing restarts it below.)
    if (r.halfway) {
      const double lapMs = nowMs - r.lapStartMs;
      r.lastLapMs = lapMs;
      if (r.bestLapMs == 0 || lapMs < r.bestLapMs) r.bestLapMs = lapMs;
      ++r.lap;
      r.lapStartMs = nowMs;
      r.halfway = false;
      if (r.lap >= laps_ && !r.finished && !r.remote) {  // (a remote finish comes from the room)
        r.finished = true;
        r.finishMs = nowMs - goMs();
      }
    } else if (r.lap == 0) {
      r.lapStartMs = nowMs;
    }
  }
  if (backwards && r.lap > 0) r.halfway = true;  // reversed over the line: count it again next time
  // Total distance: laps done + how far into this one.
  float into = r.s;
  if (r.lap == 0 && !r.halfway && r.s > L * 0.5f) into = r.s - L;  // still behind the line on the grid
  r.distance = static_cast<float>(r.lap) * L + into;
}

}  // namespace app::race
