#include "bar/hud.h"

namespace barapp {

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
      // (The live 3D race on the three monitors comes in docs/archive/RACING.md step 4.)
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
  if (g_app.race.open()) return;  // the race queues its own HUD (bar/projects/racing/game.cpp)
  queueMachineScreens(nowMs);
  if (g_app.chess.isOpen()) g_app.chess.draw(text, W, H, px, nowMs);

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
  const bool busy = g_app.jukeboxMenu.isOpen() || g_app.piano.keyboardOpen() || g_app.bartender.making() ||
                    g_app.chess.isOpen();
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

}  // namespace barapp
