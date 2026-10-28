// The bar app's game logic: the frame's phases, sitting, using things, the
// room's claims, menus and the room's shared state. Entry points: main.cpp.
// Left thumb = joystick to walk, right thumb = drag to look around, tap to
// use things. Drawing lives in bar/renderer.*; the games in their own files
// (piano, darts, racing, rhythm, bartender); the HTML menus in bar/web/menu.js.
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <cstdio>
#include <cstring>
#include <vector>

#include "bar/scene_names.h"
#include "bar/avatar.h"
#include "bar/bar_scene.h"
#include "bar/world_live.h"
#include "bar/studio_mode.h"
#include "bar/bartender.h"
#include "bar/darts.h"
#include "bar/jukebox.h"
#include "bar/piano.h"
#include "bar/players.h"
#include "bar/props.h"
#include "bar/projects/racing/game.h"
#include "bar/loader.h"
#include "bar/renderer.h"
#include "bar/test_hooks.h"
#include "bar/gpu_check.h"
#include "bar/projects/chess/chess.h"
#include "bar/projects/mania/rhythm.h"
#include "bar/shared_world.h"
#include "looped/gpu_upload.h"
#include "looped/audio.h"
#include "looped/billboard.h"
#include "looped/bridge.h"
#include "looped/camera.h"
#include "looped/fetch.h"
#include "looped/file.h"
#include "looped/gpu.h"
#include "looped/frame_time.h"
#include "looped/input.h"
#include "looped/jobs.h"
#include "looped/json.h"
#include "looped/log.h"
#include "looped/pipeline_cache.h"
#include "looped/profiler.h"
#include "looped/mesh.h"
#include "looped/net.h"
#include "looped/synth.h"
#include "looped/text.h"
#include "looped/texture.h"
#include "looped/ui.h"

using engine::Mat4;
using engine::Vec3;

void joinRoom();  // main.cpp: joins the multiplayer room once you've entered

#include "bar/app_state.h"
#include "bar/interaction.h"
#include "bar/hud.h"
#include "bar/presentation.h"
#include "bar/local_player.h"

namespace barapp {

App g_app;

void showToast(const std::string& text) {
  g_app.toast = text;
  g_app.toastEndMs = engine::frameTime().nowMs + kToastMs;
}

void playSfx(engine::synth::Effect e, Vec3 where, float gain, bool share) {
  g_app.sfx[static_cast<int>(e)].play(gain, &where);
  if (share) {
    const double v[4] = {static_cast<double>(e), where.x, where.y, where.z};
    engine::net::sendEvent("sfx", v, 4);
  }
}

// A guitar chord (E major), strummed.
void strum(Vec3 where, bool share) {
  for (int s = 0; s < 6; ++s) g_app.guitar[s].play(0.5f, &where, 0.025 * s);
  if (share) {
    const double v[4] = {-1, where.x, where.y, where.z};
    engine::net::sendEvent("sfx", v, 4);
  }
}

// Badges come from your account (bar/web/account.js). Asked at most once a
// second per badge (the prompts ask every frame), so logging in or out
// shows within a second.
bool hasBadge(const char* badge) { return engine::ui::hasBadge(badge); }

// ---- Who's using what ----

const app::OtherPlayer* playerUsing(Activity a, int param) {
  for (const app::OtherPlayer& o : g_app.others.players()) {
    if (!o.placed) continue;
    const auto act = static_cast<Activity>(static_cast<int>(o.values[app::slot::kActivity]));
    const int p = static_cast<int>(o.values[app::slot::kParam]);
    const bool seated = act == Activity::kSit || act == Activity::kDrive || act == Activity::kPiano;
    if (a == Activity::kSit ? (seated && p == param) : (act == a && p == param)) return &o;
  }
  return nullptr;
}

// Switches what you're doing (the callers already checked the rules).
void setActivity(Activity a) {
  if (g_app.act.current() == a) return;
  g_app.act.set(a, g_app.seat >= 0 ? g_app.seat : 0, engine::frameTime().nowMs);
  if (a != Activity::kEmote) g_app.emote = Emote::kNone;
}

void startEmote(Emote e) {
  std::string why;
  // Each emote is a new one, even the same again (param = how many so far).
  if (!g_app.act.request(Activity::kEmote, g_app.emoteCount + 1, engine::frameTime().nowMs, &why)) {
    showToast(why);
    return;
  }
  g_app.emote = e;
  ++g_app.emoteCount;
}

// ---- Sitting ----

void applyQuality(const char* why);  // (below, with the frame)

// Game input (touches go to a game instead of walking and looking): on
// exactly while a game that takes the whole screen is open. The one place
// that switches it, so leaving a game can't forget to give the controls back.
void syncCapture() {
  g_app.input.setCapture(g_app.race.open() || g_app.rhythm.isOpen() || g_app.chess.isOpen() ||
                         g_app.piano.keyboardOpen() || studioOn());
}
void applyMirrorDetail();

std::string seatKey(int seat) { return "seat." + std::to_string(seat); }

void standUp() {
  if (g_app.seat < 0) return;
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
  if (g_app.others.online()) engine::net::release(seatKey(g_app.seat));
  g_app.seatClaim = 0;
  if (s.kind == app::SeatKind::kSim) {
    g_app.race.close();
    syncCapture();
  }
  if (s.kind == app::SeatKind::kPianoBench) {
    g_app.piano.closeKeyboard();
    syncCapture();
  }
  // Step off where you can walk away (app::standSpot).
  Vec3 p = app::standSpot(g_app.scene, s);
  p.y = app::groundHeight(g_app.scene, p) + 1.65f;
  g_app.camera.position = p;
  g_app.eyeHeight = 1.65f;
  g_app.seat = -1;
  setActivity(Activity::kNone);
}

void logProject(int thing);

// The race on the rig you're sitting in.
void startRace(bool straightIn, bool pad) {
  if (g_app.seat < 0) return;
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
  if (s.kind != app::SeatKind::kSim) return;
  if (g_app.race.start(std::max(0, s.machine), engine::frameTime().nowMs, straightIn, pad)) {
    syncCapture();
  } else {
    showToast("Warming up the race...");  // it starts by itself in a moment
  }
}

void sitOn(int index) {
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(index)];
  if (const std::string why = g_app.act.blocked(Activity::kSit, index); !why.empty()) {
    showToast(why);
    return;
  }
  // The room decides who sits where: ask, and sit down right away; if the
  // room says someone else got there first, you stand up again (frame()).
  if (g_app.others.online()) {
    const int owner = engine::net::claimOwner(seatKey(index));
    if (owner > 0) {
      showToast(engine::net::playerName(owner) + " is sitting there.");
      LOG_INFO("Seat %d: %s is sitting there.", index, engine::net::playerName(owner).c_str());
      return;
    }
  }
  if (g_app.held >= 0) {
    showToast("Put the glass down first.");
    return;
  }
  standUp();
  g_app.seat = index;
  if (g_app.others.online()) g_app.seatClaim = engine::net::claim(seatKey(index));
  g_app.camera.position = app::seatedEye(s.hip, s.yaw, s.lowSeat);
  g_app.camera.yaw = s.yaw;
  g_app.camera.pitch = s.kind == app::SeatKind::kPianoBench ? engine::radians(-58.0f) : 0;
  if (static_cast<size_t>(index) < g_app.scene.seatThing.size()) logProject(g_app.scene.seatThing[static_cast<size_t>(index)]);
  if (s.kind == app::SeatKind::kSim) {
    setActivity(Activity::kDrive);
    // Pick a game: the race with bots or Meridian (bar/web/menu.js "sim"). The
    // ?sit= test option goes straight into the race.
    if (g_app.hooks.sit == index) startRace(!g_app.hooks.garage, g_app.hooks.pad);
    else engine::ui::openPanel("sim");
  } else if (s.kind == app::SeatKind::kPianoBench) {
    setActivity(Activity::kPiano);
    g_app.piano.openKeyboard();
    syncCapture();
  } else {
    setActivity(Activity::kSit);
    showToast(std::string(s.name) + "\nMove the joystick to stand up");
  }
}



// Slab test: distance along the ray to the box, or -1 if it misses.
float rayBoxDistance(Vec3 origin, Vec3 dir, Vec3 boxMin, Vec3 boxMax) {
  float tNear = 0, tFar = 1e9f;
  const float o[3] = {origin.x, origin.y, origin.z}, d[3] = {dir.x, dir.y, dir.z};
  const float lo[3] = {boxMin.x, boxMin.y, boxMin.z}, hi[3] = {boxMax.x, boxMax.y, boxMax.z};
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(d[i]) < 1e-8f) {
      if (o[i] < lo[i] || o[i] > hi[i]) return -1;
      continue;
    }
    float t0 = (lo[i] - o[i]) / d[i], t1 = (hi[i] - o[i]) / d[i];
    if (t0 > t1) std::swap(t0, t1);
    tNear = std::max(tNear, t0);
    tFar = std::min(tFar, t1);
    if (tNear > tFar) return -1;
  }
  return tNear;
}

// The world-space ray through a point on the screen (0..1 fractions).
// Where a ray first meets a turned box (center, half sizes, turn), or -1.
float rayTurnedBoxDistance(Vec3 origin, Vec3 dir, const app::Collider& c) {
  const engine::Quat back = engine::conjugate(c.rotation);
  return rayBoxDistance(back.rotate(origin - c.center), back.rotate(dir), Vec3{} - c.half, c.half);
}

