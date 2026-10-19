#include "app/rhythm.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/audio.h"
#include "engine/font.h"
#include "engine/frame_time.h"
#include "engine/log.h"
#include "engine/ui.h"

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
const Vec4 kPink{1.0f, 0.4f, 0.75f, 1};
constexpr const char* kDifficultyName[kDifficultyCount] = {"EASY", "NORMAL", "HARD", "INSANE"};
const Vec4 kDifficultyColor[kDifficultyCount] = {{0.45f, 0.9f, 0.5f, 1}, {0.45f, 0.75f, 1.0f, 1},
                                                 {1.0f, 0.72f, 0.28f, 1}, {1.0f, 0.35f, 0.55f, 1}};
constexpr float kTypicalStars[kDifficultyCount] = {1.4f, 2.4f, 3.6f, 4.7f};

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

void button(engine::TextRenderer& text, float x, float y, float w, float h, const char* label, float px, bool active,
            Vec4 color = kPink) {
  text.addRect(x, y, w, h, active ? withAlpha(color, 0.85f) : Vec4{0.1f, 0.08f, 0.14f, 0.9f});
  text.addRect(x, y + h - 2 * px, w, 2 * px, withAlpha(color, active ? 1.0f : 0.6f));
  engine::TextStyle s;
  s.align = 0.5f;
  s.color = active ? Vec4{0.05f, 0.03f, 0.08f, 1} : Vec4{1, 1, 1, 1};
  s.outline.w = active ? 0 : 0.6f;
  const float cap = std::min(h * 0.4f, 13 * px);
  text.addScreen(label, x + w / 2, y + (h - cap) / 2, cap, s);
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
  return songs_[static_cast<size_t>(song)].title + "|" + kDifficultyName[static_cast<int>(d)];
}

void RhythmGame::open(std::vector<SongChoice> extra) {
  songs_ = builtInSongs();
  for (SongChoice& s : extra) {
    if (std::none_of(songs_.begin(), songs_.end(), [&](const SongChoice& o) { return o.url == s.url; })) {
      if (s.artist.empty()) s.artist = "From the jukebox";
      songs_.push_back(std::move(s));
    }
  }
  song_ = std::clamp(song_, 0, std::max(0, static_cast<int>(songs_.size()) - 1));
  wheel_ = static_cast<float>(song_);
  scrollSpeed_ = std::clamp(static_cast<int>(engine::settings::remembered("rhSpeed", 18)), 8, 40);
  // Bests and star ratings kept on this phone.
  for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
    for (int d = 0; d < kDifficultyCount; ++d) {
      const std::string key = bestKey(i, static_cast<Difficulty>(d));
      const double acc = engine::settings::remembered(("rhAcc." + key).c_str(), -1);
      if (acc >= 0) best_[key] = {acc, static_cast<int>(engine::settings::remembered(("rhScore." + key).c_str(), 0))};
      const double st = engine::settings::remembered(("rhStars." + key).c_str(), -1);
      if (st >= 0) starCache_[key] = static_cast<float>(st);
    }
  }
  state_ = State::kMenu;
  engine::audio::setMusicGain(0.15f);  // turn the jukebox down while you play
  engine::ui::setBodyClass("racing", true);  // (hides the page's top bar: the game has the whole screen)
  LOG_INFO("Rhythm game opened (%zu songs).", songs_.size());
}

void RhythmGame::close() {
  engine::Track::stop();
  engine::audio::setMusicGain(1.0f);
  engine::ui::setBodyClass("racing", false);
  state_ = State::kClosed;
}

float RhythmGame::columnWidth(float W, float H) const { return std::min(W * 0.22f, H * 0.13f); }

float RhythmGame::laneX(int lane, float W, float H) const {
  const float cw = columnWidth(W, H);
  return (W - 4 * cw) / 2 + (static_cast<float>(lane) + 0.5f) * cw;
}

double RhythmGame::accuracy() const { return judgements_ ? points_ / (300.0 * judgements_) : 1.0; }

int RhythmGame::score() const {
  return totalJudgements_ ? static_cast<int>(std::lround(1e6 * scorePoints_ / (320.0 * totalJudgements_))) : 0;
}

float RhythmGame::stars(int song, Difficulty d, bool* known) const {
  const auto it = starCache_.find(bestKey(song, d));
  if (known) *known = it != starCache_.end();
  return it != starCache_.end() ? it->second : kTypicalStars[static_cast<int>(d)];
}

