#include "engine/log.h"

#include <emscripten/emscripten.h>

namespace engine {

void setStatus(const char* text) {
  EM_ASM({
    var el = document.getElementById('status');
    if (el) el.textContent = UTF8ToString($0);
  }, text);
}

}  // namespace engine
