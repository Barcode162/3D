#include "bar/projects/mania/rhythm.h"
#include "bar/projects/mania/pack.h"
#include "bar/projects/mania/stars.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "looped/audio.h"
#include "looped/bridge.h"
#include "looped/fetch.h"
#include "looped/font.h"
#include "looped/frame_time.h"
#include "looped/json.h"
#include "looped/log.h"
#include "looped/ui.h"

using engine::Vec2;
using engine::Vec4;

namespace app {
namespace {

// osu!mania-style timing windows (ms either side of the note), and what
// each judgement is worth. Accuracy counts MAX like 300; score gives MAX a
// little more.
constexpr double kWindowMs[5] = {16, 40, 73, 103, 127};  // MAX, 300, 200, 100, 50
constexpr double kMissMs = 150;  // presses earlier than this do nothing; notes later than this are missed
constexpr double kAccValue[6] = {300, 300, 200, 100, 50, 0};
constexpr double kScoreValue[6] = {320, 300, 200, 100, 50, 0};
constexpr float kHealthGain[6] = {0.02f, 0.016f, 0.006f, -0.01f, -0.03f, -0.08f};
constexpr double kLeadIn = 2.5;  // seconds before the song starts
constexpr const char* kJudgeName[6] = {"MAX", "300", "200", "100", "50", "MISS"};
const Vec4 kJudgeColor[6] = {{1.0f, 1.0f, 0.95f, 1}, {1.0f, 0.82f, 0.28f, 1}, {0.4f, 1.0f, 0.45f, 1},
                             {0.35f, 0.65f, 1.0f, 1},  {0.7f, 0.7f, 0.75f, 1},  {1.0f, 0.25f, 0.25f, 1}};
// Columns like the osu!mania 4K default: white outside, blue inside.
const Vec4 kLaneColor[4] = {{0.92f, 0.94f, 1.0f, 1}, {0.3f, 0.72f, 1.0f, 1}, {0.3f, 0.72f, 1.0f, 1},
                            {0.92f, 0.94f, 1.0f, 1}};
constexpr const char* kDifficultyName[kDifficultyCount] = {"EASY", "NORMAL", "HARD", "INSANE"};
const Vec4 kDifficultyColor[kDifficultyCount] = {{0.45f, 0.9f, 0.5f, 1}, {0.45f, 0.75f, 1.0f, 1},
                                                 {1.0f, 0.72f, 0.28f, 1}, {1.0f, 0.35f, 0.55f, 1}};
constexpr float kTypicalStars[kDifficultyCount] = {0.9f, 1.7f, 2.7f, 3.3f};  // osu!mania stars of the auto-made charts (J117.7)

Vec4 withAlpha(Vec4 c, float a) { return {c.x, c.y, c.z, c.w * a}; }
Vec4 mix(Vec4 a, Vec4 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t}; }

// MAX shimmers through the colours, like osu!'s rainbow 300g.
Vec4 maxColor(double nowMs) {
  const float h = static_cast<float>(std::fmod(nowMs / 600.0, 1.0)) * 6.0f;
  const float x = 1 - std::fabs(std::fmod(h, 2.0f) - 1);
  const int k = static_cast<int>(h);
  const float rgb[6][3] = {{1, x, 0}, {x, 1, 0}, {0, 1, x}, {0, x, 1}, {x, 0, 1}, {1, 0, x}};
  return {0.55f + 0.45f * rgb[k][0], 0.55f + 0.45f * rgb[k][1], 0.55f + 0.45f * rgb[k][2], 1};
}

const char* gradeOf(double acc, bool anyMiss) {
  if (acc >= 0.9999) return "SS";
  if (acc >= 0.95 && !anyMiss) return "S";
  if (acc >= 0.95) return "A";
  if (acc >= 0.9) return "A";
  if (acc >= 0.8) return "B";
  if (acc >= 0.7) return "C";
  return "D";
}
Vec4 gradeColor(const std::string& g) {
  if (g == "SS") return {0.95f, 0.95f, 1.0f, 1};
  if (g == "S") return {1.0f, 0.82f, 0.3f, 1};
  if (g == "A") return {0.45f, 1.0f, 0.5f, 1};
  if (g == "B") return {0.4f, 0.7f, 1.0f, 1};
  if (g == "C") return {0.8f, 0.45f, 1.0f, 1};
  return {1.0f, 0.35f, 0.35f, 1};
}

std::string timeText(float s) {
  char b[16];
  const int t = static_cast<int>(s + 0.5f);
  std::snprintf(b, sizeof(b), "%d:%02d", t / 60, t % 60);
  return b;
}

// ---- Skins: how the notes look, and the menus' colours ----
enum NoteStyle { kBars, kCircles, kBubbles, kNeon, kArrows };
struct Skin {
  const char* name;
  NoteStyle notes;
  Vec4 lane[4];
  Vec4 stage;              // the playfield behind the notes
  Vec4 accent, accent2;    // buttons, highlights
  Vec4 bgTop, bgBottom;    // the menus' background
  Vec4 card, ink;          // song cards; outlines
};
const Skin kSkins[] = {
    {"CLASSIC", kBars, {{0.92f, 0.94f, 1.0f, 1}, {0.3f, 0.72f, 1.0f, 1}, {0.3f, 0.72f, 1.0f, 1}, {0.92f, 0.94f, 1.0f, 1}},
     {0, 0, 0, 0.9f}, {1.0f, 0.4f, 0.75f, 1}, {0.3f, 0.72f, 1.0f, 1}, {0.1f, 0.06f, 0.16f, 1}, {0.03f, 0.02f, 0.06f, 1},
     {0.12f, 0.1f, 0.18f, 0.95f}, {0.02f, 0.01f, 0.04f, 1}},
    {"CIRCLES", kCircles, {{0.97f, 0.97f, 1.0f, 1}, {1.0f, 0.42f, 0.72f, 1}, {1.0f, 0.42f, 0.72f, 1}, {0.97f, 0.97f, 1.0f, 1}},
     {0.02f, 0.02f, 0.05f, 0.9f}, {1.0f, 0.42f, 0.72f, 1}, {0.55f, 0.85f, 1.0f, 1}, {0.08f, 0.1f, 0.24f, 1},
     {0.02f, 0.03f, 0.08f, 1}, {0.12f, 0.14f, 0.3f, 0.95f}, {0.02f, 0.02f, 0.06f, 1}},
    {"BUBBLE", kBubbles, {{1.0f, 0.5f, 0.75f, 1}, {0.4f, 0.95f, 0.72f, 1}, {1.0f, 0.88f, 0.35f, 1}, {0.42f, 0.74f, 1.0f, 1}},
     {0.06f, 0.04f, 0.14f, 0.88f}, {1.0f, 0.5f, 0.8f, 1}, {0.45f, 0.8f, 1.0f, 1}, {0.32f, 0.16f, 0.5f, 1},
     {0.1f, 0.2f, 0.45f, 1}, {0.25f, 0.2f, 0.5f, 0.95f}, {0.12f, 0.05f, 0.22f, 1}},
    {"NEON", kNeon, {{0.2f, 1.0f, 1.0f, 1}, {1.0f, 0.25f, 0.9f, 1}, {1.0f, 0.25f, 0.9f, 1}, {0.2f, 1.0f, 1.0f, 1}},
     {0.0f, 0.0f, 0.02f, 0.93f}, {0.2f, 1.0f, 1.0f, 1}, {1.0f, 0.25f, 0.9f, 1}, {0.05f, 0.0f, 0.12f, 1},
     {0.0f, 0.02f, 0.06f, 1}, {0.06f, 0.04f, 0.14f, 0.95f}, {0.0f, 0.0f, 0.0f, 1}},
    {"ARROWS", kArrows, {{0.78f, 0.45f, 1.0f, 1}, {0.3f, 0.72f, 1.0f, 1}, {0.35f, 1.0f, 0.5f, 1}, {1.0f, 0.42f, 0.38f, 1}},
     {0.03f, 0.02f, 0.05f, 0.9f}, {1.0f, 0.8f, 0.25f, 1}, {0.35f, 1.0f, 0.5f, 1}, {0.14f, 0.06f, 0.1f, 1},
     {0.03f, 0.02f, 0.05f, 1}, {0.18f, 0.1f, 0.16f, 0.95f}, {0.02f, 0.01f, 0.02f, 1}},
};
constexpr int kSkinCount = static_cast<int>(sizeof(kSkins) / sizeof(kSkins[0]));

Vec4 darker(Vec4 c, float k) { return {c.x * k, c.y * k, c.z * k, c.w}; }
Vec4 lighter(Vec4 c, float k) { return {c.x + (1 - c.x) * k, c.y + (1 - c.y) * k, c.z + (1 - c.z) * k, c.w}; }

// A rounded rectangle, as a fan of triangles (no overlaps, so see-through
// colours stay even).
void roundRect(engine::TextRenderer& text, float x, float y, float w, float h, float r, Vec4 c) {
  r = std::max(0.0f, std::min({r, w / 2, h / 2}));
  if (r < 1) return text.addRect(x, y, w, h, c);
  constexpr int kSeg = 5;
  Vec2 pts[4 * (kSeg + 1)];
  const Vec2 centers[4] = {{x + w - r, y + r}, {x + w - r, y + h - r}, {x + r, y + h - r}, {x + r, y + r}};
  int n = 0;
  for (int k = 0; k < 4; ++k) {
    for (int i = 0; i <= kSeg; ++i) {
      const float a = (static_cast<float>(k) - 1 + static_cast<float>(i) / kSeg) * 1.5707963f;
      pts[n++] = {centers[k].x + std::cos(a) * r, centers[k].y + std::sin(a) * r};
    }
  }
  const Vec2 mid{x + w / 2, y + h / 2};
  for (int i = 0; i < n; ++i) {
    const Vec2 q[4] = {mid, pts[i], pts[(i + 1) % n], pts[(i + 1) % n]};
    text.addScreenQuad(q, c);
  }
}

// A glossy "bubble" button: dark outline, a drop shadow, the body, a
// shine across its top.
void bubble(engine::TextRenderer& text, float x, float y, float w, float h, const char* label, float px, Vec4 color,
            bool active, Vec4 ink, float textScale = 1) {
  const float r = std::min(h * 0.5f, 18 * px), o = 2 * px;
  roundRect(text, x - o, y - o + 3 * px, w + 2 * o, h + 2 * o, r + o, withAlpha(ink, 0.55f));
  roundRect(text, x - o, y - o, w + 2 * o, h + 2 * o, r + o, ink);
  const Vec4 body = active ? color : mix(darker(color, 0.45f), Vec4{0.12f, 0.1f, 0.2f, 1}, 0.4f);
  roundRect(text, x, y, w, h, r, body);
  roundRect(text, x + 2 * px, y + h * 0.55f, w - 4 * px, h * 0.45f - 2 * px, r * 0.8f, withAlpha(darker(body, 0.7f), 0.6f));
  roundRect(text, x + w * 0.07f, y + h * 0.1f, w * 0.86f, h * 0.3f, r * 0.6f, Vec4{1, 1, 1, active ? 0.38f : 0.16f});
  if (!label || !*label) return;
  engine::TextStyle s;
  s.align = 0.5f;
  s.color = {1, 1, 1, 1};
  s.outline = ink;
  s.outlineWidth = 0.8f;
  const float cap = std::min(h * 0.38f, 13 * px) * textScale;
  text.addScreen(label, x + w / 2, y + (h - cap) / 2, cap, s);
}

// The combo counter's colour: dark blue at the start, brighter blue, green,
// yellow, orange, and red from 1000.
Vec4 comboColor(int combo) {
  static const struct { float at; Vec4 c; } kStops[] = {
      {0, {0.12f, 0.22f, 0.62f, 1}},   {100, {0.3f, 0.7f, 1.0f, 1}},   {250, {0.3f, 1.0f, 0.45f, 1}},
      {450, {1.0f, 0.95f, 0.3f, 1}},   {700, {1.0f, 0.58f, 0.15f, 1}}, {1000, {1.0f, 0.18f, 0.18f, 1}}};
  const float c = static_cast<float>(combo);
  for (size_t i = 1; i < sizeof(kStops) / sizeof(kStops[0]); ++i) {
    if (c <= kStops[i].at) return mix(kStops[i - 1].c, kStops[i].c, (c - kStops[i - 1].at) / (kStops[i].at - kStops[i - 1].at));
  }
  return kStops[5].c;
}

// An arrow for the ARROWS skin, pointing left, down, up or right by lane.
void arrow(engine::TextRenderer& text, float cx, float cy, float size, int lane, Vec4 c) {
  static const float kAngle[4] = {3.14159265f, 1.5707963f, -1.5707963f, 0};  // screen y points down
  const float a = kAngle[lane & 3], ca = std::cos(a), sa = std::sin(a);
  const auto P = [&](float u, float v) { return Vec2{cx + (u * ca - v * sa) * size, cy + (u * sa + v * ca) * size}; };
  const Vec2 head[4] = {P(1.0f, 0), P(0.05f, 0.9f), P(0.05f, -0.9f), P(0.05f, -0.9f)};
  text.addScreenQuad(head, c);
  const Vec2 shaft[4] = {P(-0.85f, -0.36f), P(0.1f, -0.36f), P(0.1f, 0.36f), P(-0.85f, 0.36f)};
  text.addScreenQuad(shaft, c);
}

// One note head (or a receptor's fill) in the skin's style, centred.
void noteHead(engine::TextRenderer& text, const Skin& sk, int lane, float cx, float cy, float cw, float px, Vec4 c,
              double nowMs) {
  const float r = cw * 0.4f;
  switch (sk.notes) {
    case kBars: {
      const float nh = cw * 0.3f;
      text.addRect(cx - cw / 2 + 2 * px, cy - nh / 2, cw - 4 * px, nh, c);
      text.addRect(cx - cw / 2 + 2 * px, cy - nh / 2, cw - 4 * px, nh * 0.28f, {1, 1, 1, 0.45f * c.w});
      text.addRect(cx - cw / 2 + 2 * px, cy + nh / 2 - 2 * px, cw - 4 * px, 2 * px, {0, 0, 0, 0.35f * c.w});
      break;
    }
    case kCircles:
      text.addShape(engine::font::kShapeDisc, cx, cy, r, c, withAlpha(sk.ink, c.w), 0.5f);
      text.addShape(engine::font::kShapeDisc, cx, cy, r * 0.55f, withAlpha(lighter(c, 0.6f), c.w));
      break;
    case kBubbles:
      text.addShape(engine::font::kShapeDisc, cx, cy + 2 * px, r * 1.04f, withAlpha(darker(c, 0.5f), 0.6f * c.w));
      text.addShape(engine::font::kShapeDisc, cx, cy, r, c, withAlpha(sk.ink, 0.9f * c.w), 0.45f);
      text.addShape(engine::font::kShapeDisc, cx, cy + r * 0.2f, r * 0.72f, withAlpha(darker(c, 0.85f), 0.8f * c.w));
      text.addShape(engine::font::kShapeDisc, cx - r * 0.32f, cy - r * 0.34f, r * 0.26f, {1, 1, 1, 0.75f * c.w});
      text.addShape(engine::font::kShapeDisc, cx + r * 0.05f, cy - r * 0.52f, r * 0.1f, {1, 1, 1, 0.6f * c.w});
      break;
    case kNeon: {
      const float pulse = 0.85f + 0.15f * static_cast<float>(std::sin(nowMs / 140.0 + lane));
      text.addShape(engine::font::kShapeDisc, cx, cy, r * 1.35f, withAlpha(c, 0.12f * c.w * pulse));
      text.addShape(engine::font::kShapeRing, cx, cy, r, c);
      text.addShape(engine::font::kShapeDisc, cx, cy, r * 0.28f, withAlpha(lighter(c, 0.7f), c.w));
      break;
    }
    case kArrows:
      arrow(text, cx, cy + 2 * px, r * 1.08f, lane, withAlpha(sk.ink, c.w));
      arrow(text, cx, cy, r, lane, c);
      arrow(text, cx - r * 0.06f, cy - r * 0.08f, r * 0.55f, lane, withAlpha(lighter(c, 0.55f), c.w));
      break;
  }
}

// A row of little star discs (filled up to `stars`, out of 7).
void starRow(engine::TextRenderer& text, float x, float y, float r, float stars, Vec4 color) {
  for (int k = 0; k < 7; ++k) {
    const float fill = std::clamp(stars - static_cast<float>(k), 0.0f, 1.0f);
    text.addShape(engine::font::kShapeDisc, x + static_cast<float>(k) * r * 2.6f, y, r,
                  fill > 0 ? withAlpha(color, 0.35f + 0.65f * fill) : Vec4{1, 1, 1, 0.12f});
  }
}

std::string clip(const std::string& s, size_t n) { return s.size() > n ? s.substr(0, n - 1) + "." : s; }

}  // namespace

