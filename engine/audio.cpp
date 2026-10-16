#include "engine/audio.h"

#include <emscripten/emscripten.h>

namespace engine {

// Quoted property names keep the Closure compiler from renaming them.
namespace audio {

bool ready() { return EM_ASM_INT({ return window['bar3dAudio']['ready']() ? 1 : 0; }); }

void setListener(Vec3 p, Vec3 f, Vec3 u) {
  EM_ASM({ window['bar3dAudio']['setListener']($0, $1, $2, $3, $4, $5, $6, $7, $8); },
         p.x, p.y, p.z, f.x, f.y, f.z, u.x, u.y, u.z);
}

void setSongPickerRect(float x, float y, float w, float h) {
  EM_ASM({ window['bar3dAudio']['setSongPickerRect']($0, $1, $2, $3); }, x, y, w, h);
}

int userSongCount() { return EM_ASM_INT({ return window['bar3dAudio']['userSongCount'](); }); }

namespace {
std::string userSongString(int index, bool url) {
  char buffer[1024];
  EM_ASM({
    var a = window['bar3dAudio'];
    stringToUTF8($1 ? a['userSongUrl']($0) : a['userSongName']($0), $2, $3);
  }, index, url, buffer, sizeof(buffer));
  return buffer;
}
}  // namespace

std::string userSongName(int index) { return userSongString(index, false); }
std::string userSongUrl(int index) { return userSongString(index, true); }

}  // namespace audio

void MusicSource::init(float refDistance) {
  id_ = EM_ASM_INT({ return window['bar3dAudio']['createSource']($0); }, refDistance);
}

void MusicSource::play(const std::string& url, double offsetSeconds) {
  EM_ASM({ window['bar3dAudio']['play']($0, UTF8ToString($1), $2); }, id_, url.c_str(), offsetSeconds);
}

void MusicSource::pause() { EM_ASM({ window['bar3dAudio']['pause']($0); }, id_); }

void MusicSource::resume() { EM_ASM({ window['bar3dAudio']['resume']($0); }, id_); }

bool MusicSource::playing() const {
  return EM_ASM_INT({ return window['bar3dAudio']['playing']($0) ? 1 : 0; }, id_);
}

bool MusicSource::finished() const {
  return EM_ASM_INT({ return window['bar3dAudio']['finished']($0) ? 1 : 0; }, id_);
}

void MusicSource::setPosition(Vec3 p) {
  EM_ASM({ window['bar3dAudio']['setPosition']($0, $1, $2, $3); }, id_, p.x, p.y, p.z);
}

}  // namespace engine
