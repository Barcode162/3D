#pragma once
// The menus are HTML panels over the game (web/menu.js): the emote wheel,
// avatar picker, settings, piano song list... They talk to C++ in two ways:
// - commands: the menu queues short messages ("emote", 3), C++ takes them
//   each frame;
// - settings: values the player chose (look speed, volumes...), kept in the
//   browser and read here when they change.
#include <string>
#include <vector>

namespace engine::ui {

struct Command {
  std::string name;
  double a = 0, b = 0;
  std::string text;
};
void takeCommands(std::vector<Command>& out);

// Tells the menu what's going on (as JSON), e.g. which look you wear.
void setInfo(const std::string& json);
// Shows a short message at the top of the screen (HTML toast).
void toast(const std::string& text);
// Opens a menu panel by name ("emotes", "piano", "bartender"...).
void openPanel(const std::string& name);

}  // namespace engine::ui

namespace engine::settings {
// Changes every time the player changes a setting.
int version();
double number(const char* key, double fallback);
}  // namespace engine::settings
