#include "app/bartender.h"

#include <cmath>
#include <cstdio>

#include "engine/log.h"

using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

constexpr int kSips = 4;
constexpr double kSipMs = 2200;
constexpr double kStepMs = 900;   // a step's animation and sound
constexpr double kShakeMs = 1600;

struct Rect {
  float x, y, w, h;
  bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};
Rect nextButton(float W, float H) { return {W * 0.15f, H * 0.7f, W * 0.7f, H * 0.08f}; }
Rect cancelButton(float W, float H) { return {W * 0.3f, H * 0.8f, W * 0.4f, H * 0.05f}; }

// Which sound a step makes, from its words.
int sfxFor(const char* step) {
  const std::string s = step;
  if (s.find("Shake") != std::string::npos) return Bartender::kShakeSfx;
  if (s.find("Pour") != std::string::npos || s.find("Pull") != std::string::npos || s.find("Top") != std::string::npos ||
      s.find("Brew") != std::string::npos)
    return Bartender::kPourSfx;
  if (s.find("glass") != std::string::npos || s.find("Glass") != std::string::npos || s.find("Ice") != std::string::npos ||
      s.find("cup") != std::string::npos)
    return Bartender::kClinkSfx;
  return Bartender::kClickSfx;
}

}  // namespace

const std::vector<DrinkRecipe>& drinkRecipes() {
  static const std::vector<DrinkRecipe> r = {
      {"Beer", {0.95f, 0.62f, 0.12f}, {"Grab a pint glass", "Pull the tap", "Let the foam settle"}},
      {"Old Fashioned", {0.72f, 0.33f, 0.08f},
       {"Grab a rocks glass", "Sugar and bitters", "Muddle", "Ice", "Pour the whiskey", "Stir", "Orange peel"}},
      {"Martini", {0.88f, 0.9f, 0.78f}, {"Chill a martini glass", "Ice in the mixing glass", "Pour the gin",
                                         "Pour the vermouth", "Stir", "Strain", "Add an olive"}},
      {"Mojito", {0.78f, 0.95f, 0.72f}, {"Grab a highball glass", "Mint and lime", "Muddle", "Pour the rum", "Ice",
                                         "Top with soda", "Mint sprig"}},
      {"Margarita", {0.85f, 0.95f, 0.5f}, {"Salt the rim of a glass", "Pour the tequila", "Squeeze a lime",
                                           "Pour the triple sec", "Shake with ice", "Strain"}},
      {"Cosmopolitan", {0.95f, 0.32f, 0.5f}, {"Chill a coupe glass", "Pour the vodka", "Pour the cranberry",
                                              "Squeeze a lime", "Shake with ice", "Strain", "Lime twist"}},
      {"Whiskey", {0.78f, 0.45f, 0.14f}, {"Grab a rocks glass", "Pour two fingers"}},
      {"Red wine", {0.5f, 0.04f, 0.12f}, {"Grab a wine glass", "Pour the wine"}},
      {"Gin & Tonic", {0.86f, 0.96f, 0.96f}, {"Grab a highball glass", "Ice", "Pour the gin", "Top with tonic",
                                              "Lime wedge"}},
      {"Coffee", {0.25f, 0.14f, 0.07f}, {"Grab a cup", "Grind the beans", "Brew the coffee"}},
  };
  return r;
}

void Bartender::init(const Vec3 spots[8], const engine::Sound* sounds[kSfxCount]) {
  for (int i = 0; i < 8; ++i) spots_[i] = spots[i];
  for (int i = 0; i < kSfxCount; ++i) sounds_[i] = sounds[i];
}

void Bartender::startShift() {
  onShift_ = true;
  message_ = "Your shift started. Pick a drink to make.";
  messageMs_ = 0;
}

void Bartender::endShift() {
  onShift_ = false;
  making_ = -1;
}