// The nearest living thing a tap's ray touches (models whose kind has a "tap"
// rule now: a lamp, a table, a candle, the counter), or -1. Small things by
// their model's box; big ones (the counter, the back bar, the piano) by their
// solid shapes, since their boxes cover the floor around them (E4.1).
int touchAt(Vec3 dir, float* distance) {
  const Vec3 eye = g_app.camera.position;
  int best = -1;
  float nearest = kTapReach;
  std::vector<int> big;
  for (size_t m = 0; m < g_app.scene.models.size() && m < g_app.scene.modelThing.size(); ++m) {
    const int thing = g_app.scene.modelThing[m];
    if (!g_app.world.reacts(thing)) continue;
    engine::Aabb box;
    if (!g_app.renderer.modelBox(m, &box)) continue;
    if (std::max(box.max.x - box.min.x, box.max.z - box.min.z) > 2.5f) {
      big.push_back(thing);
      continue;
    }
    const float t = rayBoxDistance(eye, dir, box.min, box.max);
    if (t >= 0 && t < nearest) nearest = t, best = thing;
  }
  for (size_t k = 0; k < g_app.scene.colliders.size() && k < g_app.scene.colliderThing.size() && !big.empty(); ++k) {
    const int thing = g_app.scene.colliderThing[k];
    if (std::find(big.begin(), big.end(), thing) == big.end()) continue;
    const float t = rayTurnedBoxDistance(eye, dir, g_app.scene.colliders[k]);
    if (t >= 0 && t < nearest) nearest = t, best = thing;
  }
  *distance = nearest;
  return best;
}

Vec3 screenRay(engine::Vec2 tap) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  const Vec3 up = engine::cross(right, forward);
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  const float tanHalf = std::tan(cam.fovY * 0.5f);
  const float x = (tap.x * 2 - 1) * tanHalf * aspect, y = (1 - tap.y * 2) * tanHalf;
  return engine::normalize(forward + right * x + up * y);
}

// Tapping another player opens their person card.
// The other player a ray hits (index into others.players()), or -1.
int personAt(Vec3 dir, float* dist) {
  const Vec3 eye = g_app.camera.position;
  int best = -1;
  float nearest = 6.0f;
  const auto& players = g_app.others.players();
  for (size_t i = 0; i < players.size(); ++i) {
    const app::OtherPlayer& o = players[i];
    if (!o.placed) continue;
    const float t = rayBoxDistance(eye, dir, o.eye - Vec3{0.3f, 1.7f, 0.3f}, o.eye + Vec3{0.3f, 0.45f, 0.3f});
    if (t >= 0 && t < nearest) {
      nearest = t;
      best = static_cast<int>(i);
    }
  }
  if (dist) *dist = nearest;
  return best;
}

// The seat a ray hits within reach, or -1.
int seatHit(Vec3 dir, float reach, float* dist) {
  int best = -1;
  float nearest = reach;
  for (size_t i = 0; i < g_app.scene.seats.size(); ++i) {
    const app::Seat& s = g_app.scene.seats[i];
    const float t = rayBoxDistance(g_app.camera.position, dir, s.boundsMin, s.boundsMax);
    if (t >= 0 && t < nearest) {
      nearest = t;
      best = static_cast<int>(i);
    }
  }
  if (dist) *dist = nearest;
  return best;
}

// Builds a customizer style into look `index` (-1: a new one). Returns the index.
int buildStyle(const app::AvatarStyle& st, int index) {
  app::Avatar a;
  a.build(app::lookFromStyle(st));
  if (index < 0) return static_cast<int>(g_app.renderer.addAvatarMesh(a.mesh()));
  g_app.renderer.setAvatarMesh(static_cast<size_t>(index), a.mesh());
  return index;
}

// The look index for another player's custom style (room slot `slot`).
int lookForSlot(int slot) {
  const auto st = g_app.slotStyles.find(slot);
  if (st == g_app.slotStyles.end()) return 0;
  const std::string key = st->second.key();
  auto it = g_app.slotLooks.find(slot);
  if (it == g_app.slotLooks.end()) {
    it = g_app.slotLooks.emplace(slot, App::SlotLook{key, buildStyle(st->second, -1)}).first;
  } else if (it->second.key != key) {
    buildStyle(st->second, it->second.index);
    it->second.key = key;
  }
  return it->second.index;
}

// Puts on your customizer look (and remembers it on this phone).
void applyStyle(const app::AvatarStyle& st, bool remember) {
  g_app.style = st;
  g_app.style.clamp();
  g_app.customLook = true;
  g_app.selfCustomLook = buildStyle(g_app.style, g_app.selfCustomLook);
  g_app.renderer.setSelfAvatar(static_cast<size_t>(g_app.selfCustomLook));
  g_app.styleSentSlot = -1;  // tell the room again
  if (remember) {
    engine::settings::remember("avCustom", 1);
    for (int k = 0; k < app::AvatarStyle::kFields; ++k) engine::settings::remember(("av" + std::to_string(k)).c_str(), g_app.style.v[k]);
  }
}


// The room decides who uses a machine (one at a time). False if someone
// else has it; otherwise asks for it (and you start right away).
bool claimMachine(const char* key, const char* what) {
  if (!g_app.others.online()) return true;
  const int owner = engine::net::claimOwner(key);
  if (owner > 0) {
    showToast(engine::net::playerName(owner) + " is " + what + ". One player at a time.");
    LOG_INFO("Machine %s: %s is %s.", key, engine::net::playerName(owner).c_str(), what);
    return false;
  }
  g_app.machineKey = key;
  g_app.machineClaim = engine::net::claim(key);
  return true;
}

// The world thing a thing to use belongs to (assets/world/places/bar.json).
int thingOf(const app::Interactable& it) {
  const auto& items = g_app.scene.interactables;
  const size_t i = static_cast<size_t>(&it - items.data());
  return i < g_app.scene.useThing.size() ? g_app.scene.useThing[i] : -1;
}

// A thing that opens a project (its "project" part: the arcade cabinet, 4K
// Mania, the rigs, chess, darts) says which, for reports and the hub.
void logProject(int thing) {
  if (thing < 0) return;
  const engine::Json& p = g_app.world.world().part(thing, "project");
  if (p.isObject()) LOG_INFO("Project: %s (%s).", p["name"].string().c_str(), p["id"].string().c_str());
}

void use(const app::Interactable& it) {
  using app::Action;
  const Vec3 where = (it.boundsMin + it.boundsMax) * 0.5f;
  logProject(thingOf(it));
  switch (it.action) {
    case Action::kJukebox: g_app.jukeboxMenu.open(); break;
    case Action::kRhythm: {
      if (const std::string why = g_app.act.blocked(Activity::kRhythm, 0); !why.empty()) {
        showToast(why);
        break;
      }
      if (!claimMachine("machine.rhythm", "playing")) break;
      std::vector<app::RhythmGame::SongChoice> songs;
      for (const app::Song& s : g_app.jukebox.songs()) songs.push_back({s.title, "", s.url});
      g_app.rhythm.open(std::move(songs));
      syncCapture();
      setActivity(Activity::kRhythm);
      break;
    }
    // The mirror's modes are its states (full / avatars / off): tapping the
    // glass steps through them, a button picks one (assets/world/kinds/bar.json).
    case Action::kMirror:
    case Action::kMirrorMode:
      g_app.world.touch(thingOf(it));
      playSfx(engine::synth::Effect::kClick, where, 0.4f);
      break;
    case Action::kDarts: {
      if (const std::string why = g_app.act.blocked(Activity::kDarts, 0); !why.empty()) {
        showToast(why);
        break;
      }
      if (!claimMachine("machine.darts", "throwing")) break;
      teleport(g_app.scene.dartOche, engine::kPi);  // facing the board (+z)
      g_app.darts.setOnline(g_app.others.online() && g_app.machineKey == "machine.darts");
      g_app.darts.start();
      setActivity(Activity::kDarts);
      break;
    }
    case Action::kBartender: {
      if (g_app.bartender.onShift()) {
        g_app.bartender.endShift();
        showToast("Shift over. Thanks!");
        break;
      }
      if (!hasBadge("bartender")) {
        showToast("Only bartenders can work here.\nThe bar's owner can give you the badge.");
        break;
      }
      g_app.bartender.startShift();
      teleport(g_app.scene.bartenderSpot, engine::kPi);  // facing the counter and the room (+z)
      engine::ui::openPanel("bartenderShift");
      break;
    }
    case Action::kDrinkShelf: {
      if (!g_app.bartender.drinkAt(it.value)) break;
      if (g_app.bartender.held()) {
        showToast("Finish your drink first.");
        break;
      }
      if (g_app.bartender.take(it.value)) {
        playSfx(engine::synth::Effect::kClink, where, 0.5f);
        showToast("Cheers! Tap anywhere to take a sip.");
      }
      break;
    }
    case Action::kPianoSongs: engine::ui::openPanel("piano"); break;
    case Action::kBell: playSfx(engine::synth::Effect::kBell, where, 0.8f, true); break;
    case Action::kGuitar: strum(where, true); break;
    case Action::kMic:
      g_app.onMic = !g_app.onMic;
      showToast(g_app.onMic ? "You're on the mic: everyone in the bar hears you clearly." : "Off the mic.");
      break;
    // The switch and the fire are things with rules (assets/world/kinds/bar.json):
    // a touch follows them here and in the room, which shares the result.
    case Action::kLightSwitch:
      g_app.world.touch(thingOf(it));
      playSfx(engine::synth::Effect::kClick, where);
      break;
    case Action::kFireplace:
      g_app.world.touch(thingOf(it));
      showToast(g_app.world.stateOf("fire_flames") == "lit" ? "The fire crackles back to life." : "The fire dies down.");
      break;
    case Action::kDoor:
      playSfx(engine::synth::Effect::kKnock, where, 0.8f, true);
      showToast("The street outside isn't built yet.");
      break;
    case Action::kBooks:  // the lounge bookshelf: real books to read (J99, bar/web/reader.js)
      engine::ui::openPanel("books");
      playSfx(engine::synth::Effect::kClick, where, 0.4f);
      break;
    case Action::kChess: {
      if (g_app.seat >= 0) standUp();
      const std::string why = g_app.chess.open(g_app.others.online());
      if (!why.empty()) {
        showToast(why);
        break;
      }
      syncCapture();
      playSfx(engine::synth::Effect::kClick, where, 0.6f, true);
      break;
    }
    case Action::kTipJar:
      playSfx(engine::synth::Effect::kClink, where, 0.6f, true);
      showToast("You drop a coin in the tip jar. The bartenders thank you!");
      break;
    case Action::kTaps:
    case Action::kCoffee:
      if (g_app.bartender.onShift()) {
        g_app.bartender.make(it.action == Action::kTaps ? 0 : 9);  // beer / coffee
      } else {
        showToast("Ask a bartender for a drink.");
      }
      break;
    case Action::kArcade:
      // The machine's game full screen (bar/web/menu.js GAMES), like Meridian.
      if (it.value == 0) engine::ui::openPanel("platformer");
      break;
    case Action::kTv:
    case Action::kNone: break;
  }
}

