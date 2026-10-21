// What the frame shows, worked out right before drawing (RESTRUCTURE 4.1c):
// the moving shapes (piano lights, darts, drinks) and every avatar's pose,
// yours and the other players' (seated, on a machine, walking). Moved out of
// game.cpp so the frame's phases stay short.
#include "bar/presentation.h"

namespace barapp {

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

// Moving shapes (piano lights, darts, drinks) and every avatar's pose.
void presentScene(double nowMs) {
  const engine::FirstPersonCamera& cam = g_app.camera;
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
}

void presentAvatars(double nowMs) {
  const engine::FirstPersonCamera& cam = g_app.camera;
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
    int look = o.look;
    if (look == app::kCustomLook) {  // their own style, sent as world key a<slot>
      look = lookForSlot(engine::net::slotOfPlayer(o.id));
    }
    g_app.otherLooks.push_back(look);
    const int drink = static_cast<int>(o.values[app::slot::kHeldDrink]);
    if (drink > 0) {
      const Vec3 hand = o.eye + Vec3{std::cos(o.yaw) * 0.22f + std::sin(o.yaw) * 0.25f, -0.55f,
                                     std::sin(o.yaw) * 0.22f - std::cos(o.yaw) * 0.25f};
      app::Bartender::addHeldDrink(g_app.boxes, drink, hand, o.yaw);
    }
  }
  g_app.otherBones.resize(g_app.otherTransforms.size());
  g_app.renderer.setOthers(g_app.otherTransforms, g_app.otherBones, g_app.otherLooks);
  // Earned credits: the coin pops out in front of you (?coin = once at start, for tests).
  if (g_app.darts.payouts() != g_app.coinPayoutsSeen || (g_app.hooks.coin && !g_app.coinShownForTest && g_app.entered)) {
    g_app.coinPayoutsSeen = g_app.darts.payouts();
    g_app.coinShownForTest = true;
    g_app.coinPop.start(nowMs, cam.position, cam.flatForward());
  }
  g_app.coinPop.update(nowMs, g_app.renderer, g_app.scene.coinModel);
  {  // The bar's world: every thing plays its loop (bar/world_bar.h).
    app::BarWorld::Inputs in;
    in.nowMs = nowMs;
    in.eye = cam.position;
    in.songSeconds = g_app.jukebox.songTime();
    if (const app::Song* s = g_app.jukebox.currentSong()) in.songUrl = s->url;
    g_app.world.update(in, g_app.boxes, g_app.renderer);
    g_app.lightsDim = g_app.world.stateOf("light-switch") == "dim";
    g_app.fireOn = g_app.world.stateOf("fire_flames") != "out";
  }
  g_app.renderer.setBoxes(g_app.boxes);
  g_app.renderer.streamRooms(cam.position, nowMs);  // nearby rooms' models
}

}  // namespace barapp
