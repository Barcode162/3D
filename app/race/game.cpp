#include "app/race/game.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

#include "app/players.h"
#include "app/race/cars.h"
#include "engine/bridge.h"
#include "engine/drive/car_spec.h"
#include "engine/font.h"
#include "engine/net.h"
#include "engine/frame_time.h"
#include "engine/jobs.h"
#include "engine/ui.h"
#include "engine/log.h"
#include "engine/profiler.h"

using engine::Vec2;
using engine::Vec3;
using engine::Vec4;

namespace app::race {
namespace {

constexpr int kBots = 7;
constexpr int kLaps = 3;
const char* kCameraNames[] = {"CHASE", "FAR", "HOOD", "BUMPER"};

std::string timeText(double ms) {
  if (ms <= 0) return "-:--.---";
  char b[32];
  const int total = static_cast<int>(ms);
  std::snprintf(b, sizeof(b), "%d:%02d.%03d", total / 60000, (total / 1000) % 60, total % 1000);
  return b;
}

using engine::wrapAngle;

}  // namespace

void Game::prepare() {
  if (prepared_) return;
  prepared_ = true;
  // Track and racing line first, then the meshes (once the race's shaders
  // have compiled; the job waits a frame and tries again until then).
  engine::queueJob([this] {
    const double t0 = engine::clockMs();
    world_.init();
    LOG_INFO("Race track built in %.0f ms.", engine::clockMs() - t0);
  }, engine::JobLane::kNormal, "race-track");
  struct Upload {
    static void queue(Game* g) {
      engine::queueJob([g] {
        if (!g->view_.pipelinesReady()) {
          queue(g);  // try again later
          return;
        }
        const double t0 = engine::clockMs();
        g->ready_ = g->view_.init(*g->gpu_, g->world_.track());
        LOG_INFO("Race track ready in %.0f ms.", engine::clockMs() - t0);
      }, engine::JobLane::kNormal, "race-meshes");
    }
  };
  Upload::queue(this);
}

bool Game::online() const { return engine::net::status() == engine::net::Status::kConnected; }

bool Game::start(int rig, double nowMs, bool straightIn, bool pad) {
  if (!ready_) {
    prepare();  // (normally already done after you entered)
    pendingRig_ = rig;
    pendingStraight_ = straightIn;
    pendingPad_ = pad;
    LOG_INFO("Race: warming up.");
    return false;
  }
  pendingRig_ = -1;
  rig_ = rig & 1;
  open_ = true;
  startMs_ = nowMs;
  joined_ = false;
  claimReq_ = 0;
  choice_.car = std::clamp(static_cast<int>(engine::settings::remembered("raceCar", 0)), 0, kCarCount - 1);
  choice_.paint = std::clamp(static_cast<int>(engine::settings::remembered("racePaint", 0)), 0, kPaintCount - 1);
  choice_.rim = std::clamp(static_cast<int>(engine::settings::remembered("raceRim", 0)), 0, kRimCount - 1);
  if (!humVoice_) humVoice_ = hum_.play(0.0f, nullptr, 0, 0.6f, engine::Sound::kEffects, true);
  // The page's top bar (menu, mic, log) would sit on the race buttons.
  engine::ui::setBodyClass("racing", true);
  build_ = engine::ui::buildLabel();
  if (pad) beginPractice(nowMs, true);
  else if (straightIn) beginRace(nowMs, false);
  else openGarage(nowMs);
  return true;
}

bool Game::tryPending(double nowMs) {
  return pendingRig_ >= 0 && ready_ && start(pendingRig_, nowMs, pendingStraight_, pendingPad_);
}

void Game::setCarSetup(const std::string& json) {
  engine::drive::Setup s;
  if (!engine::drive::parseSetup(json, s)) {
    LOG_ERROR("Car setup: not a JSON object.");
    return;
  }
  world_.setSetup(s);
}

// Practice with the Ridgeback GS (the new physics): the skid pad or the Ring,
// just you, no lights or laps. Your paint and rims from the garage.
void Game::beginPractice(double nowMs, bool pad) {
  RacerSetup me;
  me.model = kRidgeback;
  me.paint = paintColor(choice_.paint);
  me.rim = rimColor(choice_.rim);
  world_.startPractice(me, pad);
  screen_ = Screen::kRace;
  practice_ = true;
  roomRace_ = false;
  remoteRacer_ = remotePlayer_ = -1;
  resultsMs_ = 0;
  startMs_ = nowMs;
  controls_.reset();
  const Racer& r = world_.racers()[static_cast<size_t>(world_.you())];
  camYaw_ = engine::Vehicle::headingOf(r.car.forward());
  camPos_ = r.car.position;
  LOG_INFO("Practice: Ridgeback GS on the %s (%s physics).", pad ? "skid pad" : "Ring",
           world_.setup().oldPhysics ? "old" : "new");
}

void Game::close() {
  pendingRig_ = -1;
  if (!open_) return;
  open_ = false;
  // Getting up gives the rig back (the room takes you out of its race).
  const std::string key = "machine.rig" + std::to_string(rig_);
  if (online() && engine::net::claimOwner(key) == 0) engine::net::release(key);
  joined_ = false;
  roomRace_ = false;
  engine::ui::setBodyClass("racing", false);
  engine::Sound::stop(humVoice_, 0, 0.3f);
  humVoice_ = 0;
}

void Game::openGarage(double nowMs) {
  screen_ = Screen::kGarage;
  practice_ = false;
  garageMs_ = nowMs;
  wantReady_ = false;
  roomRace_ = false;
  remoteRacer_ = remotePlayer_ = -1;
  resultsMs_ = 0;
  showChoice();
  if (online() && joined_) sendRace("join");
}

void Game::showChoice() {
  RacerSetup me;
  me.grid = rig_;
  me.model = choice_.car;
  me.paint = paintColor(choice_.paint);
  me.rim = rimColor(choice_.rim);
  world_.start({me}, 0, kLaps, 0, 3.6e6);  // just your car, on the grid; no lights
}

void Game::pickChanged() {
  engine::settings::remember("raceCar", choice_.car);
  engine::settings::remember("racePaint", choice_.paint);
  engine::settings::remember("raceRim", choice_.rim);
  showChoice();
  // A new pick while ready: back to choosing (press READY again).
  wantReady_ = false;
  if (online() && joined_) sendRace("join");
}

void Game::sendRace(const char* op, bool solo) {
  char args[160];
  std::snprintf(args, sizeof(args), "{\"op\":\"%s\",\"rig\":%d,\"car\":%d,\"paint\":%d,\"rim\":%d%s}", op, rig_, choice_.car,
                choice_.paint, choice_.rim, solo ? ",\"solo\":1" : "");
  const int id = engine::net::request("race", args);
  if (id > 0) requests_.push_back(id);
}

void Game::onRoomRace(const std::vector<double>& v) {
  if (v.size() < 16) return;
  room_.phase = static_cast<int>(v[0]);
  room_.goAt = v[1];
  room_.laps = std::max(1, static_cast<int>(v[2]));
  room_.seed = static_cast<int>(v[3]);
  for (int k = 0; k < 2; ++k) {
    const size_t b = 4 + static_cast<size_t>(k) * 6;
    RoomRig& r = room_.rigs[k];
    r.slot = static_cast<int>(v[b]);
    r.car = static_cast<int>(v[b + 1]);
    r.paint = static_cast<int>(v[b + 2]);
    r.rim = static_cast<int>(v[b + 3]);
    r.state = static_cast<int>(v[b + 4]);
    r.timeMs = v[b + 5] * 1000;
  }
  haveRoom_ = true;
}

void Game::updateNet(double nowMs) {
  // Answers we don't need (a refusal just means the room knows better).
  std::erase_if(requests_, [](int id) {
    std::string answer;
    return engine::net::takeAnswer(id, answer);
  });
  if (!online()) {
    joined_ = false;
    return;
  }
  // The rig is the claim machine.rig<n>; ask again after a reconnect.
  const std::string key = "machine.rig" + std::to_string(rig_);
  if (claimReq_ && engine::net::claimResult(claimReq_) >= 0) claimReq_ = 0;
  const int owner = engine::net::claimOwner(key);
  if (owner != 0) {
    joined_ = false;
    if (owner < 0 && !claimReq_) claimReq_ = engine::net::claim(key);
    return;
  }
  if (!joined_) {
    joined_ = true;
    if (screen_ == Screen::kGarage) sendRace(wantReady_ ? "ready" : "join");
  }
  if (!haveRoom_) return;
  const RoomRig& mine = room_.rigs[rig_];
  const bool meInRoom = mine.slot >= 0 && engine::net::playerOfSlot(mine.slot) == 0;
  // The room started a race with you in it.
  if (screen_ == Screen::kGarage && room_.phase == 1 && meInRoom && mine.state == kRacing && room_.goAt != raceGoAt_) {
    beginRace(nowMs, true);
  }
  if (screen_ != Screen::kRace || !roomRace_) return;
  // The other player: finished (the room's time), left, or still racing.
  if (remoteRacer_ >= 0) {
    Racer& them = world_.racers()[static_cast<size_t>(remoteRacer_)];
    const RoomRig& other = room_.rigs[1 - rig_];
    const bool still = other.slot >= 0 && engine::net::playerOfSlot(other.slot) == remotePlayer_ &&
                       (other.state == kRacing || other.state == kFinished || other.state == kDnf);
    if (!still && !them.finished) them.hidden = true;
    if (still && other.state == kFinished && !them.finished) {
      them.finished = true;
      them.finishMs = other.timeMs;
    }
  }
  Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  if (meInRoom && mine.state == kDnf && !me.finished) {
    me.finished = true;  // time's up (a minute after the winner)
    me.finishMs = 0;
  }
}

void Game::beginRace(double nowMs, bool fromRoom) {
  std::vector<RacerSetup> people;
  RacerSetup me;
  me.model = choice_.car;
  me.paint = paintColor(choice_.paint);
  me.rim = rimColor(choice_.rim);
  remoteRacer_ = remotePlayer_ = -1;
  remoteLive_ = false;
  remoteName_.clear();
  int laps = kLaps;
  uint32_t seed = static_cast<uint32_t>(nowMs);
  double goIn = -1;
  if (fromRoom) {
    laps = room_.laps;
    seed = static_cast<uint32_t>(room_.seed);
    goIn = std::max(0.0, (room_.goAt - engine::net::serverTime()) * 1000);
    raceGoAt_ = room_.goAt;
    const RoomRig& other = room_.rigs[1 - rig_];
    if (other.slot >= 0 && other.state == kRacing) {
      RacerSetup them;
      them.you = false;
      them.grid = 1 - rig_;
      them.model = std::clamp(other.car, 0, kCarCount - 1);
      them.paint = paintColor(other.paint);
      them.rim = rimColor(other.rim);
      remotePlayer_ = engine::net::playerOfSlot(other.slot);
      remoteName_ = engine::net::playerName(remotePlayer_);
      them.name = remoteName_.empty() ? "Rival" : remoteName_.substr(0, 10);
      people.push_back(them);
    }
  }
  // Two players: the front row, just you two. Alone: 4th or 5th of 8.
  const bool duel = !people.empty();
  me.grid = duel ? rig_ : 3 + rig_;
  people.push_back(me);
  world_.start(people, duel ? 0 : kBots, laps, seed, goIn);
  for (size_t i = 0; i < world_.racers().size(); ++i) {
    if (world_.racers()[i].remote) remoteRacer_ = static_cast<int>(i);
  }
  screen_ = Screen::kRace;
  practice_ = false;
  roomRace_ = fromRoom;
  finishSent_ = false;
  wantReady_ = false;
  resultsMs_ = 0;
  startMs_ = nowMs;
  controls_.reset();
  const Racer& r = world_.racers()[static_cast<size_t>(world_.you())];
  camYaw_ = engine::Vehicle::headingOf(r.car.forward());
  camPos_ = r.car.position;
  LOG_INFO("Race: %s, %d laps.", duel ? ("you vs " + (remoteName_.empty() ? std::string("rival") : remoteName_)).c_str() : "you vs bots",
           laps);
}

// The other player's car: their states, played back 100 ms in the past
// (between two of them), or carried on along their heading for a moment
// when the next one is late.
void Game::updateRemote(double nowMs, float dt) {
  if (remoteRacer_ < 0 || !players_ || remotePlayer_ <= 0) return;
  Racer& them = world_.racers()[static_cast<size_t>(remoteRacer_)];
  if (them.hidden) return;
  float v[5];
  double late = players_->dataAt(remotePlayer_, nowMs, 100, v, 2);
  if (late < 0 || (v[0] == 0 && v[1] == 0)) return;  // nothing from them yet: on the grid
  if (!remoteLive_) {
    remoteLive_ = true;
    LOG_INFO("Race: %s's car is live (%.0f, %.0f).", them.name.c_str(), v[0], v[1]);
  }
  late = std::min(late, 300.0) / 1000;
  const float x = v[0] - std::sin(v[2]) * v[3] * static_cast<float>(late);
  const float z = v[1] - std::cos(v[2]) * v[3] * static_cast<float>(late);
  world_.setRemote(remoteRacer_, x, z, v[2], v[3], v[4], dt);
}

std::vector<Game::Hit> Game::garageButtons(float W, float H) const {
  std::vector<Hit> out;
  const float u = std::min(W, H) / 400.0f;
  const float x0 = 0.05f * W;
  out.push_back({x0, 8 * u, 60 * u, 24 * u, 6, 0});  // leave
  for (int k = 0; k < kCarCount; ++k) out.push_back({x0 + static_cast<float>(k) * 64 * u, 58 * u, 60 * u, 60 * u, 1, k});
  out.push_back({x0, 264 * u, 250 * u, 26 * u, 7, 0});  // skid pad practice (the new physics)
  for (int k = 0; k < kPaintCount; ++k) {
    out.push_back({x0 + static_cast<float>(k % 6) * 26 * u, 140 * u + static_cast<float>(k / 6) * 26 * u, 22 * u, 22 * u, 2, k});
  }
  for (int k = 0; k < kRimCount; ++k) out.push_back({x0 + static_cast<float>(k) * 26 * u, 214 * u, 22 * u, 22 * u, 3, k});
  const float bw = 150 * u, bx = W - 0.05f * W - bw;
  out.push_back({bx, H - 64 * u, bw, 44 * u, 4, 0});  // ready
  if (wantReady_) out.push_back({bx, H - 100 * u, bw, 28 * u, 5, 0});  // race the bots instead
  return out;
}

bool Game::updateGarage(double nowMs, float dt, const engine::InputFrame& input) {
  const float W = static_cast<float>(gpu_->width), H = static_cast<float>(gpu_->height);
  int action = 0, arg = 0;
  const std::vector<Hit> hits = garageButtons(W, H);
  for (const engine::RawPress& p : input.presses) {
    if (!p.down || p.key >= 0) continue;
    const float px = p.pos.x * W, py = p.pos.y * H;
    for (const Hit& h : hits) {
      if (px >= h.x && px <= h.x + h.w && py >= h.y && py <= h.y + h.h) {
        action = h.action;
        arg = h.arg;
      }
    }
  }
  const bool enter = input.keyDown("Enter");
  if (enter && !enterWas_) action = 4;
  enterWas_ = enter;
  if (input.keyDown("Escape")) action = 6;
  switch (action) {
    case 1: if (choice_.car != arg) { choice_.car = arg; pickChanged(); } break;
    case 2: if (choice_.paint != arg) { choice_.paint = arg; pickChanged(); } break;
    case 3: if (choice_.rim != arg) { choice_.rim = arg; pickChanged(); } break;
    case 4:
      if (wantReady_) {  // a second tap: not ready after all
        wantReady_ = false;
        if (online() && joined_) sendRace("unready");
      } else if (!online()) {
        beginRace(nowMs, false);  // no room: you and the bots
      } else {
        wantReady_ = true;
        if (joined_) sendRace("ready");
      }
      break;
    case 5:
      wantReady_ = false;
      // The room's race is free: it runs yours. Else (someone else is racing
      // in it) a race of your own, just on this phone.
      if (online() && joined_ && haveRoom_ && room_.phase == 0) sendRace("ready", true);
      else beginRace(nowMs, false);
      break;
    case 6: return true;
    case 7:
      if (wantReady_ && online() && joined_) sendRace("unready");
      wantReady_ = false;
      beginPractice(nowMs, true);
      return false;
    default: break;
  }
  world_.update(dt);  // (the car settles on its springs)
  garageCamera(nowMs);
  engine::Sound::set(humVoice_, 0.08f, 0.45f);
  return false;
}

std::string Game::otherRigText() const {
  if (!online()) return "OFFLINE: YOU RACE THE BOTS";
  if (!haveRoom_) return "CONNECTING TO THE ROOM...";
  const RoomRig& other = room_.rigs[1 - rig_];
  const int who = other.slot >= 0 ? engine::net::playerOfSlot(other.slot) : -1;
  if (who <= 0) return wantReady_ ? "STARTING..." : "OTHER RIG IS EMPTY: READY RACES THE BOTS";
  std::string name = engine::net::playerName(who);
  if (name.empty()) name = "SOMEONE";
  for (char& c : name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  switch (other.state) {
    case kReady: return name + " IS READY!";
    case kRacing: return name + " IS RACING. WAIT OR RACE THE BOTS";
    case kFinished:
    case kDnf: return name + " IS LOOKING AT THE RESULTS";
    default: return name + " IS PICKING A CAR";
  }
}

bool Game::update(double nowMs, float dt, const engine::InputFrame& input) {
  if (!open_) return false;
  updateNet(nowMs);
  if (screen_ == Screen::kGarage) return updateGarage(nowMs, dt, input);
  Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const float W = static_cast<float>(gpu_->width), H = static_cast<float>(gpu_->height);
  const engine::bridge::Frame& page = engine::bridge::frame();
  if (page.cssW > 0 && page.cssH > 0) {
    controls_.setSafeArea(page.safe[0] / static_cast<float>(page.cssW), page.safe[2] / static_cast<float>(page.cssW),
                          page.safe[3] / static_cast<float>(page.cssH));
  }
  controls_.showSetup = carModel(me.model).newPhysics;
  controls_.showPlace = practice_;
  controls_.update(dt, input, me.car, manualGears_, W / std::max(H, 1.0f));
  const bool* pressed = controls_.pressed;
  if (pressed[Controls::kLeave]) return true;
  if (pressed[Controls::kCamera]) cameraMode_ = (cameraMode_ + 1) % 4;
  if (pressed[Controls::kReset] && world_.started()) world_.requestReset(world_.you());
  if (pressed[Controls::kSetup]) engine::ui::openPanel("carSetup");
  if (pressed[Controls::kPlace]) {
    beginPractice(nowMs, !world_.practiceOnPad());
    return false;
  }
  me.input = controls_.car;
  me.wheel = controls_.wheel();
  me.assists.autoGears = !manualGears_;
  // Your time goes to the room (it checks it and tells the other rig).
  if (roomRace_ && me.finished && !finishSent_ && me.finishMs > 0) {
    finishSent_ = true;
    char args[96];
    std::snprintf(args, sizeof(args), "{\"op\":\"finish\",\"rig\":%d,\"ms\":%.0f}", rig_, me.finishMs);
    const int id = engine::net::request("race", args);
    if (id > 0) requests_.push_back(id);
  }
  // After the finish: tap anywhere on the results to go back to the garage.
  if (me.finished && resultsMs_ == 0) resultsMs_ = nowMs;
  if (resultsMs_ > 0 && nowMs - resultsMs_ > 1500) {
    // (Pointers: the race captures input, so a tap shows up as a press.)
    bool again = input.keyDown("Enter");
    for (const engine::RawPress& p : input.presses) {
      if (p.down && p.key < 0 && p.pos.x > 0.3f && p.pos.x < 0.7f && p.pos.y > 0.3f && p.pos.y < 0.8f) again = true;
    }
    if (again) {
      enterWas_ = true;  // (the same Enter press doesn't also press READY)
      openGarage(nowMs);
      return false;
    }
  }
  if (roomRace_ && !world_.started() && engine::net::serverTime() > 0) {
    world_.setGoIn((raceGoAt_ - engine::net::serverTime()) * 1000);
  }
  world_.update(dt);
  updateRemote(nowMs, dt);
  updateCamera(dt);
  // Engine sound from the revs and the throttle.
  const float revs = (me.car.rpm - 900) / 6600;
  engine::Sound::set(humVoice_, 0.1f + 0.25f * me.car.throttleUsed + 0.05f * revs, 0.45f + 1.6f * revs);
  return false;
}

// The garage: the camera slowly circles your car, which sits on the right.
void Game::garageCamera(double nowMs) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const engine::Vehicle car = world_.shownCar(me);
  const float aspect = static_cast<float>(gpu_->width) / std::max(static_cast<float>(gpu_->height), 1.0f);
  camera_.fovY = std::min(2 * std::atan(std::tan(engine::radians(70.0f) * 0.5f) / aspect), engine::radians(70.0f));
  const float a = static_cast<float>((nowMs - garageMs_) * 0.00022) + 2.3f;
  const Vec3 around{std::sin(a), 0, std::cos(a)};
  const Vec3 eye = car.position + around * 8.6f + Vec3{0, 2.1f, 0};
  const Vec3 right = engine::normalize(engine::cross(car.position - eye, Vec3{0, 1, 0}));
  camera_.eye = eye;
  camera_.target = car.position - right * (aspect > 1.2f ? 3.0f : 0.0f) + Vec3{0, 0.1f, 0};
  camera_.up = {0, 1, 0};
}

void Game::updateCamera(float dt) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const engine::Vehicle car = world_.shownCar(me);  // between physics steps: no judder at speed
  const float speed = car.speed();
  const float aspect = static_cast<float>(gpu_->width) / std::max(static_cast<float>(gpu_->height), 1.0f);
  // About 90 degrees across in landscape; a bit wider at speed.
  const float hfov = engine::radians(88.0f + engine::clampf(std::fabs(speed) * 0.12f, 0, 10));
  camera_.fovY = std::min(2 * std::atan(std::tan(hfov * 0.5f) / aspect), engine::radians(80.0f));
  const Vec3 fwd = car.forward();
  float heading = engine::Vehicle::headingOf(fwd);
  if (controls_.lookingBack) heading += engine::kPi;
  if (cameraMode_ <= 1) {
    // Chase: the camera trails the car's heading with a little lag, so turns
    // and slides read clearly.
    const float lag = 1 - std::exp(-dt * 6.0f);
    camYaw_ += wrapAngle(heading - camYaw_) * lag;
    const Vec3 back{std::sin(camYaw_), 0, std::cos(camYaw_)};  // behind = opposite of forward
    const float zoom = world_.setup().cameraDistance;
    const float distance = (cameraMode_ == 0 ? 5.4f : 8.5f) * zoom, height = (cameraMode_ == 0 ? 1.65f : 2.6f) * zoom;
    Vec3 eye = car.position + back * distance + Vec3{0, height, 0};
    const Vec3 target = car.position + Vec3{-back.x, 0, -back.z} * 2.5f + Vec3{0, 0.75f, 0};
    // Keep above the ground.
    int hint = me.hint;
    const float ground = world_.groundY(eye, hint);
    eye.y = std::max(eye.y, ground + 0.6f);
    camera_.eye = eye;
    camera_.target = target;
    camera_.up = {0, 1, 0};
  } else {
    // Hood / bumper: fixed to the car (the horizon follows the car's roll a little).
    const Vec3 local = cameraMode_ == 2 ? Vec3{0, 0.42f, -0.55f} : Vec3{0, -0.12f, -2.3f};
    const Vec3 eye = car.worldPoint(local);
    Vec3 dir = controls_.lookingBack ? -fwd : fwd;
    camera_.eye = eye;
    camera_.target = eye + dir * 10.0f;
    camera_.up = engine::normalize(Vec3{0, 1, 0} * 3.0f + car.up());
  }
}

