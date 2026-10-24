#pragma once
// Studio mode (docs/archive/STUDIO.md S2): the bar's own engine inside the Studio
// page (studio/index.html frames /bar/?studio=1). No room, no HUD, no body:
// a free camera, and a tap picks a thing instead of using it. The page talks
// to it with bar3dBridge.command("st.*", a, b, text) and hears back through
// the "studio" page operation (bar/web/bridge.js → window.bar3dStudioSink).
#include <vector>

#include "bar/app_state.h"
#include "looped/ui.h"

namespace barapp {

bool studioOn();
// Once the scene is built: tells the page what's in the world.
void studioStart();
// Each frame, instead of walking and using things.
void studioInput(float dt);
// A command from the Studio page ("st.select", "st.cam", ...). True if it was one.
bool studioCommand(const engine::ui::Command& c);
// After the world has moved this frame: the selection's glow, the X-ray boxes.
void studioPresent(std::vector<app::SceneRenderer::Box>& boxes, double nowMs);

}  // namespace barapp
