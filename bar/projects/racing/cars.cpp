#include "bar/projects/racing/cars.h"

#include <algorithm>
#include <vector>

#include "bar/projects/racing/world.h"
#include "looped/drive/car_spec.h"

namespace app::race {
namespace {

using engine::Vec3;

// Bigger engine, heavier, less grip: slides when you ask it to.
engine::CarSpec muscleCar() {
  engine::CarSpec s = sportsCar();
  s.mass = 1520;
  for (auto& p : s.torqueCurve) p[1] *= 1.35f;
  s.front.z *= 1.04f;
  s.rear.z *= 1.04f;
  s.front.halfTrack *= 1.04f;
  s.rear.halfTrack *= 1.04f;
  s.front.downforce = 0.06f;
  s.rear.downforce = 0.08f;
  s.rear.grip = 1.12f;
  s.rear.antiRoll = 12000;
  s.dragArea = 0.78f;
  s.brakeTorque = 6800;
  return s;
}

// Light and short: less power, turns in like a go-kart.
engine::CarSpec roadster() {
  engine::CarSpec s = sportsCar();
  s.mass = 1060;
  for (auto& p : s.torqueCurve) p[1] *= 0.78f;
  s.front.z *= 0.92f;
  s.rear.z *= 0.92f;
  s.front.halfTrack *= 0.96f;
  s.rear.halfTrack *= 0.96f;
  s.front.spring *= 0.85f;
  s.rear.spring *= 0.85f;
  s.front.downforce = 0.08f;
  s.rear.downforce = 0.1f;
  s.front.grip = 1.08f;
  s.rear.grip = 1.25f;
  s.brakeTorque = 5200;
  s.dragArea = 0.6f;
  return s;
}

// The Ridgeback GS in the old physics' terms: the same size, mass, engine
// and gears (for drawing, collisions and the "Old physics" comparison).
engine::CarSpec ridgebackOld() {
  const engine::drive::CarSpec d = engine::drive::ridgebackGS();
  engine::CarSpec s = sportsCar();
  const float L = d.wheelbase() / (s.front.z - s.rear.z);
  s.mass = d.mass;
  s.inertia = {d.inertiaPitch, d.inertiaYaw, d.inertiaRoll};
  s.cgHeight = d.cgHeight;
  s.halfExtents = {s.halfExtents.x * 1.06f, s.halfExtents.y, s.halfExtents.z * L};
  s.boxOffset = {0, s.boxOffset.y, s.boxOffset.z * L};
  s.front.z = d.front.z;
  s.rear.z = d.rear.z;
  s.front.halfTrack = d.front.halfTrack;
  s.rear.halfTrack = d.rear.halfTrack;
  s.front.brakeShare = 0.62f;
  s.rear.brakeShare = 0.38f;
  s.tire.peakSlipAngle = d.front.tire.peakSlipAngle;
  s.tire.mu = 1.25f;
  s.torqueCurve = d.torqueCurve;
  s.idleRpm = d.idleRpm;
  s.maxRpm = d.redline;
  s.engineInertia = d.engineInertia;
  s.clutchTorque = d.clutchTorque;
  s.gears = d.gears;
  s.reverseGear = d.reverseGear;
  s.finalDrive = d.finalDrive;
  s.brakeTorque = 2 * (d.front.brakeTorque + d.rear.brakeTorque);
  s.handbrakeTorque = d.handbrakeTorque;
  s.maxSteer = d.maxRoadWheel() / 0.85f;  // (CarInput::steer 1 = 0.85 of maxSteer)
  s.dragArea = d.dragArea;
  return s;
}

const Vec3 kPaintColors[kPaintCount] = {
    {0.75f, 0.02f, 0.02f}, {0.02f, 0.15f, 0.7f},  {0.9f, 0.55f, 0.02f}, {0.04f, 0.4f, 0.1f},
    {0.85f, 0.85f, 0.87f}, {0.02f, 0.02f, 0.02f}, {0.45f, 0.05f, 0.55f}, {0.0f, 0.5f, 0.6f},
    {0.9f, 0.3f, 0.5f},    {0.95f, 0.85f, 0.05f}, {0.3f, 0.32f, 0.35f},  {0.55f, 0.9f, 0.1f}};
const char* kPaintNames[kPaintCount] = {"RED",   "BLUE",  "ORANGE", "GREEN", "WHITE", "BLACK",
                                        "PURPLE", "TEAL", "PINK",   "YELLOW", "GREY", "LIME"};
const Vec3 kRimColors[kRimCount] = {{0.7f, 0.7f, 0.72f}, {0.08f, 0.08f, 0.09f}, {0.85f, 0.65f, 0.2f}, {0.9f, 0.1f, 0.08f},
                                    {0.1f, 0.4f, 0.95f}, {0.95f, 0.95f, 0.95f}, {0.1f, 0.9f, 0.5f},   {0.9f, 0.3f, 0.8f}};

}  // namespace

const CarModel& carModel(int index) {
  static const std::vector<CarModel> models = {
      {"GT", "BALANCED. GRIPS AND GOES.", sportsCar(), {1, 1, true}, 0.75f, 0.8f, 0.55f},
      {"MUSCLE", "HUGE POWER. LOVES TO SLIDE.", muscleCar(), {1.04f, 1.04f, false}, 0.95f, 0.5f, 0.85f},
      {"ROADSTER", "LIGHT AND SHARP. CORNER KING.", roadster(), {0.92f, 0.96f, false}, 0.5f, 0.95f, 0.25f},
      {"GS", "RIDGEBACK GS: THE NEW PHYSICS. V8, 7 GEARS.", ridgebackOld(), {1.063f, 1.06f, true}, 0.9f, 0.85f, 0.7f,
       true},
  };
  return models[static_cast<size_t>(std::clamp(index, 0, kCarCount - 1))];
}

Vec3 paintColor(int index) { return kPaintColors[std::clamp(index, 0, kPaintCount - 1)]; }
Vec3 rimColor(int index) { return kRimColors[std::clamp(index, 0, kRimCount - 1)]; }
const char* paintName(int index) { return kPaintNames[std::clamp(index, 0, kPaintCount - 1)]; }

}  // namespace app::race