void Game::render(const wgpu::TextureView& target, engine::TextRenderer& text, double nowMs) {
  if (!open_ || !ready_) return;
  const float W = static_cast<float>(gpu_->width), H = static_cast<float>(gpu_->height);
  if (screen_ == Screen::kGarage) queueGarage(text, W, H, nowMs);
  else queueHud(text, W, H, gpu_->pixelRatio, nowMs);
  engine::profiler().drawOverlay(text, W, H, gpu_->pixelRatio);
  text.upload(gpu_->queue);
  std::vector<CarDraw> cars;
  for (const Racer& r : world_.racers()) {
    if (!r.hidden) cars.push_back(View::carDraw(world_.shownCar(r), r.paint, r.car.brakeUsed, r.model, r.rim));
  }
  view_.render(target, gpu_->depthView, gpu_->width, gpu_->height, camera_, cars, world_.lightsOn(),
               static_cast<float>(nowMs / 1000), &text);
}

float Game::steer() const {
  if (!open_) return 0;
  const engine::Vehicle& car = world_.racers()[static_cast<size_t>(world_.you())].car;
  return car.wheels[0].steer / car.spec().maxSteer;
}

void Game::stateFor(float out[5]) const {
  for (int i = 0; i < 5; ++i) out[i] = 0;
  if (!open_) return;
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  out[0] = me.car.position.x;
  out[1] = me.car.position.z;
  out[2] = engine::Vehicle::headingOf(me.car.forward());
  out[3] = me.car.speed();
  out[4] = engine::clampf(me.input.steer, -1, 1);
}

