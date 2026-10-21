// The bar app: walk around in first person, use things, play music.
// Left thumb = joystick to walk, right thumb = drag to look around, tap to
// use things. The game itself is bar/game.cpp (the frame's phases), with
// bar/hud.cpp (on-screen text) and bar/local_player.cpp (you); drawing lives
// in bar/renderer.*; the games in their own files; the HTML menus in bar/web/.
#include <emscripten/emscripten.h>

#include "bar/app_state.h"

using namespace barapp;

// Joins the multiplayer room: only once you've entered, so others don't
// see you standing at the door while your bar is still loading.
void joinRoom() {
  if (g_app.joined || !g_app.sceneReady) return;
  g_app.joined = true;
  g_app.others.start();
  g_app.shared.start(g_app.others.online(), g_app.physics.bodyCount());
  engine::ui::entered();
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