std::vector<RhythmGame::SongChoice> RhythmGame::builtInSongs() {
  const std::string d = "assets/music/rhythm/";
  return {
      {"Pixel Peeker Polka", "Kevin MacLeod", d + "pixel-peeker-polka-faster.m4a", 202},
      {"Itty Bitty 8 Bit", "Kevin MacLeod", d + "itty-bitty-8-bit.m4a", 194},
      {"Run Amok", "Kevin MacLeod", d + "run-amok.m4a", 107},
      {"Voxel Revolution", "Kevin MacLeod", d + "voxel-revolution.m4a", 130},
      {"Monkeys Spinning Monkeys", "Kevin MacLeod", d + "monkeys-spinning-monkeys.m4a", 125},
      {"Galactic Rap", "Kevin MacLeod", d + "galactic-rap.m4a", 142},
      {"Electro Cabello", "Kevin MacLeod", d + "electro-cabello.m4a", 191},
      {"Neon Laser Horizon", "Kevin MacLeod", d + "neon-laser-horizon.m4a", 179},
      {"Funkorama", "Kevin MacLeod", d + "funkorama.m4a", 201},
      {"Overworld", "Kevin MacLeod", d + "overworld.m4a", 173},
      {"Hyperfun", "Kevin MacLeod", d + "hyperfun.m4a", 233},
      {"Blip Stream", "Kevin MacLeod", d + "blip-stream.m4a", 285},
  };
}

std::string RhythmGame::bestKey(int song, Difficulty d) const {
  if (song < 0 || song >= static_cast<int>(songs_.size())) return "";
  const SongChoice& s = songs_[static_cast<size_t>(song)];
  if (s.pack()) return "pack:" + s.packId;  // a Training map has one chart
  return s.title + "|" + kDifficultyName[static_cast<int>(d)];
}

void RhythmGame::open(std::vector<SongChoice> extra) {
  songs_ = builtInSongs();
  for (SongChoice& s : extra) {
    if (std::none_of(songs_.begin(), songs_.end(), [&](const SongChoice& o) { return o.url == s.url; })) {
      if (s.artist.empty()) s.artist = "From the jukebox";
      songs_.push_back(std::move(s));
    }
  }
  addPackSongs();
  loadPack();
  training_ = engine::settings::remembered("rhTraining", 0) != 0;
  song_ = std::clamp(song_, 0, std::max(0, static_cast<int>(songs_.size()) - 1));
  rebuildView();
  wheel_ = static_cast<float>(viewPos(song_));
  scrollSpeed_ = std::clamp(static_cast<int>(engine::settings::remembered("rhSpeed", 18)), 8, 40);
  skin_ = std::clamp(static_cast<int>(engine::settings::remembered("rhSkin", 2)), 0, kSkinCount - 1);
  guard_ = engine::settings::remembered("rhGuard", 1) != 0;
  offsetMs_ = std::clamp(engine::settings::remembered("rhOffset", 0), -300.0, 300.0);
  // Bests and star ratings kept on this phone.
  for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
    for (int d = 0; d < kDifficultyCount; ++d) {
      const std::string key = bestKey(i, static_cast<Difficulty>(d));
      const double acc = engine::settings::remembered(("rhAcc." + key).c_str(), -1);
      if (acc >= 0) best_[key] = {acc, static_cast<int>(engine::settings::remembered(("rhScore." + key).c_str(), 0))};
      const double st = engine::settings::remembered(("rhStars2." + key).c_str(), -1);
      if (st >= 0) starCache_[key] = static_cast<float>(st);
    }
  }
  state_ = State::kMenu;
  particles_.clear();
  touchFx_.clear();
  previewSong_ = lastPicked_ = -1;
  engine::audio::setMusicGain(0.15f);  // turn the jukebox down while you play
  engine::ui::setBodyClass("racing", true);  // (hides the page's top bar: the game has the whole screen)
  LOG_INFO("Rhythm game opened (%zu songs).", songs_.size());
}

void RhythmGame::close() {
  engine::Track::stop();
  engine::audio::previewStop();
  previewSong_ = -1;
  engine::audio::setMusicGain(1.0f);
  engine::ui::setBodyClass("racing", false);
  state_ = State::kClosed;
  syncGuard();
}

// The gesture guard is on while you play or test (if not turned off in the test).
void RhythmGame::syncGuard() {
  const bool want = guard_ && (state_ == State::kPlaying || state_ == State::kWaitTap || state_ == State::kTest ||
                               state_ == State::kSync);
  if (want == guardOn_) return;
  guardOn_ = want;
  engine::bridge::op(want ? "[\"touchGuard\",1]" : "[\"touchGuard\",0]");
}

// Sideways the playfield is wider (taps use the whole screen anyway).
float RhythmGame::columnWidth(float W, float H) const { return H >= W ? W * 0.22f : std::min(W * 0.11f, H * 0.2f); }

float RhythmGame::laneX(int lane, float W, float H) const {
  const float cw = columnWidth(W, H);
  return (W - 4 * cw) / 2 + (static_cast<float>(lane) + 0.5f) * cw;
}

double RhythmGame::accuracy() const { return judgements_ ? points_ / (300.0 * judgements_) : 1.0; }

int RhythmGame::score() const {
  return totalJudgements_ ? static_cast<int>(std::lround(1e6 * scorePoints_ / (320.0 * totalJudgements_))) : 0;
}

float RhythmGame::stars(int song, Difficulty d, bool* known) const {
  if (song >= 0 && song < static_cast<int>(songs_.size()) && songs_[static_cast<size_t>(song)].pack()) {
    if (known) *known = true;
    return songs_[static_cast<size_t>(song)].packStars;
  }
  const auto it = starCache_.find(bestKey(song, d));
  if (known) *known = it != starCache_.end();
  return it != starCache_.end() ? it->second : kTypicalStars[static_cast<int>(d)];
}

void RhythmGame::startSong() {
  if (songs_.empty()) return;
  song_ = std::clamp(song_, 0, static_cast<int>(songs_.size()) - 1);
  engine::audio::previewStop();
  previewSong_ = -1;
  const SongChoice& s = songs_[static_cast<size_t>(song_)];
  if (s.pack() && chartLoads_[s.packId] != 2 && chartLoads_[s.packId] != 1) {
    chartLoads_[s.packId] = 1;
    const std::string id = s.packId;
    engine::fetchFile(s.chartUrl, [this, id](bool ok, std::vector<uint8_t> bytes) {
      PackMap m;
      std::string err;
      if (ok && parsePackMap(std::string(bytes.begin(), bytes.end()), m, &err)) {
        charts_[id] = m.chart;
        chartLoads_[id] = 2;
      } else {
        chartLoads_[id] = 3;
        LOG_ERROR("4K Mania: the Training map %s didn't load (%s).", id.c_str(), err.c_str());
      }
    });
  }
  engine::Track::load(s.url);
  state_ = State::kLoading;
  analyzeShown_ = false;
}

void RhythmGame::beginPlay(const Chart& chart) {
  notes_.clear();
  totalJudgements_ = 0;
  for (const ChartNote& n : chart.notes) {
    notes_.push_back({n.time, n.end, n.lane});
    totalJudgements_ += n.end > 0 ? 2 : 1;
  }
  duration_ = chart.length;
  bpm_ = chart.bpm;
  firstOpen_ = 0;
  std::fill(std::begin(counts_), std::end(counts_), 0);
  combo_ = maxCombo_ = judgements_ = 0;
  comboMilestone_ = 0;
  points_ = scorePoints_ = 0;
  health_ = 1;
  bursts_.clear();
  ticks_.clear();
  particles_.clear();
  touchFx_.clear();
  lastJudgeMs_ = -1e9;
  std::fill(std::begin(lanePressed_), std::end(lanePressed_), 0);
  fingerLane_.clear();
  newBest_ = false;
  lastPlayFrameMs_ = 0;
  state_ = engine::Track::play(kLeadIn) ? State::kPlaying : State::kWaitTap;
}

void RhythmGame::judge(Note& n, Judge j, double nowMs, double errorMs, bool tail) {
  if (!tail) {
    n.judged = true;
    n.headJudge = j;
  }
  ++counts_[j];
  ++judgements_;
  points_ += kAccValue[j];
  scorePoints_ += kScoreValue[j];
  health_ = std::clamp(health_ + kHealthGain[j], 0.0f, 1.0f);
  if (j == kMiss) {
    combo_ = 0;
    comboMilestone_ = 0;
  } else {
    maxCombo_ = std::max(maxCombo_, ++combo_);
    bursts_.push_back({n.lane, j, nowMs});
    if (!tail) ticks_.push_back({errorMs, nowMs, j});
  }
  lastJudge_ = j;
  lastJudgeMs_ = nowMs;
}

void RhythmGame::finish() {
  engine::Track::stop();
  const double acc = accuracy();
  const std::string key = bestKey(song_, difficulty_);
  Best& b = best_[key];
  if (score() > b.score || (b.score == 0 && acc > b.acc)) {
    newBest_ = b.score > 0 || b.acc > 0;
    b = {acc, score()};
    engine::settings::remember(("rhAcc." + key).c_str(), acc);
    engine::settings::remember(("rhScore." + key).c_str(), b.score);
  }
  LOG_INFO("Rhythm game result: %.2f%%, score %d (%d MAX, %d 300, %d 200, %d 100, %d 50, %d miss), max combo %d.",
           acc * 100, score(), counts_[kMax], counts_[k300], counts_[k200], counts_[k100], counts_[k50], counts_[kMiss],
           maxCombo_);
  state_ = State::kResults;
  finishMs_ = engine::clockMs();
}

// ---- Effects: particles and tap rings ----

float RhythmGame::rand01() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return static_cast<float>(rng_ >> 8) / 16777216.0f;
}

void RhythmGame::spawn(float x, float y, float vx, float vy, float life, float size, Vec4 color, float gravity) {
  if (particles_.size() >= 420) return;
  particles_.push_back({x, y, vx, vy, 0, life, size, gravity, color});
}

// Sparks shooting up from a lane's keys when a note is hit.
void RhythmGame::burst(int lane, Judge j, float W, float H, double nowMs) {
  const float u = std::min(W, H) / 430.0f, cw = columnWidth(W, H);
  const float x = laneX(lane, W, H), y = H * 0.83f;
  const Skin& sk = kSkins[skin_];
  const int n = j == kMax ? 12 : j == k300 ? 9 : 6;
  for (int k = 0; k < n; ++k) {
    const Vec4 c = k % 3 == 0 ? Vec4{1, 1, 1, 1} : (j == kMax ? maxColor(nowMs + k * 90.0) : sk.lane[lane]);
    spawn(x + (rand01() - 0.5f) * cw * 0.6f, y, (rand01() - 0.5f) * 260 * u, -(160 + rand01() * 340) * u,
          0.35f + rand01() * 0.3f, (1.6f + rand01() * 2.2f) * u, c, 900 * u);
  }
}