// Lights the button for each mirror's current mode (also after another
// player changes it).
void updateMirrorButtons() {
  std::vector<int>& shown = g_app.mirrorButtonsShown;
  shown.resize(g_app.scene.mirrors.size(), -1);
  for (size_t i = 0; i < g_app.scene.mirrors.size(); ++i) {
    const app::Mirror& m = g_app.scene.mirrors[i];
    const int mode = !m.on ? 2 : m.mode == app::MirrorMode::kFull ? 0 : 1;
    if (shown[i] == mode) continue;
    shown[i] = mode;
    for (int b = 0; b < 3; ++b) {
      if (m.buttonModels[b] >= 0) g_app.renderer.setModelBaseGlow(static_cast<size_t>(m.buttonModels[b]), b == mode ? 1.6f : 0.05f);
    }
  }
}


// Steps physics, keeps a held thing in front of you, and moves the models.
void updatePhysics(float dt) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  if (g_app.held >= 0) {
    // In your right hand, a bit below eye level.
    const Vec3 right = cam.flatRight();
    g_app.physics.body(static_cast<size_t>(g_app.held)).holdTarget =
        cam.position + cam.forward() * 0.5f + right * 0.12f + Vec3{0, -0.28f, 0};
  }
  g_app.shared.update(engine::frameTime().nowMs, dt, g_app.physics, g_app.scene, g_app.jukebox, g_app.held);
  g_app.physics.step(dt);
  // Each model is drawn where its body is this frame (between the last two
  // physics steps), and only re-sent to the renderer when that changed.
  for (size_t i = 0; i < g_app.bodyOfModel.size(); ++i) {
    const int body = g_app.bodyOfModel[i];
    if (body < 0) continue;
    const engine::Body& b = g_app.physics.body(static_cast<size_t>(body));
    engine::Vec3& shownP = g_app.shownPosition[static_cast<size_t>(body)];
    engine::Quat& shownQ = g_app.shownRotation[static_cast<size_t>(body)];
    const engine::Quat& q = b.shownRotation;
    if (shownP == b.shownPosition && shownQ.x == q.x && shownQ.y == q.y && shownQ.z == q.z && shownQ.w == q.w) continue;
    shownP = b.shownPosition;
    shownQ = q;
    g_app.renderer.setModelTransform(i, shownP, shownQ);
  }
  g_app.chess.pose(g_app.renderer);
}

// Makes one interactable's parts glow (or stop glowing).
void setGlow(int index, float glow) {
  if (index < 0) return;
  const app::Interactable& it = g_app.scene.interactables[static_cast<size_t>(index)];
  g_app.renderer.setGlow(it.firstObject, it.objectCount, it.firstModel, it.modelCount, glow);
}

// Is this interactable there right now? (A drink spot only when it has a drink.)
bool available(const app::Interactable& it) {
  if (it.action == app::Action::kDrinkShelf) return g_app.bartender.drinkAt(it.value) != 0;
  return true;
}

// Menu commands (bar/web/menu.js) and settings.
void handleCommands() {
  std::vector<engine::ui::Command> commands;
  engine::ui::takeCommands(commands);
  for (const engine::ui::Command& c : commands) {
    if (studioCommand(c)) continue;  // the Studio's (bar/studio_mode.cpp)
    // Checked first: an emote in range, drinks only on shift, standing up only when seated.
    if (!app::commandAllowed(c.name, c.a, g_app.seat >= 0, g_app.bartender.onShift())) {
      LOG_INFO("Menu: ignored \"%s\" (not now).", c.name.c_str());
      continue;
    }
    if (c.name == "emote") startEmote(static_cast<Emote>(static_cast<int>(c.a)));
    else if (c.name == "look") {
      g_app.look = std::clamp(static_cast<int>(c.a), 0, static_cast<int>(app::avatarLooks().size()) - 1);
      g_app.customLook = false;
      engine::settings::remember("avCustom", 0);
      g_app.renderer.setSelfAvatar(static_cast<size_t>(g_app.look));
      showToast(std::string("You're now wearing: ") + app::avatarLooks()[static_cast<size_t>(g_app.look)].name);
    } else if (c.name == "pianoSong") {
      g_app.piano.playSong(static_cast<int>(c.a));
    } else if (c.name == "pianoStop") {
      g_app.piano.stopSong();
    } else if (c.name == "pianoAdd") {
      if (!c.text2.empty()) g_app.piano.addUserSong(c.text, c.text2);  // name, file URL
    } else if (c.name == "midi") {
      // From a MIDI keyboard: a = status byte, b = note * 1000 + velocity.
      const int status = static_cast<int>(c.a) & 0xf0, note = static_cast<int>(c.b) / 1000, vel = static_cast<int>(c.b) % 1000;
      if (status == 0x90 && vel > 0) g_app.piano.keyDown(note, static_cast<float>(vel) / 127.0f, true);
      else if (status == 0x80 || status == 0x90) g_app.piano.keyUp(note, true);
      else if (status == 0xb0 && note == 64) g_app.piano.setPedal(vel >= 64, true);  // shared, like the on-screen pedal
    } else if (c.name == "avatarStyle") {
      // From the customizer: the 17 choices as a JSON array.
      app::AvatarStyle st;
      const engine::Json j = engine::Json::parse(c.text);
      if (j.isArray()) {
        for (int k = 0; k < app::AvatarStyle::kFields && k < static_cast<int>(j.size()); ++k)
          st.v[k] = static_cast<int>(j[static_cast<size_t>(k)].number());
        applyStyle(st, true);
      }
    } else if (c.name == "gpuCheck") {  // Settings → Help Claude fix lag → Check the GPU (J107.2)
      app::startGpuCheck(engine::clockMs());
    } else if (c.name == "chessMove") {  // the full-screen board (J101, bar/web/chess.js): a = from * 64 + to
      g_app.chess.pageMove(static_cast<int>(c.a) / 64, static_cast<int>(c.a) % 64, engine::clockMs());
    } else if (c.name == "chessNew") {
      g_app.chess.pageNewGame(engine::clockMs(), static_cast<int>(c.a));  // a: 0 same, 1..5 a bot, 100 a person (J28)
    } else if (c.name == "chessFull") {
      g_app.chess.setFull(c.a != 0);
    } else if (c.name == "chessLeave") {
      if (g_app.chess.isOpen()) {
        g_app.chess.close();
        syncCapture();
      }
    } else if (c.name == "avatarPreview") {
      g_app.avatarPreview = c.a != 0;
      if (g_app.avatarPreview) {
        g_app.previewEye = g_app.camera.position;
        g_app.previewYaw = g_app.previewStartYaw = g_app.camera.yaw;
        g_app.previewAimed = false;
      } else {
        g_app.camera.position = g_app.previewEye;
        g_app.camera.yaw = g_app.previewStartYaw;
        g_app.camera.pitch = 0;
      }
      g_app.renderer.setSelfWhole(g_app.avatarPreview);
    } else if (c.name == "make") {
      g_app.bartender.make(static_cast<int>(c.a));
    } else if (c.name == "endShift") {
      g_app.bartender.endShift();
    } else if (c.name == "standUp") {
      standUp();
    } else if (c.name == "simRace") {
      startRace();
    } else if (c.name == "simPad") {
      startRace(false, true);  // the skid pad with the Ridgeback GS (the new physics)
    } else if (c.name == "carSetup") {
      g_app.race.setCarSetup(c.text);
    } else if (c.name == "meridianOpen") {
      g_app.meridianOpen = true;
      LOG_INFO("Meridian: open (the bar pauses drawing).");
    } else if (c.name == "meridianClosed") {
      g_app.meridianOpen = false;
    } else if (c.name == "fps") {
      // Draw at most N frames a second from now on (0: as many as it can), as
      // ?fps=N does from the start: a test quiets a page that's only waiting,
      // so the other page has the machine (GitHub's software drawing, KI-5).
      g_app.gpu.maxFps = std::max(0.0, c.a);
      LOG_INFO("Drawing at most %g frames a second (fps command).", g_app.gpu.maxFps);
    }
  }
  const int v = engine::settings::version();
  if (v != g_app.settingsVersion) {
    const bool first = g_app.settingsVersion < 0;
    g_app.settingsVersion = v;
    g_app.lookSpeed = static_cast<float>(engine::settings::number("lookSpeed", 1));
    g_app.invertLook = engine::settings::number("invertLook", 0) != 0 ? -1.0f : 1.0f;
    g_app.showNames = engine::settings::number("showNames", 1) != 0;
    // The crosshair (Settings → Pointer, bar/web/crosshair.js): taps use the centre.
    g_app.crosshair = engine::settings::number("crosshair", 0) != 0;
    engine::profiler().overlay = g_app.hooks.perf || engine::settings::number("perf", 0) != 0;
    const int quality = g_app.hooks.quality >= 0 ? g_app.hooks.quality
                                                 : static_cast<int>(engine::settings::number("quality", 0));
    if (first || quality != g_app.quality.mode()) {
      g_app.quality.setMode(quality, engine::clockMs());
      applyQuality(first ? nullptr : "setting");
    }
    // Power (J111.3): Cooler draws 30 a second, EXTREME never eases.
    const int power = static_cast<int>(engine::settings::number("power", 1));
    if (first || power != g_app.quality.power()) {
      g_app.quality.setPower(power, engine::clockMs());
      const double testFps = g_app.hooks.number("fps", 0);  // the tests' ?fps= wins
      g_app.gpu.maxFps = testFps > 0 ? testFps : power == engine::QualityController::kCooler ? 30 : 0;
      static const char* kPower[] = {"Cooler (30 pictures a second)", "Balanced", "EXTREME (never eases)"};
      LOG_INFO("Power: %s.", kPower[g_app.quality.power()]);
      applyQuality(first ? nullptr : "power");
    }
    g_app.mirrorSetting = g_app.hooks.mirrorQ >= 0 ? g_app.hooks.mirrorQ
                                                   : static_cast<int>(engine::settings::number("mirrorQ", 0));
    applyMirrorDetail();
    // Lighting (L1): new, or the lighting before it (docs/handbook/manual/light.md).
    const bool classic = g_app.hooks.lightClassic >= 0 ? g_app.hooks.lightClassic == 1
                                                       : engine::settings::number("lighting", 0) != 0;
    if (first || classic != g_app.renderer.classicLighting()) {
      g_app.renderer.setClassicLighting(classic);
      g_app.renderer.setTestLighting(g_app.hooks.lightTest);
      LOG_INFO("Lighting: %s.", g_app.hooks.lightTest ? "test" : classic ? "classic" : "new");
    }
    g_app.renderer.setEyes(g_app.hooks.feature("eyes"));
    g_app.renderer.setGiEnabled(g_app.hooks.feature("bounce"));
    // Lamp shadows, soft shade, light beams, bounced light's detail: the
    // quality level and their own switches (applyQuality).
    applyQuality(nullptr);
    g_app.renderer.setBeamStrength(g_app.hooks.number("haze", 1));
    g_app.renderer.setRoughFix(g_app.hooks.feature("roughfix"));
    g_app.renderer.setBounceStrength(g_app.hooks.number("bouncek", 1));
    g_app.renderer.setGiView(static_cast<int>(g_app.hooks.number("giview", 0)));
    g_app.camera.fovY = engine::radians(std::clamp(static_cast<float>(engine::settings::number("fov", 70)), 50.0f, 100.0f));
    // Your saved look, at start only (later the menu sends a "look" command).
    const int look = first ? static_cast<int>(engine::settings::number("look", g_app.look)) : g_app.look;
    if (look != g_app.look && look >= 0 && look < static_cast<int>(app::avatarLooks().size())) {
      g_app.look = look;
      g_app.renderer.setSelfAvatar(static_cast<size_t>(look));
    }
    // Or the look you made in the customizer.
    if (first && engine::settings::remembered("avCustom", 0) != 0) {
      app::AvatarStyle st;
      for (int k = 0; k < app::AvatarStyle::kFields; ++k)
        st.v[k] = static_cast<int>(engine::settings::remembered(("av" + std::to_string(k)).c_str(), st.v[k]));
      applyStyle(st, false);
    }
  }
}

