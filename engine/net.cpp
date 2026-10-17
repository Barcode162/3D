#include "engine/net.h"

#include <emscripten/emscripten.h>

#include <cstdlib>

#include "engine/json.h"

namespace engine::net {

// Quoted property names keep the Closure compiler from renaming them.

bool start() { return EM_ASM_INT({ return window['bar3dNet']['start']() ? 1 : 0; }); }

Status status() { return static_cast<Status>(EM_ASM_INT({ return window['bar3dNet']['status'](); })); }

std::string room() {
  char buffer[64];
  EM_ASM({ stringToUTF8(window['bar3dNet']['room'](), $0, $1); }, buffer, sizeof(buffer));
  return buffer;
}

void setState(double timeMs, const float* values, int count) {
  EM_ASM({
    var v = [];
    for (var i = 0; i < $2; i++) v.push(Math.round(HEAPF32[($1 >> 2) + i] * 1000) / 1000);
    window['bar3dNet']['setState']($0, v);
  }, timeMs, values, count);
}

void receive(std::vector<Packet>& out) {
  constexpr int kStride = 3 + kMaxValues;
  constexpr int kCapacity = 64;
  static double buffer[kCapacity * kStride];
  const int n = EM_ASM_INT({ return window['bar3dNet']['take'](HEAPF64, $0 >> 3, $1, $2); },
                           buffer, kCapacity, kMaxValues);
  for (int i = 0; i < n; ++i) {
    const double* p = buffer + i * kStride;
    Packet packet;
    packet.player = static_cast<int>(p[0]);
    packet.time = p[1];
    packet.count = static_cast<int>(p[2]);
    for (int k = 0; k < packet.count; ++k) packet.values[k] = static_cast<float>(p[3 + k]);
    out.push_back(packet);
  }
}

bool present(int player) { return EM_ASM_INT({ return window['bar3dNet']['present']($0) ? 1 : 0; }, player); }

std::string playerName(int player) {
  char buffer[64];
  EM_ASM({ stringToUTF8(window['bar3dNet']['name']($0), $1, $2); }, player, buffer, sizeof(buffer));
  return buffer;
}

std::string playerAccount(int player) {
  char buffer[32];
  EM_ASM({ stringToUTF8(window['bar3dNet']['account']($0), $1, $2); }, player, buffer, sizeof(buffer));
  return buffer;
}

int playerAvatarRev(int player) { return EM_ASM_INT({ return window['bar3dNet']['avatarRev']($0); }, player); }

std::string avatarUrl(const std::string& account, int revision) {
  char buffer[512];
  EM_ASM({
    var a = window['bar3dAccount'];
    stringToUTF8(a ? a['avatarUrl'](UTF8ToString($0), $1) : "", $2, $3);
  }, account.c_str(), revision, buffer, sizeof(buffer));
  return buffer;
}

void showCard(const std::string& account, const std::string& name) {
  EM_ASM({
    var a = window['bar3dAccount'];
    if (a) a['showCard'](UTF8ToString($0), UTF8ToString($1));
  }, account.c_str(), name.c_str());
}

void setWorld(const std::string& key, const double* values, int count) {
  EM_ASM({
    var v = [];
    for (var i = 0; i < $2; i++) v.push(HEAPF64[($1 >> 3) + i]);
    window['bar3dNet']['setWorld'](UTF8ToString($0), v);
  }, key.c_str(), values, count);
}

void receiveWorld(std::vector<WorldEntry>& out) {
  char* text = static_cast<char*>(EM_ASM_PTR({ return stringToNewUTF8(window['bar3dNet']['takeWorld']()); }));
  const Json list = Json::parse(text);
  std::free(text);
  for (const Json& e : list.items()) {
    WorldEntry entry;
    entry.key = e[0].string();
    entry.fromLowerId = e[1].integer() != 0;
    for (const Json& v : e[2].items()) entry.values.push_back(v.number());
    out.push_back(std::move(entry));
  }
}

void sendEvent(const std::string& type, const double* values, int count) {
  EM_ASM({
    var v = [];
    for (var i = 0; i < $2; i++) v.push(Math.round(HEAPF64[($1 >> 3) + i] * 1000) / 1000);
    window['bar3dNet']['sendEvent'](UTF8ToString($0), v);
  }, type.c_str(), values, count);
}

void receiveEvents(std::vector<Event>& out) {
  char* text = static_cast<char*>(EM_ASM_PTR({ return stringToNewUTF8(window['bar3dNet']['takeEvents']()); }));
  const Json list = Json::parse(text);
  std::free(text);
  for (const Json& e : list.items()) {
    Event ev;
    ev.player = e[0].integer();
    ev.type = e[1].string();
    for (const Json& v : e[2].items()) ev.values.push_back(v.number());
    out.push_back(std::move(ev));
  }
}

bool isLower(int player) { return EM_ASM_INT({ return window['bar3dNet']['isLower']($0) ? 1 : 0; }, player) != 0; }

double serverTime() { return EM_ASM_DOUBLE({ return window['bar3dNet']['serverTime'](); }); }

}  // namespace engine::net
