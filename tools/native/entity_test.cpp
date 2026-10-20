// Stable entity ids (app/entities.h) on the real bar layout: every id and
// net id is unique, and the list equals app/entity_ids.txt. A deliberate
// layout change: run with UPDATE=1 to rewrite the list, and check the diff.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "app/bar_scene.h"
#include "app/entities.h"

int main() {
  int failures = 0;
  auto check = [&](bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++failures;
  };
  const app::BarScene scene = app::buildBarScene();
  const std::vector<app::Entity> all = app::listEntities(scene);
  std::set<std::string> ids;
  std::set<uint16_t> nets;
  std::ostringstream list;
  for (const app::Entity& e : all) {
    if (!ids.insert(e.id).second) check(false, "id used twice: " + e.id);
    if (!nets.insert(e.netId).second) check(false, "net id used twice: " + std::to_string(e.netId) + " (" + e.id + ")");
    list << e.netId << " " << e.id << "\n";
  }
  check(ids.size() == all.size(), std::to_string(all.size()) + " entities, every id unique");
  check(nets.size() == all.size(), "every net id unique");
  const char* path = "app/entity_ids.txt";
  if (std::getenv("UPDATE")) {
    std::ofstream(path) << list.str();
    std::printf("wrote %s\n", path);
  } else {
    std::ifstream f(path);
    std::stringstream saved;
    saved << f.rdbuf();
    check(saved.str() == list.str(), std::string("the ids equal ") + path + " (a layout change? UPDATE=1 rewrites it)");
  }
  if (failures) { std::printf("ENTITY TEST FAILED (%d)\n", failures); return EXIT_FAILURE; }
  std::printf("ENTITY TEST PASSED\n");
  return 0;
}
