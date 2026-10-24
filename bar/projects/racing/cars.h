#pragma once
// The cars you can pick in the garage (upgrade plan item 1), their colours
// and how each one looks. Physics numbers live in each model's CarSpec;
// tools/native/steering_test.cpp checks every model's feel (CAR=n).
#include "looped/math.h"
#include "looped/vehicle.h"

namespace app::race {

// How a model's body differs from the base shape (bar/projects/racing/car_model.cpp).
struct CarStyle {
  float length = 1, width = 1;  // scales the body (the axles in the spec match)
  bool wing = true;
};

struct CarModel {
  const char* name;
  const char* blurb;  // one line for the garage
  engine::CarSpec spec;
  CarStyle style;
  float speed, handling, weight;  // 0..1 bars for the garage
  // Drives on the new physics (looped/drive, docs/plan/jobs/W20.md); `spec`
  // above is then its stand-in for the old physics (Car setup → Old physics),
  // drawing and collisions.
  bool newPhysics = false;
};

constexpr int kCarCount = 4, kPaintCount = 12, kRimCount = 8;
constexpr int kRidgeback = 3;  // the Vance Ridgeback GS (new physics)
const CarModel& carModel(int index);
engine::Vec3 paintColor(int index);
engine::Vec3 rimColor(int index);
const char* paintName(int index);

}  // namespace app::race
