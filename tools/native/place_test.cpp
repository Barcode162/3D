// Places in words (J116.3, looped/words/place.h): the light lab written in
// words lands where the hand-made room had every thing (within 1 cm), its
// world file is made from its words, its cause rule is read, and broken
// sentences each give their line, column and a hint. The bar, a list place
// (J118.1, looped/words/place_list.h): its world file is made from its words,
// and the list's values, ids and broken lines read as they should.
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
  // The bar, a list place (J118.1).
  {
    std::string barJson;
    Place unused;
    int n = 0;
    const bool read = make("bar", barJson, unused, &n);
    check(read && n > 500, "the bar's words read without a problem (" + std::to_string(n) + " things)");
    check(readText("assets/world/places/bar.json") == barJson, "assets/world/places/bar.json is made from bar.looped (node tools/words.mjs place bar)");
    const KindsOf kinds = [](const std::string&) { return std::vector<std::string>{"bar_stool", "hall", "light-bulb", "glow_prop"}; };
    auto list = [&](const std::string& body, std::string* out, Problems* probs) {
      int count = 0;
      return placeListToJson("place a test\n  kinds from k.json\n" + body, "test", kinds, *out, count, *probs);
    };
    std::string out;
    Problems probs;
    check(list("  the air: fog 0.1 0.2 0.3, fogDensity 0.008\n  a room: id bar, min -1 0 -1, max 1 3 1\n  a point door at 1 2 3\n"
               "  the hall at 0 0 0\n  a bar stool at 1 0 2, turned -16.5, scaled 0.97\n  a bar stool at 2 0 2, called left, held by hall\n"
               "  a light bulb at 0 2 0\n    its light: color 1 0.5 0, dims yes, shape tube\n    a use: action toggle, box -1 -1 -1 to 1 1 1, prompt \"Say \\\"hi\\\"\"\n",
               &out, &probs),
          "a list place reads");
    const Json j = Json::parse(out);
    const Json& things = j["things"];
    check(things.items().size() == 4 && things.items()[0]["id"].string() == "hall" && things.items()[1]["id"].string() == "bar_stool-1" &&
              things.items()[2]["id"].string() == "left" && things.items()[2]["parent"].string() == "hall" && things.items()[3]["kind"].string() == "light-bulb",
          "ids: `the hall` is hall, the first stool bar_stool-1, `called left` left (held by hall), `light bulb` the kind light-bulb");
    const Json& light = things.items()[3]["parts"]["light"];
    const Json& use = things.items()[3]["parts"]["uses"].items()[0];
    check(j["air"]["fog"].items().size() == 3 && j["air"]["fogDensity"].number() == 0.008 && j["rooms"].items().size() == 1 &&
              j["points"]["door"].items()[2].number() == 3 && things.items()[1]["turn"].number() == -16.5 && light["dims"].boolean() &&
              light["shape"].string() == "tube" && use["box"].items().size() == 2 && use["prompt"].string() == "Say \"hi\"",
          "values: numbers, lists, lists joined by `to`, yes, bare words and quoted text with a quote inside");
    struct Broken {
      std::string body, expect;
    } broken[] = {
        {"  a bar stol at 0 0 0\n", "no kind called `bar stol`"},
        {"  a bar stool at 0 0 0, tilted 4\n", "can't read `tilted 4`"},
        {"  a bar stool at 0 0 0, called x\n  the hall at 0 0 0, called x\n", "two things are called `x`"},
        {"  a bar stool at 0 0 0, held by nobody\n", "no thing is called that"},
        {"  a bar stool at 0 0 0\n    its light: lumens\n", "has no value"},
        {"  a bar stool at 0 0 0\n    its light: lumens 1 to two\n", "`to` joins lists of numbers"},
        {"    its light: lumens 1\n", "belongs under a thing"},
    };
    for (const Broken& b : broken) {
      std::string o;
      Problems pr;
      list(b.body, &o, &pr);
      const std::string got = pr.empty() ? "" : pr[0].text();
      check(got.find(b.expect) != std::string::npos, "broken list: " + b.body.substr(0, b.body.find('\n')) + "   → " + got);
    }
  }
  if (failures) {
    std::printf("place_test: %d failed\n", failures);
    return 1;
  }
  std::printf("place_test: all passed\n");
  return 0;
}
