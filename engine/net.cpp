#include "engine/net.h"

#include <emscripten/emscripten.h>

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

}  // namespace engine::net
