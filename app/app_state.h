#pragma once
// The bar app's state and the helpers its parts share (app/main.cpp: the
// frame loop, sitting, using things, the room; app/hud.cpp: on-screen text;
// app/local_player.cpp: your camera, walking and what you hold).
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <algorithm>
#include <cctype>
#include <map>
#include <cstdio>
#include <cstring>
#include <vector>
#include "app/activity.h"
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
#include "engine/quality.h"
#include "app/test_hooks.h"
#include "app/chess.h"
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

namespace barapp {


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
  int mirrorSetting = 0;  // Settings → Graphics → Mirror (0 full, 1 simple)
  engine::QualityController quality;  // Settings → Graphics (how many pixels to draw)
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
  app::ChessGame chess;    // the lounge's chessboard
  // The avatar customizer: your choices (when `customLook`), the looks built
  // so far (by AvatarStyle::key, as look indexes after the presets), and
  // everyone else's choices by room slot.
  app::AvatarStyle style;
  bool customLook = false;
  int selfCustomLook = -1;  // look index of your custom mesh (rebuilt in place)
  std::map<int, app::AvatarStyle> slotStyles;
  struct SlotLook { std::string key; int index; };
  std::map<int, SlotLook> slotLooks;  // a mesh per room slot, rebuilt when its style changes
  int styleSentSlot = -1;
  bool avatarPreview = false;  // the customizer is open: the camera shows you from the front
  Vec3 previewEye;
  float previewYaw = 0;
  float previewStartYaw = 0;
  float previewSpin = 0;  // dragging turns your body (the camera stays)
  bool previewAimed = false;  // turned toward the open side yet
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
  app::ActivityManager act;  // what you're doing (app/activity.h)
  int seat = -1;               // the seat you're on
  int seatClaim = 0;           // the room's answer to "may I sit here?" (engine::net::claim)
  bool meridianOpen = false;   // Meridian (web/meridian/) covers the screen: the bar stops drawing
  std::string machineKey;      // the machine you're using (darts, 4K Mania), claimed in the room
  int machineClaim = 0;
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
  int nextBook = 0;                 // the bookshelf's next title
  std::vector<int> mirrorButtonsShown;  // per mirror: the mode its buttons show
  size_t infoSongCount = SIZE_MAX;  // piano songs the menu knows (SIZE_MAX: nothing sent yet)
  std::string infoState;            // what the menu was last told about you
};

extern App g_app;

// app/main.cpp
void showToast(const std::string& text);
void playSfx(engine::synth::Effect e, Vec3 where, float gain = 0.8f, bool share = false);
void strum(Vec3 where, bool share);
bool hasBadge(const char* badge);
const app::OtherPlayer* playerUsing(Activity a, int param);
void setActivity(Activity a);
void startEmote(Emote e);
void standUp();
void startRace(bool straightIn = false, bool pad = false);
void sitOn(int index);
float rayBoxDistance(Vec3 origin, Vec3 dir, Vec3 boxMin, Vec3 boxMax);
Vec3 screenRay(engine::Vec2 tap);
int personAt(Vec3 dir, float* dist);
int seatHit(Vec3 dir, float reach, float* dist = nullptr);
int buildStyle(const app::AvatarStyle& st, int index);
int lookForSlot(int slot);
void applyStyle(const app::AvatarStyle& st, bool remember);
void setLights(bool dim, bool fire);
bool claimMachine(const char* key, const char* what);
void use(const app::Interactable& it);
void updateMirrorButtons();
void updatePhysics(float dt);
void setGlow(int index, float glow);
bool available(const app::Interactable& it);
void handleCommands();
void sendInfo();
void handleShared(std::vector<engine::net::WorldEntry>& world, std::vector<engine::net::Event>& events);
void updateInteraction(double nowMs, float dt);
void selfState(float extra[kCount - app::slot::kActivity]);
Mat4 otherTransform(const app::OtherPlayer& o, app::AvatarMotion& motion);
void updateVoice(double nowMs);
void applyTestHooks();
void updateLoadingScreen();
void frame(double rafMs);
void applyTestCamera();
void makeSounds();
void updateArcadeMusic(float dt);
void onGpuReady(bool ok);

}  // namespace barapp