void RhythmGame::updateEffects(double nowMs, float W, float H) {
  const float dt = effectsMs_ > 0 ? static_cast<float>(std::min(0.1, (nowMs - effectsMs_) / 1000.0)) : 0;
  effectsMs_ = nowMs;
  const float u = std::min(W, H) / 430.0f;
  const Skin& sk = kSkins[skin_];
  for (Particle& p : particles_) {
    p.age += dt;
    p.vy += p.gravity * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
  }
  particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) { return p.age >= p.life; }),
                   particles_.end());
  touchFx_.erase(std::remove_if(touchFx_.begin(), touchFx_.end(), [nowMs](const TouchFx& t) { return nowMs - t.ms > 220; }),
                 touchFx_.end());
  if (state_ == State::kMenu && !songs_.empty()) {
    // Sparkles flowing out of the picked song's card, and bubbles rising
    // slowly behind everything.
    const MenuLayout m = menuLayout(W, H);
    const float mid = m.wheel.y + m.wheel.h / 2 + (static_cast<float>(viewPos(song_)) - wheel_) * m.card * 1.12f;
    const float x0 = m.wheel.x + m.wheel.w * 0.04f, w = m.wheel.w * 0.92f;
    spawnDebt_ += dt * 45;
    while (spawnDebt_ >= 1) {
      spawnDebt_ -= 1;
      const float side = rand01();
      float x, y, vx, vy;
      if (side < 0.5f) {  // along the top and bottom edges
        x = x0 + rand01() * w;
        y = mid + (side < 0.25f ? -0.5f : 0.5f) * m.card;
        vx = (rand01() - 0.5f) * 40 * u;
        vy = (side < 0.25f ? -1.0f : 1.0f) * (20 + rand01() * 40) * u;
      } else {  // out of the ends
        const bool leftEnd = side < 0.75f;
        x = leftEnd ? x0 : x0 + w;
        y = mid + (rand01() - 0.5f) * m.card;
        vx = (leftEnd ? -1.0f : 1.0f) * (30 + rand01() * 60) * u;
        vy = (rand01() - 0.5f) * 30 * u;
      }
      const float pick = rand01();
      spawn(x, y, vx, vy, 0.7f + rand01() * 0.7f, (1.2f + rand01() * 1.8f) * u,
            pick < 0.4f ? sk.accent : pick < 0.8f ? sk.accent2 : Vec4{1, 1, 1, 1});
    }
  }
  if (state_ != State::kPlaying) {
    bubbleDebt_ += dt * 3;
    while (bubbleDebt_ >= 1) {
      bubbleDebt_ -= 1;
      spawn(rand01() * W, H + 20 * u, (rand01() - 0.5f) * 10 * u, -(25 + rand01() * 35) * u, 9, (6 + rand01() * 16) * u,
            withAlpha(rand01() < 0.5f ? sk.accent : sk.accent2, 0.12f));
    }
  } else {
    // Sparks streaming from a hold note being held.
    for (const Note& n : notes_) {
      if (n.time > songTime_ + 0.2) break;
      if (!n.holding || rand01() > dt * 30) continue;
      spawn(laneX(n.lane, W, H) + (rand01() - 0.5f) * columnWidth(W, H) * 0.5f, H * 0.83f, (rand01() - 0.5f) * 80 * u,
            -(80 + rand01() * 160) * u, 0.3f + rand01() * 0.2f, (1.2f + rand01()) * u, lighter(sk.lane[n.lane], 0.4f), 300 * u);
    }
  }
}

// ---- Song select ----

RhythmGame::MenuLayout RhythmGame::menuLayout(float W, float H) const {
  MenuLayout m;
  if (H >= W) {  // phone upright: the info on top, the wheel below
    m.info = {W * 0.05f, H * 0.035f, W * 0.9f, H * 0.2f};
    const float dy = H * 0.25f, dw = (W * 0.9f - 3 * W * 0.02f) / 4;
    for (int d = 0; d < kDifficultyCount; ++d) m.diff[d] = {W * 0.05f + static_cast<float>(d) * (dw + W * 0.02f), dy, dw, H * 0.065f};
    m.speedDown = {W * 0.05f, H * 0.335f, W * 0.09f, H * 0.045f};
    m.speedUp = {W * 0.30f, H * 0.335f, W * 0.09f, H * 0.045f};
    m.autoBtn = {W * 0.41f, H * 0.335f, W * 0.17f, H * 0.045f};
    m.test = {W * 0.60f, H * 0.335f, W * 0.16f, H * 0.045f};
    m.sync = {W * 0.78f, H * 0.335f, W * 0.17f, H * 0.045f};
    m.tabSongs = {W * 0.05f, H * 0.392f, W * 0.43f, H * 0.042f};
    m.tabPack = {W * 0.52f, H * 0.392f, W * 0.43f, H * 0.042f};
    m.wheel = {0, H * 0.44f, W, H * 0.43f};
    m.card = H * 0.068f;
    m.back = {W * 0.05f, H * 0.89f, W * 0.18f, H * 0.065f};
    m.skin = {W * 0.27f, H * 0.89f, W * 0.3f, H * 0.065f};
    m.play = {W * 0.61f, H * 0.89f, W * 0.34f, H * 0.065f};
  } else {  // sideways: the info on the left, the wheel on the right
    m.info = {W * 0.03f, H * 0.05f, W * 0.43f, H * 0.36f};
    const float dw = (W * 0.43f - 3 * W * 0.01f) / 4;
    for (int d = 0; d < kDifficultyCount; ++d) m.diff[d] = {W * 0.03f + static_cast<float>(d) * (dw + W * 0.01f), H * 0.45f, dw, H * 0.12f};
    m.speedDown = {W * 0.03f, H * 0.625f, W * 0.045f, H * 0.09f};
    m.speedUp = {W * 0.15f, H * 0.625f, W * 0.045f, H * 0.09f};
    m.autoBtn = {W * 0.205f, H * 0.625f, W * 0.07f, H * 0.09f};
    m.test = {W * 0.285f, H * 0.625f, W * 0.07f, H * 0.09f};
    m.sync = {W * 0.365f, H * 0.625f, W * 0.095f, H * 0.09f};
    m.tabSongs = {W * 0.52f, H * 0.03f, W * 0.22f, H * 0.09f};
    m.tabPack = {W * 0.76f, H * 0.03f, W * 0.22f, H * 0.09f};
    m.wheel = {W * 0.49f, H * 0.14f, W * 0.51f, H * 0.86f};
    m.card = H * 0.125f;
    m.back = {W * 0.03f, H * 0.8f, W * 0.09f, H * 0.13f};
    m.skin = {W * 0.14f, H * 0.8f, W * 0.13f, H * 0.13f};
    m.play = {W * 0.29f, H * 0.8f, W * 0.17f, H * 0.13f};
  }
  return m;
}

void RhythmGame::pickSong(int index) {
  if (songs_.empty()) return;
  song_ = std::clamp(index, 0, static_cast<int>(songs_.size()) - 1);
}

// ---- Songs or Training (J117.9) ----

void RhythmGame::setTraining(bool on) {
  if (on == training_) return;
  training_ = on;
  engine::settings::remember("rhTraining", on ? 1 : 0);
  rebuildView();
  wheel_ = static_cast<float>(viewPos(song_));
}

void RhythmGame::rebuildView() {
  view_.clear();
  for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
    if (songs_[static_cast<size_t>(i)].pack() == training_) view_.push_back(i);
  }
  if (!view_.empty() && std::find(view_.begin(), view_.end(), song_) == view_.end()) {
    // The Training map you were last on, or the first one not yet cleared.
    int pick = view_.front();
    if (training_) {
      const int remembered = static_cast<int>(engine::settings::remembered("rhPackPick", -1));
      for (int i : view_) {
        if (!cleared(i)) {
          pick = i;
          break;
        }
      }
      if (remembered >= 0) {
        for (int i : view_) {
          if (songs_[static_cast<size_t>(i)].stage * 3 + (songs_[static_cast<size_t>(i)].skill == "speed" ? 0 :
              songs_[static_cast<size_t>(i)].skill == "chords" ? 1 : 2) == remembered) pick = i;
        }
      }
    }
    song_ = pick;
  }
}

int RhythmGame::viewPos(int song) const {
  const auto it = std::find(view_.begin(), view_.end(), song);
  return it == view_.end() ? 0 : static_cast<int>(it - view_.begin());
}

void RhythmGame::pickPos(int pos) {
  if (view_.empty()) return;
  song_ = view_[static_cast<size_t>(std::clamp(pos, 0, static_cast<int>(view_.size()) - 1))];
  const SongChoice& s = songs_[static_cast<size_t>(song_)];
  if (s.pack()) {
    engine::settings::remember("rhPackPick", s.stage * 3 + (s.skill == "speed" ? 0 : s.skill == "chords" ? 1 : 2));
  }
}

bool RhythmGame::cleared(int song) const {
  const auto it = best_.find(bestKey(song, difficulty_));
  return it != best_.end() && it->second.acc >= 0.95;
}

void RhythmGame::loadPack() {
  if (packState_ != 0) return;
  packState_ = 1;
  engine::fetchFile("assets/music/rhythm/pack/pack.json", [this](bool ok, std::vector<uint8_t> bytes) {
    std::string err;
    engine::JsonLimits limits;
    limits.maxString = 1 << 14;
    const engine::Json j = ok ? engine::Json::parse(std::string(bytes.begin(), bytes.end()), &err, limits) : engine::Json();
    if (!j["maps"].isArray()) {
      packState_ = 3;
      LOG_ERROR("4K Mania: the Training pack didn't load (%s).", ok ? err.c_str() : "no file");
      return;
    }
    for (const engine::Json& e : j["maps"].items()) {
      SongChoice s;
      s.title = e["title"].string();
      s.artist = e["artist"].string();
      s.url = e["song"].string();
      s.seconds = static_cast<float>(e["length"].number());
      s.packId = e["id"].string();
      s.chartUrl = e["chart"].string();
      s.skill = e["skill"].string();
      s.about = e["about"].string();
      s.licence = e["licence"].string();
      s.stage = e["stage"].integer(1);
      s.packStars = static_cast<float>(e["stars"].number());
      packSongs_.push_back(std::move(s));
    }
    packState_ = 2;
    LOG_INFO("4K Mania: the Training pack has %zu maps.", packSongs_.size());
    if (isOpen()) {
      addPackSongs();
      rebuildView();
      wheel_ = static_cast<float>(viewPos(song_));
    }
  });
}

void RhythmGame::addPackSongs() {
  for (const SongChoice& p : packSongs_) {
    if (std::any_of(songs_.begin(), songs_.end(), [&](const SongChoice& o) { return o.packId == p.packId; })) continue;
    songs_.push_back(p);
    const std::string key = bestKey(static_cast<int>(songs_.size()) - 1, difficulty_);
    const double acc = engine::settings::remembered(("rhAcc." + key).c_str(), -1);
    if (acc >= 0) best_[key] = {acc, static_cast<int>(engine::settings::remembered(("rhScore." + key).c_str(), 0))};
  }
}

void RhythmGame::menuPress(Vec2 p, float W, float H) {
  const MenuLayout m = menuLayout(W, H);
  if (m.tabSongs.contains(p.x, p.y)) return setTraining(false);
  if (m.tabPack.contains(p.x, p.y)) return setTraining(true);
  for (int d = 0; d < kDifficultyCount && !training_; ++d) {
    if (m.diff[d].contains(p.x, p.y)) difficulty_ = static_cast<Difficulty>(d);
  }
  if (m.speedDown.contains(p.x, p.y) || m.speedUp.contains(p.x, p.y)) {
    scrollSpeed_ = std::clamp(scrollSpeed_ + (m.speedUp.contains(p.x, p.y) ? 1 : -1), 8, 40);
    engine::settings::remember("rhSpeed", scrollSpeed_);
  }
  if (m.skin.contains(p.x, p.y)) {
    skin_ = (skin_ + 1) % kSkinCount;
    engine::settings::remember("rhSkin", skin_);
  }
  if (m.autoBtn.contains(p.x, p.y)) autoplay = !autoplay;
  if (m.test.contains(p.x, p.y)) return openTest(engine::clockMs());
  if (m.sync.contains(p.x, p.y)) return openSync();
  if (m.play.contains(p.x, p.y) && !view_.empty()) startSong();
  if (m.back.contains(p.x, p.y)) close();
  if (m.wheel.contains(p.x, p.y) && !view_.empty()) {
    const float step = m.card * 1.12f, mid = m.wheel.y + m.wheel.h / 2;
    const int i = static_cast<int>(std::lround(wheel_ + (p.y - mid) / step));
    if (i < 0 || i >= static_cast<int>(view_.size())) return;
    if (view_[static_cast<size_t>(i)] == song_) startSong();  // a second tap on the picked song plays it
    else pickPos(i);
  }
}

void RhythmGame::menuSwipe(float dy, float W, float H) {
  const MenuLayout m = menuLayout(W, H);
  pickPos(viewPos(song_) - static_cast<int>(std::lround(dy / (m.card * 1.12f))));
}

void RhythmGame::resultsPress(Vec2 p, float W, float H) {
  const float bw = std::min(W * 0.28f, H * 0.4f), bh = std::min(H * 0.07f, 60.0f * 3), y = H * 0.88f;
  for (int i = 0; i < 3; ++i) {
    const float x = W / 2 + (static_cast<float>(i) - 1.5f) * (bw + W * 0.02f) + W * 0.01f;
    if (Rect{x, y, bw, bh}.contains(p.x, p.y)) {
      if (i == 0) startSong();
      if (i == 1) state_ = State::kMenu;
      if (i == 2) close();
    }
  }
}

// ---- Each frame ----

