#include "app/piano.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "engine/fetch.h"
#include "engine/jobs.h"
#include "engine/log.h"
#include "engine/synth.h"

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
// On-screen keyboard layout (device pixels): two octaves at the bottom.
constexpr int kWhitesShown = 15;  // two octaves and the next C
Rect keyboardArea(float W, float H) { return {0, H * 0.72f, W, H * 0.28f}; }
Rect topButton(int i, float W, float H) {
  const float bw = W / 5.0f;
  return {bw * static_cast<float>(i) + 3, H * 0.655f, bw - 6, H * 0.06f};
}
const char* kButtons[5] = {"< OCT", "OCT >", "PEDAL", "SONGS", "STAND"};

}  // namespace

void Piano::init(const BarScene& scene) {
  position_ = scene.pianoPosition;
  yaw_ = scene.pianoYaw;
  rotation_ = engine::Quat::axisAngle({0, 1, 0}, yaw_);
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

int Piano::playKey(int i, float gain, double when) {
  const Vec3 where = world({0, 1.0f, 0.8f});
  return samples_[sampleOf(i)].play(gain, &where, when, rateFor(i), engine::Sound::kPiano);
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
  h.lastMs = emscripten_get_now();
  if (h.voice[i] > 0) engine::Sound::stop(h.voice[i], 0, 0.05f);
  h.voice[i] = playKey(i, 0.25f + 0.75f * velocity, 0);
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
  h.lastMs = emscripten_get_now();
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
  h.lastMs = emscripten_get_now();
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
    const int voice = playKey(i, 0.2f + 0.7f * n.velocity, when);
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

int Piano::keyAt(float x, float y, float W, float H) const {
  const Rect area = keyboardArea(W, H);
  if (!area.contains(x, y)) return -1;
  const float ww = W / kWhitesShown;
  const int c0 = 12 * (octave_ + 1);  // MIDI key of the lowest C shown
  // Black keys first (they sit on top, in the upper 60%).
  if (y < area.y + area.h * 0.6f) {
    static const int kBlackAfterWhite[7] = {1, 1, 0, 1, 1, 1, 0};  // C# D# - F# G# A# -
    static const int kWhiteSemis[7] = {0, 2, 4, 5, 7, 9, 11};
    for (int w = 0; w + 1 < kWhitesShown; ++w) {
      if (!kBlackAfterWhite[w % 7]) continue;
      const float bx = ww * static_cast<float>(w + 1);
      if (std::fabs(x - bx) < ww * 0.32f) return c0 + 12 * (w / 7) + kWhiteSemis[w % 7] + 1;
    }
  }
  static const int kWhiteSemis[7] = {0, 2, 4, 5, 7, 9, 11};
  const int w = std::clamp(static_cast<int>(x / ww), 0, kWhitesShown - 1);
  return c0 + 12 * (w / 7) + kWhiteSemis[w % 7];
}

bool Piano::updateKeyboard(const std::vector<engine::Input::RawPress>& presses, float W, float H) {
  bool standUp = false;
  prepareSamples();
  for (const engine::Input::RawPress& p : presses) {
    const float x = p.pos.x * W, y = p.pos.y * H;
    if (p.key == 4 && p.down) standUp = true;  // Escape
    if (p.down) {
      for (int b = 0; b < 5; ++b) {
        if (!topButton(b, W, H).contains(x, y)) continue;
        if (b == 0) octave_ = std::max(0, octave_ - 1);
        if (b == 1) octave_ = std::min(6, octave_ + 1);
        if (b == 2) setPedal(!hands_[kYou].pedal, true);
        if (b == 3) songsRequest_ = true;
        if (b == 4) standUp = true;
      }
      // Keyboard lanes D F J K (computers) play C D E F.
      int key = p.key >= 0 && p.key < 4 ? 12 * (octave_ + 1) + std::array<int, 4>{0, 2, 4, 5}[static_cast<size_t>(p.key)]
                                        : keyAt(x, y, W, H);
      if (key >= 0) {
        keyDown(key, 0.75f, true);
        fingers_.push_back({p.id, key});
      }
    } else {
      for (size_t f = 0; f < fingers_.size(); ++f) {
        if (fingers_[f].id != p.id) continue;
        keyUp(fingers_[f].key, true);
        fingers_.erase(fingers_.begin() + static_cast<long>(f));
        break;
      }
    }
  }
  return standUp;
}

void Piano::drawKeyboard(engine::TextRenderer& text, float W, float H, float px) const {
  const Rect area = keyboardArea(W, H);
  text.addRect(0, H * 0.645f, W, H * 0.08f, {0.05f, 0.03f, 0.07f, 0.85f});
  for (int b = 0; b < 5; ++b) {
    const Rect r = topButton(b, W, H);
    const auto mine = hands_.find(kYou);
    const bool on = b == 2 && mine != hands_.end() && mine->second.pedal;
    text.addRect(r.x, r.y, r.w, r.h, on ? Vec4{1.0f, 0.42f, 0.78f, 0.9f} : Vec4{0.16f, 0.12f, 0.2f, 0.95f});
    engine::TextStyle s;
    s.align = 0.5f;
    const float cap = std::min(r.h * 0.32f, 10 * px);
    text.addScreen(kButtons[b], r.x + r.w / 2, r.y + (r.h - cap) / 2, cap, s);
  }
  const float ww = W / kWhitesShown;
  const int c0 = 12 * (octave_ + 1);
  static const int kWhiteSemis[7] = {0, 2, 4, 5, 7, 9, 11};
  const auto mine = hands_.find(kYou);
  auto down = [&](int key) {
    const int i = key - kFirstKey;
    if (i < 0 || i >= 88 || mine == hands_.end()) return false;
    const Hands& h = mine->second;
    return h.voice[i] > 0 && std::find(h.pedalHeld.begin(), h.pedalHeld.end(), i) == h.pedalHeld.end();
  };
  for (int w = 0; w < kWhitesShown; ++w) {
    const int key = c0 + 12 * (w / 7) + kWhiteSemis[w % 7];
    const Vec4 c = down(key) ? Vec4{1.0f, 0.75f, 0.88f, 1} : Vec4{0.95f, 0.93f, 0.88f, 1};
    text.addRect(ww * static_cast<float>(w) + 1, area.y, ww - 2, area.h, c);
    if (w % 7 == 0) {
      char label[8];
      std::snprintf(label, sizeof(label), "C%d", key / 12 - 1);
      engine::TextStyle s;
      s.align = 0.5f;
      s.color = {0.2f, 0.2f, 0.25f, 1};
      s.outline.w = 0;
      text.addScreen(label, ww * (static_cast<float>(w) + 0.5f), area.y + area.h - 18 * px, 7 * px, s);
    }
  }
  static const int kBlackAfterWhite[7] = {1, 1, 0, 1, 1, 1, 0};
  for (int w = 0; w + 1 < kWhitesShown; ++w) {
    if (!kBlackAfterWhite[w % 7]) continue;
    const int key = c0 + 12 * (w / 7) + kWhiteSemis[w % 7] + 1;
    const float bx = ww * static_cast<float>(w + 1);
    const Vec4 c = down(key) ? Vec4{0.9f, 0.3f, 0.6f, 1} : Vec4{0.05f, 0.05f, 0.06f, 1};
    text.addRect(bx - ww * 0.3f, area.y, ww * 0.6f, area.h * 0.6f, c);
  }
  engine::TextStyle s;
  s.align = 0.5f;
  const std::string playing = song_ >= 0 ? "Playing: " + songs_[static_cast<size_t>(song_)].title : "Piano: play the keys";
  text.addScreen(playing, W * 0.5f, H * 0.615f, 9 * px, s);
}

}  // namespace app