void RhythmGame::startSong() {
  if (songs_.empty()) return;
  song_ = std::clamp(song_, 0, static_cast<int>(songs_.size()) - 1);
  engine::Track::load(songs_[static_cast<size_t>(song_)].url);
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
  points_ = scorePoints_ = 0;
  health_ = 1;
  bursts_.clear();
  ticks_.clear();
  lastJudgeMs_ = -1e9;
  std::fill(std::begin(lanePressed_), std::end(lanePressed_), 0);
  fingerLane_.clear();
  newBest_ = false;
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

// ---- Song select ----

RhythmGame::MenuLayout RhythmGame::menuLayout(float W, float H) const {
  MenuLayout m;
  if (H >= W) {  // phone upright: the info on top, the wheel below
    m.info = {W * 0.05f, H * 0.035f, W * 0.9f, H * 0.2f};
    const float dy = H * 0.25f, dw = (W * 0.9f - 3 * W * 0.02f) / 4;
    for (int d = 0; d < kDifficultyCount; ++d) m.diff[d] = {W * 0.05f + static_cast<float>(d) * (dw + W * 0.02f), dy, dw, H * 0.065f};
    m.speedDown = {W * 0.05f, H * 0.33f, W * 0.13f, H * 0.045f};
    m.speedUp = {W * 0.52f, H * 0.33f, W * 0.13f, H * 0.045f};
    m.autoBtn = {W * 0.7f, H * 0.33f, W * 0.25f, H * 0.045f};
    m.wheel = {0, H * 0.395f, W, H * 0.47f};
    m.card = H * 0.068f;
    m.back = {W * 0.05f, H * 0.89f, W * 0.2f, H * 0.065f};
    m.play = {W * 0.3f, H * 0.89f, W * 0.65f, H * 0.065f};
  } else {  // sideways: the info on the left, the wheel on the right
    m.info = {W * 0.03f, H * 0.05f, W * 0.43f, H * 0.36f};
    const float dw = (W * 0.43f - 3 * W * 0.01f) / 4;
    for (int d = 0; d < kDifficultyCount; ++d) m.diff[d] = {W * 0.03f + static_cast<float>(d) * (dw + W * 0.01f), H * 0.45f, dw, H * 0.12f};
    m.speedDown = {W * 0.03f, H * 0.62f, W * 0.06f, H * 0.09f};
    m.speedUp = {W * 0.25f, H * 0.62f, W * 0.06f, H * 0.09f};
    m.autoBtn = {W * 0.34f, H * 0.62f, W * 0.12f, H * 0.09f};
    m.wheel = {W * 0.49f, 0, W * 0.51f, H};
    m.card = H * 0.125f;
    m.back = {W * 0.03f, H * 0.8f, W * 0.1f, H * 0.13f};
    m.play = {W * 0.15f, H * 0.8f, W * 0.31f, H * 0.13f};
  }
  return m;
}

void RhythmGame::pickSong(int index) {
  if (songs_.empty()) return;
  song_ = std::clamp(index, 0, static_cast<int>(songs_.size()) - 1);
}

void RhythmGame::menuPress(Vec2 p, float W, float H) {
  const MenuLayout m = menuLayout(W, H);
  for (int d = 0; d < kDifficultyCount; ++d) {
    if (m.diff[d].contains(p.x, p.y)) difficulty_ = static_cast<Difficulty>(d);
  }
  if (m.speedDown.contains(p.x, p.y) || m.speedUp.contains(p.x, p.y)) {
    scrollSpeed_ = std::clamp(scrollSpeed_ + (m.speedUp.contains(p.x, p.y) ? 1 : -1), 8, 40);
    engine::settings::remember("rhSpeed", scrollSpeed_);
  }
  if (m.autoBtn.contains(p.x, p.y)) autoplay = !autoplay;
  if (m.play.contains(p.x, p.y)) startSong();
  if (m.back.contains(p.x, p.y)) close();
  if (m.wheel.contains(p.x, p.y) && !songs_.empty()) {
    const float step = m.card * 1.12f, mid = m.wheel.y + m.wheel.h / 2;
    const int i = static_cast<int>(std::lround(wheel_ + (p.y - mid) / step));
    if (i < 0 || i >= static_cast<int>(songs_.size())) return;
    if (i == song_) startSong();  // a second tap on the picked song plays it
    else pickSong(i);
  }
}

void RhythmGame::menuSwipe(float dy, float W, float H) {
  const MenuLayout m = menuLayout(W, H);
  pickSong(song_ - static_cast<int>(std::lround(dy / (m.card * 1.12f))));
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
      if (state_ == State::kPlaying || state_ == State::kWaitTap) {
        engine::Track::stop();
        state_ = State::kMenu;
      } else {
        close();
      }
      return;
    }
  }
  // The wheel glides to the picked song.
  const float dt = wheelMs_ > 0 ? static_cast<float>(std::min(0.1, (nowMs - wheelMs_) / 1000.0)) : 0;
  wheelMs_ = nowMs;
  wheel_ += (static_cast<float>(song_) - wheel_) * (1 - std::exp(-dt * 12));

  if (state_ == State::kMenu || state_ == State::kResults) {
    for (const engine::Input::RawPress& r : presses) {
      const Vec2 p{r.pos.x * W, r.pos.y * H};
      if (r.key >= 0 && r.key < 4 && r.down && state_ == State::kMenu) {  // keys: D/F song, J difficulty, K play
        if (r.key == 0) pickSong(song_ - 1);
        if (r.key == 1) pickSong(song_ + 1);
        if (r.key == 2) difficulty_ = static_cast<Difficulty>((static_cast<int>(difficulty_) + 1) % kDifficultyCount);
        if (r.key == 3) startSong();
        continue;
      }
      if (r.key >= 0) continue;
      if (r.down) {
        fingerDown_[r.id] = p;
        continue;
      }
      // A finger lifted: a swipe on the wheel scrolls, else it's a tap.
      const auto it = fingerDown_.find(r.id);
      if (it == fingerDown_.end()) continue;
      const Vec2 start = it->second;
      fingerDown_.erase(it);
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
    const std::string key = songs_[static_cast<size_t>(song_)].url + "#" + kDifficultyName[static_cast<int>(difficulty_)];
    auto cached = charts_.find(key);
    if (cached == charts_.end()) {
      float rate = 44100;
      std::vector<float> samples = engine::Track::takeSamples(rate);
      const double t0 = engine::clockMs();
      Chart chart = makeChart(samples.data(), samples.size(), rate, difficulty_);
      LOG_INFO("Chart made in %.0f ms.", engine::clockMs() - t0);
      // Star rating: mostly notes per second, plus chords and holds.
      if (chart.length > 0 && !chart.notes.empty()) {
        int chords = 0, holds = 0;
        for (size_t i = 0; i < chart.notes.size(); ++i) {
          if (i && chart.notes[i].time - chart.notes[i - 1].time < 1e-6) ++chords;
          if (chart.notes[i].end > 0) ++holds;
        }
        const double active = std::max(10.0, chart.notes.back().time - chart.notes.front().time);
        const double nps = static_cast<double>(chart.notes.size()) / active;
        const float st = static_cast<float>(std::clamp(0.35 + 0.62 * nps + 2.5 * chords / static_cast<double>(chart.notes.size()) +
                                                           0.8 * holds / static_cast<double>(chart.notes.size()), 0.5, 9.0));
        const std::string sk = bestKey(song_, difficulty_);
        starCache_[sk] = st;
        engine::settings::remember(("rhStars." + sk).c_str(), st);
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
  songTime_ = engine::Track::time();
  const float cw = columnWidth(W, H);
  const float x0 = (W - 4 * cw) / 2;
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
    if (r.down && r.key < 0 && p.x < W * 0.16f && p.y < H * 0.1f) {  // the pause / quit button
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
        if (lanePressed_[lane] == 0 && !autoplay) release(lane, t);
      }
      continue;
    }
    const int lane = r.key >= 0 && r.key < 4 ? r.key : std::clamp(static_cast<int>(std::floor((p.x - x0) / cw)), 0, 3);
    fingerLane_[r.id] = lane;
    ++lanePressed_[lane];
    laneDownMs_[lane] = nowMs;
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
    if (hit->end > 0) {
      if (j == kMiss) hit->tailDone = true, judge(*hit, kMiss, nowMs, 0, true);
      else hit->holding = true;
    }
  }
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    Note& n = notes_[i];
    if (n.time > songTime_ + 0.2) break;
    if (!n.judged) {
      if (autoplay && n.time <= songTime_) {
        judge(n, kMax, nowMs, 0);
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
      }
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

void RhythmGame::draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  if (state_ == State::kClosed) return;
  // Dim the bar behind the game.
  text.addRect(0, 0, W, H, {0.015f, 0.01f, 0.03f, state_ == State::kPlaying ? 0.86f : 0.9f});
  if (state_ == State::kMenu) return drawMenu(text, W, H, px, nowMs);
  if (state_ == State::kResults) return drawResults(text, W, H, px);
  if (state_ == State::kLoading || state_ == State::kAnalyzing || state_ == State::kWaitTap) {
    engine::TextStyle center;
    center.align = 0.5f;
    const SongChoice& s = songs_[static_cast<size_t>(song_)];
    text.addScreen(clip(s.title, 26), W / 2, H * 0.3f, 14 * px, center);
    center.color = kDifficultyColor[static_cast<int>(difficulty_)];
    text.addScreen(kDifficultyName[static_cast<int>(difficulty_)], W / 2, H * 0.3f + 22 * px, 9 * px, center);
    center.color = {1, 1, 1, 0.9f};
    const char* msg = state_ == State::kLoading ? "Loading the song..." : state_ == State::kAnalyzing ?
                      "Making the chart..." : "Tap to start";
    text.addScreen(msg, W / 2, H * 0.52f, 11 * px, center);
    for (int k = 0; k < 8; ++k) {  // spinner
      const float a = static_cast<float>(nowMs / 160.0) + static_cast<float>(k) * 0.785f;
      text.addShape(engine::font::kShapeDisc, W / 2 + std::cos(a) * 24 * px, H * 0.45f + std::sin(a) * 24 * px,
                    (2.0f + static_cast<float>(k) * 0.5f) * px, withAlpha(k % 2 ? kPink : kLaneColor[1], 0.3f + 0.08f * static_cast<float>(k)));
    }
    return;
  }
  drawPlay(text, W, H, px, nowMs);
}

void RhythmGame::drawMenu(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const MenuLayout m = menuLayout(W, H);
  engine::TextStyle st;
  // The wheel: cards above and below the picked song; the picked one slides out.
  if (!songs_.empty()) {
    const float step = m.card * 1.12f, mid = m.wheel.y + m.wheel.h / 2;
    for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
      const float off = static_cast<float>(i) - wheel_;
      const float y = mid + off * step - m.card / 2;
      if (y + m.card < m.wheel.y || y > m.wheel.y + m.wheel.h) continue;
      const bool sel = i == song_;
      const float slide = std::clamp(std::fabs(off), 0.0f, 3.0f) * m.wheel.w * 0.025f;
      const float x = m.wheel.x + m.wheel.w * 0.04f + slide, w = m.wheel.w * 0.92f - slide;
      text.addRect(x, y, w, m.card, sel ? Vec4{0.22f, 0.1f, 0.25f, 0.95f} : Vec4{0.08f, 0.07f, 0.12f, 0.85f});
      text.addRect(x, y, 4 * px, m.card, sel ? kPink : withAlpha(kLaneColor[1], 0.5f));
      if (sel) text.addRect(x, y + m.card - 2 * px, w, 2 * px, kPink);
      const SongChoice& s = songs_[static_cast<size_t>(i)];
      st.align = 0;
      st.color = {1, 1, 1, sel ? 1.0f : 0.8f};
      const float big = std::min(m.card * 0.3f, 12 * px);
      text.addScreen(clip(s.title, 24), x + 12 * px, y + m.card * 0.18f, big, st);
      st.color = {0.75f, 0.75f, 0.85f, 0.85f};
      std::string sub = s.artist;
      if (s.seconds > 0) sub += "  " + timeText(s.seconds);
      text.addScreen(clip(sub, 34), x + 12 * px, y + m.card * 0.6f, big * 0.6f, st);
      // Best grade on this difficulty.
      const auto it = best_.find(bestKey(i, difficulty_));
      if (it != best_.end()) {
        const std::string g = gradeOf(it->second.acc, it->second.acc < 0.999);
        st.align = 1;
        st.color = gradeColor(g);
        text.addScreen(g, x + w - 10 * px, y + m.card * 0.22f, m.card * 0.5f, st);
      }
    }
  }
  // The picked song's details.
  text.addRect(m.info.x, m.info.y, m.info.w, m.info.h, {0.06f, 0.05f, 0.1f, 0.92f});
  text.addRect(m.info.x, m.info.y, m.info.w, 3 * px, kPink);
  st.align = 0;
  st.color = kPink;
  const float pad = 10 * px;
  text.addScreen("4K MANIA", m.info.x + pad, m.info.y + pad, 9 * px, st);
  if (!songs_.empty()) {
    const SongChoice& s = songs_[static_cast<size_t>(song_)];
    st.color = {1, 1, 1, 1};
    const float tsize = std::min(m.info.h * 0.17f, 17 * px);
    text.addScreen(clip(s.title, 22), m.info.x + pad, m.info.y + pad + 16 * px, tsize, st);
    st.color = {0.8f, 0.8f, 0.9f, 1};
    text.addScreen(s.artist, m.info.x + pad, m.info.y + pad + 20 * px + tsize, 8 * px, st);
    char line[96];
    bool known = false;
    const float sr = stars(song_, difficulty_, &known);
    std::snprintf(line, sizeof(line), "%s%s  %.1f STARS", s.seconds > 0 ? ("LENGTH " + timeText(s.seconds) + "   ").c_str() : "",
                  kDifficultyName[static_cast<int>(difficulty_)], sr);
    const float y3 = m.info.y + pad + 34 * px + tsize;
    text.addScreen(known ? line : (std::string(line) + " (ABOUT)").c_str(), m.info.x + pad, y3, 7 * px, st);
    starRow(text, m.info.x + pad + 4 * px, y3 + 18 * px, 4 * px, sr, kDifficultyColor[static_cast<int>(difficulty_)]);
    const auto it = best_.find(bestKey(song_, difficulty_));
    if (it != best_.end()) {
      const std::string g = gradeOf(it->second.acc, it->second.acc < 0.999);
      std::snprintf(line, sizeof(line), "BEST  %.2f%%  %07d", it->second.acc * 100, it->second.score);
      st.color = gradeColor(g);
      text.addScreen(g + "  " + line, m.info.x + pad, y3 + 30 * px, 8 * px, st);
    } else {
      st.color = {0.7f, 0.7f, 0.8f, 0.8f};
      text.addScreen("NOT PLAYED YET", m.info.x + pad, y3 + 30 * px, 7 * px, st);
    }
  }
  // Difficulties, with their stars.
  for (int d = 0; d < kDifficultyCount; ++d) {
    const Rect& r = m.diff[d];
    const bool on = static_cast<int>(difficulty_) == d;
    text.addRect(r.x, r.y, r.w, r.h, on ? withAlpha(kDifficultyColor[d], 0.9f) : Vec4{0.1f, 0.08f, 0.14f, 0.9f});
    text.addRect(r.x, r.y + r.h - 2 * px, r.w, 2 * px, kDifficultyColor[d]);
    engine::TextStyle s;
    s.align = 0.5f;
    s.color = on ? Vec4{0.05f, 0.03f, 0.08f, 1} : Vec4{1, 1, 1, 1};
    s.outline.w = on ? 0 : 0.6f;
    const float cap = std::min(r.h * 0.28f, 9 * px);
    text.addScreen(kDifficultyName[d], r.x + r.w / 2, r.y + r.h * 0.18f, cap, s);
    char b[16];
    bool known = false;
    const float sr = songs_.empty() ? kTypicalStars[d] : stars(song_, static_cast<Difficulty>(d), &known);
    std::snprintf(b, sizeof(b), known ? "%.1f*" : "~%.1f*", sr);
    text.addScreen(b, r.x + r.w / 2, r.y + r.h * 0.58f, cap * 0.8f, s);
  }
  // Scroll speed and auto.
  button(text, m.speedDown.x, m.speedDown.y, m.speedDown.w, m.speedDown.h, "-", px, false);
  button(text, m.speedUp.x, m.speedUp.y, m.speedUp.w, m.speedUp.h, "+", px, false);
  {
    char b[32];
    std::snprintf(b, sizeof(b), "SPEED %d", scrollSpeed_);
    engine::TextStyle s;
    s.align = 0.5f;
    const float cx = (m.speedDown.x + m.speedDown.w + m.speedUp.x) / 2;
    text.addScreen(b, cx, m.speedDown.y + m.speedDown.h * 0.3f, std::min(m.speedDown.h * 0.4f, 9 * px), s);
  }
  button(text, m.autoBtn.x, m.autoBtn.y, m.autoBtn.w, m.autoBtn.h, autoplay ? "AUTO: ON" : "AUTO: OFF", px, autoplay,
         kLaneColor[1]);
  button(text, m.back.x, m.back.y, m.back.w, m.back.h, "BACK", px, false);
  button(text, m.play.x, m.play.y, m.play.w, m.play.h, "PLAY", px, true);
  if (songs_.empty()) {
    engine::TextStyle s;
    s.align = 0.5f;
    text.addScreen("No songs", m.wheel.x + m.wheel.w / 2, m.wheel.y + m.wheel.h / 2, 12 * px, s);
  }
  (void)nowMs;
}

void RhythmGame::drawPlay(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  const float cw = columnWidth(W, H), x0 = (W - 4 * cw) / 2;
  const float hitY = H * 0.83f, keyH = cw * 0.5f, topY = -cw;
  const double approach = approachSeconds();
  const float noteH = cw * 0.3f;
  const auto yOf = [&](double t) { return hitY - static_cast<float>((t - songTime_) / approach) * (hitY - topY); };
  // Stage: black columns, thin separators, bright edges, the hit line.
  text.addRect(x0 - 3 * px, 0, 4 * cw + 6 * px, H, {0, 0, 0, 0.9f});
  for (int l = 1; l < 4; ++l) text.addRect(x0 + static_cast<float>(l) * cw - 0.5f * px, 0, px, hitY, {1, 1, 1, 0.07f});
  text.addRect(x0 - 3 * px, 0, 2 * px, H, {1, 1, 1, 0.35f});
  text.addRect(x0 + 4 * cw + px, 0, 2 * px, H, {1, 1, 1, 0.35f});
  // Column lights: a glow rising from a key while it's held (and a moment after).
  for (int l = 0; l < 4; ++l) {
    const float since = static_cast<float>(nowMs - laneDownMs_[l]);
    const float on = lanePressed_[l] ? 1.0f : std::clamp(1.0f - since / 160.0f, 0.0f, 1.0f);
    if (on <= 0) continue;
    const float x = x0 + static_cast<float>(l) * cw;
    for (int k = 0; k < 10; ++k) {
      const float h = H * 0.034f;
      text.addRect(x + px, hitY - static_cast<float>(k + 1) * h, cw - 2 * px, h,
                   withAlpha(kLaneColor[l], on * 0.22f * (1.0f - static_cast<float>(k) / 10.0f)));
    }
  }
  // Notes: bars, and hold notes (a body up to the end, with a cap).
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    const Note& n = notes_[i];
    if (n.time - songTime_ > approach * 1.1) break;
    const bool hold = n.end > 0;
    if (n.judged && !hold) continue;
    if (hold && n.tailDone && !n.holding) continue;
    const Vec4 c = kLaneColor[n.lane];
    const float x = x0 + static_cast<float>(n.lane) * cw;
    const float headY = n.holding ? hitY : yOf(n.time);
    const bool dead = hold && n.judged && !n.holding;  // missed head: greyed
    if (hold) {
      const float endY = yOf(n.end);
      if (endY < H && headY > -noteH) {
        const float bw = cw * 0.78f, bx = x + (cw - bw) / 2;
        const float top = std::max(endY, -noteH), bottom = std::min(headY, H);
        const Vec4 body = dead ? Vec4{0.35f, 0.35f, 0.4f, 0.5f} : withAlpha(c, n.holding ? 0.85f : 0.6f);
        text.addRect(bx, top, bw, bottom - top, body);
        text.addRect(bx, top, bw, noteH * 0.45f, dead ? body : withAlpha(c, 1));  // the end cap
      }
    }
    if (n.judged && !n.holding) continue;
    if (headY < -noteH || headY > H + noteH) continue;
    const Vec4 head = dead ? Vec4{0.4f, 0.4f, 0.45f, 0.7f} : c;
    text.addRect(x + 2 * px, headY - noteH / 2, cw - 4 * px, noteH, head);
    text.addRect(x + 2 * px, headY - noteH / 2, cw - 4 * px, noteH * 0.28f, {1, 1, 1, 0.45f});
    text.addRect(x + 2 * px, headY + noteH / 2 - 2 * px, cw - 4 * px, 2 * px, {0, 0, 0, 0.35f});
  }
  // The hit line and the keys under it (lit while pressed).
  text.addRect(x0, hitY - 1.5f * px, 4 * cw, 3 * px, {1, 1, 1, 0.75f});
  for (int l = 0; l < 4; ++l) {
    const float x = x0 + static_cast<float>(l) * cw;
    const bool down = lanePressed_[l] > 0 || nowMs - laneDownMs_[l] < 60;
    text.addRect(x + 3 * px, hitY + 4 * px, cw - 6 * px, keyH, down ? withAlpha(kLaneColor[l], 0.9f) : Vec4{0.1f, 0.1f, 0.14f, 1});
    text.addRect(x + 3 * px, hitY + 4 * px, cw - 6 * px, 3 * px, withAlpha(kLaneColor[l], down ? 1.0f : 0.5f));
    text.addRect(x + cw * 0.3f, hitY + 4 * px + keyH * 0.45f, cw * 0.4f, keyH * 0.15f, down ? Vec4{1, 1, 1, 0.9f} : Vec4{1, 1, 1, 0.18f});
  }
  // Hit flashes over the hit line.
  for (const Burst& b : bursts_) {
    const float t = static_cast<float>((nowMs - b.ms) / 300.0);
    const float x = x0 + static_cast<float>(b.lane) * cw;
    const Vec4 c = b.judge == kMax ? maxColor(nowMs) : kJudgeColor[b.judge];
    const float grow = cw * 0.15f * t;
    text.addRect(x - grow, hitY - noteH * (0.6f + t), cw + 2 * grow, noteH * (1.2f + 2 * t), withAlpha(c, 0.45f * (1 - t)));
  }
  // Combo and judgement (osu!mania puts them in the stage's middle).
  engine::TextStyle center;
  center.align = 0.5f;
  center.outlineWidth = 0.7f;
  const double sinceJudge = nowMs - lastJudgeMs_;
  if (combo_ >= 2) {
    char c[16];
    std::snprintf(c, sizeof(c), "%d", combo_);
    const float pop = 1.0f + 0.18f * static_cast<float>(std::max(0.0, 1.0 - sinceJudge / 80.0));
    center.color = {1, 1, 1, 0.92f};
    text.addScreen(c, W / 2, H * 0.36f, std::min(cw * 0.42f, 30 * px) * pop, center);
  }
  if (sinceJudge < 500) {
    const Vec4 c = lastJudge_ == kMax ? maxColor(nowMs) : kJudgeColor[lastJudge_];
    const float fade = static_cast<float>(std::min(1.0, (500 - sinceJudge) / 150.0));
    center.color = withAlpha(c, fade);
    center.outline.w = 0.85f * fade;
    const float pop = 1.0f + 0.3f * static_cast<float>(std::max(0.0, 1.0 - sinceJudge / 90.0));
    text.addScreen(kJudgeName[lastJudge_], W / 2, H * 0.47f, std::min(cw * 0.3f, 20 * px) * pop, center);
    center.outline.w = 0.85f;
  }
  // Timing-error bar under the keys: the windows' colours and recent hits
  // (left = early, right = late).
  {
    const float bw = std::min(cw * 3.0f, W * 0.8f), bx = W / 2 - bw / 2;
    const float by = std::min(hitY + keyH + 14 * px, H - 8 * px);
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
    (void)bx;
  }
  // Health (right of the stage, rising from the hit line).
  {
    const float hx = x0 + 4 * cw + 6 * px, hy = H * 0.12f, hh = hitY - hy;
    text.addRect(hx, hy, 6 * px, hh, {1, 1, 1, 0.1f});
    const Vec4 c = mix(Vec4{1, 0.25f, 0.25f, 1}, Vec4{0.4f, 1, 0.5f, 1}, health_);
    text.addRect(hx, hy + hh * (1 - health_), 6 * px, hh * health_, c);
  }
  // Score, accuracy, progress, the pause button, the song's name.
  char line[64];
  engine::TextStyle right;
  right.align = 1;
  right.outlineWidth = 0.7f;
  std::snprintf(line, sizeof(line), "%07d", score());
  text.addScreen(line, W - 10 * px, 12 * px, 15 * px, right);
  std::snprintf(line, sizeof(line), "%.2f%%", accuracy() * 100);
  text.addScreen(line, W - 10 * px, 32 * px, 9 * px, right);
  const float progress = duration_ > 0 ? std::clamp(static_cast<float>(songTime_ / duration_), 0.0f, 1.0f) : 0;
  text.addRect(0, 0, W, 3 * px, {1, 1, 1, 0.12f});
  text.addRect(0, 0, W * progress, 3 * px, kPink);
  engine::TextStyle left;
  left.outlineWidth = 0.7f;
  text.addRect(10 * px, 10 * px, 30 * px, 30 * px, {0.15f, 0.12f, 0.2f, 0.85f});
  text.addRect(19 * px, 18 * px, 4 * px, 14 * px, {1, 1, 1, 0.9f});
  text.addRect(27 * px, 18 * px, 4 * px, 14 * px, {1, 1, 1, 0.9f});
  left.color = {1, 1, 1, 0.75f};
  text.addScreen(clip(songs_[static_cast<size_t>(song_)].title, 20), 48 * px, 14 * px, 7 * px, left);
  left.color = kDifficultyColor[static_cast<int>(difficulty_)];
  text.addScreen(std::string(kDifficultyName[static_cast<int>(difficulty_)]) + (autoplay ? "  AUTO" : ""), 48 * px, 26 * px, 6 * px, left);
  if (songTime_ < 0) {
    std::snprintf(line, sizeof(line), "%d", static_cast<int>(std::ceil(-songTime_)));
    center.color = {1, 1, 1, 1};
    text.addScreen(songTime_ < -kLeadIn + 0.5 ? "READY" : line, W / 2, H * 0.3f, std::min(cw * 0.4f, 28 * px), center);
  }
}