void RhythmGame::update(double nowMs, const std::vector<engine::Input::RawPress>& presses, float W, float H) {
  if (state_ == State::kClosed) return;
  for (const engine::Input::RawPress& r : presses) {
    if (r.key == 4 && r.down) {  // Escape
      if (state_ == State::kSync) {
        engine::Track::stop();
        state_ = State::kMenu;
      } else if (state_ == State::kTest) {
        LOG_INFO("%s", testSummary().c_str());
        state_ = State::kMenu;
      } else if (state_ == State::kPlaying || state_ == State::kWaitTap) {
        engine::Track::stop();
        state_ = State::kMenu;
      } else {
        close();
      }
      return;
    }
  }
  syncGuard();
  if (state_ == State::kTest) return updateTest(nowMs, presses, W, H);
  if (state_ == State::kSync) return updateSync(nowMs, presses, W, H);
  // The wheel glides to the picked song.
  const float dt = wheelMs_ > 0 ? static_cast<float>(std::min(0.1, (nowMs - wheelMs_) / 1000.0)) : 0;
  wheelMs_ = nowMs;
  wheel_ += (static_cast<float>(viewPos(song_)) - wheel_) * (1 - std::exp(-dt * 12));
  updateEffects(nowMs, W, H);

  // The song select (and results) play the picked song's preview, a moment
  // after you stop scrolling.
  if ((state_ == State::kMenu || state_ == State::kResults) && !songs_.empty() && !view_.empty()) {
    if (song_ != lastPicked_) {
      lastPicked_ = song_;
      pickedMs_ = nowMs;
    }
    if (previewSong_ != song_ && nowMs - pickedMs_ > 350) {
      previewSong_ = song_;
      const SongChoice& s = songs_[static_cast<size_t>(song_)];
      engine::audio::previewPlay(s.url, s.seconds > 0 ? s.seconds * 0.4 : 40);
    }
  } else if (previewSong_ >= 0) {
    engine::audio::previewStop();
    previewSong_ = -1;
  }

  if (state_ == State::kMenu || state_ == State::kResults) {
    for (const engine::Input::RawPress& r : presses) {
      const Vec2 p{r.pos.x * W, r.pos.y * H};
      if (r.key >= 0 && r.key < 4 && r.down && state_ == State::kMenu) {  // keys: D/F song, J difficulty, K play
        if (r.key == 0) pickPos(viewPos(song_) - 1);
        if (r.key == 1) pickPos(viewPos(song_) + 1);
        if (r.key == 2) difficulty_ = static_cast<Difficulty>((static_cast<int>(difficulty_) + 1) % kDifficultyCount);
        if (r.key == 3) startSong();
        continue;
      }
      if (r.key >= 0) continue;
      if (r.down) {
        fingerDown_[r.id] = p;
        touchFx_.push_back({p.x, p.y, nowMs, -1});
        continue;
      }
      // A finger lifted: a swipe on the wheel scrolls, else it's a tap.
      const auto it = fingerDown_.find(r.id);
      if (it == fingerDown_.end()) continue;
      const Vec2 start = it->second;
      fingerDown_.erase(it);
      if (r.cancelled) continue;
      const float dy = p.y - start.y;
      if (state_ == State::kMenu && std::fabs(dy) > H * 0.03f && menuLayout(W, H).wheel.contains(start.x, start.y)) {
        menuSwipe(dy, W, H);
      } else if (state_ == State::kMenu) {
        menuPress(start, W, H);
      } else {
        resultsPress(start, W, H);
      }
      if (state_ == State::kClosed) return;
    }
    return;
  }
  fingerDown_.clear();

  if (state_ == State::kLoading) {
    const engine::Track::State s = engine::Track::state();
    if (s == engine::Track::State::kFailed) {
      LOG_ERROR("Rhythm game: couldn't load %s.", songs_[static_cast<size_t>(song_)].url.c_str());
      engine::ui::toast("Couldn't load that song. Try another one.");
      state_ = State::kMenu;
    }
    if (s == engine::Track::State::kReady) state_ = State::kAnalyzing;
    return;
  }
  if (state_ == State::kAnalyzing) {
    if (!analyzeShown_) {  // let "making the chart" show for a frame first
      analyzeShown_ = true;
      return;
    }
    const SongChoice& picked = songs_[static_cast<size_t>(song_)];
    if (picked.pack()) {  // a hand-built map (J117.9): its chart file, not one made by listening
      const int loaded = chartLoads_[picked.packId];
      if (loaded == 1) return;  // still coming
      float rate = 44100;
      engine::Track::takeSamples(rate);  // (frees the page's copy of the samples)
      if (loaded != 2) {
        engine::ui::toast("Couldn't load that map. Try again.");
        chartLoads_.erase(picked.packId);
        state_ = State::kMenu;
        return;
      }
      beginPlay(charts_[picked.packId]);
      return;
    }
    const std::string key = picked.url + "#" + kDifficultyName[static_cast<int>(difficulty_)];
    auto cached = charts_.find(key);
    if (cached == charts_.end()) {
      float rate = 44100;
      std::vector<float> samples = engine::Track::takeSamples(rate);
      const double t0 = engine::clockMs();
      Chart chart = makeChart(samples.data(), samples.size(), rate, difficulty_);
      LOG_INFO("Chart made in %.0f ms (%zu samples at %.0f Hz).", engine::clockMs() - t0, samples.size(), rate);
      // Star rating: osu!mania's own formula (stars.h, J117.7).
      if (chart.length > 0 && !chart.notes.empty()) {
        const float st = static_cast<float>(starRating(chart.notes));
        const std::string sk = bestKey(song_, difficulty_);
        starCache_[sk] = st;
        engine::settings::remember(("rhStars2." + sk).c_str(), st);
      }
      cached = charts_.emplace(key, std::move(chart)).first;
    }
    beginPlay(cached->second);
    return;
  }
  if (state_ == State::kWaitTap) {
    for (const engine::Input::RawPress& r : presses) {
      if (r.down && engine::Track::play(kLeadIn)) {
        state_ = State::kPlaying;
        break;
      }
    }
    return;
  }

  // ---- Playing ----
  // One clock for everything at this frame's time (J117.2); the picture is
  // drawn a frame ahead, for when it reaches your eyes.
  songTime_ = engine::Track::timeAt(nowMs) - offsetMs_ / 1000.0;  // as you hear it (the sync's offset, J117.6)
  if (lastPlayFrameMs_ > 0) {
    const double dt = std::clamp((nowMs - lastPlayFrameMs_) / 1000.0, 0.004, 0.034);
    drawLead_ += (dt - drawLead_) * 0.1;
  }
  lastPlayFrameMs_ = nowMs;
  const float cw = columnWidth(W, H);
  const float x0 = (W - 4 * cw) / 2;
  const bool sideways = W > H;
  const auto release = [&](int lane, double t) {
    for (size_t i = firstOpen_; i < notes_.size(); ++i) {
      Note& n = notes_[i];
      if (n.time > t + 1) break;
      if (n.lane != lane || !n.holding) continue;
      n.holding = false;
      n.tailDone = true;
      const double err = (t - n.end) * 1000;
      // Let go too early: the tail is missed; near the end it's judged by
      // how close (the tail's windows are half again as wide).
      Judge j = kMiss;
      for (int k = 0; k < 5; ++k) {
        if (std::fabs(err) <= kWindowMs[k] * 1.5) {
          j = static_cast<Judge>(k);
          break;
        }
      }
      if (err > 0) j = n.headJudge == kMax ? kMax : k300;
      judge(n, j, nowMs, err, true);
    }
  };
  for (const engine::Input::RawPress& r : presses) {
    const Vec2 p{r.pos.x * W, r.pos.y * H};
    if (r.down && r.key < 0 && p.x < W * 0.12f && p.y < H * 0.09f) {  // the pause / quit button
      engine::Track::stop();
      state_ = State::kMenu;
      return;
    }
    const double t = songTime_ - (nowMs - r.ms) / 1000.0;  // when it happened, in song time
    if (!r.down) {
      const auto it = fingerLane_.find(r.id);
      if (it != fingerLane_.end()) {
        const int lane = it->second;
        lanePressed_[lane] = std::max(0, lanePressed_[lane] - 1);
        fingerLane_.erase(it);
        // The phone cancelling a touch (its own gestures) isn't you letting
        // go: a hold note being held then counts as held to its end.
        if (lanePressed_[lane] == 0 && !autoplay && !r.cancelled) release(lane, t);
      }
      continue;
    }
    // Which lane: sideways the whole screen is four tall zones (anywhere in
    // a zone counts, above or below the keys); upright, the column under
    // the finger.
    int lane;
    if (r.key >= 0 && r.key < 4) lane = r.key;
    else if (sideways) lane = std::clamp(static_cast<int>(p.x / (W / 4)), 0, 3);
    else lane = std::clamp(static_cast<int>(std::floor((p.x - x0) / cw)), 0, 3);
    fingerLane_[r.id] = lane;
    ++lanePressed_[lane];
    laneDownMs_[lane] = nowMs;
    if (r.key < 0) touchFx_.push_back({p.x, p.y, nowMs, lane});
    if (autoplay) continue;
    Note* hit = nullptr;
    for (size_t i = firstOpen_; i < notes_.size(); ++i) {
      Note& n = notes_[i];
      if ((n.time - t) * 1000 > kMissMs) break;
      if (n.judged || n.lane != lane || (t - n.time) * 1000 > kMissMs) continue;
      hit = &n;
      break;
    }
    if (!hit) continue;
    const double err = (t - hit->time) * 1000;
    Judge j = kMiss;
    for (int k = 0; k < 5; ++k) {
      if (std::fabs(err) <= kWindowMs[k]) {
        j = static_cast<Judge>(k);
        break;
      }
    }
    judge(*hit, j, nowMs, err);
    if (j != kMiss) burst(lane, j, W, H, nowMs);
    if (hit->end > 0) {
      if (j == kMiss) hit->tailDone = true, judge(*hit, kMiss, nowMs, 0, true);
      else hit->holding = true;
    }
  }
  // Autoplay ticks on every note (J117.8), scheduled a moment ahead on the
  // song's clock: you hear whether the chart sits exactly on the music.
  if (autoplay) {
    for (size_t i = firstOpen_; i < notes_.size(); ++i) {
      Note& n = notes_[i];
      if (n.time > songTime_ + offsetMs_ / 1000.0 + 0.25) break;
      if (n.ticked) continue;
      n.ticked = true;
      const bool chord = (i > 0 && std::fabs(notes_[i - 1].time - n.time) < 0.002);
      if (!chord) engine::Track::tick(n.time, false);
    }
  }
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    Note& n = notes_[i];
    if (n.time > songTime_ + 0.2) break;
    if (!n.judged) {
      if (autoplay && n.time <= songTime_) {
        judge(n, kMax, nowMs, 0);
        burst(n.lane, kMax, W, H, nowMs);
        laneDownMs_[n.lane] = nowMs;
        if (n.end > 0) n.holding = true;
      } else if ((songTime_ - n.time) * 1000 > kMissMs) {
        judge(n, kMiss, nowMs, 0);
        if (n.end > 0) {  // a missed hold: its end is missed too
          n.tailDone = true;
          judge(n, kMiss, nowMs, 0, true);
        }
      }
    }
    if (n.holding) {
      if (autoplay) laneDownMs_[n.lane] = nowMs;
      if (songTime_ >= n.end) {  // held to the end
        n.holding = false;
        n.tailDone = true;
        judge(n, n.headJudge == kMax ? kMax : k300, nowMs, 0, true);
        burst(n.lane, n.headJudge == kMax ? kMax : k300, W, H, nowMs);
      }
    }
  }
  // Every 100 combo: a shower of sparks from the counter.
  if (combo_ / 100 > comboMilestone_) {
    comboMilestone_ = combo_ / 100;
    const float u = std::min(W, H) / 430.0f;
    const Vec4 c = comboColor(combo_);
    for (int k = 0; k < 36; ++k) {
      const float a = static_cast<float>(k) / 36 * 6.2831853f, sp = (120 + rand01() * 160) * u;
      spawn(W / 2, H * 0.4f, std::cos(a) * sp, std::sin(a) * sp, 0.6f + rand01() * 0.3f, (1.5f + rand01() * 2) * u,
            k % 2 ? c : Vec4{1, 1, 1, 1}, 200 * u);
    }
  }
  while (firstOpen_ < notes_.size() && notes_[firstOpen_].judged &&
         (notes_[firstOpen_].end <= 0 || notes_[firstOpen_].tailDone)) {
    ++firstOpen_;
  }
  bursts_.erase(std::remove_if(bursts_.begin(), bursts_.end(), [nowMs](const Burst& b) { return nowMs - b.ms > 300; }),
                bursts_.end());
  ticks_.erase(std::remove_if(ticks_.begin(), ticks_.end(), [nowMs](const Tick& t) { return nowMs - t.ms > 3000; }),
               ticks_.end());
  double lastNote = 0;
  for (const Note& n : notes_) lastNote = std::max(lastNote, std::max(n.time, n.end));
  if (songTime_ > std::max(lastNote + 2.0, std::min(duration_, lastNote + 6.0))) finish();
}

// ---- Drawing ----

namespace {
// The menus' background: the skin's colours top to bottom.
void backdrop(engine::TextRenderer& text, const Skin& sk, float W, float H, float alpha) {
  constexpr int kBands = 24;
  for (int b = 0; b < kBands; ++b) {
    const float t = static_cast<float>(b) / (kBands - 1);
    text.addRect(0, H * static_cast<float>(b) / kBands, W, H / kBands + 1, withAlpha(mix(sk.bgTop, sk.bgBottom, t), alpha));
  }
}
}  // namespace

