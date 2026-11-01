// Places in words (J116.3, looped/words/place.h): the light lab written in
// words lands where the hand-made room had every thing (within 1 cm), its
// world file is made from its words, its cause rule is read, and broken
// sentences each give their line, column and a hint.
#define WORDS_TOOL_NO_MAIN
#include "tools/native/words_tool.cpp"

#include <cmath>
#include <cstdlib>

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++failures;
}

// A broken place: the first problem's text.
std::string firstProblem(Words& w, const std::string& body) {
  Place place;
  Problems p;
  readPlace(w, "place a test\n" + body, "test", materialIds(), modelSize, place, p);
  return p.empty() ? "" : p[0].text();
}
}  // namespace

int main() {
  Place place;
  std::string json;
  check(make("lab", json, place), "the light lab's words read without a problem");
  // Where the hand-made lab (before J116.3) had each thing.
  struct Want {
    const char* name;
    Vec3 at;
    float turn;
  } wants[] = {{"table", {0, 0, 0}, 0},         {"candle", {0, 0.785f, 0}, 0},           {"stool-1", {0.78f, 0, 0}, 90},
               {"stool-2", {-0.78f, 0, 0}, -90}, {"pendant-lamp", {-1.4f, 2.94f, -1.2f}, 0}, {"bulb", {2.5f, 2.55f, 2.5f}, 0},
               {"neon-tube", {-2.96f, 2.1f, 0.6f}, 90}, {"sign", {0.8f, 2.15f, -2.98f}, 0}};
  for (const Want& w : wants) {
    const Placed* q = place.find(w.name);
    const bool ok = q && length(q->at - w.at) < 0.011f && std::fabs(q->turn - w.turn) < 0.5f;
    char b[160];
    std::snprintf(b, sizeof b, "%s where the hand-made lab had it (%.3f %.3f %.3f, turned %.0f)", w.name, w.at.x, w.at.y, w.at.z, w.turn);
    check(ok, b);
  }
  check(place.rules.size() == 1 && place.rules[0].event == "the lights go down" && place.rules[0].things.size() == 2 &&
            std::fabs(place.rules[0].level - 0.25f) < 1e-6f,
        "when the lights go down, the pendant lamp and the bulb dim to a quarter");
  check(place.level("bulb", {"the lights go down"}) == 0.25f && place.level("bulb", {}) == 1 && place.level("sign", {"the lights go down"}) == 1,
        "the rule's runner: the bulb at a quarter while the lights are down, the sign untouched");
  check(readText("assets/world/places/lab.json") == json, "assets/world/places/lab.json is made from lab.looped (node tools/words.mjs place lab)");
  // Broken sentences.
  Words w;
  loadWords(w);
  const std::string room = "  a room 6 by 6 and 3 high, with plaster walls\n";
  struct Broken {
    std::string body, expect;
  } broken[] = {
      {"  a bulb in the far left corner\n", "a room comes first"},
      {room + "  a tabel in the middle\n", "did you mean `table`"},
      {room + "  a candle on the table\n", "isn't in this place"},
      {room + "  a table in the middel\n", "isn't a place"},
      {room + "  a bulb in the far top corner\n", "which corner?"},
      {room + "  a sign on the top wall\n", "which wall?"},
      {"  a room 6 by 6\n", "width by its depth and its height"},
      {"  a room 6 by 6 and 3 high, with marble walls\n", "isn't a material"},
      {room + "  a table in the middle\n  when the lights go down, the table wobbles\n", "what changes?"},
      {room + "  a bulb in the middle\n  when the lights go down, the bulb dims to a lot\n", "isn't an amount here"},
  };
  for (const Broken& b : broken) {
    const std::string got = firstProblem(w, b.body);
    check(got.find(b.expect) != std::string::npos, "broken: " + b.body.substr(b.body.rfind('\n', b.body.size() - 2) + 1, std::string::npos).substr(0, 60) +
                                                       "   → " + got);
  }
  if (failures) {
    std::printf("place_test: %d failed\n", failures);
    return 1;
  }
  std::printf("place_test: all passed\n");
  return 0;
}