void Game::queueGarage(engine::TextRenderer& text, float W, float H, double nowMs) {
  const float u = std::min(W, H) / 400.0f;
  const float x0 = 0.05f * W;
  engine::TextStyle st;
  st.color = {1, 1, 1, 1};
  // A dark panel behind the picks, so they read over the 3D.
  text.addRect(0, 0, x0 + 262 * u, H, {0, 0, 0, 0.5f});  // (picks, then the skid pad button at the bottom)
  st.color = {1, 0.85f, 0.3f, 1};
  text.addScreen("GARAGE", x0 + 70 * u, 14 * u, 14 * u, st);
  st.color = {1, 1, 1, 0.8f};
  text.addScreen("PICK YOUR CAR", x0, 44 * u, 7 * u, st);
  text.addScreen(std::string("PAINT: ") + paintName(choice_.paint), x0, 128 * u, 7 * u, st);
  text.addScreen("RIMS", x0, 202 * u, 7 * u, st);
  const CarModel& model = carModel(choice_.car);
  st.color = {0.75f, 0.95f, 1, 0.95f};
  text.addScreen(model.blurb, x0, 248 * u, 6.5f * u, st);
  const std::vector<Hit> hits = garageButtons(W, H);
  for (const Hit& h : hits) {
    switch (h.action) {
      case 6: {  // leave
        text.addRect(h.x, h.y, h.w, h.h, {0.05f, 0.05f, 0.08f, 0.7f});
        engine::TextStyle t;
        t.align = 0.5f;
        text.addScreen("LEAVE", h.x + h.w * 0.5f, h.y + h.h * 0.5f - 3.5f * u, 7 * u, t);
        break;
      }
      case 1: {  // a car
        const bool on = h.arg == choice_.car;
        const CarModel& m = carModel(h.arg);
        text.addRect(h.x, h.y, h.w, h.h, on ? Vec4{1, 0.42f, 0.78f, 0.55f} : Vec4{0.1f, 0.1f, 0.14f, 0.75f});
        if (on) text.addRect(h.x, h.y + h.h - 3 * u, h.w, 3 * u, {1, 0.85f, 0.3f, 1});
        engine::TextStyle t;
        t.align = 0.5f;
        text.addScreen(m.name, h.x + h.w * 0.5f, h.y + 5 * u, 7 * u, t);
        const char* labels[3] = {"SPD", "HND", "WGT"};
        const float bars[3] = {m.speed, m.handling, m.weight};
        engine::TextStyle l;
        l.color = {1, 1, 1, 0.7f};
        for (int k = 0; k < 3; ++k) {
          const float y = h.y + 20 * u + static_cast<float>(k) * 12 * u;
          text.addScreen(labels[k], h.x + 3 * u, y, 4.5f * u, l);
          text.addRect(h.x + 19 * u, y + 1 * u, 37 * u, 4 * u, {0, 0, 0, 0.5f});
          text.addRect(h.x + 19 * u, y + 1 * u, 37 * u * bars[k], 4 * u, {0.3f, 0.9f, 1, 0.95f});
        }
        if (m.newPhysics) {
          engine::TextStyle n;
          n.align = 0.5f;
          n.color = {1, 0.85f, 0.3f, 1};
          text.addScreen("NEW", h.x + h.w * 0.5f, h.y + h.h - 9 * u, 5 * u, n);
        }
        break;
      }
      case 2:
      case 3: {  // a colour
        const Vec3 c = h.action == 2 ? paintColor(h.arg) : rimColor(h.arg);
        const bool on = h.arg == (h.action == 2 ? choice_.paint : choice_.rim);
        if (on) text.addRect(h.x - 2 * u, h.y - 2 * u, h.w + 4 * u, h.h + 4 * u, {1, 1, 1, 1});
        text.addRect(h.x, h.y, h.w, h.h, {c.x, c.y, c.z, 1});
        break;
      }
      case 4: {  // ready
        const float pulse = 0.75f + 0.25f * std::sin(static_cast<float>(nowMs) * 0.006f);
        text.addRect(h.x, h.y, h.w, h.h, wantReady_ ? Vec4{0.9f, 0.6f, 0.1f, 0.8f * pulse} : Vec4{0.1f, 0.75f, 0.3f, 0.85f});
        engine::TextStyle t;
        t.align = 0.5f;
        text.addScreen(wantReady_ ? "WAITING..." : "READY", h.x + h.w * 0.5f, h.y + 9 * u, 14 * u, t);
        if (wantReady_) text.addScreen("TAP TO CANCEL", h.x + h.w * 0.5f, h.y + 30 * u, 5.5f * u, t);
        break;
      }
      case 7: {  // skid pad
        text.addRect(h.x, h.y, h.w, h.h, {0.1f, 0.35f, 0.75f, 0.8f});
        engine::TextStyle t;
        t.align = 0.5f;
        text.addScreen("SKID PAD: TRY THE NEW PHYSICS (GS)", h.x + h.w * 0.5f, h.y + h.h * 0.5f - 3.5f * u, 7 * u, t);
        break;
      }
      case 5: {  // race the bots instead
        text.addRect(h.x, h.y, h.w, h.h, {0.05f, 0.05f, 0.08f, 0.75f});
        engine::TextStyle t;
        t.align = 0.5f;
        text.addScreen("RACE THE BOTS NOW", h.x + h.w * 0.5f, h.y + h.h * 0.5f - 3.5f * u, 7 * u, t);
        break;
      }
      default: break;
    }
  }
  // Who's on the other rig (top right).
  {
    engine::TextStyle t;
    t.align = 1;
    t.color = {1, 1, 1, 0.9f};
    const std::string other = otherRigText();
    text.addRect(W - 0.05f * W - 230 * u, 10 * u, 230 * u, 20 * u, {0, 0, 0, 0.5f});
    text.addScreen(other, W - 0.05f * W - 6 * u, 16 * u, 6.5f * u, t);
    t.color = {1, 1, 1, 0.45f};
    text.addScreen("build " + build_, W - 6 * u, H - 9 * u, 5 * u, t);
  }
  if (H > W) {
    engine::TextStyle t;
    t.align = 0.5f;
    t.color = {1, 1, 0.6f, 1};
    text.addRect(0, H * 0.42f, W, 40 * u, {0, 0, 0, 0.6f});
    text.addScreen("TURN YOUR PHONE SIDEWAYS", W * 0.5f, H * 0.42f + 12 * u, 12 * u, t);
  }
}

