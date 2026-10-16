#pragma once
// Multiplayer messaging through a relay server (the small bridge lives in
// web/net.js, the server in server/relay.php). Each player sends a few
// numbers (their "state") about 10 times a second; everyone else's states
// arrive here. What the numbers mean is up to the app.
#include <string>
#include <vector>

namespace engine::net {

enum class Status { kOff, kConnecting, kConnected, kFailing };

constexpr int kMaxValues = 16;  // numbers per state or world entry

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
// Their account id ("" = a guest) and profile picture revision (0 = none).
std::string playerAccount(int player);
int playerAvatarRev(int player);
// Where a profile picture is downloaded from ("" = no accounts here).
std::string avatarUrl(const std::string& account, int revision);
// Opens the person card for a player (an HTML panel over the game).
void showCard(const std::string& account, const std::string& name);
// True while an account panel (login, card...) covers the game.
bool panelOpen();

// Shared world: short named entries (key = 1-8 lowercase letters/digits)
// of up to kMaxValues numbers that everyone in the room sees the same. The
// last change wins. You don't get your own changes back.
struct WorldEntry {
  std::string key;
  bool fromLowerId = false;  // writer's id sorts before ours (for tie-breaks)
  std::vector<double> values;
};
void setWorld(const std::string& key, const double* values, int count);
void receiveWorld(std::vector<WorldEntry>& out);
// The server's clock in seconds (the same for everyone), 0 = not known yet.
double serverTime();

}  // namespace engine::net