// Tells the menus what's going on: the lists once (the piano's again when it
// grows), and your state whenever it changes.
void sendInfo() {
  const auto& songs = g_app.piano.songs();
  if (songs.size() != g_app.infoSongCount) {
    engine::JsonWriter w;
    w.beginObject();
    if (g_app.infoSongCount == SIZE_MAX) {
      w.key("looks").beginArray();
      for (const auto& l : app::avatarLooks()) w.value(l.name);
      w.endArray().key("avatarPalette").raw(app::avatarPaletteJson()).key("drinks").beginArray();
      for (const auto& d : app::drinkRecipes()) w.value(d.name);
      w.endArray().key("emotes").beginArray();
      for (int i = 1; i < static_cast<int>(Emote::kCount); ++i) w.value(app::emoteName(static_cast<Emote>(i)));
      w.endArray();
    }
    w.key("songs").beginArray();
    for (const auto& song : songs) w.value(song.title);
    w.endArray().endObject();
    engine::ui::setInfo(w.str());
    g_app.infoSongCount = songs.size();
  }
  engine::JsonWriter w;
  w.beginObject()
      .key("look").value(g_app.customLook ? -1 : g_app.look)
      .key("avatarStyle").beginArray();
  for (int k = 0; k < app::AvatarStyle::kFields; ++k) w.value(g_app.style.v[k]);
  w.endArray()
      .key("pianoSong").value(g_app.piano.currentSong())
      .key("shift").value(g_app.bartender.onShift() ? 1 : 0)
      .key("seated").value(g_app.seat >= 0 ? 1 : 0)
      .endObject();
  if (w.str() != g_app.infoState) {
    g_app.infoState = w.str();
    engine::ui::setInfo(g_app.infoState);
  }
}

// Lights, the fireplace, sounds and piano notes from other players.
void handleShared(std::vector<engine::net::WorldEntry>& world, std::vector<engine::net::Event>& events) {
  for (const engine::net::WorldEntry& e : world) {
    // A thing's state, decided by the room (world key t<net>, docs/archive/ENGINE.md E3).
    if (e.key.size() >= 2 && e.key[0] == 't' && std::isdigit(static_cast<unsigned char>(e.key[1])) && e.values.size() >= 2)
      g_app.world.applyShared(static_cast<uint16_t>(std::atoi(e.key.c_str() + 1)), static_cast<int>(e.values[0]), e.values[1]);
    if (e.key == "rc") g_app.race.onRoomRace(e.values);
    if (e.key == "ch") g_app.chess.onRoom(e.values);  // the lounge's chess game
    // Another player's custom avatar (world key a<slot>, bar/avatar.h AvatarStyle).
    if (e.key.size() >= 2 && e.key[0] == 'a' && std::isdigit(static_cast<unsigned char>(e.key[1])) &&
        e.values.size() >= app::AvatarStyle::kFields) {
      app::AvatarStyle st;
      for (int k = 0; k < app::AvatarStyle::kFields; ++k) st.v[k] = static_cast<int>(e.values[static_cast<size_t>(k)]);
      st.clamp();
      g_app.slotStyles[std::atoi(e.key.c_str() + 1)] = st;
    }
  }
  for (const engine::net::Event& e : events) {
    if (e.type != "sfx" || e.values.size() < 4) continue;
    const Vec3 at{static_cast<float>(e.values[1]), static_cast<float>(e.values[2]), static_cast<float>(e.values[3])};
    const int effect = static_cast<int>(e.values[0]);
    if (effect < 0) strum(at, false);
    else if (effect < static_cast<int>(engine::synth::Effect::kCount)) playSfx(static_cast<engine::synth::Effect>(effect), at);
  }
}

void updateArcadeMusic(float dt);

