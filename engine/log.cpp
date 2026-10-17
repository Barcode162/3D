#include "engine/log.h"

#include <emscripten/emscripten.h>

#include <cstdarg>
#include <cstdio>

namespace engine {

void logLine(LogLevel level, const char* format, ...) {
  char text[1024];
  va_list args;
  va_start(args, format);
  std::vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  EM_ASM({
    var t = UTF8ToString($1);
    if (window['bar3dLog']) window['bar3dLog']['add'](['info', 'warn', 'error', 'fatal'][$0], t);
    else console.log(t);
  }, static_cast<int>(level), text);
}

void setStatus(const char* text) {
  EM_ASM({
    var el = document.getElementById('status');
    if (el) el.textContent = UTF8ToString($0);
  }, text);
}

}  // namespace engine
