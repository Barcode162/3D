#include "bar/piano.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "looped/frame_time.h"
#include "looped/fetch.h"
#include "looped/jobs.h"
#include "looped/log.h"
#include "looped/synth.h"

using engine::Vec3;
using engine::Vec4;

namespace app {
namespace {

constexpr int kFirstKey = 21;          // A0
constexpr double kLookAhead = 0.35;    // seconds of notes scheduled ahead
constexpr double kStartDelay = 0.4;    // a new song starts this long after you pick it

// The keyboard and strings in the piano's model space (tools/assets/piano.py):
// keys along x (bass at -x), the player at -z.
constexpr float kWhiteW = 0.02345f;
bool isBlack(int i) {  // i = 0..87 (A0..C8)
  const int n = (i + 9) % 12;
  return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}
float keyX(int i) {
  static float xs[88];
  static bool made = false;
  if (!made) {
    int whites = 0;
    for (int k = 0; k < 88; ++k) whites += isBlack(k) ? 0 : 1;
    const float start = -whites * kWhiteW / 2;
    int w = 0;
    for (int k = 0; k < 88; ++k) {
      if (isBlack(k)) {
        xs[k] = start + w * kWhiteW;  // on the line between its neighbors
      } else {
        xs[k] = start + w * kWhiteW + kWhiteW / 2;
        ++w;
      }
    }
    made = true;
  }
  return xs[std::clamp(i, 0, 87)];
}

// How far back the case reaches at x (piano.py's inside_z_end, sampled
// every 5 cm from x = -0.70).
float insideEnd(float x) {
  static const float kTable[] = {2.017f, 2.084f, 2.119f, 2.139f, 2.148f, 2.15f,  2.144f, 2.13f,  2.107f, 2.069f,
                                 2.005f, 1.841f, 1.7f,   1.604f, 1.528f, 1.466f, 1.411f, 1.361f, 1.316f, 1.275f,
                                 1.235f, 1.196f, 1.157f, 1.117f, 1.074f, 1.028f, 0.973f, 0.902f, 0.793f};
  const float f = std::clamp((x + 0.70f) / 0.05f, 0.0f, 27.999f);
  const int i = static_cast<int>(f);
  return kTable[i] + (kTable[i + 1] - kTable[i]) * (f - static_cast<float>(i));
}

// A string's two ends for key i: the bass strings fan across the others.
void stringEnds(int i, Vec3& a, Vec3& b) {
  if (i < 20) {
    const float t = static_cast<float>(i) / 19.0f;
    const float x = -0.69f + 0.29f * t, ex = -0.62f + t * 0.55f;
    a = {x, 0.905f, 0.45f};
    b = {ex, 0.90f, insideEnd(ex) - 0.07f};
  } else {
    const float x = -0.36f + 1.02f * static_cast<float>(i - 20) / 67.0f;
    a = {x, 0.875f, 0.42f};
    b = {x * 0.97f + 0.01f, 0.875f, insideEnd(x) - 0.06f};
  }
}

struct Rect {
  float x, y, w, h;
  bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};
// The on-screen keyboard (J108.2), in device pixels (px = device pixels per
// CSS pixel): from the bottom up, the keys (about 40% of the screen), a strip
// with all 88 keys (the window you play, drag it to slide), the buttons and
// the song playing.
struct KeyLayout {
  float W, H, px;
  Rect keys, strip, buttons;
  int whites;  // white keys in view
};
KeyLayout keyLayout(float W, float H, float px) {
  KeyLayout k{W, H, px, {}, {}, {}, 0};
  const float bottom = H - 6 * px;
  const float keyH = std::min(H * 0.4f, W * 0.95f);
  k.keys = {0, bottom - keyH, W, keyH};
  k.strip = {6 * px, k.keys.y - 34 * px, W - 12 * px, 26 * px};
  k.buttons = {6 * px, k.strip.y - 52 * px, W - 12 * px, 44 * px};
  k.whites = std::clamp(static_cast<int>(std::lround(W / (46 * px))), 7, 24);
  return k;
}
Rect topButton(int i, const KeyLayout& k) {
  const float bw = k.buttons.w / 3.0f;
  return {k.buttons.x + bw * static_cast<float>(i) + 3 * k.px, k.buttons.y, bw - 6 * k.px, k.buttons.h};
}
const char* kButtons[3] = {"SONGS", "PEDAL", "STAND"};
// The 52 white keys of the 88, as MIDI keys (A0 = 21 ... C8 = 108).
const std::array<int, 52>& whiteKeys() {
  static const std::array<int, 52> w = [] {
    std::array<int, 52> out{};
    int n = 0;
    for (int key = 21; key <= 108; ++key)
      if (!isBlack(key - 21)) out[static_cast<size_t>(n++)] = key;
    return out;
  }();
  return w;
}

}  // namespace

void Piano::init(const BarScene& scene) {
  position_ = scene.pianoPosition;
  yaw_ = scene.pianoYaw;
  rotation_ = engine::Quat::axisAngle({0, 1, 0}, yaw_);
  LOG_INFO("TEMP piano at %.2f %.2f %.2f", position_.x, position_.y, position_.z);
  LOG_INFO("TEMP piano at %.2f %.2f %.2f", position_.x, position_.y, position_.z);
  const char* files[10][2] = {
      {"Fur Elise - Beethoven", "01-fur-elise"},
      {"Moonlight Sonata - Beethoven", "02-moonlight-sonata"},
      {"Prelude in C - Bach", "03-bach-prelude-c"},
      {"Rondo alla Turca - Mozart", "04-rondo-alla-turca"},
      {"Nocturne Op 9 No 2 - Chopin", "05-nocturne-op9-2"},
      {"Minute Waltz - Chopin", "06-minute-waltz"},
      {"Clair de Lune - Debussy", "07-clair-de-lune"},
      {"Gymnopedie No 1 - Satie", "08-gymnopedie-1"},
      {"Sonata Facile - Mozart", "09-sonata-facile"},
      {"Prelude in E minor - Chopin", "10-prelude-e-minor"},
  };
  for (const auto& f : files) songs_.push_back({f[0], std::string("assets/midi/") + f[1] + ".mid", true});
  loaded_.resize(songs_.size());
}

void Piano::addUserSong(const std::string& title, const std::string& url) {
  songs_.push_back({title, url, false});
  loaded_.emplace_back();
  LOG_INFO("Piano: added your song \"%s\".", title.c_str());
}

Vec3 Piano::world(Vec3 local) const { return position_ + rotation_.rotate(local); }

float Piano::rateFor(int i) {
  return std::pow(2.0f, static_cast<float>(i + kFirstKey - sampleKey(sampleOf(i))) / 12.0f);
}

void Piano::ensureSample(int i) {
  engine::Sound& s = samples_[sampleOf(i)];
  if (!s.valid()) s.create(engine::synth::pianoNote(sampleKey(sampleOf(i))), engine::synth::kRate);
}

void Piano::prepareSamples() {
  if (preparing_) return;
  preparing_ = true;
  // Middle of the keyboard first (most played), then outwards; one per job.
  for (int d = 0; d < kSamples; ++d) {
    const int s = kSamples / 2 + ((d % 2) ? -(d + 1) / 2 : d / 2);
    if (s < 0 || s >= kSamples) continue;
    engine::queueJob([this, s] { ensureSample(3 * s); }, engine::JobLane::kBackground, "piano-sample");
  }
}

void Piano::loadRecorded() {
  if (recordedAsked_) return;
  recordedAsked_ = true;
  static const char* kNames[4] = {"A", "C", "Ds", "Fs"};
  static const int kLayerOrder[kLayers] = {1, 2, 0, 3};  // the middle loudness first, the loudest last
  static const int kLayerFile[kLayers] = {4, 8, 12, 16};  // Salamander's layers (of 16)
  for (int l : kLayerOrder) {
    for (int d = 0; d < kSamples; ++d) {  // middle of the keyboard first, then outwards
      const int s = kSamples / 2 + ((d % 2) ? -(d + 1) / 2 : d / 2);
      if (s < 0 || s >= kSamples) continue;
      const int key = 21 + 3 * s;
      recorded_[s][l].load(std::string("assets/piano/") + kNames[s % 4] + std::to_string((key - 12) / 12) + "v" +
                           std::to_string(kLayerFile[l]) + ".mp3");
    }
  }
  LOG_INFO("Piano: loading the recorded grand piano (Salamander, %d notes).", kSamples * kLayers);
}

int Piano::playKey(int i, float velocity, double when) {
  // All notes come from one shared 3D emitter at the piano (cheaper than a
  // 3D panner per note).
  if (emitter_ == 0) emitter_ = engine::audio::createEmitter(world({0, 1.0f, 0.8f}), true);
  loadRecorded();
  // The nearest recorded note (at most a semitone away), in the layer for
  // this loudness (its tone: soft is mellow, hard is bright); the gain makes
  // the rest of the range, pianissimo (about -24 dB) to fortissimo (0 dB),
  // J108.1.
  const int s = std::clamp((i + 1) / 3, 0, kSamples - 1);
  const float v = std::clamp(velocity, 0.0f, 1.0f);
  const int layer = v < 0.3f ? 0 : v < 0.55f ? 1 : v < 0.8f ? 2 : 3;
  const float gain = 0.06f + 0.94f * std::pow(v, 1.8f);
  const float rate = std::pow(2.0f, static_cast<float>(i - 3 * s) / 12.0f);
  for (int l : {layer, 1, 2, 0, 3}) {  // that layer, or the nearest that has arrived
    if (recorded_[s][l].loaded()) return recorded_[s][l].playAt(emitter_, gain, when, rate);
  }
  return samples_[sampleOf(i)].playAt(emitter_, gain, when, rateFor(i));
}

void Piano::releaseAll(Hands& h) {
  for (int i = 0; i < 88; ++i) {
    if (h.voice[i] > 0) engine::Sound::stop(h.voice[i], 0, 0.2f);
    h.voice[i] = 0;
  }
  h.pedalHeld.clear();
  h.pedal = false;
}

// A key is lit while any player holds it (not just the pedal).
bool Piano::keyLit(int i, double& since) const {
  bool lit = false;
  for (const auto& [player, h] : hands_) {
    if (h.voice[i] > 0) {
      lit = true;
      since = std::min(since, engine::audio::now() - h.start[i]);
    }
  }
  return lit;
}

std::string Piano::nowPlaying() const {
  if (song_ < 0) return "";
  return songs_[static_cast<size_t>(song_)].title;
}

void Piano::playSong(int index, double serverStart, bool fromNetwork) {
  if (index < 0 || index >= static_cast<int>(songs_.size())) return;
  stopSong(true);
  song_ = index;
  nextNote_ = 0;
  songServerStart_ = serverStart;
  owner_ = !fromNetwork;
  if (!fromNetwork) {
    // Start a moment from now; tell the others when (built-in songs only:
    // your own files stay on your phone, so the others' piano goes quiet).
    const double now = engine::net::serverTime();
    songServerStart_ = now > 0 ? now + kStartDelay : 0;
    const bool shared = songs_[static_cast<size_t>(index)].builtIn;
    const double v[2] = {shared ? static_cast<double>(index) : -1.0, shared ? songServerStart_ : 0.0};
    engine::net::setWorld("pso", v, 2);
  }
  waitingToLoad_ = true;
  LoadedSong& l = loaded_[static_cast<size_t>(index)];
  if (!l.ready && !l.loading) {
    l.loading = true;
    engine::fetchFile(songs_[static_cast<size_t>(index)].url, [this, index](bool ok, std::vector<uint8_t> bytes) {
      LoadedSong& song = loaded_[static_cast<size_t>(index)];
      song.loading = false;
      if (!ok || !engine::midi::parse(bytes, song.midi)) {
        LOG_ERROR("Piano: could not read %s", songs_[static_cast<size_t>(index)].url.c_str());
        if (song_ == index) song_ = -1;
        return;
      }
      song.ready = true;
      LOG_INFO("Piano: %s loaded (%zu notes, %.0f s).", songs_[static_cast<size_t>(index)].title.c_str(),
               song.midi.notes.size(), song.midi.duration);
    });
  }
  LOG_INFO("Piano: playing %s.", songs_[static_cast<size_t>(index)].title.c_str());
}

void Piano::stopSong(bool fromNetwork) {
  if (song_ < 0) return;
  for (const Active& a : active_) engine::Sound::stop(a.voice, 0, 0.2f);  // also the ones not started yet
  active_.clear();
  song_ = -1;
  waitingToLoad_ = false;
  if (!fromNetwork) {
    const double v[2] = {-1, 0};
    engine::net::setWorld("pso", v, 2);
  }
}

void Piano::keyDown(int key, float velocity, bool share, int player) {
  const int i = key - kFirstKey;
  if (i < 0 || i >= 88) return;
  ensureSample(i);  // only if its background job hasn't made it yet
  Hands& h = hands_[player];
  h.lastMs = engine::frameTime().nowMs;
  if (h.voice[i] > 0) engine::Sound::stop(h.voice[i], 0, 0.05f);
  h.voice[i] = playKey(i, velocity, 0);
  h.start[i] = engine::audio::now();
  h.pedalHeld.erase(std::remove(h.pedalHeld.begin(), h.pedalHeld.end(), i), h.pedalHeld.end());
  if (share) {
    const double v[3] = {static_cast<double>(key), velocity, 1};
    engine::net::sendEvent("pk", v, 3);
  }
}

void Piano::keyUp(int key, bool share, int player) {
  const int i = key - kFirstKey;
  const auto it = hands_.find(player);
  if (i < 0 || i >= 88 || it == hands_.end() || it->second.voice[i] == 0) return;
  Hands& h = it->second;
  h.lastMs = engine::frameTime().nowMs;
  if (h.pedal) {
    h.pedalHeld.push_back(i);
  } else {
    engine::Sound::stop(h.voice[i], 0, 0.15f);
    h.voice[i] = 0;
  }
  if (share) {
    const double v[3] = {static_cast<double>(key), 0, 0};
    engine::net::sendEvent("pk", v, 3);
  }
}

void Piano::setPedal(bool down, bool share, int player) {
  Hands& h = hands_[player];
  h.lastMs = engine::frameTime().nowMs;
  if (h.pedal == down) return;
  h.pedal = down;
  if (!down) {
    for (int i : h.pedalHeld) {
      engine::Sound::stop(h.voice[i], 0, 0.2f);
      h.voice[i] = 0;
    }
    h.pedalHeld.clear();
  }
  if (share) {
    const double v[3] = {-1, 0, down ? 1.0 : 0.0};
    engine::net::sendEvent("pk", v, 3);
  }
}

void Piano::update(double nowMs, std::vector<engine::net::WorldEntry>& entries, std::vector<engine::net::Event>& events) {
  // Someone who went quiet for 10 s (or left) lets go of everything.
  for (auto it = hands_.begin(); it != hands_.end();) {
    if (it->first != kYou && nowMs - it->second.lastMs > 10000) {
      releaseAll(it->second);
      it = hands_.erase(it);
    } else {
      ++it;
    }
  }
  for (const engine::net::WorldEntry& e : entries) {
    if (e.key != "pso" || e.values.size() < 2) continue;
    const int index = static_cast<int>(e.values[0]);
    if (index < 0) stopSong(true);
    else if (index != song_ || std::fabs(e.values[1] - songServerStart_) > 1.0) playSong(index, e.values[1], true);
  }
  for (const engine::net::Event& e : events) {
    if (e.type != "pk" || e.values.size() < 3) continue;
    const int key = static_cast<int>(e.values[0]);
    if (key < 0) setPedal(e.values[2] != 0, false, e.player);
    else if (e.values[2] != 0) keyDown(key, static_cast<float>(e.values[1]), false, e.player);
    else keyUp(key, false, e.player);
  }

  const double now = engine::audio::now();
  // Schedule further ahead when frames come slowly, so no note is late.
  const double lookAhead = std::clamp(now - lastUpdate_ + 0.2, kLookAhead, 2.0);
  lastUpdate_ = now;
  // Lights of finished notes go out.
  active_.erase(std::remove_if(active_.begin(), active_.end(), [&](const Active& a) { return a.endTime < now; }),
                active_.end());
  if (song_ < 0) return;
  LoadedSong& l = loaded_[static_cast<size_t>(song_)];
  if (!l.ready) return;
  // Joined after someone else's song ended (its start is still in the
  // shared world): nothing to play or prepare. Checked on the server clock,
  // so it works before sound is unlocked.
  if (!owner_ && songServerStart_ > 0) {
    const double server = engine::net::serverTime();
    if (server > 0 && server > songServerStart_ + l.midi.duration + 1.0) {
      LOG_INFO("Piano: song already ended.");
      song_ = -1;
      return;
    }
  }
  if (now <= 0) return;
  const std::vector<engine::midi::Note>& notes = l.midi.notes;
  if (waitingToLoad_) {
    waitingToLoad_ = false;
    // Line the song's start up with the server clock (so everyone hears the
    // same bar), or start shortly from now.
    const double server = engine::net::serverTime();
    songStartSound_ = songServerStart_ > 0 && server > 0 ? now - (server - songServerStart_) : now + kStartDelay;
    // Joined mid-song: start at the first note still ahead (notes are
    // sorted by start time).
    const double since = now - 0.05 - songStartSound_;
    nextNote_ = static_cast<size_t>(
        std::lower_bound(notes.begin(), notes.end(), since,
                         [](const engine::midi::Note& n, double t) { return n.start < t; }) -
        notes.begin());
  }
  // Samples are made by background jobs (prepareSamples); a note whose
  // sample isn't ready yet is skipped rather than made inside the frame.
  prepareSamples();
  while (nextNote_ < notes.size()) {
    const engine::midi::Note& n = notes[nextNote_];
    const double t = songStartSound_ + n.start;
    if (t > now + lookAhead) break;
    const int i = n.key - kFirstKey;
    ++nextNote_;
    if (t < now - 0.05 || i < 0 || i >= 88) continue;  // joined late: skip notes already past
    if (!sampleReady(i)) continue;
    const double when = t - now;
    const int voice = playKey(i, n.velocity, when);
    engine::Sound::stop(voice, when + n.length, 0.18f);
    active_.push_back({n.key, t + n.length, t, n.velocity, voice});
  }
  if (nextNote_ >= notes.size() && now > songStartSound_ + l.midi.duration + 1.0) {
    LOG_INFO("Piano: song finished.");
    song_ = -1;
    if (owner_) {  // so people who join later don't prepare a finished song
      const double v[2] = {-1, 0};
      engine::net::setWorld("pso", v, 2);
      owner_ = false;
    }
  }
}

void Piano::hands(float& left, float& right) const {
  const double now = engine::audio::now();
  float sumL = 0, sumR = 0;
  int nL = 0, nR = 0;
  auto add = [&](int key) {
    const float x = (static_cast<float>(key) - 64.5f) / 44.0f;
    if (key < 60) {
      sumL += x;
      ++nL;
    } else {
      sumR += x;
      ++nR;
    }
  };
  for (const Active& a : active_) {
    if (a.startTime <= now && now - a.startTime < 0.6) add(a.key);
  }
  for (int i = 0; i < 88; ++i) {
    double since = 99;
    if (keyLit(i, since)) add(i + kFirstKey);
  }
  left = nL ? sumL / static_cast<float>(nL) : -0.15f;
  right = nR ? sumR / static_cast<float>(nR) : 0.15f;
}

void Piano::addBoxes(std::vector<SceneRenderer::Box>& out, double nowMs) const {
  const double now = engine::audio::now();
  float glowKey[88] = {}, since[88];
  for (float& s : since) s = 99;
  for (const Active& a : active_) {
    if (a.startTime > now) continue;
    const int i = a.key - kFirstKey;
    if (i < 0 || i >= 88) continue;
    glowKey[i] = std::max(glowKey[i], 0.4f + 0.6f * a.velocity);
    since[i] = std::min(since[i], static_cast<float>(now - a.startTime));
  }
  for (int i = 0; i < 88; ++i) {
    double held = 99;
    if (keyLit(i, held)) {
      glowKey[i] = 1.0f;
      since[i] = std::min(since[i], static_cast<float>(held));
    }
  }
  const float t = static_cast<float>(nowMs / 1000.0);
  for (int i = 0; i < 88; ++i) {
    if (glowKey[i] <= 0) continue;
    // The key: a colored light on top (bass keys blue, treble pink), a bit down.
    const bool black = isBlack(i);
    const float x = keyX(i);
    SceneRenderer::Box key;
    key.center = world(black ? Vec3{x, 0.738f, -0.055f} : Vec3{x, 0.727f, -0.078f});
    key.size = black ? Vec3{0.012f, 0.012f, 0.095f} : Vec3{0.0215f, 0.008f, 0.152f};
    key.rotation = rotation_;
    const float hue = static_cast<float>(i) / 87.0f;
    key.color = Vec3{0.3f + 0.7f * hue, 0.45f, 1.0f - 0.6f * hue};
    key.glow = 2.2f * glowKey[i];
    out.push_back(key);
    // Its string(s): glowing gold, shimmering while it rings (fading in 1.5 s).
    Vec3 a, b;
    stringEnds(i, a, b);
    const float ring = std::max(0.0f, 1.0f - since[i] / 1.5f);
    if (ring <= 0) continue;
    SceneRenderer::Box str;
    const Vec3 d = b - a;
    str.center = world((a + b) * 0.5f);
    const float shake = 0.004f + 0.003f * ring * std::fabs(std::sin(t * 90.0f + static_cast<float>(i)));
    str.size = {shake, 0.003f, engine::length(d)};
    str.rotation = rotation_ * engine::Quat::axisAngle({0, 1, 0}, std::atan2(d.x, d.z));
    str.color = {1.0f, 0.8f, 0.4f};
    str.glow = 2.5f * ring;
    str.metallic = 1;
    str.roughness = 0.3f;
    out.push_back(str);
  }
}

// ---- On-screen keyboard ----

void Piano::closeKeyboard() {
  keyboard_ = false;
  for (const Finger& f : fingers_) keyUp(f.key, true);
  fingers_.clear();
  setPedal(false, true);
}

// The key under (x, y) and how hard: higher up the key is softer, lower is
// louder (pianissimo to fortissimo, J108.1). -1 when it's not on a key.
int Piano::keyAt(float x, float y, float W, float H, float px, float* velocity) const {
  const KeyLayout k = keyLayout(W, H, px);
  if (!k.keys.contains(x, y)) return -1;
  const float ww = k.keys.w / static_cast<float>(k.whites);
  const int first = std::clamp(firstWhite_, 0, 52 - k.whites);
  const float rel = (y - k.keys.y) / k.keys.h;
  if (velocity) *velocity = std::clamp(0.12f + 0.95f * rel, 0.1f, 1.0f);
  const auto& whites = whiteKeys();
  if (rel < 0.62f) {  // black keys first: they sit on top
    for (int w = 0; w + 1 < k.whites; ++w) {
      const int key = whites[static_cast<size_t>(first + w)];
      if (key + 1 > 108 || !isBlack(key + 1 - 21)) continue;
      const float bx = ww * static_cast<float>(w + 1);
      if (std::fabs(x - bx) < ww * 0.3f) {
        if (velocity) *velocity = std::clamp(0.12f + 0.95f * rel / 0.62f, 0.1f, 1.0f);
        return key + 1;
      }
    }
  }
  const int w = std::clamp(static_cast<int>(x / ww), 0, k.whites - 1);
  return whites[static_cast<size_t>(first + w)];
}

bool Piano::updateKeyboard(const engine::InputFrame& frame, float W, float H, float px) {
  bool standUp = false;
  prepareSamples();
  const KeyLayout k = keyLayout(W, H, px);
  const int maxFirst = 52 - k.whites;
  firstWhite_ = std::clamp(firstWhite_, 0, maxFirst);
  // The strip: the window follows the finger (tap or drag).
  const auto stripTo = [&](float x) {
    const float t = std::clamp((x - k.strip.x) / k.strip.w, 0.0f, 1.0f);
    firstWhite_ = std::clamp(static_cast<int>(std::lround(t * 52.0f - k.whites * 0.5f)), 0, maxFirst);
  };
  for (const engine::Input::RawPress& p : frame.presses) {
    const float x = p.pos.x * W, y = p.pos.y * H;
    if (p.key == 4 && p.down) standUp = true;  // Escape
    if (p.down) {
      for (int b = 0; b < 3; ++b) {
        if (!topButton(b, k).contains(x, y)) continue;
        if (b == 0) songsRequest_ = true;
        if (b == 1) setPedal(!hands_[kYou].pedal, true);
        if (b == 2) standUp = true;
      }
      if (k.strip.contains(x, y)) {
        stripTo(x);
        stripFinger_ = p.id;
        continue;
      }
      // Keyboard lanes D F J K (computers) play the first four white keys in view.
      float velocity = 0.75f;
      const int key = p.key >= 0 && p.key < 4 ? whiteKeys()[static_cast<size_t>(firstWhite_ + p.key)]
                                              : keyAt(x, y, W, H, px, &velocity);
      if (key >= 0) {
        keyDown(key, velocity, true);
        fingers_.push_back({p.id, key, velocity});
      }
    } else {
      if (p.id == stripFinger_) stripFinger_ = -1;
      for (size_t f = 0; f < fingers_.size(); ++f) {
        if (fingers_[f].id != p.id) continue;
        keyUp(fingers_[f].key, true);
        fingers_.erase(fingers_.begin() + static_cast<long>(f));
        break;
      }
    }
  }
  // Fingers that slide: along the strip it moves the window; over the keys a
  // glissando (the old key lets go, the new one plays).
  for (const engine::Input::Pointer& ptr : frame.pointers) {
    const float x = ptr.pos.x * W, y = ptr.pos.y * H;
    if (ptr.id == stripFinger_) {
      stripTo(x);
      continue;
    }
    for (Finger& f : fingers_) {
      if (f.id != ptr.id) continue;
      float velocity = f.velocity;
      const int key = keyAt(x, y, W, H, px, &velocity);
      if (key >= 0 && key != f.key) {
        keyUp(f.key, true);
        keyDown(key, f.velocity, true);
        f.key = key;
      }
    }
  }
  return standUp;
}

void Piano::drawKeyboard(engine::TextRenderer& text, float W, float H, float px) const {
  const KeyLayout k = keyLayout(W, H, px);
  const int first = std::clamp(firstWhite_, 0, 52 - k.whites);
  const auto& whites = whiteKeys();
  // A panel behind it all.
  text.addRect(0, k.buttons.y - 30 * px, W, H - (k.buttons.y - 30 * px), {0.04f, 0.03f, 0.05f, 0.88f});
  engine::TextStyle st;
  st.align = 0.5f;
  const std::string playing = song_ >= 0 ? "Playing: " + songs_[static_cast<size_t>(song_)].title
                                         : "Press higher on a key for soft, lower for loud";
  text.addScreen(playing, W * 0.5f, k.buttons.y - 22 * px, 8 * px, st);
  // Buttons.
  const auto mine = hands_.find(kYou);
  for (int b = 0; b < 3; ++b) {
    const Rect r = topButton(b, k);
    const bool on = b == 1 && mine != hands_.end() && mine->second.pedal;
    text.addRect(r.x, r.y, r.w, r.h, on ? Vec4{0.95f, 0.72f, 0.35f, 0.95f} : Vec4{0.17f, 0.14f, 0.2f, 0.95f});
    text.addRect(r.x, r.y, r.w, 2 * px, {1, 1, 1, 0.12f});
    engine::TextStyle s;
    s.align = 0.5f;
    if (on) s.color = {0.1f, 0.06f, 0.02f, 1};
    const float cap = std::min(r.h * 0.3f, 10 * px);
    text.addScreen(b == 1 && on ? "PEDAL ON" : kButtons[b], r.x + r.w / 2, r.y + (r.h - cap) / 2, cap, s);
  }
  // The 88-key strip: every white key a sliver, black keys dark, the window
  // you play outlined.
  const float sw = k.strip.w / 52.0f;
  text.addRect(k.strip.x, k.strip.y, k.strip.w, k.strip.h, {0.12f, 0.1f, 0.12f, 1});
  for (int w = 0; w < 52; ++w) {
    const bool in = w >= first && w < first + k.whites;
    text.addRect(k.strip.x + sw * static_cast<float>(w) + 0.5f * px, k.strip.y + 2 * px, sw - px, k.strip.h - 4 * px,
                 in ? Vec4{0.95f, 0.92f, 0.86f, 1} : Vec4{0.45f, 0.43f, 0.4f, 1});
    const int key = whites[static_cast<size_t>(w)];
    if (key + 1 <= 108 && isBlack(key + 1 - 21))
      text.addRect(k.strip.x + sw * static_cast<float>(w + 1) - sw * 0.3f, k.strip.y + 2 * px, sw * 0.6f,
                   (k.strip.h - 4 * px) * 0.6f, {0.05f, 0.05f, 0.06f, 1});
  }
  const float wx = k.strip.x + sw * static_cast<float>(first);
  for (const Rect& e : {Rect{wx - px, k.strip.y - px, sw * k.whites + 2 * px, 2 * px},
                        Rect{wx - px, k.strip.y + k.strip.h - px, sw * k.whites + 2 * px, 2 * px},
                        Rect{wx - 2 * px, k.strip.y - px, 2 * px, k.strip.h + 2 * px},
                        Rect{wx + sw * k.whites, k.strip.y - px, 2 * px, k.strip.h + 2 * px}})
    text.addRect(e.x, e.y, e.w, e.h, {0.95f, 0.72f, 0.35f, 1});
  // The keys: ivory with a shaded lip, pressed keys sink and take a warm tint;
  // black keys with a glossy top.
  auto down = [&](int key) {
    const int i = key - kFirstKey;
    if (i < 0 || i >= 88 || mine == hands_.end()) return false;
    const Hands& h = mine->second;
    return h.voice[i] > 0 && std::find(h.pedalHeld.begin(), h.pedalHeld.end(), i) == h.pedalHeld.end();
  };
  const float ww = k.keys.w / static_cast<float>(k.whites);
  text.addRect(0, k.keys.y - 3 * px, W, 3 * px, {0.5f, 0.08f, 0.08f, 1});  // the felt strip
  for (int w = 0; w < k.whites; ++w) {
    const int key = whites[static_cast<size_t>(first + w)];
    const bool d = down(key);
    const float x = ww * static_cast<float>(w) + px, kw = ww - 2 * px, h = k.keys.h - (d ? 0 : 6 * px);
    text.addRect(x, k.keys.y, kw, h, d ? Vec4{0.98f, 0.86f, 0.7f, 1} : Vec4{0.97f, 0.95f, 0.9f, 1});
    text.addRect(x, k.keys.y, kw, 10 * px, {0, 0, 0, d ? 0.18f : 0.1f});  // shade under the black keys' rail
    if (!d) text.addRect(x, k.keys.y + h, kw, 6 * px, {0.72f, 0.69f, 0.63f, 1});  // the front lip
    if (key % 12 == 0) {
      char label[8];
      std::snprintf(label, sizeof(label), "C%d", key / 12 - 1);
      engine::TextStyle s;
      s.align = 0.5f;
      s.color = {0.35f, 0.33f, 0.3f, 1};
      s.outline.w = 0;
      text.addScreen(label, x + kw / 2, k.keys.y + h - 20 * px, 7 * px, s);
    }
  }
  for (int w = 0; w + 1 < k.whites; ++w) {
    const int key = whites[static_cast<size_t>(first + w)] + 1;
    if (key > 108 || !isBlack(key - 21)) continue;
    const bool d = down(key);
    const float bx = ww * static_cast<float>(w + 1), bw = ww * 0.58f, bh = k.keys.h * 0.62f - (d ? 0 : 4 * px);
    text.addRect(bx - bw / 2, k.keys.y, bw, bh, d ? Vec4{0.45f, 0.25f, 0.12f, 1} : Vec4{0.06f, 0.055f, 0.065f, 1});
    text.addRect(bx - bw / 2 + 2 * px, k.keys.y, bw - 4 * px, bh * 0.85f, d ? Vec4{0.55f, 0.32f, 0.16f, 1}
                                                                          : Vec4{0.14f, 0.13f, 0.15f, 1});
    text.addRect(bx - bw / 2 + 3 * px, k.keys.y + 4 * px, 2 * px, bh * 0.7f, {1, 1, 1, 0.12f});  // the gloss
  }
}

}  // namespace app