void RhythmGame::draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  if (state_ == State::kClosed) return;
  const Skin& sk = kSkins[skin_];
  if (state_ == State::kPlaying) text.addRect(0, 0, W, H, {0.015f, 0.01f, 0.03f, 0.86f});  // dim the bar behind
  else backdrop(text, sk, W, H, 0.96f);
  // Particles behind the menus (bubbles) and over the play (sparks).
  const auto drawParticles = [&]() {
    for (const Particle& p : particles_) {
      const float k = p.age / p.life;
      const float a = std::min(1.0f, (1 - k) * 2.5f) * std::min(1.0f, p.age * 12);
      text.addShape(engine::font::kShapeDisc, p.x, p.y, p.size * (1 - 0.4f * k), withAlpha(p.color, p.color.w * a));
    }
  };
  if (state_ != State::kPlaying) drawParticles();
  // Tap rings: a tiny flash where a finger lands.
  const auto drawTaps = [&]() {
    for (const TouchFx& t : touchFx_) {
      const float k = static_cast<float>((nowMs - t.ms) / 220.0);
      const Vec4 c = t.lane >= 0 ? sk.lane[t.lane] : sk.accent;
      text.addShape(engine::font::kShapeRing, t.x, t.y, (10 + 26 * k) * px, withAlpha(lighter(c, 0.3f), 0.8f * (1 - k)));
      text.addShape(engine::font::kShapeDisc, t.x, t.y, (7 - 5 * k) * px, withAlpha(Vec4{1, 1, 1, 1}, 0.6f * (1 - k)));
    }
  };
  if (state_ == State::kMenu) {
    drawMenu(text, W, H, px, nowMs);
    drawTaps();
    return;
  }
  if (state_ == State::kResults) {
    drawResults(text, W, H, px);
    drawTaps();
    return;
  }
  if (state_ == State::kTest) {
    drawTest(text, W, H, px, nowMs);
    return;
  }
  if (state_ == State::kSync) {
    drawSync(text, W, H, px, nowMs);
    return;
  }
  if (state_ == State::kLoading || state_ == State::kAnalyzing || state_ == State::kWaitTap) {
    engine::TextStyle center;
    center.align = 0.5f;
    center.outline = sk.ink;
    center.outlineWidth = 0.8f;
    const SongChoice& s = songs_[static_cast<size_t>(song_)];
    text.addScreen(clip(s.title, 26), W / 2, H * 0.3f, 14 * px, center);
    center.color = kDifficultyColor[static_cast<int>(difficulty_)];
    text.addScreen(s.pack() ? "TRAINING  STAGE " + std::to_string(s.stage) : std::string(kDifficultyName[static_cast<int>(difficulty_)]),
                   W / 2, H * 0.3f + 22 * px, 9 * px, center);
    center.color = {1, 1, 1, 0.9f};
    const char* msg = state_ == State::kLoading ? "Loading the song..." : state_ == State::kAnalyzing ?
                      "Making the chart..." : "Tap to start";
    text.addScreen(msg, W / 2, H * 0.55f, 11 * px, center);
    for (int k = 0; k < 4; ++k) {  // four bouncing notes in the skin's style
      const float bounce = std::fabs(std::sin(static_cast<float>(nowMs / 260.0) + static_cast<float>(k) * 0.7f));
      noteHead(text, sk, k, W / 2 + (static_cast<float>(k) - 1.5f) * 34 * px, H * 0.47f - bounce * 16 * px, 34 * px, px,
               sk.lane[k], nowMs);
    }
    return;
  }
  drawPlay(text, W, H, px, nowMs);
  drawParticles();
  drawTaps();
}

void RhythmGame::drawMenu(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const MenuLayout m = menuLayout(W, H);
  const Skin& sk = kSkins[skin_];
  engine::TextStyle st;
  st.outline = sk.ink;
  st.outlineWidth = 0.75f;
  // The wheel: bubbly cards above and below the picked song; the picked one
  // slides out, glows and bobs gently.
  if (!view_.empty()) {
    const float step = m.card * 1.12f, mid = m.wheel.y + m.wheel.h / 2;
    for (int pos = 0; pos < static_cast<int>(view_.size()); ++pos) {
      const int i = view_[static_cast<size_t>(pos)];
      const float off = static_cast<float>(pos) - wheel_;
      const bool sel = i == song_;
      const float bob = sel ? std::sin(static_cast<float>(nowMs / 420.0)) * 2 * px : 0;
      const float y = mid + off * step - m.card / 2 + bob;
      if (y + m.card < m.wheel.y || y > m.wheel.y + m.wheel.h) continue;
      const float slide = std::clamp(std::fabs(off), 0.0f, 3.0f) * m.wheel.w * 0.025f;
      const float x = m.wheel.x + m.wheel.w * 0.04f + slide, w = m.wheel.w * 0.92f - slide;
      const float r = m.card * 0.45f;
      if (sel) {
        const float glow = 0.5f + 0.5f * std::sin(static_cast<float>(nowMs / 300.0));
        roundRect(text, x - 6 * px, y - 6 * px, w + 12 * px, m.card + 12 * px, r + 6 * px, withAlpha(sk.accent, 0.18f + 0.12f * glow));
      }
      roundRect(text, x - 2 * px, y - 2 * px + 3 * px, w + 4 * px, m.card + 4 * px, r + 2 * px, withAlpha(sk.ink, 0.5f));
      roundRect(text, x - 2 * px, y - 2 * px, w + 4 * px, m.card + 4 * px, r + 2 * px, sel ? sk.accent : sk.ink);
      roundRect(text, x, y, w, m.card, r, sel ? mix(sk.card, sk.accent, 0.35f) : sk.card);
      roundRect(text, x + w * 0.03f, y + m.card * 0.08f, w * 0.94f, m.card * 0.3f, r * 0.6f, {1, 1, 1, sel ? 0.16f : 0.07f});
      const SongChoice& s = songs_[static_cast<size_t>(i)];
      const float big = std::min(m.card * 0.28f, 12 * px);
      if (s.pack()) {
        // The stage number in a bubble, in its skill's colour.
        const Vec4 c = s.skill == "speed" ? Vec4{0.42f, 0.74f, 1.0f, 1} : s.skill == "chords" ? Vec4{1.0f, 0.5f, 0.75f, 1}
                                                                                              : Vec4{0.4f, 0.95f, 0.72f, 1};
        text.addShape(engine::font::kShapeDisc, x + m.card * 0.5f, y + m.card / 2, m.card * 0.38f, withAlpha(darker(c, 0.5f), 1), sk.ink, 0.5f);
        st.align = 0.5f;
        st.color = c;
        text.addScreen(std::to_string(s.stage), x + m.card * 0.5f, y + m.card * 0.5f - big * 0.55f, big * 1.1f, st);
      } else {
        // A bubble on the left in the lane colours.
        noteHead(text, sk, i % 4, x + m.card * 0.5f, y + m.card / 2, m.card * 0.9f, px, sk.lane[i % 4], nowMs);
      }
      st.align = 0;
      st.color = {1, 1, 1, sel ? 1.0f : 0.85f};
      text.addScreen(clip(s.title, 22), x + m.card * 1.05f, y + m.card * 0.2f, big, st);
      st.color = {0.85f, 0.85f, 0.95f, 0.9f};
      std::string sub = s.artist;
      if (s.pack()) {
        char b[48];
        std::snprintf(b, sizeof(b), "  %s  %.1f*", s.skill == "speed" ? "SPEED" : s.skill == "chords" ? "CHORDS" : "TECH", s.packStars);
        sub = "STAGE " + std::to_string(s.stage) + b;
      } else if (s.seconds > 0) {
        sub += "  " + timeText(s.seconds);
      }
      text.addScreen(clip(sub, 30), x + m.card * 1.05f, y + m.card * 0.6f, big * 0.6f, st);
      // Best grade on this difficulty, in a badge.
      const auto it = best_.find(bestKey(i, difficulty_));
      if (it != best_.end()) {
        const std::string g = gradeOf(it->second.acc, it->second.acc < 0.999);
        const float gr = m.card * 0.32f, gx = x + w - gr - 8 * px, gy = y + m.card / 2;
        if (s.pack() && it->second.acc >= 0.95) {  // cleared: a ring around the grade
          text.addShape(engine::font::kShapeRing, gx, gy, gr * 1.18f, {0.45f, 1.0f, 0.55f, 1});
        }
        text.addShape(engine::font::kShapeDisc, gx, gy, gr, withAlpha(darker(gradeColor(g), 0.45f), 0.95f), sk.ink, 0.5f);
        st.align = 0.5f;
        st.color = gradeColor(g);
        text.addScreen(g, gx, gy - gr * 0.45f, gr * 0.9f, st);
      }
    }
  }
  // The picked song's details, on a rounded panel.
  roundRect(text, m.info.x - 2 * px, m.info.y - 2 * px + 4 * px, m.info.w + 4 * px, m.info.h + 4 * px, 18 * px, withAlpha(sk.ink, 0.5f));
  roundRect(text, m.info.x - 2 * px, m.info.y - 2 * px, m.info.w + 4 * px, m.info.h + 4 * px, 18 * px, sk.ink);
  roundRect(text, m.info.x, m.info.y, m.info.w, m.info.h, 16 * px, mix(sk.card, sk.bgTop, 0.3f));
  roundRect(text, m.info.x + 6 * px, m.info.y + 5 * px, m.info.w - 12 * px, m.info.h * 0.22f, 12 * px, {1, 1, 1, 0.07f});
  st.align = 0;
  st.color = sk.accent;
  const float pad = 12 * px;
  text.addScreen("4K MANIA", m.info.x + pad, m.info.y + pad, 9 * px, st);
  if (!songs_.empty()) {
    const SongChoice& s = songs_[static_cast<size_t>(song_)];
    st.color = {1, 1, 1, 1};
    const float tsize = std::min(m.info.h * 0.17f, 17 * px);
    text.addScreen(clip(s.title, 22), m.info.x + pad, m.info.y + pad + 16 * px, tsize, st);
    st.color = {0.85f, 0.85f, 0.95f, 1};
    text.addScreen(s.pack() ? s.artist + "  (" + s.licence + ")" : s.artist, m.info.x + pad, m.info.y + pad + 20 * px + tsize, 8 * px, st);
    char line[128];
    bool known = false;
    const float sr = stars(song_, difficulty_, &known);
    std::snprintf(line, sizeof(line), "%s%s  %.1f STARS", s.seconds > 0 ? ("LENGTH " + timeText(s.seconds) + "   ").c_str() : "",
                  kDifficultyName[static_cast<int>(difficulty_)], sr);
    if (s.pack()) {
      std::snprintf(line, sizeof(line), "STAGE %d  %s  %.2f STARS", s.stage,
                    s.skill == "speed" ? "SPEED" : s.skill == "chords" ? "CHORDS & JACKS" : "TECH", sr);
    }
    const float y3 = m.info.y + pad + 34 * px + tsize;
    text.addScreen(known ? line : (std::string(line) + " (ABOUT)").c_str(), m.info.x + pad, y3, 7 * px, st);
    starRow(text, m.info.x + pad + 4 * px, y3 + 18 * px, 4 * px, sr, kDifficultyColor[static_cast<int>(difficulty_)]);
    const auto it = best_.find(bestKey(song_, difficulty_));
    if (s.pack()) {  // what this map trains, in a line
      st.color = {0.92f, 0.88f, 1.0f, 1};
      text.addScreen(clip(s.about, 50), m.info.x + pad, y3 + 42 * px, 6.5f * px, st);
    }
    if (it != best_.end()) {
      const std::string g = gradeOf(it->second.acc, it->second.acc < 0.999);
      std::snprintf(line, sizeof(line), "BEST  %.2f%%  %07d%s", it->second.acc * 100, it->second.score,
                    s.pack() && it->second.acc >= 0.95 ? "  CLEARED" : s.pack() ? "  (95% CLEARS IT)" : "");
      st.color = gradeColor(g);
      text.addScreen(g + "  " + line, m.info.x + pad, y3 + 30 * px, 8 * px, st);
    } else if (s.pack()) {
      st.color = {0.8f, 0.8f, 0.9f, 0.85f};
      text.addScreen("NOT PLAYED YET  (95% CLEARS IT)", m.info.x + pad, y3 + 30 * px, 7 * px, st);
    } else {
      st.color = {0.8f, 0.8f, 0.9f, 0.85f};
      text.addScreen("NOT PLAYED YET", m.info.x + pad, y3 + 30 * px, 7 * px, st);
    }
    // A little note playing along with the preview.
    const float beat = std::fabs(std::sin(static_cast<float>(nowMs / 300.0)));
    noteHead(text, sk, song_ % 4, m.info.x + m.info.w - 26 * px, m.info.y + 24 * px - beat * 5 * px, 30 * px, px,
             sk.lane[song_ % 4], nowMs);
  }
  // Difficulties, as bubble pills with their stars (songs); Training: the
  // stages, each lit by how many of its three maps are cleared.
  if (training_) {
    const Rect a = m.diff[0], z = m.diff[kDifficultyCount - 1];
    const float x0 = a.x, w = z.x + z.w - a.x, cw = w / 10;
    int clearedTotal = 0;
    for (int st10 = 1; st10 <= 10; ++st10) {
      int n = 0, total = 0;
      bool here = false;
      for (int i : view_) {
        const SongChoice& c = songs_[static_cast<size_t>(i)];
        if (c.stage != st10) continue;
        ++total;
        n += cleared(i) ? 1 : 0;
        here |= i == song_;
      }
      clearedTotal += n;
      const float bx = x0 + static_cast<float>(st10 - 1) * cw;
      const Vec4 c = n == total && total > 0 ? Vec4{0.45f, 1.0f, 0.55f, 1} : sk.accent2;
      roundRect(text, bx + 2 * px, a.y, cw - 4 * px, a.h, 8 * px, withAlpha(c, here ? 0.85f : 0.25f + 0.2f * static_cast<float>(n)));
      engine::TextStyle s;
      s.align = 0.5f;
      s.outline = sk.ink;
      s.outlineWidth = 0.8f;
      text.addScreen(std::to_string(st10), bx + cw / 2, a.y + a.h * 0.18f, std::min(a.h * 0.3f, 9 * px), s);
      char b[8];
      std::snprintf(b, sizeof(b), "%d/%d", n, total);
      text.addScreen(b, bx + cw / 2, a.y + a.h * 0.6f, std::min(a.h * 0.22f, 6 * px), s);
    }
    (void)clearedTotal;
  }
  for (int d = 0; d < kDifficultyCount && !training_; ++d) {
    const Rect& r = m.diff[d];
    const bool on = static_cast<int>(difficulty_) == d;
    const float lift = on ? 3 * px : 0;
    bubble(text, r.x, r.y - lift, r.w, r.h, "", px, kDifficultyColor[d], on, sk.ink);
    engine::TextStyle s;
    s.align = 0.5f;
    s.outline = sk.ink;
    s.outlineWidth = 0.8f;
    const float cap = std::min(r.h * 0.28f, 9 * px);
    text.addScreen(kDifficultyName[d], r.x + r.w / 2, r.y - lift + r.h * 0.16f, cap, s);
    char b[16];
    bool known = false;
    const float sr = songs_.empty() ? kTypicalStars[d] : stars(song_, static_cast<Difficulty>(d), &known);
    std::snprintf(b, sizeof(b), known ? "%.1f*" : "~%.1f*", sr);
    text.addScreen(b, r.x + r.w / 2, r.y - lift + r.h * 0.56f, cap * 0.8f, s);
  }
  // Scroll speed and auto.
  bubble(text, m.speedDown.x, m.speedDown.y, m.speedDown.w, m.speedDown.h, "-", px, sk.accent2, true, sk.ink, 1.4f);
  bubble(text, m.speedUp.x, m.speedUp.y, m.speedUp.w, m.speedUp.h, "+", px, sk.accent2, true, sk.ink, 1.4f);
  {
    char b[32];
    std::snprintf(b, sizeof(b), "SPEED %d", scrollSpeed_);
    engine::TextStyle s;
    s.align = 0.5f;
    s.outline = sk.ink;
    s.outlineWidth = 0.8f;
    const float cx = (m.speedDown.x + m.speedDown.w + m.speedUp.x) / 2;
    text.addScreen(b, cx, m.speedDown.y + m.speedDown.h * 0.3f, std::min(m.speedDown.h * 0.4f, 9 * px), s);
  }
  bubble(text, m.autoBtn.x, m.autoBtn.y, m.autoBtn.w, m.autoBtn.h, autoplay ? "AUTO ON" : "AUTO", px, sk.accent2,
         autoplay, sk.ink);
  bubble(text, m.test.x, m.test.y, m.test.w, m.test.h, "TEST", px, sk.accent2, true, sk.ink);
  bubble(text, m.sync.x, m.sync.y, m.sync.w, m.sync.h, "SYNC", px, sk.accent, true, sk.ink);
  bubble(text, m.tabSongs.x, m.tabSongs.y, m.tabSongs.w, m.tabSongs.h, "SONGS", px, sk.accent2, !training_, sk.ink);
  bubble(text, m.tabPack.x, m.tabPack.y, m.tabPack.w, m.tabPack.h, "TRAINING", px, sk.accent, training_, sk.ink);
  bubble(text, m.back.x, m.back.y, m.back.w, m.back.h, "BACK", px, {1.0f, 0.4f, 0.4f, 1}, true, sk.ink);
  // The skin button shows the skin's name and its four notes.
  bubble(text, m.skin.x, m.skin.y, m.skin.w, m.skin.h, "", px, mix(sk.card, sk.accent2, 0.5f), true, sk.ink);
  {
    engine::TextStyle s;
    s.align = 0.5f;
    s.outline = sk.ink;
    s.outlineWidth = 0.8f;
    const float cap = std::min(m.skin.h * 0.22f, 8 * px);
    text.addScreen(std::string("SKIN: ") + sk.name, m.skin.x + m.skin.w / 2, m.skin.y + m.skin.h * 0.12f, cap, s);
    const float ns = std::min(m.skin.w / 5.2f, m.skin.h * 0.42f);
    for (int l = 0; l < 4; ++l) {
      noteHead(text, sk, l, m.skin.x + m.skin.w / 2 + (static_cast<float>(l) - 1.5f) * ns * 1.05f, m.skin.y + m.skin.h * 0.66f,
               ns, px, sk.lane[l], nowMs);
    }
  }
  const float playPulse = 1 + 0.03f * std::sin(static_cast<float>(nowMs / 250.0));
  const float pw = m.play.w * playPulse, ph = m.play.h * playPulse;
  bubble(text, m.play.x - (pw - m.play.w) / 2, m.play.y - (ph - m.play.h) / 2, pw, ph, "PLAY", px, sk.accent, true, sk.ink, 1.3f);
  if (view_.empty()) {
    engine::TextStyle s;
    s.align = 0.5f;
    s.outline = sk.ink;
    s.outlineWidth = 0.8f;
    const char* what = !training_ ? "No songs" : packState_ == 3 ? "The Training pack didn't load" : "Loading the Training pack...";
    text.addScreen(what, m.wheel.x + m.wheel.w / 2, m.wheel.y + m.wheel.h / 2, 9 * px, s);
  }
}