void Bartender::make(int type) {
  if (!onShift_ || type < 0 || type >= static_cast<int>(drinkRecipes().size())) return;
  if (freeSlot() < 0) {
    message_ = "The counter is full: wait until someone takes a drink.";
    return;
  }
  making_ = type;
  step_ = 0;
  stepMs_ = -1e9;
  message_ = std::string("Making: ") + drinkRecipes()[static_cast<size_t>(type)].name;
}

int Bartender::freeSlot() const {
  for (int i = 0; i < 8; ++i) {
    if (!slots_[i]) return i;
  }
  return -1;
}

bool Bartender::tap(engine::Vec2 tap, float W, float H, double nowMs, Vec3 eye) {
  if (making_ < 0) return false;
  const float x = tap.x * W, y = tap.y * H;
  if (cancelButton(W, H).contains(x, y)) {
    making_ = -1;
    message_ = "Cancelled.";
    return true;
  }
  if (!nextButton(W, H).contains(x, y)) return false;
  const double busy = shakeStep_ ? kShakeMs : kStepMs;
  if (nowMs < stepMs_ + busy) return true;  // still doing the last step
  const DrinkRecipe& r = drinkRecipes()[static_cast<size_t>(making_)];
  if (step_ >= r.steps.size()) {
    serve();
    return true;
  }
  const int sfx = sfxFor(r.steps[step_]);
  shakeStep_ = sfx == kShakeSfx;
  if (sounds_[sfx]) sounds_[sfx]->play(0.7f, &eye);
  stepMs_ = nowMs;
  ++step_;
  return true;
}

void Bartender::serve() {
  const int slot = freeSlot();
  if (slot < 0) return;
  slots_[slot] = making_ + 1;
  const double v[1] = {static_cast<double>(slots_[slot])};
  engine::net::setWorld("dk" + std::to_string(slot), v, 1);
  message_ = std::string(drinkRecipes()[static_cast<size_t>(making_)].name) + " served on the counter!";
  LOG_INFO("Bartender: served a %s at spot %d.", drinkRecipes()[static_cast<size_t>(making_)].name, slot);
  if (sounds_[kClinkSfx]) sounds_[kClinkSfx]->play(0.8f, &spots_[slot]);
  making_ = -1;
}

bool Bartender::take(int slot) {
  if (slot < 0 || slot >= 8 || !slots_[slot] || held_) return false;
  held_ = slots_[slot];
  sipsLeft_ = kSips;
  slots_[slot] = 0;
  const double v[1] = {0};
  engine::net::setWorld("dk" + std::to_string(slot), v, 1);
  return true;
}

void Bartender::sip(double nowMs) {
  if (!held_ || nowMs < sipMs_ + kSipMs) return;
  sipMs_ = nowMs;
  if (--sipsLeft_ <= 0) held_ = 0;  // finished: the empty glass goes back (crudely: it disappears)
}

bool Bartender::sipping(double nowMs, float& actionTime) const {
  actionTime = static_cast<float>((nowMs - sipMs_) / 1000.0);
  return nowMs < sipMs_ + kSipMs;
}

Activity Bartender::activity(double nowMs, float& actionTime) const {
  actionTime = static_cast<float>((nowMs - stepMs_) / 1000.0);
  if (making_ >= 0 && nowMs < stepMs_ + (shakeStep_ ? kShakeMs : kStepMs)) return Activity::kBartend;
  return Activity::kNone;
}

void Bartender::update(double nowMs, std::vector<engine::net::WorldEntry>& world) {
  (void)nowMs;
  for (const engine::net::WorldEntry& e : world) {
    if (e.key.size() != 3 || e.key[0] != 'd' || e.key[1] != 'k' || e.values.empty()) continue;
    const int slot = e.key[2] - '0';
    if (slot >= 0 && slot < 8) slots_[slot] = static_cast<int>(e.values[0]);
  }
}

