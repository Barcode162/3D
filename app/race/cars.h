#pragma once
// The cars you can pick in the garage (upgrade plan item 1), their colours
// and how each one looks. Physics numbers live in each model's CarSpec;
// tools/native/steering_test.cpp checks every model's feel (CAR=n).
#include "engine/math.h"
#include "engine/vehicle.h"

namespace app::race {

// How a model's body differs from the base shape (app/race/car_model.cpp).
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
};

constexpr int kCarCount = 3, kPaintCount = 12, kRimCount = 8;
const CarModel& carModel(int index);
engine::Vec3 paintColor(int index);
engine::Vec3 rimColor(int index);
const char* paintName(int index);

}  // namespace app::race