void RhythmGame::drawPlay(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const Skin& sk = kSkins[skin_];
  const float cw = columnWidth(W, H), x0 = (W - 4 * cw) / 2;
  const float hitY = H * 0.83f, keyH = cw * 0.5f, topY = -cw;
  const double approach = approachSeconds();
  const bool round = sk.notes != kBars;
  const float noteH = round ? cw * 0.8f : cw * 0.3f;
  const double shown = songTime_ + drawLead_;  // the song's time when this frame is on screen
  const auto yOf = [&](double t) { return hitY - static_cast<float>((t - shown) / approach) * (hitY - topY); };
  // Sideways: the four tall tap zones across the whole screen, lit while held.
  if (W > H) {
    for (int l = 0; l < 4; ++l) {
      const float since = static_cast<float>(nowMs - laneDownMs_[l]);
      const float on = lanePressed_[l] ? 1.0f : std::clamp(1.0f - since / 160.0f, 0.0f, 1.0f);
      if (on > 0) text.addRect(W / 4 * static_cast<float>(l), 0, W / 4, H, withAlpha(sk.lane[l], 0.07f * on));
      if (l > 0) text.addRect(W / 4 * static_cast<float>(l) - 0.5f * px, H * 0.1f, px, H * 0.9f, {1, 1, 1, 0.05f});
      // A soft pad at the bottom of each zone.
      roundRect(text, W / 4 * static_cast<float>(l) + 10 * px, H - 26 * px, W / 4 - 20 * px, 16 * px, 8 * px,
                withAlpha(sk.lane[l], 0.12f + 0.3f * on));
    }
  }
  // Stage: the skin's background, thin separators, bright edges.
  text.addRect(x0 - 3 * px, 0, 4 * cw + 6 * px, H, sk.stage);
  for (int l = 1; l < 4; ++l) text.addRect(x0 + static_cast<float>(l) * cw - 0.5f * px, 0, px, hitY, {1, 1, 1, 0.07f});
  text.addRect(x0 - 3 * px, 0, 2 * px, H, withAlpha(sk.accent2, 0.45f));
  text.addRect(x0 + 4 * cw + px, 0, 2 * px, H, withAlpha(sk.accent2, 0.45f));
  // Column lights: a glow rising from a key while it's held (and a moment after).
  for (int l = 0; l < 4; ++l) {
    const float since = static_cast<float>(nowMs - laneDownMs_[l]);
    const float on = lanePressed_[l] ? 1.0f : std::clamp(1.0f - since / 160.0f, 0.0f, 1.0f);
    if (on <= 0) continue;
    const float x = x0 + static_cast<float>(l) * cw;
    for (int k = 0; k < 10; ++k) {
      const float h = H * 0.034f;
      text.addRect(x + px, hitY - static_cast<float>(k + 1) * h, cw - 2 * px, h,
                   withAlpha(sk.lane[l], on * 0.22f * (1.0f - static_cast<float>(k) / 10.0f)));
    }
  }
  // Receptors (round skins): rings where the notes land, filled while pressed.
  if (round) {
    text.addRect(x0, hitY - 0.5f * px, 4 * cw, px, {1, 1, 1, 0.25f});
    for (int l = 0; l < 4; ++l) {
      const float cx = x0 + (static_cast<float>(l) + 0.5f) * cw;
      const float since = static_cast<float>(nowMs - laneDownMs_[l]);
      const bool down = lanePressed_[l] > 0 || since < 60;
      const float pop = down ? 0.9f : 1.0f + 0.06f * std::clamp(1 - (since - 60) / 120, 0.0f, 1.0f);
      if (sk.notes == kArrows) {
        arrow(text, cx, hitY, cw * 0.44f * pop, l, withAlpha(sk.lane[l], down ? 0.9f : 0.35f));
        arrow(text, cx, hitY, cw * 0.3f * pop, l, down ? lighter(sk.lane[l], 0.5f) : withAlpha(sk.stage, 1));
      } else {
        if (down) text.addShape(engine::font::kShapeDisc, cx, hitY, cw * 0.4f * pop, withAlpha(sk.lane[l], 0.45f));
        text.addShape(engine::font::kShapeRing, cx, hitY, cw * 0.42f * pop, withAlpha(lighter(sk.lane[l], 0.2f), down ? 1.0f : 0.6f));
      }
    }
  }
  // Notes: heads, and hold notes (a body up to the end, with a cap).
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    const Note& n = notes_[i];
    if (n.time - shown > approach * 1.1) break;
    const bool hold = n.end > 0;
    if (n.judged && !hold) continue;
    if (hold && n.tailDone && !n.holding) continue;
    const Vec4 c = sk.lane[n.lane];
    const float x = x0 + static_cast<float>(n.lane) * cw, cx = x + cw / 2;
    const float headY = n.holding ? hitY : yOf(n.time);
    const bool dead = hold && n.judged && !n.holding;  // missed head: greyed
    if (hold) {
      const float endY = yOf(n.end);
      if (endY < H && headY > -noteH) {
        const float bw = round ? cw * 0.5f : cw * 0.78f, bx = x + (cw - bw) / 2;
        const float top = std::max(endY, -noteH), bottom = std::min(headY, H);
        const Vec4 body = dead ? Vec4{0.35f, 0.35f, 0.4f, 0.5f} : withAlpha(c, n.holding ? 0.85f : 0.6f);
        if (round) {
          roundRect(text, bx, top - bw / 2, bw, bottom - top + bw / 2, bw / 2, body);
          roundRect(text, bx + bw * 0.2f, top - bw / 2 + 2 * px, bw * 0.25f, std::max(0.0f, bottom - top), bw * 0.12f,
                    {1, 1, 1, dead ? 0.05f : 0.22f});
          if (!dead) text.addShape(engine::font::kShapeDisc, cx, top, bw * 0.62f, withAlpha(lighter(c, 0.3f), 1), sk.ink, 0.4f);
        } else {
          text.addRect(bx, top, bw, bottom - top, body);
          text.addRect(bx, top, bw, noteH * 0.45f, dead ? body : withAlpha(c, 1));  // the end cap
        }
      }
    }
    if (n.judged && !n.holding) continue;
    if (headY < -noteH || headY > H + noteH) continue;
    noteHead(text, sk, n.lane, cx, headY, cw, px, dead ? Vec4{0.4f, 0.4f, 0.45f, 0.7f} : c, nowMs);
  }
  // Bar skin: the hit line and the keys under it (lit while pressed).
  if (!round) {
    text.addRect(x0, hitY - 1.5f * px, 4 * cw, 3 * px, {1, 1, 1, 0.75f});
    for (int l = 0; l < 4; ++l) {
      const float x = x0 + static_cast<float>(l) * cw;
      const bool down = lanePressed_[l] > 0 || nowMs - laneDownMs_[l] < 60;
      text.addRect(x + 3 * px, hitY + 4 * px, cw - 6 * px, keyH, down ? withAlpha(sk.lane[l], 0.9f) : Vec4{0.1f, 0.1f, 0.14f, 1});
      text.addRect(x + 3 * px, hitY + 4 * px, cw - 6 * px, 3 * px, withAlpha(sk.lane[l], down ? 1.0f : 0.5f));
      text.addRect(x + cw * 0.3f, hitY + 4 * px + keyH * 0.45f, cw * 0.4f, keyH * 0.15f, down ? Vec4{1, 1, 1, 0.9f} : Vec4{1, 1, 1, 0.18f});
    }
  }
  // Hit flashes over the hit line.
  for (const Burst& b : bursts_) {
    const float t = static_cast<float>((nowMs - b.ms) / 300.0);
    const float x = x0 + static_cast<float>(b.lane) * cw;
    const Vec4 c = b.judge == kMax ? maxColor(nowMs) : kJudgeColor[b.judge];
    if (round) {
      text.addShape(engine::font::kShapeRing, x + cw / 2, hitY, cw * (0.42f + 0.35f * t), withAlpha(c, 0.8f * (1 - t)));
      text.addShape(engine::font::kShapeDisc, x + cw / 2, hitY, cw * (0.4f + 0.1f * t), withAlpha(c, 0.3f * (1 - t)));
    } else {
      const float grow = cw * 0.15f * t;
      text.addRect(x - grow, hitY - noteH * (0.6f + t), cw + 2 * grow, noteH * (1.2f + 2 * t), withAlpha(c, 0.45f * (1 - t)));
    }
  }
  // Combo in the middle of the stage: its colour climbs from dark blue to
  // red at 1000; the judgement under it.
  engine::TextStyle center;
  center.align = 0.5f;
  center.outlineWidth = 0.85f;
  center.outline = {0, 0, 0, 0.95f};
  const double sinceJudge = nowMs - lastJudgeMs_;
  if (combo_ >= 2) {
    char c[16];
    std::snprintf(c, sizeof(c), "%d", combo_);
    // About 40 % smaller than before, a gentler pop, nothing behind it (J117.5).
    const float pop = 1.0f + 0.12f * static_cast<float>(std::max(0.0, 1.0 - sinceJudge / 80.0));
    Vec4 col = comboColor(combo_);
    if (combo_ >= 1000) col = lighter(col, 0.25f * (0.5f + 0.5f * std::sin(static_cast<float>(nowMs / 120.0))));
    const float size = std::min(cw * 0.45f, 26 * px) * pop;
    center.color = col;
    text.addScreen(c, W / 2, H * 0.4f, size, center);
  }
  if (sinceJudge < 500) {
    const Vec4 c = lastJudge_ == kMax ? maxColor(nowMs) : kJudgeColor[lastJudge_];
    const float fade = static_cast<float>(std::min(1.0, (500 - sinceJudge) / 150.0));
    center.color = withAlpha(c, fade);
    center.outline.w = 0.85f * fade;
    const float pop = 1.0f + 0.2f * static_cast<float>(std::max(0.0, 1.0 - sinceJudge / 80.0));
    const float js = std::min(cw * 0.22f, 13 * px);
    text.addScreen(kJudgeName[lastJudge_], W / 2, H * 0.4f - js * 1.6f - 6 * px, js * pop, center);
    center.outline.w = 0.95f;
  }
  // Timing-error bar under the keys: the windows' colours and recent hits
  // (left = early, right = late).
  {
    const float bw = std::min(cw * 3.0f, W * 0.8f);
    const float by = std::min(hitY + (round ? cw * 0.55f : keyH + 14 * px), H - 8 * px);
    const float scale = bw / 2 / static_cast<float>(kWindowMs[4]);
    for (int k = 4; k >= 0; --k) {
      const float hw = static_cast<float>(kWindowMs[k]) * scale;
      text.addRect(W / 2 - hw, by, 2 * hw, 3 * px, withAlpha(k == 0 ? Vec4{1, 1, 1, 1} : kJudgeColor[k], 0.5f));
    }
    for (const Tick& t : ticks_) {
      const float a = static_cast<float>(1.0 - (nowMs - t.ms) / 3000.0);
      const float x = W / 2 + static_cast<float>(std::clamp(t.errorMs, -kWindowMs[4], kWindowMs[4])) * scale;
      text.addRect(x - px, by - 5 * px, 2 * px, 13 * px, withAlpha(t.judge == kMax ? Vec4{1, 1, 1, 1} : kJudgeColor[t.judge], a));
    }
    text.addRect(W / 2 - px, by - 7 * px, 2 * px, 17 * px, {1, 1, 1, 0.9f});
  }
  // Health (right of the stage, rising from the hit line), a rounded tube.
  {
    const float hx = x0 + 4 * cw + 7 * px, hy = H * 0.12f, hh = hitY - hy;
    roundRect(text, hx - px, hy - px, 8 * px, hh + 2 * px, 4 * px, withAlpha(sk.ink, 0.8f));
    const Vec4 c = mix(Vec4{1, 0.25f, 0.25f, 1}, Vec4{0.4f, 1, 0.5f, 1}, health_);
    roundRect(text, hx, hy + hh * (1 - health_), 6 * px, hh * health_, 3 * px, c);
  }
  // Score, accuracy, progress, the pause button, the song's name.
  char line[64];
  engine::TextStyle right;
  right.align = 1;
  right.outlineWidth = 0.75f;
  std::snprintf(line, sizeof(line), "%07d", score());
  text.addScreen(line, W - 10 * px, 12 * px, 15 * px, right);
  std::snprintf(line, sizeof(line), "%.2f%%", accuracy() * 100);
  text.addScreen(line, W - 10 * px, 32 * px, 9 * px, right);
  const float progress = duration_ > 0 ? std::clamp(static_cast<float>(songTime_ / duration_), 0.0f, 1.0f) : 0;
  text.addRect(0, 0, W, 3 * px, {1, 1, 1, 0.12f});
  text.addRect(0, 0, W * progress, 3 * px, sk.accent);
  engine::TextStyle left;
  left.outlineWidth = 0.7f;
  text.addShape(engine::font::kShapeDisc, 25 * px, 25 * px, 15 * px, withAlpha(sk.card, 0.95f), sk.ink, 0.5f);
  text.addRect(20 * px, 19 * px, 3.5f * px, 12 * px, {1, 1, 1, 0.9f});
  text.addRect(27 * px, 19 * px, 3.5f * px, 12 * px, {1, 1, 1, 0.9f});
  left.color = {1, 1, 1, 0.75f};
  text.addScreen(clip(songs_[static_cast<size_t>(song_)].title, 20), 48 * px, 14 * px, 7 * px, left);
  left.color = kDifficultyColor[static_cast<int>(difficulty_)];
  const SongChoice& now = songs_[static_cast<size_t>(song_)];
  text.addScreen((now.pack() ? "STAGE " + std::to_string(now.stage) + " " + (now.skill == "speed" ? "SPEED" : now.skill == "chords" ? "CHORDS" : "TECH")
                             : std::string(kDifficultyName[static_cast<int>(difficulty_)])) + (autoplay ? "  AUTO" : ""),
                 48 * px, 26 * px, 6 * px, left);
  if (songTime_ < 0) {
    std::snprintf(line, sizeof(line), "%d", static_cast<int>(std::ceil(-songTime_)));
    center.color = {1, 1, 1, 1};
    text.addScreen(songTime_ < -kLeadIn + 0.5 ? "READY" : line, W / 2, H * 0.22f, std::min(cw * 0.4f, 28 * px), center);
  }
}

