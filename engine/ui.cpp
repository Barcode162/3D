#include "engine/ui.h"

#include <emscripten/emscripten.h>

#include <cstdlib>

#include "engine/bridge.h"
#include "engine/json.h"
#include "engine/log.h"

namespace engine::ui {

void takeCommands(std::vector<Command>& out) {
  // Collected by the page mailbox at the start of the frame (engine/bridge.h).
  const std::string& text = bridge::uiCommandsJson();
  if (text.empty()) return;
  std::string error;
  const Json list = Json::parse(text, &error);
  if (!error.empty()) {
    LOG_ERROR("Menu commands: bad JSON (%s).", error.c_str());
    return;
  }
  for (const Json& c : list.items()) {
    if (!c.isArray() || !c[0].isString()) continue;
    out.push_back({c[0].string(), c[1].number(), c[2].number(), c[3].string(), c[4].string()});
  }
}

void setInfo(const std::string& jsonObject) { bridge::op("[\"info\"," + jsonObject + "]"); }

void toast(const std::string& text) { bridge::op("[\"toast\"," + jsonQuote(text) + "]"); }

void openPanel(const std::string& name) { bridge::op("[\"panel\"," + jsonQuote(name) + "]"); }

void setBodyClass(const std::string& name, bool on) {
  bridge::op("[\"bodyClass\"," + jsonQuote(name) + "," + (on ? "1" : "0") + "]");
}

std::string buildLabel() {
  static std::string label;  // read once
  if (label.empty()) {
    char buffer[64] = {};
    EM_ASM({
      var b = document.getElementById('build');
      stringToUTF8(b ? b.textContent : "", $0, $1);
    }, buffer, sizeof(buffer));
    label = buffer;
  }
  return label;
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

void remember(const char* key, double value) {
  EM_ASM({
    try { window['localStorage']['setItem']('bar3d.game.' + UTF8ToString($0), String($1)); } catch (e) {}
  }, key, value);
}

double remembered(const char* key, double fallback) {
  return EM_ASM_DOUBLE({
    try {
      var v = window['localStorage']['getItem']('bar3d.game.' + UTF8ToString($0));
      if (v !== null && isFinite(+v)) return +v;
    } catch (e) {}
    return $1;
  }, key, fallback);
}

}  // namespace engine::settings