// Works out what you're standing at, handles taps, and keeps the music and
// your "ears" in sync with the camera.
void updateInteraction(double nowMs, float dt) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const auto& items = g_app.scene.interactables;

  // What's in front of you: small things and seats only when looked at
  // directly, big ones when you stand at them facing them.
  g_app.focused = -1;
  g_app.focusedSeat = -1;
  if (g_app.seat < 0 && !g_app.darts.playing()) {
    float bestFacing = 0.8f, directDist = kDirectReach;
    int direct = -1;
    for (size_t i = 0; i < items.size(); ++i) {
      if (!available(items[i])) continue;
      if (items[i].direct) {
        const float t = rayBoxDistance(cam.position, cam.forward(), items[i].boundsMin, items[i].boundsMax);
        if (t >= 0 && t < directDist) {
          directDist = t;
          direct = static_cast<int>(i);
        }
        continue;
      }
      const Vec3 center = (items[i].boundsMin + items[i].boundsMax) * 0.5f;
      const Vec3 toFlat{center.x - cam.position.x, 0, center.z - cam.position.z};
      const float facing = engine::dot(engine::normalize(toFlat), cam.flatForward());
      if (engine::length(toFlat) < kPromptDistance && facing > bestFacing) {
        bestFacing = facing;
        g_app.focused = static_cast<int>(i);
      }
    }
    float seatDist;
    const int seat = seatHit(cam.forward(), kDirectReach, &seatDist);
    if (direct >= 0 && (seat < 0 || directDist < seatDist)) g_app.focused = direct;
    else if (seat >= 0) {
      g_app.focusedSeat = seat;
      if (g_app.focused >= 0 && !items[static_cast<size_t>(g_app.focused)].direct) g_app.focused = -1;
    }
  }
  if (g_app.focused != g_app.highlighted) {
    setGlow(g_app.highlighted, 0);
    setGlow(g_app.focused, kHighlightGlow);
    g_app.highlighted = g_app.focused;
  }
  const int seatModel = g_app.focusedSeat >= 0 ? g_app.scene.seats[static_cast<size_t>(g_app.focusedSeat)].model : -1;
  if (seatModel != g_app.highlightedSeatModel) {
    if (g_app.highlightedSeatModel >= 0) g_app.renderer.setModelBaseGlow(static_cast<size_t>(g_app.highlightedSeatModel), 0);
    if (seatModel >= 0) g_app.renderer.setModelBaseGlow(static_cast<size_t>(seatModel), kHighlightGlow);
    g_app.highlightedSeatModel = seatModel;
  }

  // Something you could pick up, right where you're looking?
  g_app.aimed = -1;
  if (g_app.held < 0 && g_app.seat < 0 && !g_app.darts.playing()) {
    float dist;
    const int body = g_app.physics.raycastBodies(cam.position, cam.forward(), kGrabReach, &dist);
    float wallDist;
    if (body >= 0 && !(g_app.physics.raycastStatic(cam.position, cam.forward(), dist, &wallDist, nullptr))) {
      g_app.aimed = body;
    }
  }

  const float W = static_cast<float>(g_app.gpu.width), H = static_cast<float>(g_app.gpu.height);
  {
    // The chessboard: taps while playing (presses, the game captures input);
    // the pieces move for everyone else too.
    std::vector<engine::Vec2> chessTaps;
    if (g_app.chess.isOpen()) {
      for (const engine::RawPress& p : g_app.input.frame().presses) {
        if (p.down && p.key < 0) chessTaps.push_back(p.pos);
        if (p.down && p.key == 4) chessTaps.push_back({0.2f, 0.92f});  // Escape: the leave button
      }
    }
    if (g_app.chess.update(nowMs, dt, chessTaps, g_app.camera, W / std::max(H, 1.0f))) {
      g_app.chess.close();
      syncCapture();
    }
  }
  if (g_app.rhythm.isOpen()) {
    g_app.rhythm.framesDrawn = g_app.gpu.framesDrawn();  // TEST's frames drawn a second (J118)
    g_app.rhythm.update(nowMs, g_app.input.frame().presses, W, H);
    if (!g_app.rhythm.isOpen()) {
      syncCapture();
      setActivity(Activity::kNone);
    }
  } else if (g_app.race.pending()) {
    if (g_app.race.tryPending(nowMs)) syncCapture();
  } else if (g_app.race.open()) {
    if (g_app.race.update(nowMs, dt, g_app.input.frame())) standUp();
  } else if (g_app.piano.keyboardOpen()) {
    if (g_app.piano.updateKeyboard(g_app.input.frame(), W, H, g_app.gpu.pixelRatio)) standUp();
    if (g_app.piano.takeSongsRequest()) engine::ui::openPanel("piano");
  }
  g_app.darts.update(nowMs);

  // A tap uses what it lands on; tapping anywhere also works while you're
  // standing at something (easier on a small screen).
  // (A copy: using something can switch input modes, which empties the frame's taps.)
  const std::vector<engine::Vec2> taps =
      g_app.avatarPreview ? std::vector<engine::Vec2>{} : g_app.input.frame().taps;  // the customizer: drags turn you
  for (const engine::Vec2 tap : taps) {
    if (g_app.entered) g_app.introEndMs = std::min(g_app.introEndMs, nowMs + kIntroAfterInputMs);
    if (g_app.jukeboxMenu.isOpen()) {
      g_app.jukeboxMenu.tap(tap, g_app.jukebox);
      continue;
    }
    if (g_app.bartender.tap(tap, W, H, nowMs, cam.position)) continue;
    // With the crosshair on, a tap anywhere uses what the centre points at.
    const Vec3 dir = screenRay(g_app.crosshair ? engine::Vec2{0.5f, 0.5f} : tap);
    if (g_app.darts.playing()) {
      g_app.darts.throwAt(cam.position, dir, nowMs);
      g_app.act.restart(nowMs);  // the throw animation
      continue;
    }
    if (g_app.held >= 0) {
      putDownOrThrow(dir);
      continue;
    }
    // Everything the tap hits; the nearest wins (bar/interaction.h has the
    // rules: pick-ups first, players only when clearly in front).
    std::vector<app::TapTarget> hits;
    const bool standing = g_app.seat < 0;
    float d;
    if (standing) {
      if (const int body = pickableAt(dir, &d); body >= 0) hits.push_back({app::TargetKind::kProp, body, d});
    }
    if (const int person = personAt(dir, &d); person >= 0) hits.push_back({app::TargetKind::kPlayer, person, d});
    int hit = -1;
    if (standing) {
      float nearest = kTapReach;
      for (size_t i = 0; i < items.size(); ++i) {
        if (!available(items[i])) continue;
        const float t = rayBoxDistance(cam.position, dir, items[i].boundsMin, items[i].boundsMax);
        if (t >= 0 && t < nearest && (!items[i].direct || t < kDirectReach)) {
          nearest = t;
          hit = static_cast<int>(i);
        }
      }
      if (hit >= 0) hits.push_back({app::TargetKind::kThing, hit, nearest});
      if (const int seat = seatHit(dir, kDirectReach, &d); seat >= 0) hits.push_back({app::TargetKind::kSeat, seat, d});
    }
    if (const int thing = touchAt(dir, &d); thing >= 0) hits.push_back({app::TargetKind::kTouch, thing, d});
    const app::TapTarget target = app::pickTap(hits);
    if (target.kind == app::TargetKind::kTouch) {
      if (g_app.world.touch(target.index)) {
        // Its own sound (a lamp bongs, a table knocks: "touchSound", E6.2), else a click.
        int effect = static_cast<int>(engine::synth::Effect::kClick);
        float gain = 0.35f;
        g_app.world.touchSound(target.index, &effect, &gain);
        playSfx(static_cast<engine::synth::Effect>(effect), g_app.world.where(target.index), gain);
      }
      continue;
    }
    if (target.kind == app::TargetKind::kProp) {
      pickUp(target.index);
      continue;
    }
    if (target.kind == app::TargetKind::kPlayer) {
      const app::OtherPlayer& o = g_app.others.players()[static_cast<size_t>(target.index)];
      engine::net::showCard(o.account, o.name);
      continue;
    }
    if (target.kind == app::TargetKind::kSeat) {
      sitOn(target.index);
      continue;
    }
    hit = target.kind == app::TargetKind::kThing ? target.index : -1;
    if (hit < 0 && g_app.focused >= 0 && !items[static_cast<size_t>(g_app.focused)].direct) hit = g_app.focused;
    if (hit < 0 && g_app.focused >= 0 && items[static_cast<size_t>(g_app.focused)].direct) hit = g_app.focused;
    if (hit >= 0 && g_app.seat < 0) {
      use(items[static_cast<size_t>(hit)]);
      continue;
    }
    if (g_app.bartender.held()) g_app.bartender.sip(nowMs);
  }

  updateMirrorButtons();
  g_app.jukebox.update();
  if (g_app.jukebox.songsStarted() != g_app.bannerSong) {
    g_app.bannerSong = g_app.jukebox.songsStarted();
    g_app.bannerEndMs = nowMs + kBannerMs;
  }
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  engine::audio::setListener(cam.position, forward, engine::cross(right, forward));
  updateArcadeMusic(dt);
}


// The numbers others get about you (players.h slot order after kPitch).
void selfState(float extra[kCount - app::slot::kActivity]) {
  using namespace app::slot;
  auto e = [&](int s) -> float& { return extra[s - kActivity]; };  // index by slot
  const app::AvatarMotion& m = g_app.avatarMotion;
  e(kActivity) = static_cast<float>(static_cast<int>(m.activity));
  e(kParam) = g_app.seat >= 0 ? static_cast<float>(g_app.seat) : 0;
  e(kLook) = g_app.customLook ? static_cast<float>(app::kCustomLook) : static_cast<float>(g_app.look);
  g_app.voiceLevel = engine::bridge::frame().myVoiceLevel;
  e(kVoice) = std::round(g_app.voiceLevel * 20) / 20;
  e(kEmote) = static_cast<float>(static_cast<int>(g_app.emote));
  e(kEmoteCount) = static_cast<float>(g_app.emoteCount);
  float data[5] = {};
  if (g_app.race.open()) g_app.race.stateFor(data);
  else if (g_app.rhythm.isOpen()) g_app.rhythm.screenState(data);
  else if (g_app.darts.playing()) g_app.darts.stateFor(data);
  for (int i = 0; i < 5; ++i) e(kData + i) = data[i];
  e(kHeldDrink) = static_cast<float>(g_app.bartender.held());
  e(kFlags) = static_cast<float>((g_app.bartender.onShift() ? 1 : 0) | (g_app.onMic ? 2 : 0));
  e(kPose) = static_cast<float>(static_cast<int>(m.pose));
}


// Talks to the voice chat (bar/web/voice.js): where each player's voice comes
// from, who's on the mic.
// Where each other player's voice comes from: one batch, 20 times a second,
// with the frame's mailbox flush.
void updateVoice(double nowMs) {
  if (nowMs - g_app.lastVoicePlaceMs < 50 || g_app.others.players().empty()) return;
  g_app.lastVoicePlaceMs = nowMs;
  std::string list;
  for (const app::OtherPlayer& o : g_app.others.players()) {
    const int flags = static_cast<int>(o.values[app::slot::kFlags]);
    char item[96];
    std::snprintf(item, sizeof(item), "%s[%d,%.2f,%.2f,%.2f,%d]", list.empty() ? "" : ",", o.id, o.eye.x,
                  o.eye.y - 0.05f, o.eye.z, (flags & 2) ? 1 : 0);
    list += item;
  }
  engine::bridge::op("[\"voicePlace\",[" + list + "]]");
}

// Keeps the loading screen (bar/web/index.html) up to date until every model
// has arrived; then it offers "Tap to enter".
// Test options (?sit=, ?rhythm=, ...), applied once the bar is ready, as if
// you had done them yourself.
void applyTestHooks() {
  const app::TestHooks& h = g_app.hooks;
  g_app.rhythm.autoplay = h.autoplay;
  if (h.rhythmSong >= 0 || h.rhythmSong == -2) {
    std::vector<app::RhythmGame::SongChoice> songs;
    for (const app::Song& s : g_app.jukebox.songs()) songs.push_back({s.title, "", s.url});
    g_app.rhythm.open(std::move(songs));
    syncCapture();
    if (h.rhythmSong >= 0) g_app.rhythm.start(h.rhythmSong, static_cast<app::Difficulty>(std::clamp(h.rhythmDifficulty, 0, app::kDifficultyCount - 1)));
    setActivity(Activity::kRhythm);
  }
  app::SeatKind want;
  if (!h.sitKind.empty() && app::seatKindFromName(h.sitKind, &want)) {
    for (size_t i = 0; i < g_app.scene.seats.size(); ++i)
      if (g_app.scene.seats[i].kind == want) {
        g_app.hooks.sit = static_cast<int>(i);
        break;
      }
  }
  if (h.sit >= 0 && h.sit < static_cast<int>(g_app.scene.seats.size())) sitOn(h.sit);
  if (h.chess && g_app.chess.open(g_app.others.online()).empty()) syncCapture();
  if (h.emote > 0 && h.emote < static_cast<int>(Emote::kCount)) startEmote(static_cast<Emote>(h.emote));
  if (h.pianoSong >= 0) g_app.piano.playSong(h.pianoSong);
  for (const app::Interactable& it : g_app.scene.interactables) {
    if ((h.shift && it.action == app::Action::kBartender) || (h.darts && it.action == app::Action::kDarts)) use(it);
  }
}