void RhythmGame::drawResults(engine::TextRenderer& text, float W, float H, float px) const {
  const Skin& sk = kSkins[skin_];
  const double acc = accuracy();
  const std::string grade = gradeOf(acc, counts_[kMiss] > 0);
  engine::TextStyle st;
  st.align = 0.5f;
  st.outline = sk.ink;
  st.outlineWidth = 0.8f;
  // Header: the song and difficulty, on a rounded banner.
  roundRect(text, W * 0.04f, H * 0.015f, W * 0.92f, H * 0.1f, 16 * px, sk.ink);
  roundRect(text, W * 0.04f + 2 * px, H * 0.015f + 2 * px, W * 0.92f - 4 * px, H * 0.1f - 4 * px, 14 * px, mix(sk.card, sk.accent, 0.25f));
  st.color = {1, 1, 1, 1};
  text.addScreen(clip(songs_[static_cast<size_t>(song_)].title, 30), W / 2, H * 0.03f, 13 * px, st);
  st.color = kDifficultyColor[static_cast<int>(difficulty_)];
  {
    const SongChoice& hs = songs_[static_cast<size_t>(song_)];
    const std::string label = hs.pack() ? "TRAINING  STAGE " + std::to_string(hs.stage) + "  " +
                                              (hs.skill == "speed" ? "SPEED" : hs.skill == "chords" ? "CHORDS & JACKS" : "TECH")
                                        : std::string(kDifficultyName[static_cast<int>(difficulty_)]);
    text.addScreen(label + (autoplay ? "  (AUTO)" : ""), W / 2, H * 0.03f + 20 * px, 7 * px, st);
  }
  const bool upright = H >= W;
  // The grade, big, in a bubble.
  const float gs = std::min(H * 0.2f, W * 0.3f);
  const float gx = upright ? W / 2 : W * 0.27f, gy = upright ? H * 0.16f : H * 0.2f;
  const float gr = gs * 0.85f, gcy = gy + gs / 2;
  text.addShape(engine::font::kShapeDisc, gx, gcy + 4 * px, gr, withAlpha(sk.ink, 0.6f));
  text.addShape(engine::font::kShapeDisc, gx, gcy, gr, withAlpha(darker(gradeColor(grade), 0.4f), 1), sk.ink, 0.4f);
  text.addShape(engine::font::kShapeDisc, gx - gr * 0.35f, gcy - gr * 0.4f, gr * 0.22f, {1, 1, 1, 0.3f});
  st.color = gradeColor(grade);
  text.addScreen(grade, gx, gy + gs * 0.1f, gs * 0.8f, st);
  const SongChoice& played = songs_[static_cast<size_t>(song_)];
  if (played.pack()) {
    st.color = acc >= 0.95 ? Vec4{0.45f, 1.0f, 0.55f, 1} : Vec4{1.0f, 0.75f, 0.4f, 1};
    text.addScreen(acc >= 0.95 ? "CLEARED!" : "95% TO CLEAR", gx, gy + gs + gr * 0.2f + 6 * px, 10 * px, st);
    if (newBest_) {
      st.color = sk.accent;
      text.addScreen("NEW BEST!", gx, gy + gs + gr * 0.2f + 22 * px, 8 * px, st);
    }
  } else if (newBest_) {
    st.color = sk.accent;
    text.addScreen("NEW BEST!", gx, gy + gs + gr * 0.2f + 6 * px, 10 * px, st);
  }
  // Score, accuracy, combo, the count of each judgement.
  const float cx = upright ? W / 2 : W * 0.68f;
  float y = upright ? H * 0.44f : H * 0.18f;
  char line[64];
  st.color = {1, 1, 1, 1};
  std::snprintf(line, sizeof(line), "%07d", score());
  text.addScreen(line, cx, y, 20 * px, st);
  y += 30 * px;
  std::snprintf(line, sizeof(line), "ACCURACY %.2f%%   MAX COMBO %d", acc * 100, maxCombo_);
  st.color = {0.9f, 0.9f, 1.0f, 1};
  text.addScreen(line, cx, y, 8 * px, st);
  st.color = comboColor(maxCombo_);
  y += 20 * px;
  const float colW = std::min(W * 0.4f, 150 * px);
  for (int j = 0; j < kJudgeCount; ++j) {
    const float rx = cx + (j % 2 == 0 ? -colW / 2 : colW / 2), ry = y + static_cast<float>(j / 2) * 24 * px;
    roundRect(text, rx - colW * 0.45f, ry - 5 * px, colW * 0.9f, 20 * px, 10 * px, withAlpha(sk.card, 0.85f));
    st.color = j == kMax ? Vec4{1, 1, 0.9f, 1} : kJudgeColor[j];
    std::snprintf(line, sizeof(line), "%-4s %d", kJudgeName[j], counts_[j]);
    text.addScreen(line, rx, ry, 10 * px, st);
  }
  // Buttons.
  const float bw = std::min(W * 0.28f, H * 0.4f), bh = std::min(H * 0.07f, 60.0f * 3), by = H * 0.88f;
  const char* labels[3] = {"RETRY", "SONGS", "LEAVE"};
  const Vec4 colors[3] = {sk.accent, sk.accent2, {1.0f, 0.4f, 0.4f, 1}};
  for (int i = 0; i < 3; ++i) {
    const float x = W / 2 + (static_cast<float>(i) - 1.5f) * (bw + W * 0.02f) + W * 0.01f;
    bubble(text, x, by, bw, bh, labels[i], px, colors[i], true, sk.ink);
  }
}


// ---- The sync (J117.6) ----

void RhythmGame::openSync() {
  engine::audio::previewStop();
  previewSong_ = -1;
  syncErrors_.clear();
  syncOffsetMs_ = offsetMs_;
  engine::Track::loadClicks(kSyncBpm, kSyncBeats, kSyncFirst);
  engine::Track::play(0.3);
  lastPlayFrameMs_ = 0;
  state_ = State::kSync;
  LOG_INFO("4K Mania: sync opened (offset now %.0f ms).", offsetMs_);
}

RhythmGame::Rect RhythmGame::syncButton(int i, float W, float H) const {
  const float y = H - H * 0.1f, h = H * 0.065f, w = W * 0.21f;
  return {W * 0.04f + static_cast<float>(i) * W * 0.24f, y, w, h};
}

double RhythmGame::syncMeasuredMs() const {
  if (syncErrors_.size() < 6) return syncOffsetMs_;
  std::vector<double> v = syncErrors_;
  std::nth_element(v.begin(), v.begin() + static_cast<long>(v.size() / 2), v.end());
  return v[v.size() / 2];
}

void RhythmGame::updateSync(double nowMs, const std::vector<engine::Input::RawPress>& presses, float W, float H) {
  songTime_ = engine::Track::timeAt(nowMs);  // the sound's own time, no offset: that's what is measured
  if (lastPlayFrameMs_ > 0) drawLead_ += (std::clamp((nowMs - lastPlayFrameMs_) / 1000.0, 0.004, 0.034) - drawLead_) * 0.1;
  lastPlayFrameMs_ = nowMs;
  updateEffects(nowMs, W, H);
  const double beat = 60.0 / kSyncBpm, last = kSyncFirst + (kSyncBeats - 1) * beat;
  if (songTime_ > last + 1.0) engine::Track::play(0.2);  // around again
  for (const engine::Input::RawPress& r : presses) {
    if (r.key >= 0 && r.key < 4 && !r.down) continue;
    const Vec2 p{r.pos.x * W, r.pos.y * H};
    int button = -1;
    for (int i = 0; i < 4; ++i) {
      if (r.key < 0 && syncButton(i, W, H).contains(p.x, p.y)) button = i;
    }
    if (button < 0) {
      if (!r.down) continue;
      // A tap along: how far from the nearest click, in ms (late = positive).
      const double t = songTime_ - (nowMs - r.ms) / 1000.0;
      const double k = std::round((t - kSyncFirst) / beat);
      if (k < 2 || k >= kSyncBeats) continue;  // the first clicks are for finding the beat
      const double err = (t - (kSyncFirst + k * beat)) * 1000;
      if (std::fabs(err) > beat * 400) continue;  // nowhere near a click
      syncErrors_.push_back(err);
      if (syncErrors_.size() > 32) syncErrors_.erase(syncErrors_.begin());
      if (syncErrors_.size() >= 6) syncOffsetMs_ = syncMeasuredMs();
      laneDownMs_[std::clamp(static_cast<int>(p.x / (W / 4)), 0, 3)] = nowMs;
      continue;
    }
    if (!r.down) continue;  // buttons work on the press
    if (button == 0) {
      engine::Track::stop();
      state_ = State::kMenu;
      return;
    }
    if (button == 1) syncOffsetMs_ = std::max(-300.0, syncOffsetMs_ - 5);
    if (button == 2) syncOffsetMs_ = std::min(300.0, syncOffsetMs_ + 5);
    if (button == 3) {
      offsetMs_ = std::round(syncOffsetMs_);
      engine::settings::remember("rhOffset", offsetMs_);
      LOG_INFO("4K Mania: sync saved, offset %.0f ms (%zu taps).", offsetMs_, syncErrors_.size());
      engine::ui::toast("Synced: " + std::to_string(static_cast<int>(offsetMs_)) + " ms");
      engine::Track::stop();
      state_ = State::kMenu;
      return;
    }
  }
}

