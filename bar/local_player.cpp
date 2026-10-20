#include "bar/local_player.h"

namespace barapp {

// ---- Camera and movement ----

void updateCamera(float dt) {
  // A menu or account panel covers the game: no looking or walking meanwhile.
  const bool covered = engine::bridge::frame().modalOpen;
  const engine::Vec2 look = covered && !g_app.avatarPreview ? engine::Vec2{} : g_app.input.frame().look;
  const bool menu = g_app.jukeboxMenu.isOpen() || g_app.rhythm.isOpen() || g_app.race.open();
  const float sens = kLookSensitivity * g_app.lookSpeed;
  if (!menu && !g_app.piano.keyboardOpen()) g_app.camera.look(look.x * sens, -look.y * sens * g_app.invertLook);
  const engine::Vec2 move = covered ? engine::Vec2{} : g_app.input.frame().move;
  if (look.x != 0 || look.y != 0 || move.x != 0 || move.y != 0) {
    if (g_app.entered) g_app.introEndMs = std::min(g_app.introEndMs, engine::frameTime().nowMs + kIntroAfterInputMs);
  }
  if (g_app.avatarPreview) {
    // The customizer is open: the camera stands 1.7 m in front of you and
    // looks back; dragging turns your body.
    // The panel covers the lower half, so you're framed in the top half:
    // up to 3.2 m away (closer if something is in the way). On opening you
    // turn toward the most open side. Dragging spins your body.
    const auto room = [&](float yaw) {
      const Vec3 f{std::sin(yaw), 0, -std::cos(yaw)};
      float free = 0.5f;
      for (float d = 0.6f; d <= 3.2f; d += 0.1f) {
        const Vec3 p{g_app.previewEye.x + f.x * d, g_app.previewEye.y - 0.7f, g_app.previewEye.z + f.z * d};
        if (engine::length(app::resolveCollisions(g_app.scene, p, 0.12f) - p) > 0.01f) break;
        free = d;
      }
      return free;
    };
    if (!g_app.previewAimed) {
      g_app.previewAimed = true;
      float best = -1, bestYaw = g_app.previewYaw;
      for (int k = 0; k < 16; ++k) {
        const float yaw = g_app.previewYaw + static_cast<float>(k) * engine::kPi / 8;
        const float r = room(yaw) - (k == 0 ? 0 : 0.05f);  // ties keep the way you face
        if (r > best) best = r, bestYaw = yaw;
      }
      g_app.previewYaw = bestYaw;
      g_app.previewSpin = engine::radians(g_app.hooks.previewTurn);
    }
    g_app.previewSpin += look.x * sens;
    const float dist = room(g_app.previewYaw);
    const Vec3 fwd{std::sin(g_app.previewYaw), 0, -std::cos(g_app.previewYaw)};
    const float feet = g_app.previewEye.y - 1.66f, camY = 1.15f;
    g_app.camera.position = Vec3{g_app.previewEye.x, feet + camY, g_app.previewEye.z} + fwd * dist;
    g_app.camera.yaw = g_app.previewYaw + engine::kPi;
    // The top of your head just under the top of the screen.
    g_app.camera.pitch = std::atan((1.85f - camY) / dist) - g_app.camera.fovY * 0.5f * 0.88f;
    app::advanceWalk(g_app.avatarMotion, 0, 0, dt, kWalkSpeed);
    return;
  }
  if (g_app.chess.isOpen()) {  // looking down at the chessboard
    g_app.chess.camera(g_app.camera);
    app::advanceWalk(g_app.avatarMotion, 0, 0, dt, kWalkSpeed);
    return;
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
    if (g_app.act.current() == Activity::kEmote) setActivity(Activity::kNone);
    if (g_app.darts.playing() && engine::length(g_app.camera.position - g_app.scene.dartOche) > 0.6f) {
      g_app.darts.stop();
      setActivity(Activity::kNone);
      showToast("You left the darts.");
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
// The body a tap could pick up (within reach, not behind a wall, not in
// someone else's hand), or -1; `dist` = how far along the ray.
int pickableAt(Vec3 dir, float* dist) {
  const Vec3 eye = g_app.camera.position;
  float d;
  const int body = g_app.physics.raycastBodies(eye, dir, kGrabReach, &d);
  if (body < 0) return -1;
  float wallDist;
  if (g_app.physics.raycastStatic(eye, dir, d, &wallDist, nullptr)) return -1;
  if (!g_app.shared.canPickUp(body)) return -1;  // in someone else's hand
  if (dist) *dist = d;
  return body;
}

void pickUp(int body) {
  engine::Body& b = g_app.physics.body(static_cast<size_t>(body));
  b.held = true;
  b.kinematic = false;
  b.asleep = false;
  g_app.held = body;
  g_app.shared.pickedUp(body);
}

// Moves you to a spot (the darts oche, the bartender's place), facing `yaw`.
void teleport(Vec3 eye, float yaw) {
  g_app.camera.position = eye;
  g_app.camera.yaw = yaw;
  g_app.camera.pitch = 0;
}

// Your avatar's activity this frame (and what it means for the pose).
void updateSelfMotion(double nowMs) {
  app::AvatarMotion& m = g_app.avatarMotion;
  // Emotes end by themselves.
  if (g_app.act.current() == Activity::kEmote &&
      (nowMs - g_app.act.sinceMs()) / 1000.0 > app::emoteLength(g_app.emote)) {
    setActivity(Activity::kNone);
  }
  Activity shown = g_app.act.current();
  float actionTime = static_cast<float>((nowMs - g_app.act.sinceMs()) / 1000.0);
  float t;
  if (g_app.bartender.activity(nowMs, t) == Activity::kBartend) {
    shown = Activity::kBartend;
    actionTime = t;
  } else if (g_app.act.current() == Activity::kNone && g_app.bartender.held() && g_app.bartender.sipping(nowMs, t)) {
    shown = Activity::kDrink;
    actionTime = t;
  }
  // Seated with a drink: the sip goes on the pose layer (you stay seated).
  m.pose = Activity::kNone;
  m.poseTime = 0;
  if (g_app.act.current() == Activity::kSit && g_app.bartender.held() && g_app.bartender.sipping(nowMs, t)) {
    m.pose = Activity::kDrink;
    m.poseTime = t;
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
  if (g_app.hooks.walkPose >= 0) {  // tests: a frozen walking pose
    m.walkAmount = 1;
    m.walkPhase = engine::radians(g_app.hooks.walkPose);
    m.walkDir = engine::radians(g_app.hooks.walkDir);
    m.walkCycle = 1.3f - 0.5f * std::fabs(std::sin(m.walkDir));
  }
}

}  // namespace barapp