void updateLoadingScreen() {
  const double now = engine::clockMs();
  if (!g_app.pipelinesLogged && engine::pipelines::created() > 0 && engine::pipelines::pending() == 0) {
    g_app.pipelinesLogged = true;
    LOG_INFO("Pipelines ready in %.0f ms (%d).", now - engine::pipelines::firstStartMs(), engine::pipelines::created());
  }
  if (!g_app.racePipelinesLogged && g_app.race.pipelinesReady()) {
    g_app.racePipelinesLogged = true;
    LOG_INFO("Race pipelines ready.");
  }
  const app::SceneRenderer& r = g_app.renderer;
  const bool wasLoading = !g_app.loader.filesDone();
  app::LoaderInputs in;
  in.files = r.modelFiles();
  in.filesDone = r.modelFilesDone();
  in.filesFailed = r.modelFilesFailed();
  in.criticalFailed = r.criticalFileFailed();
  in.progress = r.loadProgress();
  in.jobsPending = engine::pendingJobs();
  in.pipelinesPending = engine::pipelines::pending() + (r.ready() ? 0 : 1);
  in.warmUpDone = g_app.warmGpuDone;
  if (g_app.loader.update(in, now)) applyTestHooks();
  if (wasLoading && g_app.loader.filesDone()) {
    LOG_INFO("Textures: %d images on the GPU, %.0f MB.", engine::imageTextureCount(), engine::imageTextureMB());
  }
  // Reflections (E4.4): once every model and texture is in, the bar is
  // captured into a cube from the middle of the room.
  if (!g_app.reflectionsAsked && !g_app.hooks.noReflect && g_app.loader.filesDone() && engine::pendingJobs() == 0 && !g_app.scene.rooms.empty()) {
    g_app.reflectionsAsked = true;
    const engine::Aabb& room = g_app.scene.rooms[0];  // the first room (the bar; a home)
    const engine::Vec3 middle = g_app.scene.place == "bar" ? engine::Vec3{-1.0f, 1.8f, -1.0f}
                                                            : engine::Vec3{(room.min.x + room.max.x) / 2, 1.8f, (room.min.z + room.max.z) / 2};
    g_app.renderer.captureReflections(middle, room);
    g_app.renderer.wantGi();  // bounced light (L1.5)
  }
}

// Closes the profiler's frame on every way out of frame().
// Background jobs run last, after the frame is submitted, until a deadline:
// a few ms in the bar, more behind the loading screen.
struct ProfiledFrame {
  explicit ProfiledFrame(double nowMs) { engine::profiler().beginFrame(nowMs); }
  ~ProfiledFrame() {
    {
      PROFILE(engine::Phase::kJobs);
      engine::runJobs(engine::clockMs() + (g_app.entered ? 3.0 : 8.0));
    }
    engine::bridge::flush();  // everything for the page from this frame, in one call
    engine::profiler().endFrame(engine::clockMs());
  }
};

// ---- The frame, phase by phase ----
// Each frame runs these in order (docs/archive/RESTRUCTURE.md §2.1): input, what you
// do, physics, what the room sent, the games, the room's claims, what you
// send, then drawing. Each phase is its own function so the order is plain.

struct FrameIn {
  double nowMs;
  float dt;
  std::vector<engine::net::WorldEntry> world;  // what the room changed
  std::vector<engine::net::Event> events;      // ...and what happened in it
};

void phaseInput(FrameIn& f) {
  PROFILE(engine::Phase::kInput);
  handleCommands();
  if (studioOn()) {  // the Studio's camera and picking instead of you
    studioInput(f.dt);
    return;
  }
  updateCamera(f.dt);
  g_app.input.flushStick();
}

void phaseInteraction(FrameIn& f) {
  PROFILE(engine::Phase::kInteraction);
  if (studioOn()) return;
  updateInteraction(f.nowMs, f.dt);
  syncCapture();  // (also after anything that ended a game on its own)
}

void phasePhysics(FrameIn& f) {
  PROFILE(engine::Phase::kPhysics);
  updatePhysics(f.dt);
}

void phaseNetIn(FrameIn& f) {
  PROFILE(engine::Phase::kNet);
  f.world = g_app.shared.takeOther();
  if (g_app.others.online()) engine::net::receiveEvents(f.events);
  handleShared(f.world, f.events);
  // Your customizer look, for the others (once per room slot you get).
  const int slot = g_app.others.online() ? engine::net::mySlot() : -1;
  if (g_app.customLook && slot >= 0 && slot != g_app.styleSentSlot) {
    double v[app::AvatarStyle::kFields];
    for (int k = 0; k < app::AvatarStyle::kFields; ++k) v[k] = g_app.style.v[k];
    engine::net::setWorld("a" + std::to_string(slot), v, app::AvatarStyle::kFields);
    g_app.styleSentSlot = slot;
  }
}

// While 4K Mania is open (J117.4) it draws only its own layer, so: one frame
// queued on the GPU (a tap shows on the next frame, not two later) and no
// 30-a-second cap from Power → Cooler, so it can keep up with a 120 Hz screen.
void maniaFramePacing() {
  static bool was = false;
  const bool open = g_app.rhythm.isOpen();
  if (open == was) return;
  was = open;
  const double testFps = g_app.hooks.number("fps", 0);  // the tests' ?fps= wins
  g_app.gpu.maxFramesInFlight = open ? 1 : 2;
  g_app.gpu.maxFps = testFps > 0 ? testFps
                     : !open && g_app.quality.power() == engine::QualityController::kCooler ? 30 : 0;
  LOG_INFO("4K Mania: %s.", open ? "one frame queued, no frame cap" : "frames back to the bar's pacing");
}

void phaseGames(FrameIn& f) {
  PROFILE(engine::Phase::kGames);
  g_app.piano.update(f.nowMs, f.world, f.events);
  g_app.bartender.update(f.nowMs, f.world);
  maniaFramePacing();
}

// The room's answers about seats and machines: if it gave them to someone
// else, you stand up or leave (before your state is sent, so the others see
// you leave at once).
void phaseClaims(FrameIn&) {
  // The room said this seat is someone else's: stand up. (After a reconnect
  // that lost your place in the room, the seat is asked for again.)
  if (g_app.seat >= 0 && g_app.others.online()) {
    const int answer = engine::net::claimResult(g_app.seatClaim);
    if (answer >= 0) g_app.seatClaim = 0;
    const int owner = engine::net::claimOwner(seatKey(g_app.seat));
    if (owner > 0 || answer == 0) {
      const std::string who = owner > 0 ? engine::net::playerName(owner) : std::string("Someone");
      showToast(who + " sat there first.");
      LOG_INFO("Seat %d: %s sat there first.", g_app.seat, who.c_str());
      standUp();
    } else if (owner < 0 && g_app.seatClaim == 0 && engine::net::status() == engine::net::Status::kConnected) {
      g_app.seatClaim = engine::net::claim(seatKey(g_app.seat));
    }
  }

  // Started a machine before joining the room (or after a reconnect that
  // lost your place): ask for it now.
  if (g_app.machineKey.empty() && g_app.others.online() && engine::net::status() == engine::net::Status::kConnected &&
      (g_app.darts.playing() || g_app.rhythm.isOpen())) {
    g_app.machineKey = g_app.darts.playing() ? "machine.darts" : "machine.rhythm";
    g_app.machineClaim = engine::net::claim(g_app.machineKey);
  }
  // Darts are scored by the room while it has given you the board.
  g_app.darts.setOnline(g_app.others.online() && g_app.machineKey == "machine.darts");
  // The room said the machine you're on is someone else's: leave it.
  if (!g_app.machineKey.empty() && g_app.others.online()) {
    const int answer = engine::net::claimResult(g_app.machineClaim);
    if (answer >= 0) g_app.machineClaim = 0;
    const int owner = engine::net::claimOwner(g_app.machineKey);
    if (owner > 0 || answer == 0) {
      const std::string who = owner > 0 ? engine::net::playerName(owner) : std::string("Someone");
      LOG_INFO("Machine %s: %s is using it (got there first).", g_app.machineKey.c_str(), who.c_str());
      showToast(who + " got there first.");
      g_app.machineKey.clear();  // not ours: nothing to release
      if (g_app.darts.playing()) g_app.darts.stop();
      if (g_app.rhythm.isOpen()) {
        g_app.rhythm.close();
        syncCapture();
      }
      setActivity(Activity::kNone);
    }
  }

}

void phaseNetOut(FrameIn& f) {
  PROFILE(engine::Phase::kNet);
  updateSelfMotion(f.nowMs);
  float extra[kCount - app::slot::kActivity] = {};
  selfState(extra);
  g_app.others.sendSelf(f.nowMs, g_app.camera.position, g_app.camera.yaw, g_app.camera.pitch, extra);
  g_app.others.update(f.nowMs, f.dt, kWalkSpeed);
  updateVoice(f.nowMs);
  sendInfo();
}

