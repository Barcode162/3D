#include "engine/audio.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdio>

#include "engine/bridge.h"

namespace engine {

// Quoted property names keep the Closure compiler from renaming them.
namespace audio {

bool ready() {
  if (bridge::frame().pulledMs > 0) return bridge::frame().audioRunning;  // from the page mailbox
  return EM_ASM_INT({ return window['bar3dAudio']['ready']() ? 1 : 0; });
}

// Where your ears are: sent with the frame's mailbox flush.
void setListener(Vec3 p, Vec3 f, Vec3 u) {
  char op[256];
  std::snprintf(op, sizeof(op), "[\"listener\",%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f]", p.x, p.y, p.z, f.x,
                f.y, f.z, u.x, u.y, u.z);
  bridge::op(op);
}

void setSongPickerRect(float x, float y, float w, float h) {
  EM_ASM({ window['bar3dAudio']['setSongPickerRect']($0, $1, $2, $3); }, x, y, w, h);
}

int userSongCount() {
  if (bridge::frame().pulledMs > 0) return bridge::frame().userSongCount;  // from the page mailbox
  return EM_ASM_INT({ return window['bar3dAudio']['userSongCount'](); });
}

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

void setMusicGain(float gain) { EM_ASM({ window['bar3dAudio']['setMusicGain']($0); }, gain); }

// The sound clock: read once per frame by the page mailbox, plus the time
// since then (so it's one call per frame however often it's asked).
double now() {
  const bridge::Frame& f = bridge::frame();
  if (f.pulledMs <= 0) return EM_ASM_DOUBLE({ return window['bar3dAudio']['now'](); });
  if (f.audioTime <= 0) return 0;  // not unlocked yet
  return f.audioTime + (emscripten_get_now() - f.pulledMs) / 1000.0;
}

}  // namespace audio

void Sound::create(const std::vector<float>& samples, float rate) {
  id_ = EM_ASM_INT({ return window['bar3dAudio']['soundCreate'](HEAPF32, $0 >> 2, $1, $2); }, samples.data(),
                   static_cast<int>(samples.size()), rate);
}

int Sound::play(float gain, const Vec3* where, double when, float rate, Bus bus, bool loop) const {
  if (id_ < 0) return 0;
  const Vec3 p = where ? *where : Vec3{};
  return EM_ASM_INT({
    return window['bar3dAudio']['soundPlay']($0, $1, $2, $3, $4, $5, $6, $7, $8 ? 'piano' : 'sfx', $9);
  }, id_, gain, where != nullptr, p.x, p.y, p.z, when, rate, bus == kPiano, loop);
}

void Sound::stop(int voice, double when, float release) {
  if (voice > 0) EM_ASM({ window['bar3dAudio']['soundStop']($0, $1, $2); }, voice, when, release);
}

void Sound::set(int voice, float gain, float rate) {
  if (voice > 0) EM_ASM({ window['bar3dAudio']['soundSet']($0, $1, $2); }, voice, gain, rate);
}

void Track::load(const std::string& url) {
  EM_ASM({ window['bar3dAudio']['trackLoad'](UTF8ToString($0)); }, url.c_str());
}

Track::State Track::state() {
  return static_cast<State>(EM_ASM_INT({ return window['bar3dAudio']['trackState'](); }));
}

std::vector<float> Track::takeSamples(float& rate) {
  const int n = EM_ASM_INT({ return window['bar3dAudio']['trackLength'](); });
  rate = static_cast<float>(EM_ASM_DOUBLE({ return window['bar3dAudio']['trackRate'](); }));
  std::vector<float> samples(static_cast<size_t>(std::max(n, 0)));
  if (n > 0) {
    EM_ASM({
      var m = window['bar3dAudio']['trackTakeMono']();
      if (m) HEAPF32.set(m, $0 >> 2);
    }, samples.data());
  }
  return samples;
}

bool Track::play(double delaySeconds) {
  return EM_ASM_INT({ return window['bar3dAudio']['trackPlay']($0) ? 1 : 0; }, delaySeconds) != 0;
}

double Track::time() { return EM_ASM_DOUBLE({ return window['bar3dAudio']['trackTime'](); }); }
double Track::duration() { return EM_ASM_DOUBLE({ return window['bar3dAudio']['trackDuration'](); }); }
void Track::stop() { EM_ASM({ window['bar3dAudio']['trackStop'](); }); }

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
