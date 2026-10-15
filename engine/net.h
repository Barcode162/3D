#pragma once
// Multiplayer messaging through a relay server (the small bridge lives in
// web/net.js, the server in server/relay.php). Each player sends a few
// numbers (their "state") about 10 times a second; everyone else's states
// arrive here. What the numbers mean is up to the app.
#include <string>
#include <vector>

namespace engine::net {

enum class Status { kOff, kConnecting, kConnected, kFailing };

constexpr int kMaxValues = 8;  // numbers per state

// One state received from another player. `time` is that player's clock in
// milliseconds when they sent it (not comparable with ours directly).
struct Packet {
  int player = 0;  // stable small number per player while they're here
  double time = 0;
  int count = 0;
  float values[kMaxValues] = {};
};

// Starts sending and receiving; false if multiplayer is off for this page.
bool start();
Status status();
std::string room();

// Your latest state (sent on the next tick). `timeMs` = your clock now.
void setState(double timeMs, const float* values, int count);

// New states since the last call, oldest first.
void receive(std::vector<Packet>& out);

bool present(int player);  // still in the room?
std::string playerName(int player);

}  // namespace engine::net
