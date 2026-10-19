// The bar app: walk around in first person, use things, play music.
// Left thumb = joystick to walk, right thumb = drag to look around, tap to
// use things. Drawing lives in app/renderer.*; the games in their own files
// (piano, darts, racing, rhythm, bartender); the HTML menus in web/menu.js.
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/avatar.h"
#include "app/bar_scene.h"
#include "app/bartender.h"
#include "app/darts.h"
#include "app/jukebox.h"
#include "app/piano.h"
#include "app/players.h"
#include "app/props.h"
#include "app/race/game.h"
#include "app/loader.h"
#include "app/renderer.h"
#include "app/test_hooks.h"
#include "app/rhythm.h"
#include "app/shared_world.h"
#include "engine/gpu_upload.h"
#include "engine/audio.h"
#include "engine/billboard.h"
#include "engine/bridge.h"
#include "engine/camera.h"
#include "engine/fetch.h"
#include "engine/file.h"
#include "engine/gpu.h"
#include "engine/frame_time.h"
#include "engine/input.h"
#include "engine/jobs.h"
#include "engine/json.h"
#include "engine/log.h"
#include "engine/pipeline_cache.h"
#include "engine/profiler.h"
#include "engine/mesh.h"
#include "engine/net.h"
#include "engine/synth.h"
#include "engine/text.h"
#include "engine/texture.h"
#include "engine/ui.h"

using engine::Mat4;
using engine::Vec3;

void joinRoom();  // below: joins the multiplayer room once you've entered

namespace {

constexpr float kWalkSpeed = 2.2f;           // meters per second
constexpr float kLookSensitivity = 0.0055f;  // radians per CSS pixel (times the setting)
constexpr float kBodyRadius = 0.3f;          // meters
constexpr float kLabelShowDistance = 6.0f;   // name tags appear within this range
constexpr float kPromptDistance = 2.2f;      // ...and their prompt this close
constexpr float kDirectReach = 2.6f;         // small things: looked at within this range
constexpr float kTapReach = 4.0f;            // how far away a tap can use things
constexpr float kHighlightGlow = 0.35f;      // extra glow on the thing you're at
constexpr double kBannerMs = 7000;           // "now playing" banner duration
constexpr double kIntroMaxMs = 12000;        // title + controls hint on start...
constexpr double kIntroAfterInputMs = 1500;  // ...until shortly after you first move
constexpr float kGrabReach = 2.2f;           // how far away you can pick things up
constexpr float kPlaceReach = 2.0f;          // ...and put them down
constexpr float kThrowSpeed = 4.0f;          // meters per second
constexpr float kSeatTurn = 1.9f;            // how far (radians) you can turn your head while seated
constexpr double kToastMs = 3500;

using app::Activity;
using app::Emote;
using app::slot::kCount;

struct App {
  engine::Gpu gpu;
  engine::Input input;
  engine::FirstPersonCamera camera;
  app::BarScene scene;
  app::SceneRenderer renderer;
  std::vector<app::Avatar> avatars;  // one per look; all share the skeleton
  app::AvatarMotion avatarMotion;
  std::vector<Mat4> avatarBones, firstPersonBones;
  app::OtherPlayers others;  // multiplayer
  app::SharedWorld shared;   // glasses, jukebox and mirrors, the same for everyone
  std::vector<Mat4> otherTransforms;
  std::vector<std::vector<Mat4>> otherBones;
  std::vector<int> otherLooks;
  app::Jukebox jukebox;
  app::JukeboxMenu jukeboxMenu;
  app::RhythmGame rhythm;  // the playable 4K MANIA cabinet
  app::Piano piano;
  app::Darts darts;
  app::race::Game race;
  app::Bartender bartender;
  engine::Sound sfx[static_cast<int>(engine::synth::Effect::kCount)];
  engine::Sound guitar[6];
  engine::Sound arcadeMusic;   // the arcade's loop (engine::synth::arcadeLoop)
  int arcadeVoice = 0;
  float arcadeGain = 0;
  engine::TextRenderer text;
  engine::PictureBillboards pictures;  // profile pictures over other players' heads
  struct PictureSlot {
    std::string account;
    int rev = 0;
    bool ready = false;
    double usedMs = 0;
  };
  PictureSlot pictureSlots[engine::PictureBillboards::kSlots];
  engine::PhysicsWorld physics;
  std::vector<int> bodyOfModel;  // physics body per BarScene::models entry
  std::vector<engine::Vec3> shownPosition;  // per body, where its model was last drawn
  std::vector<engine::Quat> shownRotation;
  int held = -1;                 // body in your hand
  int aimed = -1;                // body in the middle of the screen, within reach
  std::vector<app::SceneRenderer::Box> boxes;

  // You: what you're doing.
  int look = 0;
  Activity activity = Activity::kNone;
  int seat = -1;               // the seat you're on
  int seatClaim = 0;           // the room's answer to "may I sit here?" (engine::net::claim)
  bool meridianOpen = false;   // Meridian (web/meridian/) covers the screen: the bar stops drawing
  std::string machineKey;      // the machine you're using (darts, 4K Mania), claimed in the room
  int machineClaim = 0;
  double activityStartMs = 0;
  Emote emote = Emote::kNone;
  int emoteCount = 0;
  bool onMic = false;
  float voiceLevel = 0;
  double lastVoicePlaceMs = 0;

  // The room.
  bool lightsDim = false, fireOn = true;

  // Settings (web/menu.js).
  int settingsVersion = -1;
  app::TestHooks hooks;  // ?cam, ?still, ?perf, ?sit... (app/test_hooks.h)
  app::Loader loader;    // the loading screen's stages (app/loader.h)
  bool warmGpuDone = false;  // the warm-up frame has finished on the GPU
  bool warmUpFrame = false;  // this frame is the warm-up frame
  bool pipelinesLogged = false, racePipelinesLogged = false;
  bool sceneReady = false;  // onGpuReady finished
  bool joined = false;      // in the multiplayer room
  float lookSpeed = 1, invertLook = 1;
  bool showNames = true;

  float eyeHeight = 1.65f;  // eyes above the floor you stand on
  int focused = -1;      // interactable you're standing at and facing
  int highlighted = -1;  // interactable currently drawn glowing
  int focusedSeat = -1;  // seat in the middle of the screen, within reach
  int highlightedSeatModel = -1;
  int bannerSong = 0;
  double bannerEndMs = 0;
  std::string toast;
  double toastEndMs = 0;