void Bartender::addHeldDrink(std::vector<SceneRenderer::Box>& out, int type, Vec3 hand, float yaw) {
  if (type <= 0 || type > static_cast<int>(drinkRecipes().size())) return;
  const DrinkRecipe& r = drinkRecipes()[static_cast<size_t>(type - 1)];
  const engine::Quat q = engine::Quat::axisAngle({0, 1, 0}, -yaw);
  SceneRenderer::Box glass;
  glass.center = hand;
  glass.size = {0.07f, 0.11f, 0.07f};
  glass.rotation = q;
  glass.color = r.color;
  glass.glow = 0.15f;
  glass.roughness = 0.15f;
  out.push_back(glass);
  SceneRenderer::Box rim = glass;
  rim.center = hand + Vec3{0, 0.06f, 0};
  rim.size = {0.075f, 0.012f, 0.075f};
  rim.color = {0.9f, 0.95f, 1.0f};
  rim.glow = 0.05f;
  out.push_back(rim);
}

void Bartender::addBoxes(std::vector<SceneRenderer::Box>& out) const {
  for (int i = 0; i < 8; ++i) {
    if (slots_[i]) addHeldDrink(out, slots_[i], spots_[i] + Vec3{0, 0.056f, 0}, 0);
  }
}

void Bartender::drawHud(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  engine::TextStyle s;
  s.align = 0.5f;
  if (onShift_ && making_ < 0) {
    text.addRect(W * 0.05f, H * 0.075f, W * 0.9f, H * 0.06f, {0.05f, 0.03f, 0.07f, 0.75f});
    s.color = {1.0f, 0.86f, 0.62f, 1};
    text.addScreen("ON SHIFT - open the menu, Bartender, to make a drink", W * 0.5f, H * 0.095f, 8 * px, s);
    if (!message_.empty()) text.addScreen(message_, W * 0.5f, H * 0.15f, 9 * px, s);
    return;
  }
  if (making_ < 0) return;
  const DrinkRecipe& r = drinkRecipes()[static_cast<size_t>(making_)];
  text.addRect(W * 0.08f, H * 0.12f, W * 0.84f, H * 0.5f, {0.05f, 0.03f, 0.07f, 0.85f});
  s.color = {1.0f, 0.42f, 0.78f, 1};
  text.addScreen(std::string("MAKING: ") + r.name, W * 0.5f, H * 0.14f, 13 * px, s);
  for (size_t i = 0; i < r.steps.size(); ++i) {
    engine::TextStyle line;
    line.color = i < step_ ? Vec4{0.5f, 1.0f, 0.55f, 1} : i == step_ ? Vec4{1, 1, 1, 1} : Vec4{0.6f, 0.6f, 0.65f, 1};
    char row[80];
    std::snprintf(row, sizeof(row), "%s %zu. %s", i < step_ ? "[x]" : "[ ]", i + 1, r.steps[i]);
    text.addScreen(row, W * 0.13f, H * 0.2f + static_cast<float>(i) * 22 * px, 9 * px, line);
  }
  const Rect b = nextButton(W, H);
  const bool busy = nowMs < stepMs_ + (shakeStep_ ? kShakeMs : kStepMs);
  text.addRect(b.x, b.y, b.w, b.h, busy ? Vec4{0.3f, 0.25f, 0.35f, 0.95f} : Vec4{1.0f, 0.42f, 0.78f, 0.95f});
  engine::TextStyle bs;
  bs.align = 0.5f;
  const std::string label = busy ? "..." : step_ >= r.steps.size() ? "SERVE IT" : std::string("TAP: ") + r.steps[step_];
  text.addScreen(label, b.x + b.w / 2, b.y + b.h / 2 - 6 * px, 10 * px, bs);
  const Rect c = cancelButton(W, H);
  text.addRect(c.x, c.y, c.w, c.h, {0.18f, 0.14f, 0.22f, 0.95f});
  text.addScreen("CANCEL", c.x + c.w / 2, c.y + c.h / 2 - 5 * px, 9 * px, bs);
}

}  // namespace app
