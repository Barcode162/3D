// The bar app: walk around in first person, use things, play music.
// Left thumb = joystick to walk, right thumb = drag to look around, tap to
// use things. Drawing lives in app/renderer.*.
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/bar_scene.h"
#include "app/avatar.h"
#include "app/jukebox.h"
#include "app/players.h"
#include "app/props.h"
#include "app/shared_world.h"
#include "app/renderer.h"
#include "engine/audio.h"
#include "engine/camera.h"
#include "engine/file.h"
#include "engine/gpu.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/mesh.h"
#include "engine/texture.h"
#include "engine/text.h"

using engine::Mat4;
using engine::Vec3;

namespace {

constexpr float kWalkSpeed = 2.2f;           // meters per second
constexpr float kLookSensitivity = 0.0055f;  // radians per CSS pixel
constexpr float kBodyRadius = 0.3f;          // meters
constexpr float kLabelShowDistance = 6.0f;   // name tags appear within this range
constexpr float kPromptDistance = 2.2f;      // ...and their prompt this close
constexpr float kTapReach = 4.0f;            // how far away a tap can use things
constexpr float kHighlightGlow = 0.35f;      // extra glow on the thing you're at
constexpr double kBannerMs = 7000;           // "now playing" banner duration
constexpr double kIntroMaxMs = 12000;        // title + controls hint on start...
constexpr double kIntroAfterInputMs = 1500;  // ...until shortly after you first move
constexpr float kGrabReach = 2.2f;           // how far away you can pick things up
constexpr float kPlaceReach = 2.0f;          // ...and put them down
constexpr float kThrowSpeed = 4.0f;          // meters per second

struct App {
  engine::Gpu gpu;
  engine::Input input;
  engine::FirstPersonCamera camera;
  app::BarScene scene;
  app::SceneRenderer renderer;
  app::Avatar avatar;
  app::AvatarMotion avatarMotion;
  std::vector<Mat4> avatarBones, firstPersonBones;
  app::OtherPlayers others;  // multiplayer
  app::SharedWorld shared;   // glasses, jukebox and mirrors, the same for everyone
  std::vector<Mat4> otherTransforms;
  std::vector<std::vector<Mat4>> otherBones;
  app::Jukebox jukebox;
  app::JukeboxMenu jukeboxMenu;
  engine::TextRenderer text;
  engine::PhysicsWorld physics;
  std::vector<int> bodyOfModel;  // physics body per BarScene::models entry
  std::vector<engine::Vec3> shownPosition;  // per body, where its model was last drawn
  std::vector<engine::Quat> shownRotation;
  int held = -1;                 // body in your hand
  int aimed = -1;                // body in the middle of the screen, within reach

  int focused = -1;      // interactable you're standing at and facing
  int highlighted = -1;  // interactable currently drawn glowing
  int bannerSong = 0;
  double bannerEndMs = 0;

