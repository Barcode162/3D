#pragma once
// Logging that shows up in the on-screen log panel (web/index.html routes
// stdout/stderr there), so problems are visible on a phone without devtools.
#include <cstdio>

#define LOG_INFO(...)  (std::printf("[info] " __VA_ARGS__), std::printf("\n"))
#define LOG_ERROR(...) (std::fprintf(stderr, "[error] " __VA_ARGS__), std::fprintf(stderr, "\n"))

namespace engine {
// Sets the one-line status text (FPS, resolution) at the top of the screen.
void setStatus(const char* text);
}