void Game::queueHud(engine::TextRenderer& text, float W, float H, float px, double nowMs) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const engine::Vehicle& car = me.car;
  const float aspect = W / std::max(H, 1.0f);
  const float u = std::min(W, H) / 400.0f;  // HUD unit: scales with the screen
  engine::TextStyle st;
  st.color = {1, 1, 1, 1};
  const auto rect = [&](const Controls::Rect& r, Vec4 color) { text.addRect(r.x * W, r.y * H, r.w * W, r.h * H, color); };
  const auto label = [&](const Controls::Rect& r, const char* s, float size) {
    engine::TextStyle t;
    t.align = 0.5f;
    t.color = {1, 1, 1, 0.92f};
    text.addScreen(s, (r.x + r.w * 0.5f) * W, (r.y + r.h * 0.5f) * H - size * 0.5f, size, t);
  };

  char line[96];
  st.align = 0.5f;
  // The g meter (smoothed a little so it reads).
  if (me.drive) {
    const Vec3 a = me.drive->accelLocal;
    gSmooth_ = gSmooth_ + (Vec3{a.x, a.y, a.z} * (1 / 9.81f) - gSmooth_) * 0.15f;
  }
  if (practice_) {
    // Practice: where you are, which physics, the g you pull.
    text.addRect(W * 0.5f - 95 * u, 6 * u, 190 * u, 40 * u, {0, 0, 0, 0.45f});
    text.addScreen(world_.practiceOnPad() ? "SKID PAD" : "THE RING", W * 0.5f, 11 * u, 13 * u, st);
    if (me.drive) {
      std::snprintf(line, sizeof(line), "NEW PHYSICS   %.2f G", std::hypot(gSmooth_.x, gSmooth_.z));
    } else {
      std::snprintf(line, sizeof(line), "OLD PHYSICS (SETUP TO SWITCH)");
    }
    st.color = {1, 0.9f, 0.6f, 1};
    text.addScreen(line, W * 0.5f, 30 * u, 8 * u, st);
    if (nowMs - startMs_ < 9000 && world_.practiceOnPad()) {
      engine::TextStyle t;
      t.align = 0.5f;
      t.color = {1, 1, 1, 0.8f};
      text.addRect(W * 0.5f - 150 * u, 52 * u, 300 * u, 30 * u, {0, 0, 0, 0.4f});
      text.addScreen("DRIVE THE PAINTED RING. HANDBRAKE TO SLIDE.\nSETUP CHANGES HOW THE CAR DRIVES, LIVE.", W * 0.5f, 56 * u,
                     7 * u, t);
    }
  } else {
    // Position, lap, times (top middle).
    std::snprintf(line, sizeof(line), "P%d/%d   LAP %d/%d", me.place, static_cast<int>(world_.racers().size()),
                  std::min(me.lap + 1, world_.laps()), world_.laps());
    text.addRect(W * 0.5f - 95 * u, 6 * u, 190 * u, 40 * u, {0, 0, 0, 0.45f});
    text.addScreen(line, W * 0.5f, 11 * u, 13 * u, st);
    // Lap time on the race clock (physics steps), not the wall clock.
    const double lapMs = world_.started() && !me.finished ? world_.clockMs() - me.lapStartMs : 0;
    std::snprintf(line, sizeof(line), "%s   BEST %s", timeText(lapMs).c_str(), timeText(me.bestLapMs).c_str());
    st.color = {1, 0.9f, 0.6f, 1};
    text.addScreen(line, W * 0.5f, 30 * u, 8 * u, st);
  }
  if (world_.setup().telemetry || telemetryHook_) queueTelemetry(text, W, H, u);

  // Speed, gear and revs (bottom middle, between the thumbs).
  const float cx = W * 0.5f + W * 0.06f, by = H - 18 * u;
  const float rpmFrac = engine::clampf((car.rpm - 900) / (car.spec().maxRpm - 900), 0, 1);
  const float barW = 120 * u;
  text.addRect(cx - barW * 0.5f, by - 64 * u, barW, 7 * u, {0, 0, 0, 0.5f});
  const Vec4 rpmColor = rpmFrac > 0.92f ? Vec4{1, 0.15f, 0.1f, 1} : rpmFrac > 0.75f ? Vec4{1, 0.75f, 0.1f, 1} : Vec4{0.3f, 0.9f, 1, 1};
  text.addRect(cx - barW * 0.5f, by - 64 * u, barW * rpmFrac, 7 * u, rpmColor);
  st.color = {1, 1, 1, 1};
  st.align = 0.5f;
  std::snprintf(line, sizeof(line), "%d", static_cast<int>(std::fabs(car.speed()) * 3.6f + 0.5f));
  text.addScreen(line, cx - 14 * u, by - 50 * u, 26 * u, st);
  st.color = {0.8f, 0.8f, 0.8f, 1};
  text.addScreen("KM/H", cx - 14 * u, by - 18 * u, 6 * u, st);
  const std::string gear = car.gear < 0 ? "R" : car.gear == 0 ? "N" : std::to_string(car.gear);
  st.color = rpmColor;
  text.addScreen(gear, cx + 44 * u, by - 50 * u, 22 * u, st);
  st.color = {0.8f, 0.8f, 0.8f, 1};
  text.addScreen(manualGears_ ? "MAN" : "AUTO", cx + 44 * u, by - 18 * u, 6 * u, st);

  // Standings (top left, under the leave button): the cars around you.
  if (!practice_) {
    std::vector<const Racer*> order;
    for (const Racer& r : world_.racers()) {
      if (!r.hidden) order.push_back(&r);
    }
    std::sort(order.begin(), order.end(), [](const Racer* a, const Racer* b) { return a->place < b->place; });
    engine::TextStyle t;
    float y = 0.16f * H;
    for (const Racer* r : order) {
      t.color = r == &me ? Vec4{1, 0.85f, 0.3f, 1} : Vec4{1, 1, 1, 0.85f};
      std::snprintf(line, sizeof(line), "%d %s", r->place, r->name.c_str());
      text.addRect(0.012f * W, y - 2 * u, 70 * u, 13 * u, {0, 0, 0, 0.35f});
      text.addRect(0.012f * W, y - 2 * u, 3 * u, 13 * u, {r->paint.x, r->paint.y, r->paint.z, 1});
      text.addScreen(line, 0.012f * W + 7 * u, y, 7 * u, t);
      y += 15 * u;
    }
  }

  // Start: lights are on the gantry; a big GO when they go out.
  const double go = world_.goMs(), raceNow = world_.clockMs();
  if (practice_) {
    // (no lights)
  } else if (raceNow < go) {
    const int lit = world_.lightsOn();
    for (int k = 0; k < 5; ++k) {
      text.addShape(0, W * 0.5f + (static_cast<float>(k) - 2) * 26 * u, H * 0.3f, 10 * u,
                    k < lit ? Vec4{1, 0.05f, 0.03f, 1} : Vec4{0.15f, 0.02f, 0.02f, 0.9f}, {0, 0, 0, 1}, 2 * u);
    }
    st.color = {1, 1, 1, 0.9f};
    text.addScreen(lit == 0 ? "GET READY" : "", W * 0.5f, H * 0.36f, 12 * u, st);
  } else if (raceNow < go + 1200) {
    st.color = {0.3f, 1, 0.3f, 1};
    text.addScreen("GO!", W * 0.5f, H * 0.28f, 40 * u, st);
  }

  // Results.
  if (me.finished) {
    const float bw = 260 * u, bh = 170 * u;
    text.addRect(W * 0.5f - bw * 0.5f, H * 0.5f - bh * 0.5f, bw, bh, {0, 0, 0, 0.75f});
    st.color = {1, 0.85f, 0.3f, 1};
    if (me.finishMs > 0) std::snprintf(line, sizeof(line), "FINISHED P%d", me.place);
    else std::snprintf(line, sizeof(line), "TIME'S UP");
    text.addScreen(line, W * 0.5f, H * 0.5f - bh * 0.5f + 10 * u, 16 * u, st);
    std::vector<const Racer*> order;
    for (const Racer& r : world_.racers()) {
      if (!r.hidden) order.push_back(&r);
    }
    std::sort(order.begin(), order.end(), [](const Racer* a, const Racer* b) { return a->place < b->place; });
    engine::TextStyle t;
    float y = H * 0.5f - bh * 0.5f + 36 * u;
    for (const Racer* r : order) {
      t.color = r == &me ? Vec4{1, 0.85f, 0.3f, 1} : Vec4{1, 1, 1, 0.9f};
      std::snprintf(line, sizeof(line), "%d. %-8s %s  best %s", r->place, r->name.c_str(),
                    r->finished ? timeText(r->finishMs).c_str() : "racing", timeText(r->bestLapMs).c_str());
      text.addScreen(line, W * 0.5f - bw * 0.5f + 14 * u, y, 7 * u, t);
      y += 13 * u;
    }
    st.color = {0.6f, 1, 0.6f, 1};
    text.addScreen("TAP HERE FOR THE GARAGE", W * 0.5f, H * 0.5f + bh * 0.5f - 18 * u, 8 * u, st);
  }

  // Touch controls (drawn unless driving with a pad/wheel/keys).
  const bool touch = controls_.source == Controls::Source::kTouch;
  for (int b = 0; b < Controls::kButtonCount; ++b) {
    const auto button = static_cast<Controls::Button>(b);
    const Controls::Rect r = controls_.buttonRect(button, aspect, manualGears_);
    if (r.w <= 0) continue;
    if (!touch && b != Controls::kLeave) continue;
    rect(r, controls_.buttonHeld(button) ? Vec4{1, 0.42f, 0.78f, 0.6f} : Vec4{0.05f, 0.05f, 0.08f, 0.45f});
    static const char* kLabels[] = {"LEAVE", "CAM", "RESET", "BACK", "HANDBRAKE", "GEAR -", "GEAR +", "SETUP", "PAD"};
    const char* name = b == Controls::kCamera ? kCameraNames[cameraMode_] : kLabels[b];
    if (b == Controls::kPlace) name = world_.practiceOnPad() ? "TO RING" : "TO PAD";
    label(r, name, 7 * u);
  }
  if (touch) {
    const Controls::Rect gas = controls_.gasPedal(aspect), brake = controls_.brakePedal(aspect);
    rect(brake, {0.05f, 0.05f, 0.08f, 0.35f});
    rect({brake.x, brake.y + brake.h * (1 - controls_.brakeValue()), brake.w, brake.h * controls_.brakeValue()},
         {1, 0.2f, 0.15f, 0.45f});
    label(brake, "BRAKE", 9 * u);
    rect(gas, {0.05f, 0.05f, 0.08f, 0.35f});
    rect({gas.x, gas.y + gas.h * (1 - controls_.gasValue()), gas.w, gas.h * controls_.gasValue()},
         {0.2f, 1, 0.4f, 0.45f});
    label(gas, "GAS", 11 * u);
    // Steering: a bar showing the wheel, plus where your thumb set the center.
    const float sy = H - 22 * u, sw = 0.36f * W, sx = 0.04f * W;
    text.addRect(sx, sy, sw, 6 * u, {0, 0, 0, 0.4f});
    text.addRect(sx + sw * 0.5f - 1 * u, sy - 4 * u, 2 * u, 14 * u, {1, 1, 1, 0.6f});
    const float st01 = 0.5f + 0.5f * controls_.rawSteer();
    text.addShape(0, sx + sw * st01, sy + 3 * u, 8 * u,
                  controls_.steering() ? Vec4{1, 0.42f, 0.78f, 0.95f} : Vec4{1, 1, 1, 0.5f});
    if (!controls_.steering() && nowMs - startMs_ < 9000) {
      engine::TextStyle t;
      t.align = 0.5f;
      t.color = {1, 1, 1, 0.75f};
      text.addScreen("THUMB DOWN ANYWHERE HERE\nAND SLIDE TO STEER", 0.25f * W, 0.55f * H, 8 * u, t);
    }
  } else {
    engine::TextStyle t;
    t.color = {1, 1, 1, 0.6f};
    const std::string help = controls_.source == Controls::Source::kKeyboard || controls_.source == Controls::Source::kMouse
                                 ? "W/S gas brake  A/D steer  SPACE handbrake  E/Q gears  C camera  R reset  M mouse steer  T setup  P pad/ring"
                                 : controls_.deviceName();
    text.addScreen(help, 0.012f * W, H - 14 * u, 6 * u, t);
  }
  {
    engine::TextStyle t;
    t.align = 1;
    t.color = {1, 1, 1, 0.45f};
    text.addScreen("build " + build_, W - 6 * u, H - 9 * u, 5 * u, t);
  }
  if (H > W) {
    engine::TextStyle t;
    t.align = 0.5f;
    t.color = {1, 1, 0.6f, 1};
    text.addRect(0, H * 0.42f, W, 40 * u, {0, 0, 0, 0.6f});
    text.addScreen("TURN YOUR PHONE SIDEWAYS", W * 0.5f, H * 0.42f + 12 * u, 12 * u, t);
  }
  (void)px;
}