  double introEndMs = 0;
  double lastFrameMs = 0;
  double fpsWindowStartMs = 0;
  int fpsFrames = 0;
};

App g_app;

void updateCamera(float dt) {
  const engine::Vec2 look = g_app.input.takeLookDelta();
  if (g_app.jukeboxMenu.isOpen()) {  // stand still while choosing a song
    app::advanceWalk(g_app.avatarMotion, 0, 0, dt, kWalkSpeed);
    return;
  }
  g_app.camera.look(look.x * kLookSensitivity, -look.y * kLookSensitivity);

  const engine::Vec2 move = g_app.input.move();
  if (look.x != 0 || look.y != 0 || move.x != 0 || move.y != 0) {
    g_app.introEndMs = std::min(g_app.introEndMs, emscripten_get_now() + kIntroAfterInputMs);
  }
  Vec3 p = g_app.camera.position;
  p += g_app.camera.flatForward() * (move.y * kWalkSpeed * dt);
  p += g_app.camera.flatRight() * (move.x * kWalkSpeed * dt);
  const Vec3 before = g_app.camera.position;
  g_app.camera.position = app::resolveCollisions(g_app.scene, p, kBodyRadius);
  // Drive the walk animation by how far you actually moved (walls stop it).
  const Vec3 moved = g_app.camera.position - before;
  app::advanceWalk(g_app.avatarMotion, engine::dot(moved, g_app.camera.flatForward()),
                   engine::dot(moved, g_app.camera.flatRight()), dt, kWalkSpeed);
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
  if (it.action == app::Action::kJukebox) return g_app.jukebox.prompt();
  if (it.action == app::Action::kMirror) {
    const app::Mirror& m = g_app.scene.mirrors[static_cast<size_t>(it.mirror)];
    const char* kind = m.mode == app::MirrorMode::kFull ? "Mirror" : "Avatar mirror";
    return std::string(kind) + (m.on ? "\nTap to turn off" : "\nTap to turn on");
  }
  return it.prompt;
}

// Queues this frame's text: name tags over things in the bar, the prompt
// for whatever you're standing at, and the intro title.
void queueText(double nowMs, const Mat4& viewProj) {
  engine::TextRenderer& text = g_app.text;
  const float W = static_cast<float>(g_app.gpu.width), H = static_cast<float>(g_app.gpu.height);
  const float px = g_app.gpu.pixelRatio;  // device pixels per CSS pixel
  text.begin(viewProj, g_app.gpu.width, g_app.gpu.height);

  const engine::FirstPersonCamera& cam = g_app.camera;
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  const Vec3 up = engine::cross(right, forward);
  for (size_t i = 0; i < g_app.scene.interactables.size(); ++i) {
    if (g_app.jukeboxMenu.isOpen()) break;  // keep the menu uncluttered
    if (static_cast<int>(i) == g_app.focused) continue;  // its prompt says what it is
    const app::Interactable& it = g_app.scene.interactables[i];
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
  // Other players' names float over their heads.
  for (const app::OtherPlayer& o : g_app.others.players()) {
    if (!o.placed) continue;
    engine::TextStyle style;
    style.align = 0.5f;
    style.color = {0.75f, 0.9f, 1.0f, 1.0f};
    style.outlineWidth = 0.8f;
    text.addWorld(o.name, o.eye + Vec3{0, 0.32f, 0}, right, up, 0.07f, style);
  }
  if (!g_app.jukeboxMenu.isOpen() && (g_app.held >= 0 || g_app.aimed >= 0)) {
    engine::TextStyle style;
    style.align = 0.5f;
    style.outlineWidth = 0.8f;
    const char* prompt = g_app.held >= 0 ? "Tap a table or the bar: put it down\nTap anywhere else: throw"
                                         : "Glass\nTap: pick up";
    text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, style);
  } else if (g_app.focused >= 0 && !g_app.jukeboxMenu.isOpen()) {
    const app::Interactable& it = g_app.scene.interactables[static_cast<size_t>(g_app.focused)];
    engine::TextStyle style;
    style.align = 0.5f;
    style.outlineWidth = 0.8f;
    const std::string prompt = promptFor(it);
    text.addScreen(prompt, W * 0.5f, H * 0.62f, 14 * px, style);
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

  if (nowMs < g_app.introEndMs) {
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
    text.addScreen("Left thumb: walk\nRight thumb: look around\nTap things to use them", W * 0.5f, H * 0.2f + 52 * px, 11 * px, hint);
  }
  g_app.jukeboxMenu.draw(text, g_app.jukebox, W, H, px);  // last, so it's on top
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

void use(const app::Interactable& it) {
  if (it.action == app::Action::kJukebox) g_app.jukeboxMenu.open();
  if (it.action == app::Action::kMirror) {
    app::Mirror& m = g_app.scene.mirrors[static_cast<size_t>(it.mirror)];
    m.on = !m.on;
    g_app.shared.mirrorToggled(it.mirror, m.on);
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
    // Stand it upright on the spot, facing the same way.
    b.position = eye + dir * dist + Vec3{0, 0.002f, 0};
    b.rotation = {};
    b.velocity = {};
    b.angularVelocity = {};
    LOG_INFO("Put down at %.2f, %.2f, %.2f.", b.position.x, b.position.y, b.position.z);
  } else {
    b.velocity = dir * kThrowSpeed + Vec3{0, 1.0f, 0};
    b.angularVelocity = {0, 0, 0};
    LOG_INFO("Thrown from %.2f, %.2f, %.2f.", b.position.x, b.position.y, b.position.z);
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
  g_app.shared.update(emscripten_get_now(), dt, g_app.physics, g_app.scene, g_app.jukebox, g_app.held);
  g_app.physics.step(dt);
  g_app.physics.takeImpacts();  // sounds come later (W5)
  for (size_t i = 0; i < g_app.bodyOfModel.size(); ++i) {
    const int body = g_app.bodyOfModel[i];
    if (body < 0) continue;
    const engine::Body& b = g_app.physics.body(static_cast<size_t>(body));
    engine::Vec3& shownP = g_app.shownPosition[static_cast<size_t>(body)];
    engine::Quat& shownQ = g_app.shownRotation[static_cast<size_t>(body)];
    if (shownP.x == b.position.x && shownP.y == b.position.y && shownP.z == b.position.z && shownQ.x == b.rotation.x && shownQ.y == b.rotation.y && shownQ.z == b.rotation.z &&
        shownQ.w == b.rotation.w) {
      continue;
    }
    shownP = b.position;
    shownQ = b.rotation;
    g_app.renderer.setModelTransform(i, b.position, b.rotation);
  }
}

// Makes one interactable's parts glow (or stop glowing).
void setGlow(int index, float glow) {
  if (index >= 0) g_app.renderer.setGlow(g_app.scene.interactables[static_cast<size_t>(index)], glow);
}


// Works out what you're standing at, handles taps, and keeps the music and
// your "ears" in sync with the camera.
void updateInteraction(double nowMs) {
  const engine::FirstPersonCamera& cam = g_app.camera;
  const auto& items = g_app.scene.interactables;

  g_app.focused = -1;
  float bestFacing = 0.8f;  // must be looking roughly toward it
  for (size_t i = 0; i < items.size(); ++i) {
    const Vec3 center = (items[i].boundsMin + items[i].boundsMax) * 0.5f;
    const Vec3 toFlat{center.x - cam.position.x, 0, center.z - cam.position.z};
    const float facing = engine::dot(engine::normalize(toFlat), cam.flatForward());
    if (engine::length(toFlat) < kPromptDistance && facing > bestFacing) {
      bestFacing = facing;
      g_app.focused = static_cast<int>(i);
    }
  }
  if (g_app.focused != g_app.highlighted) {
    setGlow(g_app.highlighted, 0);
    setGlow(g_app.focused, kHighlightGlow);
    g_app.highlighted = g_app.focused;
  }

  // Something you could pick up, right where you're looking?
  g_app.aimed = -1;
  if (g_app.held < 0) {
    float dist;
    const int body = g_app.physics.raycastBodies(cam.position, cam.forward(), kGrabReach, &dist);
    float wallDist;
    if (body >= 0 && !(g_app.physics.raycastStatic(cam.position, cam.forward(), dist, &wallDist, nullptr))) {
      g_app.aimed = body;
    }
  }

  // A tap uses what it lands on; tapping anywhere also works while you're
  // standing at something (easier on a small screen).
  for (const engine::Vec2 tap : g_app.input.takeTaps()) {
    g_app.introEndMs = std::min(g_app.introEndMs, nowMs + kIntroAfterInputMs);
    if (g_app.jukeboxMenu.isOpen()) {
      g_app.jukeboxMenu.tap(tap, g_app.jukebox);
      continue;
    }
    const Vec3 dir = screenRay(tap);
    if (g_app.held >= 0) {
      putDownOrThrow(dir);
      continue;
    }
    if (tryPickUp(dir)) continue;
    int hit = -1;
    float nearest = kTapReach;
    for (size_t i = 0; i < items.size(); ++i) {
      const float t = rayBoxDistance(cam.position, dir, items[i].boundsMin, items[i].boundsMax);
      if (t >= 0 && t < nearest) {
        nearest = t;
        hit = static_cast<int>(i);
      }
    }
    if (hit < 0) hit = g_app.focused;
    if (hit >= 0) use(items[static_cast<size_t>(hit)]);
  }

  g_app.jukebox.update();
  if (g_app.jukebox.songsStarted() != g_app.bannerSong) {
    g_app.bannerSong = g_app.jukebox.songsStarted();
    g_app.bannerEndMs = nowMs + kBannerMs;
  }
  const Vec3 forward = cam.forward();
  const Vec3 right = engine::normalize(engine::cross(forward, {0, 1, 0}));
  engine::audio::setListener(cam.position, forward, engine::cross(right, forward));
}

void frame() {
  const double nowMs = emscripten_get_now();
  // Clamp the time step so a stall (e.g. switching apps) doesn't teleport you.
  const float dt = std::min(static_cast<float>((nowMs - g_app.lastFrameMs) / 1000.0), 0.1f);
  g_app.lastFrameMs = nowMs;

  g_app.gpu.resizeIfNeeded();
  updateFps(nowMs);
  updateCamera(dt);
  updateInteraction(nowMs);
  updatePhysics(dt);
  g_app.others.sendSelf(nowMs, g_app.camera.position, g_app.camera.yaw, g_app.camera.pitch);
  g_app.others.update(nowMs, dt, kWalkSpeed);

  wgpu::TextureView target = g_app.gpu.beginFrame();
  if (!target) return;

  const engine::FirstPersonCamera& cam = g_app.camera;
  const float aspect = static_cast<float>(g_app.gpu.width) / static_cast<float>(g_app.gpu.height);
  const Mat4 viewProj = cam.projection(aspect) * cam.view();
  queueText(nowMs, viewProj);
  g_app.avatarMotion.pitch = cam.pitch;
  g_app.avatarMotion.time = static_cast<float>(nowMs / 1000.0);
  g_app.avatar.pose(g_app.avatarMotion, g_app.avatarBones);
  g_app.avatar.hideHead(g_app.avatarBones, g_app.firstPersonBones);
  const auto& others = g_app.others.players();
  g_app.otherTransforms.clear();
  g_app.otherBones.resize(others.size());
  for (size_t i = 0; i < others.size(); ++i) {
    const app::OtherPlayer& o = others[i];
    if (!o.placed) continue;
    g_app.avatar.pose(o.motion, g_app.otherBones[g_app.otherTransforms.size()]);
    g_app.otherTransforms.push_back(app::avatarTransform(o.eye, o.yaw, o.pitch));
  }
  g_app.otherBones.resize(g_app.otherTransforms.size());
  g_app.renderer.setOthers(g_app.otherTransforms, g_app.otherBones);
  g_app.renderer.render(target, viewProj, cam.position, app::avatarTransform(cam.position, cam.yaw, cam.pitch),
                        g_app.avatarBones, g_app.firstPersonBones, g_app.scene.mirrors, g_app.text);
}

// For testing: "?cam=x,y,z,yaw,pitch" in the page URL starts the camera
// there (yaw/pitch in degrees), so any spot can be screenshotted.
void applyTestCamera() {
  char buffer[128] = {};
  EM_ASM({
    var m = /[?&]cam=([^&]*)/.exec(location.search);
    stringToUTF8(m ? decodeURIComponent(m[1]) : "", $0, $1);
  }, buffer, sizeof(buffer));
  float x, y, z, yaw, pitch;
  if (std::sscanf(buffer, "%f,%f,%f,%f,%f", &x, &y, &z, &yaw, &pitch) != 5) return;
  g_app.camera.position = {x, y, z};
  g_app.camera.yaw = engine::radians(yaw);
  g_app.camera.pitch = engine::radians(pitch);
  LOG_INFO("Test camera at %.2f, %.2f, %.2f.", x, y, z);
}

void onGpuReady(bool ok) {
  if (!ok) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  g_app.scene = app::buildBarScene();
  app::buildPhysics(g_app.scene, g_app.physics, g_app.bodyOfModel);
  g_app.avatar.build();
  if (!g_app.renderer.init(g_app.gpu, g_app.scene, g_app.avatar.mesh()) ||
      !g_app.text.init(g_app.gpu.device, g_app.gpu.surfaceFormat, engine::Gpu::kDepthFormat)) {
    engine::setStatus("Startup failed - see log");
    return;
  }
  g_app.camera.position = g_app.scene.spawnPosition;
  g_app.camera.yaw = g_app.scene.spawnYaw;
  applyTestCamera();
  g_app.input.attach("#canvas");
  g_app.jukebox.init(g_app.scene.jukeboxSpeaker);
  g_app.others.start();
  g_app.shared.start(g_app.others.online(), g_app.physics.bodyCount(), g_app.scene.mirrors.size());
  g_app.shownPosition.assign(g_app.physics.bodyCount(), engine::Vec3{1e9f, 0, 0});
  g_app.shownRotation.assign(g_app.physics.bodyCount(), engine::Quat{});
  g_app.lastFrameMs = g_app.fpsWindowStartMs = emscripten_get_now();
  g_app.introEndMs = g_app.lastFrameMs + kIntroMaxMs;
  LOG_INFO("Running. Left thumb: walk. Right thumb: look around. Tap to use things.");
  emscripten_set_main_loop(frame, 0, false);
}

}  // namespace

int main() {
  LOG_INFO("Bar3D starting (C++ / WebAssembly).");
  g_app.gpu.init("#canvas", onGpuReady);
  return 0;
}