void RhythmGame::drawResults(engine::TextRenderer& text, float W, float H, float px) const {
  const double acc = accuracy();
  const std::string grade = gradeOf(acc, counts_[kMiss] > 0);
  engine::TextStyle st;
  st.align = 0.5f;
  st.outlineWidth = 0.7f;
  // Header: the song and difficulty.
  text.addRect(0, 0, W, H * 0.12f, {0.06f, 0.05f, 0.1f, 0.95f});
  text.addRect(0, H * 0.12f, W, 3 * px, kPink);
  st.color = {1, 1, 1, 1};
  text.addScreen(clip(songs_[static_cast<size_t>(song_)].title, 30), W / 2, H * 0.03f, 13 * px, st);
  st.color = kDifficultyColor[static_cast<int>(difficulty_)];
  text.addScreen(std::string(kDifficultyName[static_cast<int>(difficulty_)]) + (autoplay ? "  (AUTO)" : ""), W / 2,
                 H * 0.03f + 20 * px, 7 * px, st);
  const bool upright = H >= W;
  // The grade, big.
  const float gx = upright ? W / 2 : W * 0.27f, gy = upright ? H * 0.16f : H * 0.2f;
  st.color = gradeColor(grade);
  text.addScreen(grade, gx, gy, std::min(H * 0.2f, W * 0.3f), st);
  if (newBest_) {
    st.color = kPink;
    text.addScreen("NEW BEST!", gx, gy + std::min(H * 0.2f, W * 0.3f) + 10 * px, 10 * px, st);
  }
  // Score, accuracy, combo, the count of each judgement.
  const float cx = upright ? W / 2 : W * 0.68f;
  float y = upright ? H * 0.42f : H * 0.18f;
  char line[64];
  st.color = {1, 1, 1, 1};
  std::snprintf(line, sizeof(line), "%07d", score());
  text.addScreen(line, cx, y, 20 * px, st);
  y += 30 * px;
  std::snprintf(line, sizeof(line), "ACCURACY %.2f%%   MAX COMBO %d", acc * 100, maxCombo_);
  st.color = {0.85f, 0.85f, 0.95f, 1};
  text.addScreen(line, cx, y, 8 * px, st);
  y += 20 * px;
  const float colW = std::min(W * 0.4f, 150 * px);
  for (int j = 0; j < kJudgeCount; ++j) {
    const float rx = cx + (j % 2 == 0 ? -colW / 2 : colW / 2), ry = y + static_cast<float>(j / 2) * 22 * px;
    st.color = j == kMax ? Vec4{1, 1, 0.9f, 1} : kJudgeColor[j];
    std::snprintf(line, sizeof(line), "%-4s %d", kJudgeName[j], counts_[j]);
    text.addScreen(line, rx, ry, 10 * px, st);
  }
  // Buttons.
  const float bw = std::min(W * 0.28f, H * 0.4f), bh = std::min(H * 0.07f, 60.0f * 3), by = H * 0.88f;
  const char* labels[3] = {"RETRY", "SONGS", "LEAVE"};
  for (int i = 0; i < 3; ++i) {
    const float x = W / 2 + (static_cast<float>(i) - 1.5f) * (bw + W * 0.02f) + W * 0.01f;
    button(text, x, by, bw, bh, labels[i], px, i == 0);
  }
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
