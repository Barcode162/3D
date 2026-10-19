// The steering feel, measured (engine/steering.* on the race car,
// app::race::sportsCar, flat asphalt, 240 Hz): turning circle, grip at
// speed, how fast it answers, letting go, and slides. Prints a table and
// fails if a number is out of its band. FEEL=old also runs the old steering
// (the touch assist in app/race/controls.cpp before 2026-10-19) to compare.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "app/race/cars.h"
#include "app/race/world.h"
#include "engine/steering.h"
#include "engine/vehicle.h"

using namespace engine;

struct Flat : Ground {
  bool cast(Vec3 o, Vec3 d, float maxD, GroundHit& h, int&) const override {
    if (d.y >= -1e-4f) return false;
    const float t = -o.y / d.y;
    if (t < 0 || t > maxD) return false;
    h.distance = t;
    h.point = o + d * t;
    h.normal = {0, 1, 0};
    return true;
  }
};

static int failed = 0;
static void band(const char* what, float v, float lo, float hi, const char* unit) {
  const bool ok = v >= lo && v <= hi;
  std::printf("%s %-46s %7.2f %-6s (want %.2f..%.2f)\n", ok ? "ok  " : "FAIL", what, v, unit, lo, hi);
  if (!ok) ++failed;
}

// The old touch steering: full input limited to the best angle at speed.
static float oldSteer(float wheel, const Vehicle& car) {
  const CarSpec& spec = car.spec();
  const float v = std::max(std::fabs(car.speed()), 1.0f);
  const float wheelbase = spec.front.z - spec.rear.z;
  const float best = wheelbase * 1.15f * 9.81f / (v * v) + spec.tire.peakSlipAngle * 0.95f;
  return wheel * std::clamp(best / (spec.maxSteer * 0.85f), 0.22f, 1.0f);
}

struct Run {
  Vehicle car;
  Steering steering;
  Flat ground;
  CarAssists assists;
  bool old = false;
  int model = 0;  // app::race::carModel (CAR=n)
  const float dt = 1.0f / 240;
  float t = 0;
  void start(float kmh) {
    car.setup(app::race::carModel(model).spec);
    car.place({0, 0, 0}, 0);
    for (int i = 0; i < 240; ++i) step(0, Steering::Hands::kFree, 0, 0);
    if (kmh > 0) car.setSpeed(kmh / 3.6f);
    steering.reset();
    t = 0;
  }
  // One step: the wheel target, how it's held, throttle and brake.
  void step(float target, Steering::Hands hands, float throttle, float brake) {
    CarInput in;
    if (old) {
      // The old path: the thumb's value, filtered 25 ms, released = 0.
      static float w = 0;
      const float goal = hands == Steering::Hands::kFree ? 0 : target;
      w += (goal - w) * (1 - std::exp(-dt / 0.025f));
      in.steer = oldSteer(w, car);
    } else {
      in.steer = steering.update(dt, target, hands, car);
    }
    in.throttle = throttle;
    in.brake = brake;
    car.step(dt, in, assists, ground);
    t += dt;
  }
  float holdThrottle(float kmh) const { return std::clamp((kmh / 3.6f - car.speed()) * 0.4f + 0.12f, 0.0f, 1.0f); }
  float yawRate() const { return car.angularVelocity.y; }
  float bodySlipDeg() const {
    const float vx = dot(car.velocity, car.right()), vf = std::max(car.speed(), 0.1f);
    return std::fabs(std::atan2(vx, vf)) * 57.2958f;
  }
  float lateralG() const { return std::fabs(yawRate() * car.speed()) / 9.81f; }
};

