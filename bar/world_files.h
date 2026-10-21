#pragma once
// The world's files (assets/world/: kinds, places), read the same way
// everywhere: the game has them built in (CMake --embed-file), native tests
// read the repository's copy, and the room's physics (cloud/sim), which has
// no files, is handed their text first.
#include <string>

namespace app {

// The text of assets/world/<path> ("places/bar.json"), or "" if missing.
std::string worldFile(const std::string& path);
// Hands over a file's text (the room does this before building the bar).
void setWorldFile(const std::string& path, std::string text);

}  // namespace app