  double introEndMs = 0;
  bool entered = false;        // the loading screen has been closed
  double fpsWindowStartMs = 0;
  int fpsFrames = 0;
  size_t infoSongCount = SIZE_MAX;  // piano songs the menu knows (SIZE_MAX: nothing sent yet)
  std::string infoState;            // what the menu was last told about you
};

App g_app;

void showToast(const std::string& text) {
  g_app.toast = text;
  g_app.toastEndMs = engine::frameTime().nowMs + kToastMs;
}

void playSfx(engine::synth::Effect e, Vec3 where, float gain = 0.8f, bool share = false) {
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

bool hasBadge(const char* badge) {
  return EM_ASM_INT({
    var a = window['bar3dAccount'];
    if (a && a['hasBadge'] && a['hasBadge'](UTF8ToString($0))) return 1;
    // Tests on this computer: ?badge=bartender
    var local = /^localhost$|^127[.]/.test(location.hostname);
    return local && new RegExp('[?&]badge=' + UTF8ToString($0)).test(location.search) ? 1 : 0;
  }, badge) != 0;
}

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

void setActivity(Activity a) {
  if (g_app.activity == a) return;
  // Leaving darts or 4K Mania frees the machine for the others.
  if (!g_app.machineKey.empty() && (g_app.activity == Activity::kDarts || g_app.activity == Activity::kRhythm)) {
    engine::net::release(g_app.machineKey);
    g_app.machineKey.clear();
    g_app.machineClaim = 0;
  }
  g_app.activity = a;
  g_app.activityStartMs = engine::frameTime().nowMs;
  if (a != Activity::kEmote) g_app.emote = Emote::kNone;
}

void startEmote(Emote e) {
  if (g_app.seat >= 0 || g_app.rhythm.isOpen() || g_app.darts.playing()) {
    showToast("Stand up first to do an emote.");
    return;
  }
  g_app.emote = e;
  ++g_app.emoteCount;
  g_app.activity = Activity::kEmote;
  g_app.activityStartMs = engine::frameTime().nowMs;
}

// ---- Sitting ----

std::string seatKey(int seat) { return "seat." + std::to_string(seat); }

void standUp() {
  if (g_app.seat < 0) return;
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
  if (g_app.others.online()) engine::net::release(seatKey(g_app.seat));
  g_app.seatClaim = 0;
  if (s.kind == app::SeatKind::kSim) {
    g_app.race.close();
    g_app.input.setCapture(false);
  }
  if (s.kind == app::SeatKind::kPianoBench) {
    g_app.piano.closeKeyboard();
    g_app.input.setCapture(false);
  }
  // Step off where you can walk away (app::standSpot).
  Vec3 p = app::standSpot(g_app.scene, s);
  p.y = app::groundHeight(g_app.scene, p) + 1.65f;
  g_app.camera.position = p;
  g_app.eyeHeight = 1.65f;
  g_app.seat = -1;
  setActivity(Activity::kNone);
}

// The race on the rig you're sitting in.
void startRace(bool straightIn = false) {
  if (g_app.seat < 0) return;
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
  if (s.kind != app::SeatKind::kSim) return;
  if (g_app.race.start(std::max(0, s.machine), engine::frameTime().nowMs, straightIn)) {
    g_app.input.setCapture(true);
  } else {
    showToast("Warming up the race...");  // it starts by itself in a moment
  }
}

void sitOn(int index) {
  const app::Seat& s = g_app.scene.seats[static_cast<size_t>(index)];
  if (const app::OtherPlayer* o = playerUsing(Activity::kSit, index)) {
    showToast(o->name + " is sitting there.");
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
  if (s.kind == app::SeatKind::kSim) {
    setActivity(Activity::kDrive);
    // Pick a game: the race with bots or Meridian (web/menu.js "sim"). The
    // ?sit= test option goes straight into the race.
    if (g_app.hooks.sit == index) startRace(!g_app.hooks.garage);
    else engine::ui::openPanel("sim");
  } else if (s.kind == app::SeatKind::kPianoBench) {
    setActivity(Activity::kPiano);
    g_app.piano.openKeyboard();
    g_app.input.setCapture(true);
  } else {
    setActivity(Activity::kSit);
    showToast(std::string(s.name) + "\nMove the joystick to stand up");
  }
}

// ---- Camera and movement ----

void updateCamera(float dt) {
  // A menu or account panel covers the game: no looking or walking meanwhile.
  const bool covered = engine::bridge::frame().modalOpen;
  const engine::Vec2 look = covered ? engine::Vec2{} : g_app.input.frame().look;
  const bool menu = g_app.jukeboxMenu.isOpen() || g_app.rhythm.isOpen() || g_app.race.open();
  const float sens = kLookSensitivity * g_app.lookSpeed;
  if (!menu && !g_app.piano.keyboardOpen()) g_app.camera.look(look.x * sens, -look.y * sens * g_app.invertLook);
  const engine::Vec2 move = covered ? engine::Vec2{} : g_app.input.frame().move;
  if (look.x != 0 || look.y != 0 || move.x != 0 || move.y != 0) {
    if (g_app.entered) g_app.introEndMs = std::min(g_app.introEndMs, engine::frameTime().nowMs + kIntroAfterInputMs);
  }
  if (g_app.seat >= 0) {
    // Seated: the eyes stay put; the head turns only so far.
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
    const float turn = engine::wrapAngle(g_app.camera.yaw - s.yaw);
    g_app.camera.yaw = s.yaw + engine::clampf(turn, -kSeatTurn, kSeatTurn);
    g_app.camera.position = app::seatedEye(s.hip, s.yaw, s.lowSeat);
    app::advanceWalk(g_app.avatarMotion, 0, 0, dt, kWalkSpeed);
    if (engine::length({move.x, 0, move.y}) > 0.6f && !g_app.input.capturing()) standUp();
    return;
  }
  if (menu) {  // stand still in menus and games
    app::advanceWalk(g_app.avatarMotion, 0, 0, dt, kWalkSpeed);
    return;
  }
  Vec3 p = g_app.camera.position;
  p += g_app.camera.flatForward() * (move.y * kWalkSpeed * dt);
  p += g_app.camera.flatRight() * (move.x * kWalkSpeed * dt);
  const Vec3 before = g_app.camera.position;
  p = app::resolveCollisions(g_app.scene, p, kBodyRadius);
  // Step up onto (or down from) raised floors smoothly.
  const float targetY = app::groundHeight(g_app.scene, p) + g_app.eyeHeight;
  p.y += (targetY - p.y) * std::min(1.0f, dt * 12.0f);
  g_app.camera.position = p;
  // Drive the walk animation by how far you actually moved (walls stop it).
  const Vec3 moved = g_app.camera.position - before;
  app::advanceWalk(g_app.avatarMotion, engine::dot(moved, g_app.camera.flatForward()),
                   engine::dot(moved, g_app.camera.flatRight()), dt, kWalkSpeed);
  // Walking ends an emote, darts, a shift's station.
  const float walked = engine::length({moved.x, 0, moved.z});
  if (walked > 0.002f) {
    if (g_app.activity == Activity::kEmote) setActivity(Activity::kNone);
    if (g_app.darts.playing() && engine::length(g_app.camera.position - g_app.scene.dartOche) > 0.6f) {
      g_app.darts.stop();
      setActivity(Activity::kNone);
      showToast("You left the darts.");
    }
  }
}

void updateFps(double nowMs) {
  ++g_app.fpsFrames;
  const double elapsed = nowMs - g_app.fpsWindowStartMs;
  if (elapsed < 500) return;
  char text[128];
  const std::string online = g_app.others.statusText();
  std::snprintf(text, sizeof(text), "%.0f fps  |  %ux%u @%.1fx  |  %zu objects%s%s",
                g_app.fpsFrames * 1000.0 / elapsed, g_app.gpu.width, g_app.gpu.height, g_app.gpu.pixelRatio,
                g_app.renderer.objectCount(), online.empty() ? "" : "  |  ", online.c_str());
  engine::setStatus(text);
  g_app.fpsFrames = 0;
  g_app.fpsWindowStartMs = nowMs;
}

// The prompt for whatever you're standing at.
std::string promptFor(const app::Interactable& it) {
  using app::Action;
  if (it.action == Action::kJukebox) return g_app.jukebox.prompt();
  if (it.action == Action::kMirror) {
    const app::Mirror& m = g_app.scene.mirrors[static_cast<size_t>(it.mirror)];
    if (!m.on) return "Mirror: off\nTap: full mirror";
    if (m.mode == app::MirrorMode::kFull) return "Mirror: full\nTap: avatars only";
    return "Mirror: avatars only\nTap: turn off";
  }
  if (it.action == Action::kMirrorMode) {
    static const char* kNames[3] = {"Full mirror", "Avatars-only mirror", "Mirror off"};
    return std::string(kNames[it.value]) + "\nTap to choose";
  }
  if (it.action == Action::kDrinkShelf) {
    const int d = g_app.bartender.drinkAt(it.value);
    return d ? std::string(app::drinkRecipes()[static_cast<size_t>(d - 1)].name) + "\nTap: take it" : "";
  }
  if (it.action == Action::kRhythm) {
    if (const app::OtherPlayer* o = playerUsing(Activity::kRhythm, 0)) return "4K Mania\n" + o->name + " is playing";
  }
  if (it.action == Action::kDarts) {
    if (const app::OtherPlayer* o = playerUsing(Activity::kDarts, 0)) return "Darts\n" + o->name + " is throwing";
  }
  if (it.action == Action::kBartender) {
    if (g_app.bartender.onShift()) return "Bartender station\nTap: end your shift";
    return hasBadge("bartender") ? it.prompt : "Bartender station\n(needs the Bartender badge)";
  }
  if (it.action == Action::kPianoSongs) {
    const std::string playing = g_app.piano.nowPlaying();
    return playing.empty() ? it.prompt : "Playing: " + playing + "\nTap: pick a song";
  }
  if (it.action == Action::kMic) return g_app.onMic ? "Microphone (you're on it)\nTap: step off" : it.prompt;
  if (it.action == Action::kLightSwitch) return g_app.lightsDim ? "Light switch\nTap: lights up" : it.prompt;
  return it.prompt;
}

// The picture slot showing this player's profile picture, or -1 (a guest,
// no picture, or still downloading). Starts the download when needed.
int pictureSlotFor(const app::OtherPlayer& o, double nowMs) {
  if (o.account.empty() || o.avatarRev <= 0) return -1;
  auto& slots = g_app.pictureSlots;
  int found = -1, oldest = 0;
  for (int i = 0; i < static_cast<int>(engine::PictureBillboards::kSlots); ++i) {
    if (slots[i].account == o.account) found = i;
    if (slots[i].usedMs < slots[oldest].usedMs) oldest = i;
  }
  if (found < 0 || slots[found].rev != o.avatarRev) {
    const int i = found >= 0 ? found : oldest;
    slots[i] = {o.account, o.avatarRev, false, nowMs};
    const std::string url = engine::net::avatarUrl(o.account, o.avatarRev);
    if (url.empty()) return -1;
    engine::fetchFile(url, [i, account = o.account, rev = o.avatarRev](bool ok, std::vector<uint8_t> bytes) {
      if (!ok) return;
      engine::decodeImage(bytes, "image/jpeg", [i, account, rev](bool decoded, engine::Rgba8Image image) {
        auto& slot = g_app.pictureSlots[i];
        if (!decoded || slot.account != account || slot.rev != rev) return;
        g_app.pictures.setPicture(static_cast<uint32_t>(i), image);
        slot.ready = true;
      });
    });
    return -1;
  }
  slots[found].usedMs = nowMs;
  return slots[found].ready ? found : -1;
}

// The middle monitor of a sim rig, as world corners (bottom-left,
// bottom-right, top-right, top-left), slightly in front of the screen.
void simMonitor(int rig, Vec3 out[4]) {
  const Vec3 c = g_app.scene.simPosition[rig];
  // The rig faces +x; the screen is 1.08 m in front of its origin.
  const float x = c.x + 1.074f, hw = 0.36f, y0 = 1.08f - 0.21f, y1 = 1.08f + 0.21f;
  out[0] = {x, y0, c.z - hw};
  out[1] = {x, y0, c.z + hw};
  out[2] = {x, y1, c.z + hw};
  out[3] = {x, y1, c.z - hw};
}

// Others' games on the machines in the bar: the race on the sim monitors,
// the notes on the rhythm cabinet, darts and the score at the board.
void queueMachineScreens(double nowMs) {
  engine::TextRenderer& text = g_app.text;
  for (const app::OtherPlayer& o : g_app.others.players()) {
    if (!o.placed) continue;
    const auto act = static_cast<Activity>(static_cast<int>(o.values[app::slot::kActivity]));
    const float* d = o.values + app::slot::kData;
    if (act == Activity::kDrive) {
      const int seat = static_cast<int>(o.values[app::slot::kParam]);
      if (seat < 0 || seat >= static_cast<int>(g_app.scene.seats.size())) continue;
      const int rig = std::max(0, g_app.scene.seats[static_cast<size_t>(seat)].machine);
      Vec3 c[4];
      simMonitor(rig, c);
      // (The live 3D race on the three monitors comes in docs/RACING.md step 4.)
      text.addWorldQuad(c, {0.02f, 0.03f, 0.06f, 1});
      engine::TextStyle style;
      style.align = 0.5f;
      style.color = {1, 0.85f, 0.3f, 1};
      char line[64];
      std::snprintf(line, sizeof(line), "%s IS RACING\n%d KM/H", o.name.c_str(), static_cast<int>(std::fabs(d[3]) * 3.6f));
      const Vec3 mid = (c[0] + c[1] + c[2] + c[3]) * 0.25f + Vec3{-0.004f, 0.05f, 0};
      text.addWorld(line, mid, engine::normalize(c[1] - c[0]), {0, 1, 0}, 0.035f, style);
    } else if (act == Activity::kRhythm) {
      Vec3 c[4];
      const Vec3* s = g_app.scene.rhythmScreen;
      const Vec3 normal = engine::normalize(engine::cross(s[1] - s[0], s[3] - s[0]));
      for (int i = 0; i < 4; ++i) c[i] = s[i] + normal * 0.006f;
      app::RhythmGame::drawCabinet(text, c, d, nowMs);
    } else if (act == Activity::kDarts) {
      app::Darts::addStuckDarts(g_app.boxes, g_app.scene.dartBoard, d + 1, 3);
      engine::TextStyle style;
      style.align = 0.5f;
      style.color = {1, 1, 1, 1};
      char line[64];
      std::snprintf(line, sizeof(line), "%s: %d left", o.name.c_str(), static_cast<int>(d[0]));
      text.addWorld(line, g_app.scene.dartBoard + Vec3{1.7f, -0.05f, -0.06f}, {-1, 0, 0}, {0, 1, 0}, 0.04f, style);
    }
  }
}

// Queues this frame's text: name tags over things in the bar, the prompt
// for whatever you're standing at, and the intro title.
void queueText(double nowMs, const Mat4& viewProj) {
  engine::TextRenderer& text = g_app.text;
  const float W = static_cast<float>(g_app.gpu.width), H = static_cast<float>(g_app.gpu.height);
  const float px = g_app.gpu.pixelRatio;  // device pixels per CSS pixel
  text.begin(viewProj, g_app.gpu.width, g_app.gpu.height);
  g_app.pictures.begin(viewProj);
  if (g_app.rhythm.isOpen()) {  // the game covers the screen
    g_app.rhythm.draw(text, W, H, px, nowMs);
    engine::profiler().drawOverlay(text, W, H, px);
    text.upload(g_app.gpu.queue);
    return;
  }
  if (g_app.race.open()) return;  // the race queues its own HUD (app/race/game.cpp)
  queueMachineScreens(nowMs);

  const engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  const Vec3 up = engine::cross(right, forward);
  for (size_t i = 0; i < g_app.scene.interactables.size(); ++i) {
    if (g_app.jukeboxMenu.isOpen() || !g_app.showNames || g_app.darts.playing()) break;  // keep the view uncluttered
    if (static_cast<int>(i) == g_app.focused) continue;  // its prompt says what it is
    const app::Interactable& it = g_app.scene.interactables[i];
    if (it.name.empty()) continue;  // e.g. buttons: their prompt is enough
    const float dist = engine::length(it.tagPosition - cam.position);
    if (dist > kLabelShowDistance) continue;
    // Fade in as you approach; the tag always turns to face you.
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {1.0f, 0.86f, 0.62f, engine::clampf((kLabelShowDistance - dist) / 1.5f, 0, 1)};
    style.outline.w *= style.color.w;
    style.outlineWidth = 0.8f;
    text.addWorld(it.name, it.tagPosition, right, up, 0.1f, style);
  }
  // Other players' names float over their heads (green while they talk,
  // with their job), with their profile picture above when you're close.
  for (const app::OtherPlayer& o : g_app.others.players()) {
    if (!o.placed) continue;
    const int flags = static_cast<int>(o.values[app::slot::kFlags]);
    const float talk = std::max(o.voice, engine::clampf(engine::bridge::voiceLevel(o.id), 0, 1));
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = talk > 0.08f ? engine::Vec4{0.45f, 1.0f, 0.55f, 1.0f} : engine::Vec4{0.75f, 0.9f, 1.0f, 1.0f};
    style.outlineWidth = 0.8f;
    std::string name = (talk > 0.08f ? "(( " : "") + o.name + (talk > 0.08f ? " ))" : "");
    if (flags & 1) name += "\nBARTENDER";
    if (flags & 2) name += "\nON THE MIC";
    if (flags & app::kFlagAway) {
      name += "\n(away)";
      style.color = {0.6f, 0.6f, 0.65f, 0.9f};
    }
    text.addWorld(name, o.eye + Vec3{0, 0.32f, 0}, right, up, 0.07f, style);
    const float near = engine::clampf((7.0f - engine::length(o.eye - cam.position)) / 1.5f, 0, 1);
    const int slot = pictureSlotFor(o, nowMs);
    if (near > 0 && slot >= 0) {
      g_app.pictures.add(static_cast<uint32_t>(slot), o.eye + Vec3{0, 0.6f, 0}, right, up, 0.16f,
                         {1.0f, 0.42f, 0.78f}, near);
    }
  }
  g_app.pictures.upload(g_app.gpu.queue);

  engine::TextStyle promptStyle;
  promptStyle.align = 0.5f;
  promptStyle.outlineWidth = 0.8f;
  const bool busy = g_app.jukeboxMenu.isOpen() || g_app.piano.keyboardOpen() || g_app.bartender.making();
  if (!busy && (g_app.held >= 0 || g_app.aimed >= 0)) {
    const char* prompt = g_app.held >= 0 ? "Tap a table or the bar: put it down\nTap anywhere else: throw"
                                         : "Glass\nTap: pick up";
    text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, promptStyle);
  } else if (!busy && g_app.focusedSeat >= 0) {
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.focusedSeat)];
    const app::OtherPlayer* o = playerUsing(Activity::kSit, g_app.focusedSeat);
    std::string prompt = std::string(s.name) + (o ? "\n" + o->name + " is sitting here" : "\nTap: sit");
    if (!o && s.kind == app::SeatKind::kSim) prompt = "Racing sim\nTap: sit and race";
    if (!o && s.kind == app::SeatKind::kPianoBench) prompt = "Piano bench\nTap: sit and play";
    text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, promptStyle);
  } else if (!busy && g_app.focused >= 0 && g_app.seat < 0) {
    const app::Interactable& it = g_app.scene.interactables[static_cast<size_t>(g_app.focused)];
    const std::string prompt = promptFor(it);
    if (!prompt.empty()) text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, promptStyle);
  }
  if (g_app.bartender.held() && !busy) {
    engine::TextStyle s = promptStyle;
    s.color = {1.0f, 0.86f, 0.62f, 1};
    const std::string name = app::drinkRecipes()[static_cast<size_t>(g_app.bartender.held() - 1)].name;
    text.addScreen("Your " + name + "\nTap an empty spot to sip", W * 0.5f, H * 0.75f, 10 * px, s);
  }

  // "Now playing" banner with the song credit, whenever a new song starts.
  const app::Song* song = g_app.jukebox.currentSong();
  if (song && nowMs < g_app.bannerEndMs) {
    const float fade = engine::clampf(static_cast<float>((g_app.bannerEndMs - nowMs) / 1000.0), 0, 1);
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {1.0f, 0.86f, 0.62f, fade};
    style.outline.w *= fade;
    style.outlineWidth = 0.8f;
    text.addScreen("Now playing: " + song->title, W * 0.5f, H * 0.11f, 12 * px, style);
    style.color = {0.85f, 0.85f, 0.85f, fade};
    text.addScreen(song->builtIn ? app::kMusicCredit : "Your song (only on this phone)", W * 0.5f,
                   H * 0.11f + 22 * px, 9 * px, style);
  }

  if (nowMs < g_app.introEndMs && !g_app.hooks.still) {
    const float fade = engine::clampf(static_cast<float>((g_app.introEndMs - nowMs) / 1000.0), 0, 1);
    engine::TextStyle title;
    title.align = 0.5f;
    title.color = {1.0f, 0.35f, 0.7f, fade};
    title.outline.w *= fade;
    title.outlineWidth = 0.9f;
    text.addScreen("BAR3D", W * 0.5f, H * 0.2f, 34 * px, title);
    engine::TextStyle hint;
    hint.align = 0.5f;
    hint.color.w = fade;
    hint.outline.w *= fade;
    text.addScreen("Left thumb: walk\nRight thumb: look around\nTap things to use them\nMenu (top right): emotes, voice, settings",
                   W * 0.5f, H * 0.2f + 52 * px, 11 * px, hint);
  }
  if (g_app.darts.playing()) g_app.darts.drawHud(text, W, H, px);
  g_app.bartender.drawHud(text, W, H, px, nowMs);
  if (g_app.piano.keyboardOpen()) g_app.piano.drawKeyboard(text, W, H, px);
  if (nowMs < g_app.toastEndMs) {
    engine::TextStyle s;
    s.align = 0.5f;
    s.color = {1, 1, 1, engine::clampf(static_cast<float>((g_app.toastEndMs - nowMs) / 500.0), 0, 1)};
    s.outlineWidth = 0.8f;
    text.addScreen(g_app.toast, W * 0.5f, H * 0.2f, 10 * px, s);
  }
  g_app.jukeboxMenu.draw(text, g_app.jukebox, W, H, px);  // last, so it's on top
  engine::profiler().drawOverlay(text, W, H, px);
  text.upload(g_app.gpu.queue);
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
bool tapPerson(Vec3 dir) {
  const Vec3 eye = g_app.camera.position;
  const app::OtherPlayer* best = nullptr;
  float nearest = 6.0f;
  for (const app::OtherPlayer& o : g_app.others.players()) {
    if (!o.placed) continue;
    const float t = rayBoxDistance(eye, dir, o.eye - Vec3{0.3f, 1.7f, 0.3f}, o.eye + Vec3{0.3f, 0.45f, 0.3f});
    if (t >= 0 && t < nearest) {
      nearest = t;
      best = &o;
    }
  }
  if (!best) return false;
  engine::net::showCard(best->account, best->name);
  return true;
}

