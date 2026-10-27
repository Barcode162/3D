#include "bar/world_live.h"

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>

#include <cstdlib>
#include <vector>

#include "bar/world_files.h"
#include "looped/fetch.h"
#include "looped/json.h"
#include "looped/log.h"

namespace app {

namespace {
constexpr int kFormat = 1;  // the newest bundle format this engine reads
constexpr char kStore[] = "../studio/api.php";
bool started = false, done = false;
int version = 0;
std::vector<std::function<void()>> waiting;

void finish() {
  if (done) return;
  done = true;
  if (version == 0) LOG_INFO("World: built in.");
  std::vector<std::function<void()>> w;
  w.swap(waiting);
  for (auto& f : w) f();
}

void useBundle(const std::string& text) {
  std::string error;
  const engine::Json b = engine::Json::parse(text, &error, {64, 1 << 16});
  const int format = static_cast<int>(b["format"].number(1));
  if (!error.empty() || !b["files"].isObject()) {
    LOG_ERROR("World: the live world couldn't be read (%s); using the one built in.", error.c_str());
    return;
  }
  if (format > kFormat) {
    LOG_INFO("World: the live world needs a newer engine (format %d); using the one built in.", format);
    return;
  }
  for (const auto& [path, content] : b["files"].members()) setWorldFile(path, engine::jsonDump(content));
  version = static_cast<int>(b["version"].number(0));
  LOG_INFO("World: v%d (live, by %s).", version, b["author"].string().c_str());
}
}  // namespace

void fetchLiveWorld() {
  if (started) return;
  started = true;
  // In the Studio's frame: the Studio hands over its draft, so what you add
  // shows when the frame reloads (S6.1).
  char* draft = static_cast<char*>(EM_ASM_PTR({
    var b = "";
    try { var p = window['parent']; if (p && p !== window && p['bar3dStudioBundle']) b = String(p['bar3dStudioBundle']); } catch (e) {}
    return stringToNewUTF8(b);
  }));
  const std::string text = draft ? draft : "";
  free(draft);
  if (!text.empty()) {
    useBundle(text);
    return finish();
  }
  // A shared draft (S9.2: /bar/?draft=<code>): someone's draft to try before it's published.
  char* share = static_cast<char*>(EM_ASM_PTR({
    var m = /[?&]draft=([a-z0-9]{8})(?:&|$)/.exec(location['search']);
    return stringToNewUTF8(m ? m[1] : "");
  }));
  const std::string code = share ? share : "";
  free(share);
  if (!code.empty()) {
    engine::fetchOptional(std::string(kStore) + "?share=" + code, [](bool ok, std::vector<uint8_t> b) {
      if (ok) useBundle(std::string(b.begin(), b.end()));
      else LOG_ERROR("World: that shared draft isn't there any more; using the world built in.");
      finish();
    });
    return;
  }
  // The Studio is parked (J114, request 2026-10-27 #3): the bar plays the
  // world built into the game, so a world change ships with the game (edit
  // assets/world/, push). The store's live world only with ?liveworld=1.
  const bool useStore = EM_ASM_INT({ return /[?&]liveworld=1(?:&|$)/.test(location['search']) ? 1 : 0; }) != 0;
  if (!useStore) return finish();
  engine::fetchOptional(std::string(kStore) + "?live", [](bool ok, std::vector<uint8_t> bytes) {
    const engine::Json live = ok ? engine::Json::parse(std::string(bytes.begin(), bytes.end())) : engine::Json();
    const int v = static_cast<int>(live["version"].number(0));
    if (v < 1) return finish();
    engine::fetchOptional(std::string(kStore) + "?bundle=" + std::to_string(v), [](bool ok2, std::vector<uint8_t> b) {
      if (ok2) useBundle(std::string(b.begin(), b.end()));
      finish();
    });
  });
}

void whenLiveWorld(std::function<void()> then, double waitMs) {
  if (done) return then();
  waiting.push_back(std::move(then));
  emscripten_set_timeout([](void*) {
    if (!done) LOG_INFO("World: the store is slow; starting with the world built in.");
    finish();
  }, waitMs, nullptr);
}

int liveWorldVersion() { return version; }

}  // namespace app
