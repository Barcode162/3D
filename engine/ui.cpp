#include "engine/ui.h"

#include <emscripten/emscripten.h>

#include <cstdlib>

#include "engine/bridge.h"
#include "engine/json.h"

namespace engine::ui {

void takeCommands(std::vector<Command>& out) {
  // Collected by the page mailbox at the start of the frame (engine/bridge.h).
  const std::string& text = bridge::uiCommandsJson();
  if (text.size() <= 2) return;  // "[]"
  const Json list = Json::parse(text);
  for (const Json& c : list.items()) out.push_back({c[0].string(), c[1].number(), c[2].number(), c[3].string()});
}

void setInfo(const std::string& json) { bridge::op("[\"info\"," + bridge::quote(json) + "]"); }

void toast(const std::string& text) {
  EM_ASM({ var u = window['bar3dUi']; if (u) u['toast'](UTF8ToString($0)); }, text.c_str());
}

void openPanel(const std::string& name) {
  EM_ASM({ var u = window['bar3dUi']; if (u) u['open'](UTF8ToString($0)); }, name.c_str());
}

}  // namespace engine::ui

namespace engine::settings {

int version() {
  if (bridge::frame().pulledMs > 0) return bridge::frame().settingsVersion;  // from the mailbox
  return EM_ASM_INT({ var s = window['bar3dSettings']; return s ? s['version']() : 0; });
}

double number(const char* key, double fallback) {
  return EM_ASM_DOUBLE({
    var s = window['bar3dSettings'];
    var v = s ? s['get'](UTF8ToString($0)) : undefined;
    return typeof v === 'number' ? v : (v === true ? 1 : v === false ? 0 : $1);
  }, key, fallback);
}

}  // namespace engine::settings