// The telemetry overlay (Car setup → Telemetry, or ?telemetry=1): the g-g
// circle with the car's dot (how much grip you use), and per tire its load,
// slip ratio, slip angle and suspension travel.
void Game::queueTelemetry(engine::TextRenderer& text, float W, float H, float u) {
  const Racer& me = world_.racers()[static_cast<size_t>(world_.you())];
  const float boxW = 132 * u, x0 = W - boxW - 8 * u - controls_.safeRight() * W;
  const float cx = x0 + boxW * 0.5f, cy = H * 0.36f, R = 30 * u;  // the circle is 1.5 g
  text.addRect(x0, cy - 36 * u, boxW, 132 * u, {0, 0, 0, 0.45f});
  text.addShape(engine::font::kShapeRing, cx, cy, R, {1, 1, 1, 0.35f});
  text.addShape(engine::font::kShapeRing, cx, cy, R * (1 / 1.5f), {1, 1, 1, 0.25f});  // 1 g
  // The dot goes where your body is pushed: braking = up, a left turn = right.
  const float gx = engine::clampf(gSmooth_.x, -1.5f, 1.5f), gz = engine::clampf(gSmooth_.z, -1.5f, 1.5f);
  text.addShape(engine::font::kShapeDisc, cx - gx / 1.5f * R, cy - gz / 1.5f * R, 3.5f * u, {1, 0.42f, 0.78f, 1});
  engine::TextStyle t;
  t.color = {1, 1, 1, 0.85f};
  char line[64];
  std::snprintf(line, sizeof(line), "%.2f G", std::hypot(gSmooth_.x, gSmooth_.z));
  t.align = 0.5f;
  text.addScreen(line, cx, cy + R + 4 * u, 6 * u, t);
  t.align = 0;
  static const char* kCorner[4] = {"FL", "FR", "RL", "RR"};
  for (int i = 0; i < 4; ++i) {
    const engine::Vehicle::Wheel& w = me.car.wheels[static_cast<size_t>(i)];
    const float travel = me.drive ? me.drive->wheels[static_cast<size_t>(i)].travel : w.compression;
    std::snprintf(line, sizeof(line), "%s %4.1fKN S%+.2f A%+3.0f %2.0fMM", kCorner[i], w.load / 1000, w.slipRatio,
                  std::atan(w.slipAngle) * 57.3f, travel * 1000);
    t.color = w.slide > 1 ? Vec4{1, 0.5f, 0.3f, 1} : Vec4{1, 1, 1, 0.85f};
    text.addScreen(line, x0 + 4 * u, cy + R + 14 * u + static_cast<float>(i) * 9 * u, 5 * u, t);
  }
  if (me.drive) {
    std::snprintf(line, sizeof(line), "WHEEL %+4.0f  %s%s", me.drive->wheelAngle * 57.3f, me.drive->absActive ? "ABS " : "",
                  me.drive->tcActive ? "TC" : "");
    t.color = {0.7f, 0.95f, 1, 0.9f};
    text.addScreen(line, x0 + 4 * u, cy + R + 52 * u, 5 * u, t);
  }
}

}  // namespace app::race