// The seat a ray hits within reach, or -1.
int seatHit(Vec3 dir, float reach, float* dist = nullptr) {
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

void setLights(bool dim, bool fire) {
  g_app.lightsDim = dim;
  g_app.fireOn = fire;
  for (int l : g_app.scene.roomLights) g_app.renderer.setLightScale(static_cast<size_t>(l), dim ? 0.25f : 1.0f);
  if (g_app.scene.fireLight >= 0) g_app.renderer.setLightScale(static_cast<size_t>(g_app.scene.fireLight), fire ? 1.0f : 0.0f);
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

void use(const app::Interactable& it) {
  using app::Action;
  const Vec3 where = (it.boundsMin + it.boundsMax) * 0.5f;
  switch (it.action) {
    case Action::kJukebox: g_app.jukeboxMenu.open(); break;
    case Action::kRhythm: {
      if (const app::OtherPlayer* o = playerUsing(Activity::kRhythm, 0)) {
        showToast(o->name + " is playing. One player at a time.");
        break;
      }
      if (!claimMachine("machine.rhythm", "playing")) break;
      std::vector<app::RhythmGame::SongChoice> songs;
      for (const app::Song& s : g_app.jukebox.songs()) songs.push_back({s.title, "", s.url});
      g_app.rhythm.open(std::move(songs));
      g_app.input.setCapture(true);
      setActivity(Activity::kRhythm);
      break;
    }
    case Action::kMirror:
    case Action::kMirrorMode: {
      app::Mirror& m = g_app.scene.mirrors[static_cast<size_t>(it.mirror)];
      // Tapping the glass steps through full -> avatars only -> off -> full;
      // the panel's buttons pick one directly.
      int mode = !m.on ? 2 : m.mode == app::MirrorMode::kFull ? 0 : 1;
      mode = it.action == Action::kMirror ? (mode + 1) % 3 : it.value;
      m.on = mode != 2;
      if (mode != 2) m.mode = mode == 0 ? app::MirrorMode::kFull : app::MirrorMode::kAvatarsOnly;
      g_app.shared.mirrorChanged(it.mirror, m.on, mode == 1);
      break;
    }
    case Action::kDarts: {
      if (const app::OtherPlayer* o = playerUsing(Activity::kDarts, 0)) {
        showToast(o->name + " is throwing. One player at a time.");
        break;
      }
      if (!claimMachine("machine.darts", "throwing")) break;
      g_app.camera.position = g_app.scene.dartOche;
      g_app.camera.yaw = engine::kPi;  // facing the board (+z)
      g_app.camera.pitch = 0;
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
      g_app.camera.position = g_app.scene.bartenderSpot;
      g_app.camera.yaw = engine::kPi;  // facing the counter and the room (+z)
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
    case Action::kLightSwitch: {
      setLights(!g_app.lightsDim, g_app.fireOn);
      playSfx(engine::synth::Effect::kClick, where);
      const double v[2] = {g_app.lightsDim ? 1.0 : 0.0, g_app.fireOn ? 1.0 : 0.0};
      engine::net::setWorld("lt", v, 2);
      break;
    }
    case Action::kFireplace: {
      setLights(g_app.lightsDim, !g_app.fireOn);
      const double v[2] = {g_app.lightsDim ? 1.0 : 0.0, g_app.fireOn ? 1.0 : 0.0};
      engine::net::setWorld("lt", v, 2);
      showToast(g_app.fireOn ? "The fire crackles back to life." : "The fire dies down.");
      break;
    }
    case Action::kDoor:
      playSfx(engine::synth::Effect::kKnock, where, 0.8f, true);
      showToast("The street outside isn't built yet.");
      break;
    case Action::kBooks: {
      static const char* kBooks[] = {"\"The Joy of Mixology\" - you learn three new cocktails.",
                                     "\"Moby-Dick\" - Call me Ishmael...", "\"The Great Gatsby\" - a party book.",
                                     "\"Piano for Beginners\" - try the bench on the stage!",
                                     "\"Darts: The Perfect 180\" - aim for the treble 20."};
      static int next = 0;
      showToast(kBooks[next++ % 5]);
      playSfx(engine::synth::Effect::kClick, where, 0.4f);
      break;
    }
    case Action::kChess:
      showToast("You move a pawn. Your turn is over.");
      playSfx(engine::synth::Effect::kClick, where, 0.6f, true);
      break;
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
      // The machine's game full screen (web/menu.js GAMES), like Meridian.
      if (it.value == 0) engine::ui::openPanel("platformer");
      break;
    case Action::kTv:
    case Action::kNone: break;
  }
}

// Lights the button for each mirror's current mode (also after another
// player changes it).
void updateMirrorButtons() {
  static std::vector<int> shown;
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

// Holding something: a tap on a surface within reach puts it down there;
// a tap anywhere else throws it that way.
void putDownOrThrow(Vec3 dir) {
  engine::Body& b = g_app.physics.body(static_cast<size_t>(g_app.held));
  const Vec3 eye = g_app.camera.position;
  float dist;
  Vec3 normal;
  b.held = false;
  b.asleep = false;
  b.sleepTimer = 0;
  if (g_app.physics.raycastStatic(eye, dir, kPlaceReach, &dist, &normal) && normal.y > 0.7f) {
    // Stand it upright on the spot.
    g_app.physics.teleport(static_cast<size_t>(g_app.held), eye + dir * dist + Vec3{0, 0.002f, 0}, {});
    b.velocity = {};
    b.angularVelocity = {};
    LOG_INFO("Put down at %.2f, %.2f, %.2f.", b.position.x, b.position.y, b.position.z);
    g_app.shared.letGo(g_app.held, b, true);
  } else {
    b.velocity = dir * kThrowSpeed + Vec3{0, 1.0f, 0};
    b.angularVelocity = {0, 0, 0};
    LOG_INFO("Thrown from %.2f, %.2f, %.2f.", b.position.x, b.position.y, b.position.z);
    g_app.shared.letGo(g_app.held, b, false);
  }
  g_app.held = -1;
}

// Picks up the body a tap lands on, if it's within reach and nothing solid
// is in the way.
bool tryPickUp(Vec3 dir) {
  const Vec3 eye = g_app.camera.position;
  float dist;
  const int body = g_app.physics.raycastBodies(eye, dir, kGrabReach, &dist);
  if (body < 0) return false;
  float wallDist;
  if (g_app.physics.raycastStatic(eye, dir, dist, &wallDist, nullptr)) return false;
  if (!g_app.shared.canPickUp(body)) return false;  // in someone else's hand
  engine::Body& b = g_app.physics.body(static_cast<size_t>(body));
  b.held = true;
  b.kinematic = false;
  b.asleep = false;
  g_app.held = body;
  g_app.shared.pickedUp(body);
  return true;
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
}

// Makes one interactable's parts glow (or stop glowing).
void setGlow(int index, float glow) {
  if (index >= 0) g_app.renderer.setGlow(g_app.scene.interactables[static_cast<size_t>(index)], glow);
}

// Is this interactable there right now? (A drink spot only when it has a drink.)
bool available(const app::Interactable& it) {
  if (it.action == app::Action::kDrinkShelf) return g_app.bartender.drinkAt(it.value) != 0;
  return true;
}

// Menu commands (web/menu.js) and settings.
void handleCommands() {
  std::vector<engine::ui::Command> commands;
  engine::ui::takeCommands(commands);
  for (const engine::ui::Command& c : commands) {
    if (c.name == "emote") startEmote(static_cast<Emote>(static_cast<int>(c.a)));
    else if (c.name == "look") {
      g_app.look = std::clamp(static_cast<int>(c.a), 0, static_cast<int>(app::avatarLooks().size()) - 1);
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
    } else if (c.name == "make") {
      g_app.bartender.make(static_cast<int>(c.a));
    } else if (c.name == "endShift") {
      g_app.bartender.endShift();
    } else if (c.name == "standUp") {
      standUp();
    } else if (c.name == "simRace") {
      startRace();
    } else if (c.name == "meridianOpen") {
      g_app.meridianOpen = true;
      LOG_INFO("Meridian: open (the bar pauses drawing).");
    } else if (c.name == "meridianClosed") {
      g_app.meridianOpen = false;
    }
  }
  const int v = engine::settings::version();
  if (v != g_app.settingsVersion) {
    const bool first = g_app.settingsVersion < 0;
    g_app.settingsVersion = v;
    g_app.lookSpeed = static_cast<float>(engine::settings::number("lookSpeed", 1));
    g_app.invertLook = engine::settings::number("invertLook", 0) != 0 ? -1.0f : 1.0f;
    g_app.showNames = engine::settings::number("showNames", 1) != 0;
    engine::profiler().overlay = g_app.hooks.perf || engine::settings::number("perf", 0) != 0;
    // Your saved look, at start only (later the menu sends a "look" command).
    const int look = first ? static_cast<int>(engine::settings::number("look", g_app.look)) : g_app.look;
    if (look != g_app.look && look >= 0 && look < static_cast<int>(app::avatarLooks().size())) {
      g_app.look = look;
      g_app.renderer.setSelfAvatar(static_cast<size_t>(look));
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
      w.endArray().key("drinks").beginArray();
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
      .key("look").value(g_app.look)
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
    if (e.key == "lt" && e.values.size() >= 2) setLights(e.values[0] != 0, e.values[1] != 0);
    if (e.key == "rc") g_app.race.onRoomRace(e.values);  // the racing rigs' lobby (cloud/games/race.js)
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
  if (g_app.rhythm.isOpen()) {
    g_app.rhythm.update(nowMs, g_app.input.frame().presses, W, H);
    if (!g_app.rhythm.isOpen()) {
      g_app.input.setCapture(false);
      setActivity(Activity::kNone);
    }
  } else if (g_app.race.pending()) {
    if (g_app.race.tryPending(nowMs)) g_app.input.setCapture(true);
  } else if (g_app.race.open()) {
    if (g_app.race.update(nowMs, dt, g_app.input.frame())) standUp();
  } else if (g_app.piano.keyboardOpen()) {
    if (g_app.piano.updateKeyboard(g_app.input.frame().presses, W, H)) standUp();
    if (g_app.piano.takeSongsRequest()) engine::ui::openPanel("piano");
  }
  g_app.darts.update(nowMs);

  // A tap uses what it lands on; tapping anywhere also works while you're
  // standing at something (easier on a small screen).
  // (A copy: using something can switch input modes, which empties the frame's taps.)
  const std::vector<engine::Vec2> taps = g_app.input.frame().taps;
  for (const engine::Vec2 tap : taps) {
    if (g_app.entered) g_app.introEndMs = std::min(g_app.introEndMs, nowMs + kIntroAfterInputMs);
    if (g_app.jukeboxMenu.isOpen()) {
      g_app.jukeboxMenu.tap(tap, g_app.jukebox);
      continue;
    }
    if (g_app.bartender.tap(tap, W, H, nowMs, cam.position)) continue;
    const Vec3 dir = screenRay(tap);
    if (g_app.darts.playing()) {
      g_app.darts.throwAt(cam.position, dir, nowMs);
      g_app.activityStartMs = nowMs;  // the throw animation
      continue;
    }
    if (g_app.held >= 0) {
      putDownOrThrow(dir);
      continue;
    }
    if (g_app.seat < 0 && tryPickUp(dir)) continue;
    if (tapPerson(dir)) continue;
    int hit = -1;
    float nearest = kTapReach;
    for (size_t i = 0; i < items.size(); ++i) {
      if (!available(items[i])) continue;
      const float t = rayBoxDistance(cam.position, dir, items[i].boundsMin, items[i].boundsMax);
      if (t >= 0 && t < nearest && (!items[i].direct || t < kDirectReach)) {
        nearest = t;
        hit = static_cast<int>(i);
      }
    }
    float seatDist;
    const int seat = g_app.seat < 0 ? seatHit(dir, std::min(nearest, kDirectReach), &seatDist) : -1;
    if (seat >= 0) {
      sitOn(seat);
      continue;
    }
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

// Your avatar's activity this frame (and what it means for the pose).
void updateSelfMotion(double nowMs) {
  app::AvatarMotion& m = g_app.avatarMotion;
  // Emotes end by themselves.
  if (g_app.activity == Activity::kEmote &&
      (nowMs - g_app.activityStartMs) / 1000.0 > app::emoteLength(g_app.emote)) {
    setActivity(Activity::kNone);
  }
  Activity shown = g_app.activity;
  float actionTime = static_cast<float>((nowMs - g_app.activityStartMs) / 1000.0);
  float t;
  if (g_app.bartender.activity(nowMs, t) == Activity::kBartend) {
    shown = Activity::kBartend;
    actionTime = t;
  } else if (g_app.activity == Activity::kNone && g_app.bartender.held() && g_app.bartender.sipping(nowMs, t)) {
    shown = Activity::kDrink;
    actionTime = t;
  }
  m.activity = shown;
  m.emote = g_app.emote;
  m.actionTime = actionTime;
  m.headYaw = 0;
  m.lowSeat = 0;
  if (g_app.seat >= 0) {
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
    m.headYaw = engine::wrapAngle(g_app.camera.yaw - s.yaw);
    m.lowSeat = s.lowSeat;
  }
  m.steer = g_app.race.steer();
  g_app.piano.hands(m.handL, m.handR);
}

// The numbers others get about you (players.h slot order after kPitch).
void selfState(float extra[kCount - app::slot::kActivity]) {
  using namespace app::slot;
  auto e = [&](int s) -> float& { return extra[s - kActivity]; };  // index by slot
  const app::AvatarMotion& m = g_app.avatarMotion;
  e(kActivity) = static_cast<float>(static_cast<int>(m.activity));
  e(kParam) = g_app.seat >= 0 ? static_cast<float>(g_app.seat) : 0;
  e(kLook) = static_cast<float>(g_app.look);
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
}

// Seated others sit on their seat; everyone else stands where their eyes are.
Mat4 otherTransform(const app::OtherPlayer& o, app::AvatarMotion& motion) {
  const auto act = static_cast<Activity>(static_cast<int>(o.values[app::slot::kActivity]));
  const int seat = static_cast<int>(o.values[app::slot::kParam]);
  const bool seated = act == Activity::kSit || act == Activity::kDrive || act == Activity::kPiano;
  if (seated && seat >= 0 && seat < static_cast<int>(g_app.scene.seats.size())) {
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(seat)];
    motion.headYaw = engine::wrapAngle(o.yaw - s.yaw);
    motion.lowSeat = s.lowSeat;
    motion.steer = o.values[app::slot::kData + 4];
    if (act == Activity::kPiano) g_app.piano.hands(motion.handL, motion.handR);
    return app::seatedTransform(s.hip, s.yaw);
  }
  motion.headYaw = 0;
  return app::avatarTransform(o.eye, o.yaw, o.pitch);
}

// Talks to the voice chat (web/voice.js): where each player's voice comes
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

// Keeps the loading screen (web/index.html) up to date until every model
// has arrived; then it offers "Tap to enter".
// Test options (?sit=, ?rhythm=, ...), applied once the bar is ready, as if
// you had done them yourself.
void applyTestHooks() {
  const app::TestHooks& h = g_app.hooks;
  g_app.rhythm.autoplay = h.autoplay;
  if (h.rhythmSong >= 0) {
    std::vector<app::RhythmGame::SongChoice> songs;
    for (const app::Song& s : g_app.jukebox.songs()) songs.push_back({s.title, "", s.url});
    g_app.rhythm.open(std::move(songs));
    g_app.input.setCapture(true);
    g_app.rhythm.start(h.rhythmSong, static_cast<app::Difficulty>(std::clamp(h.rhythmDifficulty, 0, app::kDifficultyCount - 1)));
    setActivity(Activity::kRhythm);
  }
  if (h.sit >= 0 && h.sit < static_cast<int>(g_app.scene.seats.size())) sitOn(h.sit);
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

void frame(double rafMs) {
  using engine::Phase;
  // One time for the whole frame: the browser's frame time. The step is
  // clamped, so coming back from another app doesn't teleport anything.
  engine::beginFrameTime(rafMs);
  const engine::FrameTime& ft = engine::frameTime();
  const double nowMs = ft.nowMs;
  const float dt = ft.dt;
  // Touches queued since the last frame are read first, so the profiler
  // counts them for the frame they arrived in.
  engine::bridge::pull();  // everything the page collected since last frame, in one call
  g_app.input.setCanvasSize(g_app.gpu.cssWidth, g_app.gpu.cssHeight);
  g_app.input.poll();  // what the player did since last frame (read-only from here on)
  ProfiledFrame profiled(engine::clockMs());
  if (ft.resumed && ft.gapMs > 1000) LOG_INFO("Back after %.1f s away.", ft.gapMs / 1000.0);
  updateLoadingScreen();

  g_app.gpu.resizeIfNeeded();
  // Loading work (textures, model buffers) gets a slice of each frame: more
  // while the loading screen is up, little once you walk around.
  updateFps(nowMs);
  {
    PROFILE(Phase::kInput);
    handleCommands();
    updateCamera(dt);
    g_app.input.flushStick();
  }
  {
    PROFILE(Phase::kInteraction);
    updateInteraction(nowMs, dt);
  }
  {
    PROFILE(Phase::kPhysics);
    updatePhysics(dt);
  }
  std::vector<engine::net::WorldEntry> world;
  std::vector<engine::net::Event> events;
  {
    PROFILE(Phase::kNet);
    world = g_app.shared.takeOther();
    if (g_app.others.online()) engine::net::receiveEvents(events);
    handleShared(world, events);
  }
  {
    PROFILE(Phase::kGames);
    g_app.piano.update(nowMs, world, events);
    g_app.bartender.update(nowMs, world);
  }
  {
    PROFILE(Phase::kNet);
    updateSelfMotion(nowMs);
    float extra[kCount - app::slot::kActivity] = {};
    selfState(extra);
    g_app.others.sendSelf(nowMs, g_app.camera.position, g_app.camera.yaw, g_app.camera.pitch, extra);
    g_app.others.update(nowMs, dt, kWalkSpeed);
    updateVoice(nowMs);
    sendInfo();
  }

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
        g_app.input.setCapture(false);
      }
      setActivity(Activity::kNone);
    }
  }

  // Meridian has the screen: the bar keeps its connection but doesn't draw.
  if (g_app.meridianOpen) return;

  wgpu::TextureView target;
  {
    PROFILE(Phase::kAcquire);
    target = g_app.gpu.beginFrame();
  }
  if (!target) return;
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

  // Moving shapes: piano lights, darts, drinks.
  g_app.boxes.clear();
  g_app.piano.addBoxes(g_app.boxes, nowMs);
  g_app.darts.addBoxes(g_app.boxes, nowMs);
  g_app.bartender.addBoxes(g_app.boxes);
  if (g_app.bartender.held()) {
    float t;
    const bool sip = g_app.bartender.sipping(nowMs, t);
    const float lift = sip ? std::pow(std::max(0.0f, std::sin(t * 1.4f)), 4.0f) : 0.0f;
    const Vec3 hand = cam.position + cam.flatForward() * (0.38f - 0.15f * lift) + cam.flatRight() * (0.18f - 0.1f * lift) +
                      Vec3{0, -0.35f + 0.25f * lift, 0};
    app::Bartender::addHeldDrink(g_app.boxes, g_app.bartender.held(), hand, cam.yaw);
  }
  engine::profiler().add(Phase::kScene, engine::clockMs() - sceneStart);
  {
    PROFILE(Phase::kText);
    queueText(nowMs, viewProj);  // after the boxes: others' darts are added there
  }
  sceneStart = engine::clockMs();

  g_app.avatarMotion.pitch = cam.pitch;
  g_app.avatarMotion.time = g_app.hooks.still ? 0.0f : static_cast<float>(nowMs / 1000.0);
  const app::Avatar& body = g_app.avatars[0];  // the skeleton is the same for every look
  body.pose(g_app.avatarMotion, g_app.avatarBones);
  body.hideHead(g_app.avatarBones, g_app.firstPersonBones);
  const auto& others = g_app.others.players();
  g_app.otherTransforms.clear();
  g_app.otherLooks.clear();
  g_app.otherBones.resize(others.size());
  for (size_t i = 0; i < others.size(); ++i) {
    const app::OtherPlayer& o = others[i];
    if (!o.placed) continue;
    app::AvatarMotion motion = o.motion;
    const Mat4 transform = otherTransform(o, motion);
    body.pose(motion, g_app.otherBones[g_app.otherTransforms.size()]);
    g_app.otherTransforms.push_back(transform);
    g_app.otherLooks.push_back(o.look);
    const int drink = static_cast<int>(o.values[app::slot::kHeldDrink]);
    if (drink > 0) {
      const Vec3 hand = o.eye + Vec3{std::cos(o.yaw) * 0.22f + std::sin(o.yaw) * 0.25f, -0.55f,
                                     std::sin(o.yaw) * 0.22f - std::cos(o.yaw) * 0.25f};
      app::Bartender::addHeldDrink(g_app.boxes, drink, hand, o.yaw);
    }
  }
  g_app.otherBones.resize(g_app.otherTransforms.size());
  g_app.renderer.setOthers(g_app.otherTransforms, g_app.otherBones, g_app.otherLooks);
  g_app.renderer.setBoxes(g_app.boxes);
  Mat4 self = app::avatarTransform(cam.position, cam.yaw, cam.pitch);
  if (g_app.seat >= 0) {
    const app::Seat& s = g_app.scene.seats[static_cast<size_t>(g_app.seat)];
    self = app::seatedTransform(s.hip, s.yaw);
  }
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
  if (g_app.meridianOpen) target = 0;
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

void onGpuReady(bool ok) {
  if (!ok) {
    engine::setStatus("Startup failed - see log");
    LOG_FATAL("Startup failed: no GPU.");
    return;
  }
  engine::profiler().stage("gpu");
  g_app.hooks = app::TestHooks::parse();
  engine::prepareMipmaps(g_app.gpu.device);  // texture jobs wait for these
  g_app.scene = app::buildBarScene();
  app::buildPhysics(g_app.scene, g_app.physics, g_app.bodyOfModel);
  engine::profiler().stage("scene");
  std::vector<engine::SkinnedMeshData> meshes;
  for (const app::AvatarLook& look : app::avatarLooks()) {
    g_app.avatars.emplace_back();
    g_app.avatars.back().build(look);
    meshes.push_back(g_app.avatars.back().mesh());
  }
  engine::profiler().stage("avatars");
  if (!g_app.renderer.init(g_app.gpu, g_app.scene, meshes) ||
      !g_app.text.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat) ||
      !g_app.pictures.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat)) {
    engine::setStatus("Startup failed - see log");
    LOG_FATAL("Startup failed: the renderer could not be set up.");
    return;
  }
  engine::profiler().stage("renderer");
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
  const engine::Sound barSounds[app::Bartender::kSfxCount] = {sfx(Effect::kClink), sfx(Effect::kPour),
                                                              sfx(Effect::kShake), sfx(Effect::kClick)};
  g_app.bartender.init(g_app.scene.barTopFront, barSounds);
  g_app.sceneReady = true;  // multiplayer starts when you enter (app_enter)
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

}  // namespace

// Joins the multiplayer room: only once you've entered, so others don't
// see you standing at the door while your bar is still loading.
void joinRoom() {
  if (g_app.joined || !g_app.sceneReady) return;
  g_app.joined = true;
  g_app.others.start();
  g_app.shared.start(g_app.others.online(), g_app.physics.bodyCount(), g_app.scene.mirrors.size());
  EM_ASM({ if (window['bar3dVoice'] && window['bar3dVoice']['entered']) window['bar3dVoice']['entered'](); });
  g_app.piano.prepareSamples();  // note samples in background jobs
  g_app.race.prepare();          // the race track, so sitting in a rig is instant
}

// Called by the loading screen when you tap "enter": the intro title and
// controls hint start now, not while you were waiting.
extern "C" EMSCRIPTEN_KEEPALIVE void app_enter() {
  if (g_app.entered) return;
  g_app.entered = true;
  g_app.introEndMs = engine::clockMs() + kIntroMaxMs;
  engine::profiler().entered(engine::clockMs());
  joinRoom();
}

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  g_app.gpu.init("#canvas", onGpuReady);
  return 0;
}