// Drawing: the race, the text layer alone (loading, 4K Mania), or the bar.
// One frame's drawing into `target` (the bar, the race, or only the text).
void drawFrame(const wgpu::TextureView& target, double nowMs) {
  using engine::Phase;
  if (g_app.race.open()) {  // you dove into the race: it's the whole screen
    PROFILE(Phase::kRace);
    queueText(nowMs, Mat4{});
    g_app.race.render(target, g_app.text, nowMs);
    engine::profiler().trackGpu(g_app.gpu.queue);
    return;
  }
  // Scene prep is split around queueText, so it's timed by hand.
  double sceneStart = engine::clockMs();

  // Hidden bar: behind the loading screen (except one warm-up frame once
  // everything is in, so the GPU has seen every pipeline before you enter)
  // and under 4K Mania. Draw only the text layer.
  const bool warmUp = !g_app.entered && g_app.loader.wantsWarmUpFrame() && g_app.renderer.ready();
  if ((!g_app.entered && !warmUp) || g_app.rhythm.isOpen() || !g_app.renderer.ready()) {
    PROFILE(Phase::kText);
    queueText(nowMs, Mat4{});
    g_app.renderer.renderOverlay(target, g_app.text);
    engine::profiler().trackGpu(g_app.gpu.queue);
    return;
  }
  if (warmUp) {
    g_app.loader.warmUpFrameDrawn();
    g_app.warmUpFrame = true;  // reported to the GPU queue after this frame's submit (below)
  }

  const engine::FirstPersonCamera& cam = g_app.camera;
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  const Mat4 viewProj = cam.projection(aspect) * cam.view();

  presentScene(nowMs);
  engine::profiler().add(Phase::kScene, engine::clockMs() - sceneStart);
  {
    PROFILE(Phase::kText);
    queueText(nowMs, viewProj);  // after the boxes: others' darts are added there
  }
  sceneStart = engine::clockMs();
  presentAvatars(nowMs);
  Mat4 self = app::avatarTransform(cam.position, cam.yaw, cam.pitch);
  if (g_app.avatarPreview) self = app::avatarTransform(g_app.previewEye, g_app.previewYaw + g_app.previewSpin, 0);
  if (g_app.seat >= 0) {
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
    self = app::seatedTransform(s.hip, s.yaw);
  }
  if (studioOn()) self = Mat4::translation({0, -500, 0});  // the Studio has no body
  engine::profiler().add(Phase::kScene, engine::clockMs() - sceneStart);
  PROFILE(Phase::kRender);
  g_app.renderer.render(target, viewProj, cam.position, self, g_app.avatarBones, g_app.firstPersonBones,
                        g_app.scene.mirrors, g_app.text, g_app.pictures);
  engine::profiler().trackGpu(g_app.gpu.queue);
  if (g_app.warmUpFrame) {
    g_app.warmUpFrame = false;
    g_app.gpu.queue.OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous,
                                        [](wgpu::QueueWorkDoneStatus, wgpu::StringView) { g_app.warmGpuDone = true; });
  }
}

void phaseRender(FrameIn& f) {
  // Meridian has the screen: the bar keeps its connection but doesn't draw.
  if (g_app.meridianOpen) return;
  // The page is hidden (another app, another tab): nothing to show, so
  // nothing drawn; the logic and the connection go on. (A phone stops the
  // frames then anyway; a test's hidden page keeps them, KI-5.)
  if (engine::bridge::frame().hidden) return;
  // The GPU still busy with the frames before (or ?fps= says not yet): the
  // logic above ran; this frame draws nothing (looped/gpu.h mayDraw).
  if (!g_app.gpu.mayDraw(f.nowMs)) return;
  wgpu::TextureView target;
  {
    PROFILE(engine::Phase::kAcquire);
    target = g_app.gpu.beginFrame();
  }
  if (!target) return;
  drawFrame(target, f.nowMs);
  g_app.gpu.endFrame();
  engine::profiler().drew();
}

// Tuning the look by hand from the page's address (docs/handbook/manual/light.md):
// ?exposure= ?eyekeep= ?eyeref= ?eyemin= ?eyemax= ?whitek= ?white= override the
// place's mood, so numbers can be tried on the phone without a build.
void tuneMoodFromPage() {
  engine::Mood m = g_app.renderer.mood();
  const app::TestHooks& h = g_app.hooks;
  m.exposureComp = h.number("exposure", m.exposureComp);
  m.eyeKeep = h.number("eyekeep", m.eyeKeep);
  m.eyeReference = h.number("eyeref", m.eyeReference);
  m.eyeDarkest = h.number("eyemin", m.eyeDarkest);
  m.eyeBrightest = h.number("eyemax", m.eyeBrightest);
  m.whiteKelvin = h.number("whitek", m.whiteKelvin);
  m.whiteAmount = h.number("white", m.whiteAmount);
  m.highlightRoom = h.number("hiroom", m.highlightRoom);
  m.highlightColour = h.number("hicolour", m.highlightColour);
  g_app.renderer.setMood(m);
  g_app.renderer.setLightUnits(h.number("emissive", 8), h.number("ambient", 14));
}

// Picture quality (looped/quality.h): the level (Ultra, High, Medium, Low)
// sets every feature together (J107, the user's request 2026-10-26 #13); on
// Auto the last frames' times move it between High and Low. A feature's own
// switch (Settings → Graphics, or the page option) can still turn it off.
//
//                     Ultra  High  Medium  Low
//   pixels per CSS px   3     2     1.6    1.3   (looped/quality.h; Ultra eases to
//                                                2.5 and 2 while frames run late, J110.3)
//   lamps per pixel    16    12     10      8
//   lamp shadows       on    on     on      on   (drawn once; without them the
//                                                hanging lamps' shades darken the floor: KI-46)
//   soft shade         on    on     on     off
//   light beams       24    16     10     off   (steps along each line of sight)
//   bounced light     full  full   full   lite  (fewer lamps traced)
//   mirror            full  full  simple simple (Settings → Mirror can make it simple)
void applyQuality(const char* why) {
  static const char* kNames[] = {"Ultra", "High", "Medium", "Low"};
  static const int kLightsPerPixel[] = {16, 12, 10, 8};
  static const int kBeamSteps[] = {24, 16, 10, 0};
  const int level = g_app.quality.level();
  const double cap = g_app.quality.pixelRatioCap();
  const bool low = level == engine::QualityController::kLevelLow;
  // Ultra eases its lamps last, while pictures still fall behind (J111.2).
  const int lamps = level == engine::QualityController::kLevelUltra ? g_app.quality.ultraLamps() : kLightsPerPixel[level];
  static int lastLamps = -1;
  const bool lampsChanged = lamps != lastLamps;
  lastLamps = lamps;
  g_app.renderer.setLightsPerPixel(lamps);
  g_app.renderer.setBeamSteps(std::max(kBeamSteps[level], 4));
  // Lamp shadows (J97): ?shadows=0, Settings → Graphics → Lamp shadows.
  g_app.renderer.setLampShadows(g_app.hooks.feature("shadows") &&
                                engine::settings::number("lampShadows", 0) == 0);
  // Soft shade (J91): ?ao=0, Settings → Graphics → Soft shade.
  g_app.renderer.setSoftShade(!low && g_app.hooks.feature("ao") && engine::settings::number("softShade", 0) == 0);
  // Light beams (J91.2): ?beams=0, Settings → Graphics → Light beams.
  g_app.renderer.setLightBeams(kBeamSteps[level] > 0 && g_app.hooks.feature("beams") &&
                               engine::settings::number("lightBeams", 0) == 0);
  g_app.renderer.setGiLite(low || g_app.hooks.number("gilite", 0) != 0);
  if (cap == g_app.gpu.resolutionCap && !lampsChanged) return;
  g_app.gpu.resolutionCap = cap;
  if (why) LOG_INFO("Quality: %s (%.1fx), %s; %d lamps a pixel.", kNames[level], cap, why, lamps);
}
// The mirror's detail: the setting, or simple at Medium and Low.
void applyMirrorDetail() {
  const bool simple = g_app.quality.level() >= engine::QualityController::kLevelMedium;  // Medium and Low (J107)
  const int detail = std::max(g_app.mirrorSetting, simple ? 1 : 0);
  if (detail == g_app.renderer.mirrorDetail()) return;
  g_app.renderer.setMirrorDetail(detail);
  LOG_INFO("Mirror detail: %s.", detail ? "simple" : "full");
}
void watchQuality(double nowMs) {
  const engine::Profiler& p = engine::profiler();
  if (p.frameCount() == 0) return;
  const engine::Profiler::Frame& last = p.frame(0);
  const bool drawing = g_app.entered && !g_app.meridianOpen && !engine::frameTime().resumed &&
                       !engine::bridge::frame().modalOpen;  // a menu over the bar isn't the bar's cost
  const int before = g_app.quality.ultraStep() * 10 + g_app.quality.tier();
  if (g_app.quality.frame(nowMs, last.intervalMs, last.cpuMs, last.gpuMs, drawing, last.drawn)) {
    const bool eased = g_app.quality.ultraStep() * 10 + g_app.quality.tier() > before;
    applyQuality(eased ? "the pictures fell behind" : "steady again");
    applyMirrorDetail();
  }
}

using FramePhase = void (*)(FrameIn&);
constexpr FramePhase kPhases[] = {phaseInput, phaseInteraction, phasePhysics, phaseNetIn,
                             phaseGames, phaseClaims,      phaseNetOut,  phaseRender};

