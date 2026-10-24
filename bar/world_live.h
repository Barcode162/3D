#pragma once
// The live world (docs/archive/STUDIO.md 7.6): the Studio's store says which version
// of the world is live; the game loads it before building the scene, so a
// publish reaches the bar without a new build. Anything wrong (no store on
// this site, a network error, a format newer than this engine) → the world
// built into the game. The files land in app::setWorldFile.
#include <functional>
#include <string>

namespace app {

// Starts asking the store (once). Call early: it runs while the GPU starts.
void fetchLiveWorld();
// Calls `then` once the live world is in (or given up on), or after
// `waitMs` at the latest. Then the scene can be built.
void whenLiveWorld(std::function<void()> then, double waitMs);
// The version in use (0: the world built into the game).
int liveWorldVersion();

}  // namespace app