int main() {
  const bool compare = std::getenv("FEEL") && std::strcmp(std::getenv("FEEL"), "old") == 0;
  const int only = std::getenv("CAR") ? std::atoi(std::getenv("CAR")) : -1;
  for (int pass = 0; pass < (compare ? 2 : app::race::kCarCount); ++pass) {
    if (only >= 0 && !compare && pass != only) continue;
    Run r;
    r.old = compare && pass == 1;
    r.model = compare ? std::max(only, 0) : pass;
    // Each car has its character: the roadster grips harder, the muscle car
    // slides a little more. The GT is the reference.
    const float gripMax = r.model == 2 ? 1.32f : 1.2f, slideMax = r.model == 1 ? 4.5f : 3.5f;
    std::printf("== %s steering, car %s\n", r.old ? "OLD" : "new", app::race::carModel(r.model).name);
    using H = Steering::Hands;

    // A. Turning circle at 12 km/h, full lock.
    {
      r.start(12);
      float minX = 1e9f, maxX = -1e9f;
      while (r.t < 12) {
        r.step(1, H::kHolding, r.holdThrottle(12), 0);
        if (r.t > 2) { minX = std::min(minX, r.car.position.x); maxX = std::max(maxX, r.car.position.x); }
      }
      band("turning circle at 12 km/h (diameter)", maxX - minX, 9.0f, 13.5f, "m");
    }
    // B. Full wheel at speed: sideways grip and how much it slides.
    for (float kmh : {80.0f, 120.0f}) {
      r.start(kmh);
      float slipMax = 0, g = 0;
      while (r.t < 3) {
        r.step(1, H::kHolding, r.holdThrottle(kmh), 0);
        slipMax = std::max(slipMax, r.bodySlipDeg());
        if (r.t > 2) g = std::max(g, r.lateralG());
      }
      char what[64];
      std::snprintf(what, sizeof(what), "full wheel at %.0f km/h: sideways grip", kmh);
      band(what, g, 0.85f, gripMax, "g");
      std::snprintf(what, sizeof(what), "full wheel at %.0f km/h: slide (body slip max)", kmh);
      band(what, slipMax, 0.0f, slideMax, "deg");
    }
    // C. Half wheel at 100 km/h: how fast the car answers, and overshoot.
    {
      r.start(100);
      float steady = 0, peak = 0, t63 = -1;
      std::vector<float> yaw;
      while (r.t < 3) {
        r.step(0.5f, H::kHolding, r.holdThrottle(100), 0);
        yaw.push_back(std::fabs(r.yawRate()));
      }
      for (size_t i = yaw.size() * 5 / 6; i < yaw.size(); ++i) steady += yaw[i] / static_cast<float>(yaw.size() - yaw.size() * 5 / 6);
      for (size_t i = 0; i < yaw.size(); ++i) {
        peak = std::max(peak, yaw[i]);
        if (t63 < 0 && yaw[i] >= 0.63f * steady) t63 = static_cast<float>(i) * r.dt;
      }
      band("half wheel at 100 km/h: answer time (63%)", t63, 0.05f, 0.35f, "s");
      band("half wheel at 100 km/h: overshoot", steady > 0 ? (peak / steady - 1) * 100 : 999, 0, 15, "%");
    }
    // D. Let go after a full-wheel curve at 80 km/h: it straightens itself.
    {
      r.start(80);
      while (r.t < 2) r.step(1, H::kHolding, r.holdThrottle(80), 0);
      const float let = r.t;
      float settled = -1, slipMax = 0;
      int flips = 0;
      float lastSign = r.yawRate() > 0 ? 1.0f : -1.0f;
      while (r.t < let + 3) {
        r.step(0, H::kFree, r.holdThrottle(80), 0);
        slipMax = std::max(slipMax, r.bodySlipDeg());
        const float y = r.yawRate();
        if (std::fabs(y) > 0.03f) {
          const float s = y > 0 ? 1.0f : -1.0f;
          if (s != lastSign) { ++flips; lastSign = s; }
          settled = -1;
        } else if (settled < 0) {
          settled = r.t - let;
        }
      }
      band("let go at 80 km/h: straight again after", settled < 0 ? 99 : settled, 0.1f, 1.0f, "s");
      band("let go at 80 km/h: wobbles (yaw sign flips)", static_cast<float>(flips), 0, 1, "");
      band("let go at 80 km/h: slide", slipMax, 0, 4, "deg");
    }
    // E. Full wheel and full throttle out of a 50 km/h corner (power slide).
    {
      r.start(50);
      float slipMax = 0;
      const float h0 = Vehicle::headingOf(r.car.forward());
      float turned = 0, last = h0;
      while (r.t < 3) {
        r.step(1, H::kHolding, 1, 0);
        slipMax = std::max(slipMax, r.bodySlipDeg());
        float h = Vehicle::headingOf(r.car.forward());
        float d = h - last;
        while (d > 3.14159f) d -= 6.28318f;
        while (d < -3.14159f) d += 6.28318f;
        turned += d;
        last = h;
      }
      band("full throttle + full wheel from 50 km/h: slide", slipMax, 0, 12, "deg");
    }
    // F. Lifting off and braking mid-corner at 120 km/h (lift-off oversteer).
    {
      r.start(120);
      while (r.t < 1.5f) r.step(0.8f, H::kHolding, r.holdThrottle(120), 0);
      float slipMax = 0;
      while (r.t < 4) {
        r.step(0.8f, H::kHolding, 0, 0.6f);
        slipMax = std::max(slipMax, r.bodySlipDeg());
      }
      band("brake mid-corner at 120 km/h: slide", slipMax, 0, 8, "deg");
    }
    // G. Keys: how long from tap to full wheel when standing, and at speed.
    {
      r.start(0);
      float tFull = -1;
      while (r.t < 2 && tFull < 0) {
        r.step(1, H::kKeys, 0, 0);
        if (r.steering.wheel() > 0.99f) tFull = r.t;
      }
      if (!r.old) band("keys: tap to full wheel when standing", tFull, 0.25f, 0.6f, "s");
    }
  }
  std::printf(failed ? "STEERING TEST FAILED (%d)\n" : "STEERING TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
