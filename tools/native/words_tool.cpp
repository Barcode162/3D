// The words tool (J116.3): Looped Script's places made into the world's files.
//   build/native/words_tool place <name>   assets/world/places/<name>.looped → <name>.json
//   build/native/words_tool check          every place in words has its JSON, and it's current
//   build/native/words_tool show <name>    where each thing went, and the place's rules
// Built and run by tools/words.mjs.
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "looped/draw/model_spec.h"
#include "looped/gltf.h"
#include "looped/json.h"
#include "looped/words/place.h"
#include "looped/words/place_json.h"
#include "looped/words/words.h"

using namespace engine;
using namespace engine::words;
namespace fs = std::filesystem;

namespace {

std::string readText(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

bool loadWords(Words& w) {
  std::vector<std::string> files;
  for (const auto& e : fs::directory_iterator("assets/words"))
    if (e.path().extension() == ".looped") files.push_back(e.path().string());
  std::sort(files.begin(), files.end());
  Problems p;
  for (const std::string& f : files) w.load(readText(f), f, p);
  for (const Problem& x : p) std::printf("%s\n", x.text().c_str());
  return !hasErrors(p);
}

std::vector<std::string> materialIds() {
  std::vector<std::string> out;
  const Json j = Json::parse(readText("assets/world/materials.json"));
  for (const Json& m : j["materials"].items()) out.push_back(m["id"].string());
  return out;
}

KindIndex kindIndex() {
  KindIndex out;
  std::vector<std::string> files;
  for (const auto& e : fs::directory_iterator("assets/world/kinds"))
    if (e.path().extension() == ".json") files.push_back(e.path().filename().string());
  std::sort(files.begin(), files.end());
  for (const std::string& f : files) {
    const Json j = Json::parse(readText("assets/world/kinds/" + f));
    for (const Json& k : j["kinds"].items()) {
      const std::string& file = k["parts"]["model"]["file"].string();
      if (file.empty()) continue;
      const std::string id = k["id"].string();
      // The kind named after its model wins (round_table.glb → round_table).
      auto it = out.find(file);
      const bool named = id + ".glb" == file;
      if (it == out.end() || named) out[file] = {id, "kinds/" + f};
    }
  }
  return out;
}

ModelSize modelSize(const std::string& file) {
  static std::map<std::string, ModelSize> cache;
  if (auto it = cache.find(file); it != cache.end()) return it->second;
  ModelSize s;
  std::ifstream f("assets/models/" + file, std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  ModelData d;
  std::string error;
  if (!bytes.empty() && parseGlb(bytes, d, error)) {
    bool first = true;
    for (const ModelPrimitive& prim : d.primitives)
      for (const ModelVertex& v : prim.vertices) {
        if (first) s.min = s.max = v.position, first = false;
        s.min = {std::min(s.min.x, v.position.x), std::min(s.min.y, v.position.y), std::min(s.min.z, v.position.z)};
        s.max = {std::max(s.max.x, v.position.x), std::max(s.max.y, v.position.y), std::max(s.max.z, v.position.z)};
      }
    s.known = !first;
  }
  return cache[file] = s;
}

bool make(const std::string& name, std::string& json, Place& place) {
  Words w;
  if (!loadWords(w)) return false;
  const std::string file = "assets/world/places/" + name + ".looped";
  Problems p;
  bool ok = readPlace(w, readText(file), file, materialIds(), modelSize, place, p) &&
             placeToJson(place, name, kindIndex(), w.dictionary(), json, p);
  // Every model a thing is made of meets the model spec (looped/draw/model_spec.h), or the place is refused.
  static std::map<std::string, std::vector<draw::ModelProblem>> checked;
  for (const Placed& q : place.things) {
    std::vector<const Thing*> stack{&q.thing};
    while (!stack.empty()) {
      const Thing* t = stack.back();
      stack.pop_back();
      for (const Thing& part : t->parts) stack.push_back(&part);
      if (t->model.empty()) continue;
      auto it = checked.find(t->model);
      if (it == checked.end()) {
        std::ifstream f("assets/models/" + t->model, std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        ModelData d;
        std::string error;
        std::vector<draw::ModelProblem> probs;
        if (parseGlb(bytes, d, error)) {
          draw::ModelMeasure mm = draw::measureModel(d);
          mm.file = t->model;
          probs = draw::checkModel(mm);
        }
        it = checked.emplace(t->model, std::move(probs)).first;
      }
      for (const draw::ModelProblem& mp : it->second) {
        if (mp.cause) continue;  // shown with the rule it explains, in the checker
        p.push_back({q.line, 1, "the model " + t->model + " (" + q.word + ") breaks " + mp.rule->id + ": " + mp.measured, mp.rule->fix, false});
        ok = false;
      }
    }
  }
  for (const Problem& x : p) std::printf("%s %s\n", file.c_str(), x.text().c_str());
  return ok;
}

}  // namespace

#ifndef WORDS_TOOL_NO_MAIN
int main(int argc, char** argv) {
  const std::string cmd = argc > 1 ? argv[1] : "check";
  if (cmd == "place" || cmd == "show") {
    if (argc < 3) return std::printf("which place?\n"), 2;
    std::string json;
    Place place;
    if (!make(argv[2], json, place)) return 1;
    if (cmd == "show") {
      for (const Placed& q : place.things)
        std::printf("%-14s %-13s at %6.3f %6.3f %6.3f  turned %4.0f\n", q.name.c_str(), q.word.c_str(), q.at.x, q.at.y, q.at.z, q.turn);
      for (const Rule& r : place.rules) {
        std::printf("%s %s:", r.whilst ? "while" : "when", r.event.c_str());
        for (const std::string& t : r.things) std::printf(" %s", t.c_str());
        std::printf(" → %.2f\n", r.level);
      }
      return 0;
    }
    std::ofstream("assets/world/places/" + std::string(argv[2]) + ".json", std::ios::binary) << json;
    std::printf("wrote assets/world/places/%s.json (%zu things)\n", argv[2], place.things.size());
    return 0;
  }
  if (cmd == "check") {
    int bad = 0, n = 0;
    for (const auto& e : fs::directory_iterator("assets/world/places")) {
      if (e.path().extension() != ".looped") continue;
      const std::string name = e.path().stem().string();
      std::string json;
      Place place;
      ++n;
      if (!make(name, json, place)) {
        ++bad;
        continue;
      }
      if (readText("assets/world/places/" + name + ".json") != json) {
        std::printf("assets/world/places/%s.json isn't made from its words: run node tools/words.mjs place %s\n", name.c_str(), name.c_str());
        ++bad;
      }
    }
    if (bad) return 1;
    std::printf("ok  words: %d place(s) in words, each world file made from its words\n", n);
    return 0;
  }
  std::printf("place <name> | show <name> | check\n");
  return 2;
}
#endif
