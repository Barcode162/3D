// One-time converter (docs/ENGINE.md E2 second pass): the bar's layout as
// code (the old bar/bar_scene.cpp builder) → a place file of things, each
// with its parts in its own space. Prints compact JSON:
//   {"place": {...}, "kindParts": {"<kind>": {"<part>": ...}}}
// tools/place_format.py turns it into assets/world/places/bar.json and puts
// the parts every thing of a kind shares into assets/world/kinds/bar.json.
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "bar/bar_place.h"
#include "bar/bar_scene.h"
#include "bar/scene_names.h"
#include "looped/json.h"
#include "looped/world/kind.h"

using engine::Quat;
using engine::Vec3;
using namespace app;

namespace {

std::string num(float v) {
  if (std::fabs(v) < 5e-6f) return "0";
  char b[32];
  std::snprintf(b, sizeof b, "%.5f", v);
  std::string s = b;
  while (s.back() == '0') s.pop_back();
  if (s.back() == '.') s.pop_back();
  return s;
}
std::string vec(Vec3 v) { return "[" + num(v.x) + ", " + num(v.y) + ", " + num(v.z) + "]"; }
std::string q(const std::string& s) { return engine::jsonQuote(s); }

// Degrees, to a thousandth (so 1e-5 of rounding doesn't make two stools differ).
std::string angle(float d) {
  const float r = std::round(d * 1000) / 1000;
  return num(r == -180 ? 180 : r);
}

float yawOf(Quat r) { return 2 * std::atan2(r.y, r.w); }  // pure turns about y
float deg(float rad) {
  float d = rad * 180 / engine::kPi;
  while (d > 180) d -= 360;
  while (d <= -180) d += 360;
  return d;
}

std::string readFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

// A model file's box (its own space), from the glb's POSITION min/max.
engine::Aabb glbBounds(const std::string& file) {
  const std::string b = readFile("assets/models/" + file);
  engine::Aabb box{{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
  if (b.size() < 20) return box;
  uint32_t n;
  std::memcpy(&n, b.data() + 12, 4);
  const engine::Json j = engine::Json::parse(std::string_view(b.data() + 20, n), nullptr, {64, 1 << 20});
  for (const engine::Json& m : j["meshes"].items())
    for (const engine::Json& p : m["primitives"].items()) {
      const engine::Json& a = j["accessors"][p["attributes"]["POSITION"].integer()];
      box.min = engine::minComponents(box.min, {float(a["min"][0].number()), float(a["min"][1].number()), float(a["min"][2].number())});
      box.max = engine::maxComponents(box.max, {float(a["max"][0].number()), float(a["max"][1].number()), float(a["max"][2].number())});
    }
  return box;
}

struct Owner {
  std::string id, kind;
  Vec3 at;
  Quat rot;
  int model = -1;
  bool canOwn = false;      // may take nearby shapes (furniture, not glasses or room shells)
  engine::Aabb box;         // world box of its model
  std::map<std::string, std::vector<std::string>> lists;  // solids, blocks, floors, seats, uses
  std::map<std::string, std::string> single;              // model, light, mirror, ...
  std::map<std::string, std::vector<int>> order;          // ordered arrays: indices it holds
  int priority = 0;

  Vec3 local(Vec3 p) const { return engine::conjugate(rot).rotate(p - at); }
  Vec3 localDir(Vec3 d) const { return engine::conjugate(rot).rotate(d); }
};

std::string slug(std::string s) {
  s = s.substr(0, s.find('\n'));
  std::string o;
  for (char c : s) {
    if (std::isalnum(static_cast<unsigned char>(c))) o += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    else if (!o.empty() && o.back() != '-') o += '-';
  }
  while (!o.empty() && o.back() == '-') o.pop_back();
  return o;
}

}  // namespace

int main() {
  const BarScene s = buildBarScene();
  engine::world::Library kinds;
  kinds.add(readFile("assets/world/kinds/bar.json"), "kinds");
  kinds.resolve();

  std::vector<Owner> owners;
  std::map<std::string, int> idCount;
  auto newOwner = [&](const std::string& base, const std::string& kind, Vec3 at, Quat rot) {
    Owner o;
    o.id = base;
    o.kind = kind;
    o.at = at;
    o.rot = rot;
    owners.push_back(o);
    return static_cast<int>(owners.size() - 1);
  };

  // ---- Models: one thing each ----
  std::vector<int> ownerOfModel(s.models.size());
  const std::set<std::string> rooms = {"room", "hall", "decor_walls", "lounge", "stage", "darts_zone", "arcade",
                                       "construction", "big_mirror", "bar_counter_l", "back_bar_tall"};
  for (size_t i = 0; i < s.models.size(); ++i) {
    const ModelPlacement& m = s.models[i];
    const std::string kind = kindForModel(kinds, m.file);
    const std::string stem = m.file.substr(0, m.file.rfind('.'));
    const int o = newOwner(stem, kind, m.position, m.rotation);
    Owner& w = owners[static_cast<size_t>(o)];
    w.model = static_cast<int>(i);
    ownerOfModel[i] = o;
    w.order["models"].push_back(static_cast<int>(i));
    std::string model = "{\"file\": " + q(m.file) + ", \"material\": " + q(materialName(m.material));
    if (m.scale != 1) model += ", \"scale\": " + num(m.scale);
    w.single["model"] = model + "}";
    if (m.prop != Prop::kNone) w.single["prop"] = q(propName(m.prop));
    if (m.room >= 0) w.single["room"] = q(m.room == 1 ? "arcade" : "room-" + std::to_string(m.room));
    const bool moves = m.prop != Prop::kNone || kind == "chess" || kind == "credit_coin";
    w.canOwn = !moves && !rooms.count(kind);
    const engine::Aabb b = glbBounds(m.file);
    w.box = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
    for (int c = 0; c < 8; ++c) {
      const Vec3 p{c & 1 ? b.max.x : b.min.x, c & 2 ? b.max.y : b.min.y, c & 4 ? b.max.z : b.min.z};
      const Vec3 wp = m.position + m.rotation.rotate(p * m.scale);
      w.box.min = engine::minComponents(w.box.min, wp);
      w.box.max = engine::maxComponents(w.box.max, wp);
    }
  }
  auto ownerOfKind = [&](const char* kind) {
    for (size_t i = 0; i < owners.size(); ++i)
      if (owners[i].kind == kind) return static_cast<int>(i);
    return -1;
  };

  // ---- Lights: one thing each ----
  for (size_t i = 0; i < s.lights.size(); ++i) {
    const engine::PointLight& l = s.lights[i];
    float phase;
    const std::string kind = kindForLight(s, i, &phase);
    const int o = newOwner(kind, kind, l.position, {});
    Owner& w = owners[static_cast<size_t>(o)];
    const bool dims = std::count(s.roomLights.begin(), s.roomLights.end(), static_cast<int>(i)) > 0;
    w.single["light"] = "{\"color\": " + vec(l.color) + ", \"intensity\": " + num(l.intensity) + ", \"range\": " +
                        num(l.range) + ", \"radius\": " + num(l.radius) + (dims ? ", \"dims\": true" : "") + "}";
    if (phase != 0) w.single["phase"] = num(phase);
    w.order["lights"].push_back(static_cast<int>(i));
  }

  // Round stools turn to face where their seat faces (they look the same any
  // way round), so every stool of a kind shares one seat and one set of shapes.
  const std::set<std::string> round = {"bar_stool", "table_stool", "round_table", "candle"};
  for (const Seat& st : s.seats)
    if (st.model >= 0 && round.count(owners[size_t(ownerOfModel[size_t(st.model)])].kind))
      owners[size_t(ownerOfModel[size_t(st.model)])].rot = engine::Quat::axisAngle({0, 1, 0}, -st.yaw);

  // ---- Seats: on their model, or grouped into spots (a sofa's three places) ----
  std::map<std::string, int> seatSpot;  // name → last spot made for it
  std::map<std::string, Vec3> lastHip;
  for (size_t i = 0; i < s.seats.size(); ++i) {
    const Seat& st = s.seats[i];
    int o = -1;
    bool lit = false;
    if (st.model >= 0) {
      o = ownerOfModel[static_cast<size_t>(st.model)];
      lit = true;
    } else {
      // A model it sits in (the piano's bench, a rig's seat).
      for (size_t k = 0; k < owners.size(); ++k) {
        const Owner& w = owners[k];
        if (!w.canOwn || w.model < 0) continue;
        const engine::Aabb& b = w.box;
        if (st.hip.x > b.min.x && st.hip.x < b.max.x && st.hip.z > b.min.z && st.hip.z < b.max.z &&
            (o < 0 || (b.max - b.min).x * (b.max - b.min).z < (owners[size_t(o)].box.max - owners[size_t(o)].box.min).x *
                                                                    (owners[size_t(o)].box.max - owners[size_t(o)].box.min).z))
          o = static_cast<int>(k);
      }
      if (o < 0) {
        const auto it = seatSpot.find(st.name);
        if (it != seatSpot.end() && engine::distance(lastHip[st.name], st.hip) < 1.6f &&
            owners[size_t(it->second)].order["seats"].back() == static_cast<int>(i) - 1) {
          o = it->second;
        } else {
          o = newOwner(slug(st.name), "seat-spot", {st.hip.x, 0, st.hip.z}, {});
          seatSpot[st.name] = o;
        }
      }
    }
    lastHip[st.name] = st.hip;
    Owner& w = owners[static_cast<size_t>(o)];
    const Vec3 face = w.localDir({std::sin(st.yaw), 0, -std::cos(st.yaw)});
    std::string j = "{\"name\": " + q(st.name) + ", \"hip\": " + vec(w.local(st.hip)) + ", \"facing\": " +
                    angle(deg(std::atan2(face.x, -face.z))) + ", \"low\": " + num(st.lowSeat) + ", \"kind\": " +
                    q(seatKindName(st.kind));
    if (lit) j += ", \"lights\": true";
    if (st.machine >= 0) j += ", \"machine\": " + std::to_string(st.machine);
    w.lists["seats"].push_back(j + "}");
    w.order["seats"].push_back(static_cast<int>(i));
  }

  // ---- Things to use: on their model, else a spot of their own ----
  std::vector<int> ownerOfUse(s.interactables.size());
  for (size_t i = 0; i < s.interactables.size(); ++i) {
    const Interactable& it = s.interactables[i];
    int o;
    bool lit = false;
    if (it.modelCount == 1) {
      o = ownerOfModel[it.firstModel];
      lit = true;
    } else if (it.action == Action::kDrinkShelf) {
      o = newOwner("drink-spot", "drink-spot", s.barTopFront[it.value], {});
      owners[size_t(o)].single["drink"] = "{\"slot\": " + std::to_string(it.value) + "}";
    } else if (it.action == Action::kPianoSongs || it.action == Action::kFireplace) {
      o = ownerOfKind(it.action == Action::kPianoSongs ? "piano" : "fire_flames");  // the sheet music, the fire
    } else {
      const Vec3 c = (it.boundsMin + it.boundsMax) * 0.5f;
      o = newOwner(slug(it.name.empty() ? it.prompt : it.name), "spot", {c.x, it.boundsMin.y, c.z}, {});
    }
    ownerOfUse[i] = o;
    Owner& w = owners[static_cast<size_t>(o)];
    std::string j = "{";
    if (!it.name.empty()) j += "\"name\": " + q(it.name) + ", ";
    if (!it.prompt.empty()) j += "\"prompt\": " + q(it.prompt) + ", ";
    j += "\"action\": " + q(actionName(it.action));
    if (it.value) j += ", \"value\": " + std::to_string(it.value);
    j += ", \"tag\": " + vec(w.local(it.tagPosition));
    j += ", \"box\": [" + vec(it.boundsMin - w.at) + ", " + vec(it.boundsMax - w.at) + "]";
    if (it.direct) j += ", \"direct\": true";
    if (lit) j += ", \"lights\": true";
    if (it.mirror >= 0) j += ", \"mirror\": " + q("big_mirror");
    w.lists["uses"].push_back(j + "}");
    w.order["uses"].push_back(static_cast<int>(i));
  }

  // ---- Mirrors ----
  for (size_t i = 0; i < s.mirrors.size(); ++i) {
    const Mirror& m = s.mirrors[i];
    const int o = ownerOfKind("big_mirror");
    Owner& w = owners[static_cast<size_t>(o)];
    std::string buttons;
    for (int b : m.buttonModels) buttons += (buttons.empty() ? "" : ", ") + q("@" + std::to_string(b));
    w.single["mirror"] = "{\"center\": " + vec(w.local(m.center)) + ", \"normal\": " + vec(w.localDir(m.normal)) +
                         ", \"right\": " + vec(w.localDir(m.right)) + ", \"up\": " + vec(w.localDir(m.up)) +
                         ", \"halfWidth\": " + num(m.halfWidth) + ", \"halfHeight\": " + num(m.halfHeight) +
                         ", \"mode\": " + q(m.mode == MirrorMode::kFull ? "full" : "avatars") + ", \"buttons\": [" +
                         buttons + "]}";
    w.order["mirrors"].push_back(static_cast<int>(i));
  }

  // ---- The bar's special spots, on the things they belong to ----
  {
    Owner& d = owners[size_t(ownerOfKind("dartboard"))];
    d.single["dartboard"] = "{\"face\": " + vec(d.local(s.dartBoard)) + ", \"oche\": " + vec(d.local(s.dartOche)) + "}";
    for (size_t k = 0; k < owners.size(); ++k)
      if (owners[k].kind == "sim_rig")
        for (int r = 0; r < 2; ++r)
          if (engine::distance(owners[k].at, s.simPosition[r]) < 1e-4f) owners[k].single["rig"] = "{\"machine\": " + std::to_string(r) + "}";
    Owner& rh = owners[size_t(ownerOfKind("rhythm"))];
    std::string scr;
    for (const Vec3& c : s.rhythmScreen) scr += (scr.empty() ? "" : ", ") + vec(rh.local(c));
    rh.single["screen"] = "[" + scr + "]";
    Owner& j = owners[size_t(ownerOfKind("jukebox"))];
    j.single["speaker"] = "{\"music\": \"jukebox\", \"at\": " + vec(j.local(s.jukeboxSpeaker)) + "}";
    Owner& a = owners[size_t(ownerOfKind("arcade_cabinet"))];
    a.single["speaker"] = "{\"music\": \"arcade\", \"at\": " + vec(a.local(s.arcadeSpeaker)) + "}";
    Owner& p = owners[size_t(ownerOfKind("piano"))];
    p.single["piano"] = "{}";
    owners[size_t(ownerOfKind("credit_coin"))].single["coin"] = "{}";
    for (size_t i = 0; i < s.interactables.size(); ++i) {
      Owner& w = owners[size_t(ownerOfUse[i])];
      if (s.interactables[i].action == Action::kBartender)
        w.single["bartender"] = "{\"stand\": " + vec(w.local(s.bartenderSpot)) + "}";
      if (s.interactables[i].action == Action::kChess)
        w.single["chessboard"] = "{\"center\": " + vec(w.local(s.chessCenter)) + ", \"square\": " + num(s.chessSquare) + "}";
    }
    for (const ChessPiece& c : s.chessPieces)
      owners[size_t(ownerOfModel[c.model])].single["chess"] = "{\"piece\": " + std::to_string(c.piece) + "}";
  }

  // ---- Solid shapes, walking blockers and raised floors ----
  // Inside the smallest piece of furniture whose box holds it, else part of
  // the room shell it's in.
  const int hall = ownerOfKind("hall"), arcade = ownerOfKind("arcade");
  auto shellOf = [&](Vec3 c) { return s.rooms.size() > 1 && s.rooms[1].contains(c) ? arcade : hall; };
  auto smallestHolding = [&](Vec3 c, bool floor) {
    int best = -1;
    float bestV = 1e9f;
    for (size_t k = 0; k < owners.size(); ++k) {
      const Owner& w = owners[k];
      if (!w.canOwn || w.model < 0) continue;
      const engine::Aabb& b = w.box;
      const float m = 0.01f;
      if (c.x < b.min.x - m || c.x > b.max.x + m || c.z < b.min.z - m || c.z > b.max.z + m) continue;
      if (floor ? b.min.y > 0.05f : (c.y < b.min.y - m || c.y > b.max.y + m)) continue;
      const Vec3 d = b.max - b.min;
      const float v = d.x * d.y * d.z;
      if (v < bestV) bestV = v, best = static_cast<int>(k);
    }
    return best;
  };
  for (const Collider& c : s.colliders) {
    int o = smallestHolding(c.center, false);
    if (o < 0) o = shellOf(c.center);
    Owner& w = owners[size_t(o)];
    const Quat lr = engine::normalized(engine::conjugate(w.rot) * c.rotation);
    std::string j = "{\"at\": " + vec(w.local(c.center)) + ", \"half\": " + vec(c.half);
    if (std::fabs(lr.y) > 1e-6f && !round.count(w.kind)) j += ", \"turn\": " + angle(deg(yawOf(lr)));
    if (std::fabs(lr.x) > 1e-5f || std::fabs(lr.z) > 1e-5f) std::fprintf(stderr, "collider turned off the y axis\n");
    w.lists["solids"].push_back(j + ", \"material\": " + q(materialName(c.material)) + "}");
  }
  for (const Blocker& b : s.blockers) {
    const Vec3 c{(b.minX + b.maxX) / 2, 0, (b.minZ + b.maxZ) / 2};
    int o = smallestHolding(c, true);
    if (o < 0) o = shellOf(c);
    Owner& w = owners[size_t(o)];
    w.lists["blocks"].push_back("[" + num(b.minX - w.at.x) + ", " + num(b.minZ - w.at.z) + ", " + num(b.maxX - w.at.x) +
                                ", " + num(b.maxZ - w.at.z) + "]");
  }
  for (const FloorArea& f : s.floors) {
    Owner& w = owners[size_t(hall)];
    w.lists["floors"].push_back("[" + num(f.minX - w.at.x) + ", " + num(f.minZ - w.at.z) + ", " + num(f.maxX - w.at.x) +
                                ", " + num(f.maxZ - w.at.z) + ", " + num(f.height - w.at.y) + "]");
  }

  // ---- Order: every list (models, seats, uses, lights, mirrors) keeps its order ----
  std::map<std::string, std::vector<std::pair<int, int>>> arrays;  // name → (index, owner)
  for (size_t k = 0; k < owners.size(); ++k)
    for (const auto& [name, idx] : owners[k].order)
      if (name != "uses")  // tap areas: their order doesn't matter (found by place)
        for (int i : idx) arrays[name].push_back({i, static_cast<int>(k)});
  std::vector<std::set<int>> after(owners.size());
  std::vector<int> incoming(owners.size(), 0);
  for (auto& [name, v] : arrays) {
    std::sort(v.begin(), v.end());
    for (size_t i = 0; i + 1 < v.size(); ++i) {
      const int a = v[i].second, b = v[i + 1].second;
      if (a != b && after[size_t(a)].insert(b).second) ++incoming[size_t(b)];
    }
  }
  // Ready things in a readable order: models first in model order, then the rest as they come.
  for (size_t k = 0; k < owners.size(); ++k) {
    const Owner& w = owners[k];
    w.model >= 0 ? (owners[k].priority = w.model) : (owners[k].priority = 100000 + static_cast<int>(k));
  }
  std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> ready;
  for (size_t k = 0; k < owners.size(); ++k)
    if (!incoming[k]) ready.push({owners[k].priority, static_cast<int>(k)});
  std::vector<int> sorted;
  while (!ready.empty()) {
    const int k = ready.top().second;
    ready.pop();
    sorted.push_back(k);
    for (int b : after[size_t(k)])
      if (--incoming[size_t(b)] == 0) ready.push({owners[size_t(b)].priority, b});
  }
  if (sorted.size() != owners.size()) {
    std::fprintf(stderr, "order: a circle (%zu of %zu placed)\n", sorted.size(), owners.size());
    for (size_t k = 0; k < owners.size(); ++k) {
      if (!incoming[k]) continue;
      std::fprintf(stderr, "  %s:", owners[k].id.c_str());
      for (const auto& [n, v] : owners[k].order) {
        std::fprintf(stderr, " %s", n.c_str());
        for (int i : v) std::fprintf(stderr, " %d", i);
      }
      std::fprintf(stderr, "\n");
    }
    return 1;
  }

  // ---- Ids: the kind's name, numbered when there are several ----
  std::map<std::string, int> total, seen;
  for (const Owner& w : owners) ++total[w.id];
  std::vector<std::string> finalId(owners.size());
  for (int k : sorted) {
    Owner& w = owners[size_t(k)];
    finalId[size_t(k)] = total[w.id] > 1 ? w.id + "-" + std::to_string(++seen[w.id]) : w.id;
  }
  for (int k : sorted) {
    auto& m = owners[size_t(k)].single;
    if (!m.count("mirror")) continue;
    std::string& t = m["mirror"];
    for (int b = 0; b < 3; ++b) {
      const std::string key = q("@" + std::to_string(s.mirrors[0].buttonModels[b]));
      const size_t at = t.find(key);
      if (at != std::string::npos) t.replace(at, key.size(), q(finalId[size_t(ownerOfModel[size_t(s.mirrors[0].buttonModels[b])])]));
    }
  }

  // ---- Parts every thing of a kind shares go to the kind ----
  std::map<std::string, std::vector<int>> byKind;
  for (int k : sorted) byKind[owners[size_t(k)].kind].push_back(k);
  std::map<std::string, std::map<std::string, std::string>> kindParts;
  auto partText = [](const Owner& w, const std::string& name) -> std::string {
    if (w.single.count(name)) return w.single.at(name);
    if (!w.lists.count(name)) return "";
    std::string t = "[";
    for (size_t i = 0; i < w.lists.at(name).size(); ++i) t += (i ? ", " : "") + w.lists.at(name)[i];
    return t + "]";
  };
  const std::set<std::string> perThing = {"phase", "drink", "chess", "rig"};
  // Every round table has legs (the one by the darts had only its top).
  for (int k : byKind["round_table"])
    if (owners[size_t(k)].lists["solids"].size() == 3) {
      for (int m : byKind["round_table"]) owners[size_t(m)].lists["solids"] = owners[size_t(k)].lists["solids"];
      break;
    }
  for (auto& [kind, members] : byKind) {
    std::set<std::string> names;
    for (int k : members) {
      for (auto& [n, v] : owners[size_t(k)].single) names.insert(n);
      for (auto& [n, v] : owners[size_t(k)].lists) names.insert(n);
    }
    for (const std::string& n : names) {
      if (perThing.count(n) || kind == "spot" || kind == "seat-spot") continue;
      const std::string first = partText(owners[size_t(members[0])], n);
      bool same = !first.empty();
      for (int k : members) same = same && partText(owners[size_t(k)], n) == first;
      if (!same) continue;
      kindParts[kind][n] = first;
      for (int k : members) owners[size_t(k)].single.erase(n), owners[size_t(k)].lists.erase(n);
    }
  }

  // ---- Out ----
  std::string out = "{\"place\": {\"id\": \"bar\", \"about\": \"The bar: every thing in it and where it sits. Parts are in the thing's own space (they turn with it), except floor rectangles (blocks, floors) and tap boxes, which keep the world's axes.\", \"kinds\": [\"kinds/bar.json\"]";
  out += ", \"air\": {\"ambient\": " + vec(s.ambient) + ", \"fog\": " + vec(s.fogColor) + ", \"fogDensity\": " + num(s.fogDensity) + "}";
  out += ", \"spawn\": {\"at\": " + vec(s.spawnPosition) + ", \"yaw\": " + num(deg(s.spawnYaw)) + "}";
  out += ", \"lightGrid\": {\"min\": " + vec(s.lightGridMin) + ", \"max\": " + vec(s.lightGridMax) + "}";
  const char* roomNames[] = {"bar", "arcade", "room-3"};
  out += ", \"rooms\": [";
  for (size_t i = 0; i < s.rooms.size(); ++i)
    out += std::string(i ? ", " : "") + "{\"id\": " + q(roomNames[i]) + ", \"min\": " + vec(s.rooms[i].min) + ", \"max\": " + vec(s.rooms[i].max) + "}";
  out += "], \"doorways\": [";
  for (size_t i = 0; i < s.doorways.size(); ++i) {
    const auto& d = s.doorways[i];
    out += std::string(i ? ", " : "") + "{\"rooms\": [" + q(roomNames[d.a]) + ", " + q(roomNames[d.b]) + "], \"corners\": [" +
           vec(d.corners[0]) + ", " + vec(d.corners[1]) + ", " + vec(d.corners[2]) + ", " + vec(d.corners[3]) + "]}";
  }
  out += "], \"areas\": {\"arcade\": {\"min\": " + vec(s.arcadeMin) + ", \"max\": " + vec(s.arcadeMax) + "}}";
  out += ", \"points\": {\"arcade-door\": " + vec(s.arcadeDoor) + "}";
  out += ", \"things\": [";
  bool firstThing = true;
  for (int k : sorted) {
    const Owner& w = owners[size_t(k)];
    out += std::string(firstThing ? "" : ", ") + "{\"id\": " + q(finalId[size_t(k)]) + ", \"kind\": " + q(w.kind) +
           ", \"at\": " + vec(w.at);
    firstThing = false;
    const float yaw = deg(yawOf(w.rot));
    if (std::fabs(yaw) > 1e-4f) out += ", \"turn\": " + angle(yaw);
    std::string parts;
    std::set<std::string> names;
    for (auto& [n, v] : w.single) names.insert(n);
    for (auto& [n, v] : w.lists) names.insert(n);
    for (const std::string& n : names) {
      if (n == "phase") continue;
      parts += (parts.empty() ? "" : ", ") + q(n) + ": " + partText(w, n);
    }
    if (w.single.count("phase")) out += ", \"phase\": " + w.single.at("phase");
    if (!parts.empty()) out += ", \"parts\": {" + parts + "}";
    out += "}";
  }
  out += "]}, \"kindParts\": {";
  bool firstKind = true;
  for (auto& [kind, parts] : kindParts) {
    out += std::string(firstKind ? "" : ", ") + q(kind) + ": {";
    firstKind = false;
    bool f = true;
    for (auto& [n, t] : parts) out += std::string(f ? "" : ", ") + q(n) + ": " + t, f = false;
    out += "}";
  }
  out += "}}";
  std::puts(out.c_str());
  std::fprintf(stderr, "%zu things, %zu kinds with shared parts\n", owners.size(), kindParts.size());
  return 0;
}