void RhythmGame::drawSync(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const Skin& sk = kSkins[skin_];
  engine::TextStyle st;
  st.align = 0.5f;
  st.outline = sk.ink;
  st.outlineWidth = 0.8f;
  st.color = sk.accent;
  text.addScreen("SYNC YOUR SOUND", W / 2, H * 0.07f, 10 * px, st);
  st.color = {1, 1, 1, 1};
  text.addScreen("Tap anywhere along with the clicks you hear.", W / 2, H * 0.07f + 22 * px, 7 * px, st);
  text.addScreen("Headphones change the delay: sync again when you switch.", W / 2, H * 0.07f + 34 * px, 6 * px, st);
  // A light that flashes on each click, with the offset being set: when it
  // matches what you hear, the offset is right.
  const double beat = 60.0 / kSyncBpm;
  const double shown = songTime_ + drawLead_ - syncOffsetMs_ / 1000.0;
  const double k = std::floor((shown - kSyncFirst) / beat);
  const float since = static_cast<float>(shown - (kSyncFirst + k * beat));
  const bool accent = static_cast<int>(k) % 4 == 0;
  const float flash = k >= 0 && k < kSyncBeats ? std::clamp(1.0f - since / 0.15f, 0.0f, 1.0f) : 0.0f;
  const float r = std::min(W, H) * 0.16f;
  const Vec4 c = accent ? sk.accent : sk.accent2;
  text.addShape(engine::font::kShapeDisc, W / 2, H * 0.4f, r * (1 + 0.15f * flash), withAlpha(c, 0.15f + 0.6f * flash));
  text.addShape(engine::font::kShapeRing, W / 2, H * 0.4f, r * 1.15f, withAlpha(c, 0.8f));
  char line[96];
  std::snprintf(line, sizeof(line), "%+.0f ms", syncOffsetMs_);
  text.addScreen(line, W / 2, H * 0.4f - 10 * px, 14 * px, st);
  st.color = {0.85f, 0.83f, 0.92f, 1};
  std::snprintf(line, sizeof(line), syncErrors_.size() >= 6 ? "measured from %zu taps (saved now: %+.0f ms)"
                                                             : "tap along a few more times (%zu so far; saved now: %+.0f ms)",
                syncErrors_.size(), offsetMs_);
  text.addScreen(line, W / 2, H * 0.62f, 7 * px, st);
  // The last taps on a little bar (left = early, right = late).
  const float bw = W * 0.7f, by = H * 0.68f;
  text.addRect(W / 2 - bw / 2, by, bw, 2 * px, {1, 1, 1, 0.3f});
  text.addRect(W / 2 - px, by - 6 * px, 2 * px, 14 * px, {1, 1, 1, 0.9f});
  for (size_t i = 0; i < syncErrors_.size(); ++i) {
    const float a = 0.25f + 0.75f * static_cast<float>(i + 1) / static_cast<float>(syncErrors_.size());
    const float x = W / 2 + static_cast<float>(std::clamp(syncErrors_[i], -150.0, 150.0) / 150.0) * bw / 2;
    text.addRect(x - px, by - 4 * px, 2 * px, 10 * px, withAlpha(sk.accent, a));
  }
  static const char* kLabels[4] = {"BACK", "-5", "+5", "SAVE"};
  for (int i = 0; i < 4; ++i) {
    const Rect b = syncButton(i, W, H);
    bubble(text, b.x, b.y, b.w, b.h, kLabels[i], px, i == 0 ? Vec4{1.0f, 0.4f, 0.4f, 1} : i == 3 ? sk.accent : sk.accent2, true,
           sk.ink);
  }
  (void)nowMs;
}

// ---- The finger and frame test (J117.1) ----

void RhythmGame::openTest(double nowMs) {
  test_ = {};
  test_.openMs = test_.fpsFrom = nowMs;
  testFingers_.clear();
  lastTestLogMs_ = nowMs;
  engine::audio::previewStop();
  previewSong_ = -1;
  state_ = State::kTest;
  LOG_INFO("Finger test opened.");
}

RhythmGame::Rect RhythmGame::testButton(int i, float W, float H) const {
  const float y = H - H * 0.1f, h = H * 0.065f;
  if (i == 0) return {W * 0.05f, y, W * 0.24f, h};
  if (i == 1) return {W * 0.32f, y, W * 0.32f, h};
  return {W * 0.67f, y, W * 0.28f, h};
}

std::string RhythmGame::testSummary() const {
  const TestStats& t = test_;
  char b[512];
  const double secs = std::max(0.001, (t.lastFrameMs - t.openMs) / 1000.0);
  std::snprintf(b, sizeof(b),
                "Finger test: %.0f s, %d frames (%.0f a second now), longest gap %.0f ms, %d gaps over 50 ms; "
                "%d touches, most at once %d, lost by the phone %d, late %d (touch to frame avg %.1f ms, max %.0f ms); "
                "sound clock still on %d of %d frames, biggest step %.1f ms; gesture guard %s.",
                secs, t.frames, t.fps, t.maxGapMs, t.gaps50, t.presses, t.maxDown, t.lost, t.late,
                t.presses ? t.sumDelayMs / t.presses : 0.0, t.maxDelayMs, t.audioStill, t.audioFrames, t.audioMaxStepMs, guard_ ? "on" : "off");
  return b;
}

void RhythmGame::updateTest(double nowMs, const std::vector<engine::Input::RawPress>& presses, float W, float H) {
  TestStats& t = test_;
  if (t.lastFrameMs > 0) {
    const double gap = nowMs - t.lastFrameMs;
    t.maxGapMs = std::max(t.maxGapMs, gap);
    if (gap > 50) ++t.gaps50;
  }
  t.lastFrameMs = nowMs;
  ++t.frames;
  ++t.fpsFrames;
  if (nowMs - t.fpsFrom >= 1000) {
    t.fps = static_cast<float>(t.fpsFrames * 1000.0 / (nowMs - t.fpsFrom));
    t.fpsFrom = nowMs;
    t.fpsFrames = 0;
  }
  // The sound clock (read once per frame by the page mailbox): how far it moved.
  const double audio = engine::bridge::frame().audioTime;
  if (audio > 0 && t.lastAudio > 0) {
    ++t.audioFrames;
    const double step = (audio - t.lastAudio) * 1000;
    if (step < 0.01) ++t.audioStill;
    t.audioMaxStepMs = std::max(t.audioMaxStepMs, step);
  }
  t.lastAudio = audio;
  updateEffects(nowMs, W, H);
  for (const engine::Input::RawPress& r : presses) {
    if (r.key >= 0) continue;
    const Vec2 p{r.pos.x * W, r.pos.y * H};
    if (r.down) {
      ++t.presses;
      const double delay = std::max(0.0, nowMs - r.ms);
      t.sumDelayMs += delay;
      t.maxDelayMs = std::max(t.maxDelayMs, delay);
      if (delay > 34) ++t.late;
      testFingers_[r.id] = {p, std::clamp(static_cast<int>(p.x / (W / 4)), 0, 3), nowMs};
      t.maxDown = std::max(t.maxDown, static_cast<int>(testFingers_.size()));
      laneDownMs_[testFingers_[r.id].lane] = nowMs;
      continue;
    }
    const auto it = testFingers_.find(r.id);
    if (it == testFingers_.end()) continue;
    const Vec2 start = it->second.pos;
    testFingers_.erase(it);
    if (r.cancelled) {
      ++t.lost;
      continue;
    }
    if (testButton(0, W, H).contains(start.x, start.y)) {
      LOG_INFO("%s", testSummary().c_str());
      state_ = State::kMenu;
      return;
    }
    if (testButton(2, W, H).contains(start.x, start.y)) {
      guard_ = !guard_;
      engine::settings::remember("rhGuard", guard_ ? 1 : 0);
      LOG_INFO("4K Mania: gesture guard %s.", guard_ ? "on" : "off");
      syncGuard();
    }
    if (testButton(1, W, H).contains(start.x, start.y)) {
      LOG_INFO("%s", testSummary().c_str());
      engine::bridge::op("[\"maniaReport\"," + engine::jsonQuote(testSummary()) + ",1]");
    }
  }
  // A line in the log every 10 s, so a report sent from Settings has it too.
  if (nowMs - lastTestLogMs_ > 10000) {
    lastTestLogMs_ = nowMs;
    engine::bridge::op("[\"maniaReport\"," + engine::jsonQuote(testSummary()) + ",0]");
  }
}

void RhythmGame::drawTest(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const Skin& sk = kSkins[skin_];
  const TestStats& t = test_;
  for (int l = 0; l < 4; ++l) {
    bool held = false;
    for (const auto& f : testFingers_) held |= f.second.lane == l;
    const float since = static_cast<float>(nowMs - laneDownMs_[l]);
    const float on = held ? 1.0f : std::clamp(1.0f - since / 160.0f, 0.0f, 1.0f);
    text.addRect(W / 4 * static_cast<float>(l), 0, W / 4, H, withAlpha(sk.lane[l], 0.05f + 0.12f * on));
    if (l > 0) text.addRect(W / 4 * static_cast<float>(l) - 0.5f * px, 0, px, H, {1, 1, 1, 0.08f});
  }
  engine::TextStyle st;
  st.outline = sk.ink;
  st.outlineWidth = 0.8f;
  st.color = sk.accent;
  const float x = std::min(W, H) * 0.06f;
  float y = H * 0.05f;
  text.addScreen("FINGER TEST", x, y, 9 * px, st);
  y += 18 * px;
  st.color = {1, 1, 1, 1};
  text.addScreen("Four fingers down, tap fast, hold three.", x, y, 7.5f * px, st);
  y += 22 * px;
  char line[128];
  const auto row = [&](const char* label, const std::string& value) {
    engine::TextStyle s = st;
    s.color = {0.8f, 0.78f, 0.88f, 1};
    text.addScreen(label, x, y, 6.5f * px, s);
    s.color = {1, 1, 1, 1};
    text.addScreen(value, x, y + 11 * px, 10 * px, s);
    y += 30 * px;
  };
  row("FINGERS DOWN (MOST AT ONCE)", std::to_string(testFingers_.size()) + "  (" + std::to_string(t.maxDown) + ")");
  std::snprintf(line, sizeof(line), "%d lost   %d late   of %d", t.lost, t.late, t.presses);
  row("TOUCHES THE PHONE LOST OR HELD BACK", line);
  std::snprintf(line, sizeof(line), "%.0f a second   longest gap %.0f ms", t.fps, t.maxGapMs);
  row("FRAMES", line);
  std::snprintf(line, sizeof(line), "avg %.0f ms   max %.0f ms", t.presses ? t.sumDelayMs / t.presses : 0.0, t.maxDelayMs);
  row("TOUCH TO GAME", line);
  if (t.audioFrames > 0) {
    std::snprintf(line, sizeof(line), "still on %d%% of frames, steps up to %.0f ms", t.audioStill * 100 / t.audioFrames,
                  t.audioMaxStepMs);
  } else {
    std::snprintf(line, sizeof(line), "no sound yet (tap once)");
  }
  row("SOUND CLOCK", line);
  if (t.fps > 0 && t.fps < 75) {  // Safari's 60 a second (J117.4)
    engine::TextStyle tip = st;
    tip.color = {1.0f, 0.83f, 0.54f, 1};
    text.addScreen("For 120 a second: Settings > Apps > Safari > Advanced >", x, y, 6 * px, tip);
    text.addScreen("Feature Flags > turn off \"Prefer Page Rendering Updates near 60fps\".", x, y + 9 * px, 6 * px, tip);
  }
  // Each finger where it came down.
  for (const auto& f : testFingers_) {
    const Vec4 c = sk.lane[f.second.lane];
    text.addShape(engine::font::kShapeDisc, f.second.pos.x, f.second.pos.y, 34 * px, withAlpha(c, 0.35f));
    text.addShape(engine::font::kShapeRing, f.second.pos.x, f.second.pos.y, 34 * px, c);
  }
  const Rect b0 = testButton(0, W, H), b1 = testButton(1, W, H);
  bubble(text, b0.x, b0.y, b0.w, b0.h, "BACK", px, {1.0f, 0.4f, 0.4f, 1}, true, sk.ink);
  bubble(text, b1.x, b1.y, b1.w, b1.h, "SEND REPORT", px, sk.accent, true, sk.ink);
  const Rect b2 = testButton(2, W, H);
  bubble(text, b2.x, b2.y, b2.w, b2.h, guard_ ? "GUARD: ON" : "GUARD: OFF", px, sk.accent2, guard_, sk.ink);
  engine::TextStyle note = st;
  note.color = {0.8f, 0.78f, 0.88f, 1};
  text.addScreen("Guard: blocks the phone's own finger gestures while you play.", x, b2.y - 16 * px, 6 * px, note);
}

void RhythmGame::screenState(float out[5]) const {
  out[0] = static_cast<float>(combo_);
  out[1] = static_cast<float>(accuracy());
  int mask = 0;
  for (int l = 0; l < 4; ++l) mask |= lanePressed_[l] > 0 ? 1 << l : 0;
  out[2] = static_cast<float>(mask);
  out[3] = state_ == State::kPlaying ? 1.0f : 0.0f;
  out[4] = static_cast<float>(songTime_);
}

void RhythmGame::drawCabinet(engine::TextRenderer& text, const engine::Vec3 c[4], const float state[5], double nowMs) {
  using engine::Vec3;
  // A point on the screen: u across (0..1 left to right), v down (0..1).
  auto at = [&](float u, float v) { return c[3] + (c[2] - c[3]) * u + (c[0] - c[3]) * v; };
  auto quad = [&](float u0, float v0, float u1, float v1, Vec4 color) {
    const Vec3 p[4] = {at(u0, v1), at(u1, v1), at(u1, v0), at(u0, v0)};
    text.addWorldQuad(p, color);
  };
  const int mask = static_cast<int>(state[2]);
  const bool playing = state[3] > 0;
  for (int l = 0; l < 4; ++l) {
    const float u0 = 0.1f + 0.2f * static_cast<float>(l);
    quad(u0, 0.05f, u0 + 0.19f, 0.95f, {0.02f, 0.02f, 0.04f, 1});
    // Notes falling (a pattern from the song time, not the real chart).
    if (playing) {
      for (int k = 0; k < 4; ++k) {
        const float phase = std::fmod(state[4] * 1.3f + static_cast<float>(k) * 0.25f + static_cast<float>(l) * 0.37f, 1.0f);
        if ((k + l) % 2 == 0) quad(u0 + 0.01f, phase * 0.8f, u0 + 0.18f, phase * 0.8f + 0.03f, kLaneColor[l]);
      }
    }
    const bool held = (mask >> l) & 1;
    quad(u0 + 0.01f, 0.84f, u0 + 0.18f, 0.9f, held ? kLaneColor[l] : withAlpha(kLaneColor[l], 0.3f));
  }
  if (playing) {
    char line[48];
    std::snprintf(line, sizeof(line), "%dx  %.1f%%", static_cast<int>(state[0]), state[1] * 100.0f);
    engine::TextStyle s;
    s.align = 0.5f;
    const Vec3 right = engine::normalize(c[1] - c[0]), up = engine::normalize(c[3] - c[0]);
    text.addWorld(line, at(0.5f, 0.12f) + engine::cross(right, up) * 0.002f, right, up, 0.025f, s);
  }
  (void)nowMs;
}

}  // namespace app
