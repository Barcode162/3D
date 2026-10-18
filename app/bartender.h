#pragma once
// Bartending. Players with the "bartender" badge (given in the menu by the
// bar's owner) can start a shift at the station behind the bar, pick a
// drink and make it step by step (glass, ice, pour, shake, garnish...),
// each step with its sound and a crude animation. A finished drink appears
// on the counter for everyone; anyone can take it and sip it.
#include <string>
#include <vector>

#include "app/avatar.h"
#include "app/renderer.h"
#include "engine/audio.h"
#include "engine/net.h"
#include "engine/text.h"

namespace app {

struct DrinkRecipe {
  const char* name;
  engine::Vec3 color;
  std::vector<const char*> steps;
};
const std::vector<DrinkRecipe>& drinkRecipes();

class Bartender {
 public:
  enum Sfx { kClinkSfx, kPourSfx, kShakeSfx, kClickSfx, kSfxCount };
  void init(const engine::Vec3 spots[8], const engine::Sound sounds[kSfxCount]);

  bool onShift() const { return onShift_; }
  void startShift();
  void endShift();
  // Starts making drink `type` (from the menu).
  void make(int type);
  bool making() const { return making_ >= 0; }
  // A tap on the screen while making a drink; true if it was used.
  bool tap(engine::Vec2 tap, float W, float H, double nowMs, engine::Vec3 eye);
  void update(double nowMs, std::vector<engine::net::WorldEntry>& world);
  // Activity for your avatar (shaking, pouring) or kNone.
  Activity activity(double nowMs, float& actionTime) const;

  // Drinks on the counter.
  int drinkAt(int slot) const { return slots_[slot]; }  // type + 1, 0 = empty
  // Takes the drink from a slot into your hand; false if it's empty.
  bool take(int slot);
  int held() const { return held_; }  // type + 1, 0 = nothing
  void sip(double nowMs);
  bool sipping(double nowMs, float& actionTime) const;

  void addBoxes(std::vector<SceneRenderer::Box>& out) const;
  // A drink in someone's hand (yours in first person, or another player's).
  static void addHeldDrink(std::vector<SceneRenderer::Box>& out, int type, engine::Vec3 hand, float yaw);
  void drawHud(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;

 private:
  void serve();
  int freeSlot() const;

  engine::Vec3 spots_[8];
  int slots_[8] = {};
  engine::Sound sounds_[kSfxCount];
  bool onShift_ = false;
  int making_ = -1;
  size_t step_ = 0;
  double stepMs_ = -1e9;     // when the current step's action started
  bool shakeStep_ = false;
  int held_ = 0;
  int pendingTake_ = 0;  // the room's answer to "may I take this drink?" (engine::net::setWorldIf)
  int sipsLeft_ = 0;
  double sipMs_ = -1e9;
  std::string message_;
  double messageMs_ = 0;
};

}  // namespace app
