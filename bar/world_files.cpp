#include "bar/world_files.h"

#include <fstream>
#include <map>
#include <sstream>

namespace app {

namespace {
std::map<std::string, std::string>& given() {
  static std::map<std::string, std::string> files;
  return files;
}
}  // namespace

void setWorldFile(const std::string& path, std::string text) { given()[path] = std::move(text); }

std::string worldFile(const std::string& path) {
  const auto it = given().find(path);
  if (it != given().end()) return it->second;
#ifdef WORLD_FILES_GIVEN_ONLY
  return "";  // no files here (the room's physics): only what was handed over
#else
  std::ifstream f("assets/world/" + path, std::ios::binary);
  if (!f) return "";
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
#endif
}

}  // namespace app
