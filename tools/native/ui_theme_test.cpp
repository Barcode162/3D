// The engine's theme reader (looped/ui_theme.*, docs/archive/UI_PLAN.md R1.7): every
// theme file in ui/system/themes/ parses, its colours read as the CSS means
// them, and bad text never crashes it.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "looped/ui_theme.h"

using engine::Vec4;
using engine::ui::Theme;

static int g_failed = 0;
#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::printf("FAIL line %d: %s\n", __LINE__, #cond);               \
      ++g_failed;                                                       \
    }                                                                   \
  } while (0)

static bool near(float a, float b) { return std::fabs(a - b) < 0.002f; }
static std::string readFile(const char* path) {
  std::ifstream f(path);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

int main() {
  Vec4 c;
  CHECK(engine::ui::parseColour("#F28C2C", c) && near(c.x, 242 / 255.0f) && near(c.y, 140 / 255.0f) && near(c.z, 44 / 255.0f) && c.w == 1);
  CHECK(engine::ui::parseColour("#fff", c) && c.x == 1 && c.y == 1 && c.z == 1);
  CHECK(engine::ui::parseColour("#00000080", c) && near(c.w, 128 / 255.0f));
  CHECK(engine::ui::parseColour("rgba(22,20,18,.86)", c) && near(c.x, 22 / 255.0f) && near(c.w, 0.86f));
  CHECK(engine::ui::parseColour(" rgb(255, 0, 0) ", c) && c.x == 1 && c.y == 0);
  CHECK(!engine::ui::parseColour("linear-gradient(#000, #fff)", c));
  CHECK(!engine::ui::parseColour("#12", c));
  CHECK(!engine::ui::parseColour("#GGGGGG", c));
  CHECK(!engine::ui::parseColour("rgba(1,2)", c));
  CHECK(!engine::ui::parseColour("", c));

  const char* files[] = {"ui/system/themes/faceplate.json", "ui/system/themes/backbar.json", "ui/system/themes/obsidian.json", "ui/system/themes/jukebox.json"};
  for (const char* f : files) {
    Theme t;
    std::string err;
    bool ok = engine::ui::parseTheme(readFile(f), t, &err);
    if (!ok) std::printf("FAIL %s: %s\n", f, err.c_str());
    CHECK(ok);
    CHECK(t.tokens.size() >= 40);
    CHECK(t.colour("accent", {-1, -1, -1, -1}).x >= 0);      // every theme's accent is a plain colour
    CHECK(t.colour("bg", {-1, -1, -1, -1}).x >= 0);
    CHECK(t.length("key-r", -1) >= 0);
  }
  Theme face;
  engine::ui::parseTheme(readFile("ui/system/themes/faceplate.json"), face);
  CHECK(face.id == "faceplate" && face.name == "Faceplate" && face.dark);
  CHECK(near(face.length("panel-r"), 16) && near(face.length("display-ls"), -0.02f));
  CHECK(face.colour("panel", {9, 9, 9, 9}).x == 9);                 // a gradient: the fallback
  CHECK(face.token("nope") == nullptr);

  // A theme with both halves: the light half lays its tokens over the dark ones.
  Theme both;
  CHECK(engine::ui::parseTheme(R"({"format":1,"id":"x","scheme":"dark","tokens":{"bg":"#000000","accent":"#FF0000"},
    "other":{"scheme":"light","tokens":{"bg":"#FFFFFF"}}})", both));
  CHECK(both.hasOther && both.dark);
  Theme light = both.half(false);
  CHECK(!light.dark && light.colour("bg").x == 1 && light.colour("accent").x == 1 && light.colour("accent").y == 0);
  CHECK(both.half(true).colour("bg").x == 0);
  CHECK(face.half(false).dark);                                   // no light half: stays as it is

  Theme t;
  std::string err;
  CHECK(!engine::ui::parseTheme("{", t, &err) && !err.empty());
  CHECK(!engine::ui::parseTheme("{\"format\":2,\"id\":\"x\",\"tokens\":{}}", t));
  CHECK(!engine::ui::parseTheme("{\"format\":1,\"tokens\":{}}", t));
  CHECK(!engine::ui::parseTheme("[]", t));

  std::printf(g_failed ? "ui_theme_test: %d FAILED\n" : "ui_theme_test: ok\n", g_failed);
  return g_failed ? 1 : 0;
}