void frame(double rafMs) {
  // One time for the whole frame: the browser's frame time. The step is
  // clamped, so coming back from another app doesn't teleport anything.
  engine::beginFrameTime(rafMs);
  const engine::FrameTime& ft = engine::frameTime();
  // Touches queued since the last frame are read first, so the profiler
  // counts them for the frame they arrived in.
  engine::bridge::pull();  // everything the page collected since last frame, in one call
  g_app.input.setCanvasSize(g_app.gpu.cssWidth, g_app.gpu.cssHeight);
  g_app.input.poll();  // what the player did since last frame (read-only from here on)
  ProfiledFrame profiled(engine::clockMs());
  // Under a menu the page's own drawing costs more: not counted (J110.4).
  engine::profiler().setCovered(engine::bridge::frame().modalOpen);
  // Back from another app (a long gap between frames). A slow machine has
  // long gaps all the time (software drawing: about one a second), so at most
  // one line every 30 s: it must not push everything else out of the log.
  static double lastBackLogMs = -1e9;
  if (ft.resumed && ft.gapMs > 1000 && ft.nowMs - lastBackLogMs > 30000) {
    LOG_INFO("Back after %.1f s away.", ft.gapMs / 1000.0);
    lastBackLogMs = ft.nowMs;
  }
  updateLoadingScreen();
  // The GPU check (J107.2) steps through its features; Auto waits meanwhile.
  if (app::gpuCheckRunning()) app::updateGpuCheck(ft.nowMs);
  else watchQuality(ft.nowMs);
  g_app.gpu.resizeIfNeeded();
  updateFps(ft.nowMs);
  FrameIn f{ft.nowMs, ft.dt, {}, {}};
  for (FramePhase phase : kPhases) phase(f);
}

// For testing: "?cam=x,y,z,yaw,pitch" in the page URL starts the camera
// there (yaw/pitch in degrees), so any spot can be screenshotted.
void applyTestCamera() {
  if (!g_app.hooks.hasCam) return;
  const float* c = g_app.hooks.cam;
  const float x = c[0], y = c[1], z = c[2], yaw = c[3], pitch = c[4];
  g_app.camera.position = {x, y, z};
  g_app.camera.yaw = engine::radians(yaw);
  g_app.camera.pitch = engine::radians(pitch);
  LOG_INFO("Test camera at %.2f, %.2f, %.2f.", x, y, z);
}

// Sounds made in code: effects, guitar strings.
void makeSounds() {
  using engine::synth::Effect;
  for (int e = 0; e < static_cast<int>(Effect::kCount); ++e) {
    g_app.sfx[e].create(engine::synth::effect(static_cast<Effect>(e)), engine::synth::kRate);
  }
  const float strings[6] = {82.4f, 123.5f, 164.8f, 207.7f, 246.9f, 329.6f};  // an E major chord
  for (int s = 0; s < 6; ++s) g_app.guitar[s].create(engine::synth::pluck(strings[s], 2.5f, 0.6f), engine::synth::kRate);
  // The arcade's music takes a moment to make: in the background.
  engine::queueJob([] { g_app.arcadeMusic.create(engine::synth::arcadeLoop(), engine::synth::kRate); },
                   engine::JobLane::kNormal, "arcade-music");
}

// The arcade's music: normal inside the arcade, leaking out of its doorway
// (louder the closer you are to it), silent in the rest of the bar.
void updateArcadeMusic(float dt) {
  if (!g_app.arcadeMusic.valid()) return;
  const app::BarScene& sc = g_app.scene;
  const Vec3 p = g_app.camera.position;
  float target = 0;
  if (p.x > sc.arcadeMin.x && p.x < sc.arcadeMax.x && p.z > sc.arcadeMin.z && p.z < sc.arcadeMax.z) {
    target = 1;  // inside
  } else {
    const Vec3 d = p - sc.arcadeDoor;
    const float dist = std::sqrt(d.x * d.x + d.z * d.z);
    if (p.z < sc.arcadeDoor.z) target = 0.85f;                                  // in the doorway
    else target = 0.5f * std::pow(engine::clampf(1 - dist / 6.0f, 0, 1), 2.0f);  // in the bar, near the door
  }
  if (g_app.meridianOpen || g_app.rhythm.isOpen()) target = 0;  // playing a machine: only its own sound
  g_app.arcadeGain += (target - g_app.arcadeGain) * (1 - std::exp(-dt / 0.35f));
  constexpr float kLevel = 0.55f;
  if (g_app.arcadeGain > 0.002f) {
    if (!g_app.arcadeVoice) {
      g_app.arcadeVoice = g_app.arcadeMusic.play(0.0f, nullptr, 0, 1, engine::Sound::kEffects, true);
      if (g_app.arcadeVoice) LOG_INFO("Arcade music: on.");
    }
    if (g_app.arcadeVoice) engine::Sound::set(g_app.arcadeVoice, g_app.arcadeGain * kLevel, 1);
  } else if (g_app.arcadeVoice && target == 0) {
    engine::Sound::stop(g_app.arcadeVoice, 0, 0.2f);
    g_app.arcadeVoice = 0;
  }
}

void startScene();

void onGpuReady(bool ok) {
  // The activity rules: who else uses a seat or machine, and what leaving one does.
  g_app.act.usedBy = [](Activity kind, int param) {
    const app::OtherPlayer* o = playerUsing(kind, param);
    return o ? o->name : std::string();
  };
  g_app.act.onLeave = [](Activity ended) {
    // Leaving darts or 4K Mania frees the machine for the others.
    if (!g_app.machineKey.empty() && (ended == Activity::kDarts || ended == Activity::kRhythm)) {
      engine::net::release(g_app.machineKey);
      g_app.machineKey.clear();
      g_app.machineClaim = 0;
    }
  };
  if (!ok) {
    engine::setStatus("Startup failed - see log");
    LOG_FATAL("Startup failed: no GPU.");
    return;
  }
  engine::profiler().stage("gpu");
  g_app.hooks = app::TestHooks::parse();
  // ?fps=N: draw at most N frames a second (GitHub's tests, which draw in
  // software: the GPU stays free for their pictures and a second page).
  g_app.gpu.maxFps = g_app.hooks.number("fps", 0);
  if (g_app.gpu.maxFps > 0) LOG_INFO("Drawing at most %.0f frames a second (?fps).", g_app.gpu.maxFps);
  engine::prepareMipmaps(g_app.gpu.device);  // texture jobs wait for these
  // The world the Studio published (asked for in main()), then the rest.
  app::whenLiveWorld(startScene, 5000);
}

void startScene() {
  g_app.scene = app::buildBarScene(g_app.hooks.place);  // the bar, or another place (E7)
  g_app.world.load(g_app.scene);
  app::buildPhysics(g_app.scene, g_app.physics, g_app.bodyOfModel);
  g_app.chess.init(g_app.scene);
  engine::profiler().stage("scene");
  std::vector<engine::SkinnedMeshData> meshes;
  for (const app::AvatarLook& look : app::avatarLooks()) {
    g_app.avatars.emplace_back();
    g_app.avatars.back().build(look);
    meshes.push_back(g_app.avatars.back().mesh());
  }
  engine::profiler().stage("avatars");
  if (g_app.hooks.walkcheck) LOG_INFO("%s", app::walkReport(g_app.avatars[0]).c_str());
  // The bar's limits: 7 others in the room; 720 boxes (the Studio's X-ray edges need room, bar/studio_mode.cpp).
  if (!g_app.renderer.init(g_app.gpu, renderSceneOf(g_app.scene), meshes, {7, 720}) ||
      !g_app.text.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat) ||
      !g_app.pictures.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat)) {
    engine::setStatus("Startup failed - see log");
    LOG_FATAL("Startup failed: the renderer could not be set up.");
    return;
  }
  engine::profiler().stage("renderer");
  tuneMoodFromPage();
  g_app.camera.position = g_app.scene.spawnPosition;
  g_app.camera.yaw = g_app.scene.spawnYaw;
  applyTestCamera();
  // Keep the start height above the floor (test cameras may start higher).
  g_app.eyeHeight = g_app.camera.position.y - app::groundHeight(g_app.scene, g_app.camera.position);
  g_app.input.attach("#canvas");
  g_app.input.setCanvasSize(g_app.gpu.cssWidth, g_app.gpu.cssHeight);
  g_app.jukebox.init(g_app.scene.jukeboxSpeaker);
  makeSounds();
  using engine::synth::Effect;
  g_app.piano.init(g_app.scene);
  // Sounds are ids (copies are fine): made above in makeSounds().
  const auto sfx = [](Effect e) { return g_app.sfx[static_cast<int>(e)]; };
  g_app.darts.init(g_app.scene.dartBoard, sfx(Effect::kThunk), sfx(Effect::kWhoosh), sfx(Effect::kCheer));
  g_app.race.init(&g_app.gpu, sfx(Effect::kEngine), &g_app.others);
  g_app.race.setTelemetry(g_app.hooks.telemetry);
  const engine::Sound barSounds[app::Bartender::kSfxCount] = {sfx(Effect::kClink), sfx(Effect::kPour),
                                                              sfx(Effect::kShake), sfx(Effect::kClick)};
  g_app.bartender.init(g_app.scene.barTopFront, barSounds);
  g_app.sceneReady = true;  // multiplayer starts when you enter (app_enter)
  if (g_app.hooks.studio) studioStart();
  g_app.shownPosition.assign(g_app.physics.bodyCount(), engine::Vec3{1e9f, 0, 0});
  g_app.shownRotation.assign(g_app.physics.bodyCount(), engine::Quat{});
  g_app.fpsWindowStartMs = engine::clockMs();
  // The intro starts when you tap "enter" on the loading screen (app_enter).
  g_app.introEndMs = g_app.entered ? g_app.fpsWindowStartMs + kIntroMaxMs : 1e300;
  engine::profiler().stage("running");
  LOG_INFO("Running. Left thumb: walk. Right thumb: look around. Tap to use things.");
  LOG_INFO("Seats: %zu. Things to use: %zu.", g_app.scene.seats.size(), g_app.scene.interactables.size());
  engine::batchLogs(true);  // from here on, log lines go out with each frame's flush
  // Every animation frame of the page (it stops while the page is hidden).
  emscripten_request_animation_frame_loop([](double rafMs, void*) -> bool {
    frame(rafMs);
    return true;
  }, nullptr);
}

}  // namespace barapp

using namespace barapp;
