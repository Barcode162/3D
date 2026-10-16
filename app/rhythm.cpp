#include "app/rhythm.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/audio.h"
#include "engine/font.h"
#include "engine/log.h"

using engine::Vec2;
using engine::Vec4;

namespace app {
namespace {

// Timing windows (seconds either side of the note) and what each is worth.
constexpr double kWindow[3] = {0.045, 0.09, 0.135};  // perfect, great, good
constexpr double kMissWindow = 0.18;                  // pressing earlier than this does nothing
constexpr double kValue[4] = {300, 200, 100, 0};
constexpr double kLeadIn = 2.5;                       // seconds before the song starts
constexpr const char* kJudgeName[4] = {"PERFECT", "GREAT", "GOOD", "MISS"};
const Vec4 kJudgeColor[4] = {{0.55f, 0.95f, 1.0f, 1}, {0.5f, 1.0f, 0.55f, 1}, {1.0f, 0.85f, 0.35f, 1},
                             {1.0f, 0.35f, 0.35f, 1}};
// Lane colors (outer lanes pale blue, inner lanes pink), like a classic 4K skin.
const Vec4 kLaneColor[4] = {{0.75f, 0.88f, 1.0f, 1}, {1.0f, 0.42f, 0.78f, 1}, {1.0f, 0.42f, 0.78f, 1},
                            {0.75f, 0.88f, 1.0f, 1}};
constexpr const char* kDifficultyName[3] = {"EASY", "NORMAL", "HARD"};
constexpr double kApproach[3] = {1.15, 0.95, 0.8};  // seconds a note is on screen

struct Rect {
  float x, y, w, h;
  bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

Vec4 withAlpha(Vec4 c, float a) { return {c.x, c.y, c.z, c.w * a}; }

// Menu layout (device pixels).
Rect songRow(int i, float W, float H) { return {W * 0.07f, H * (0.17f + 0.08f * static_cast<float>(i)), W * 0.86f, H * 0.068f}; }
Rect difficultyButton(int i, float W, float H) {
  return {W * (0.07f + 0.29f * static_cast<float>(i)), H * 0.75f, W * 0.27f, H * 0.06f};
}
Rect playButton(float W, float H) { return {W * 0.2f, H * 0.835f, W * 0.6f, H * 0.065f}; }
Rect closeButton(float W, float H) { return {W * 0.3f, H * 0.915f, W * 0.4f, H * 0.05f}; }
Rect resultButton(int i, float W, float H) { return {W * 0.2f, H * (0.7f + 0.085f * static_cast<float>(i)), W * 0.6f, H * 0.065f}; }
Rect quitButton(float W, float H) { return {0, H * 0.03f, W * 0.2f, H * 0.08f}; }
constexpr int kMaxSongs = 7;

void button(engine::TextRenderer& text, const Rect& r, const char* label, float px, bool active,
            Vec4 color = {1.0f, 0.42f, 0.78f, 1}) {
  text.addRect(r.x, r.y, r.w, r.h, active ? withAlpha(color, 0.85f) : Vec4{0.12f, 0.1f, 0.16f, 0.9f});
  text.addRect(r.x, r.y + r.h - 2 * px, r.w, 2 * px, withAlpha(color, active ? 1.0f : 0.5f));
  engine::TextStyle s;
  s.align = 0.5f;
  s.color = active ? Vec4{0.05f, 0.03f, 0.08f, 1} : Vec4{1, 1, 1, 1};
  s.outline.w = active ? 0 : 0.6f;
  const float cap = std::min(r.h * 0.38f, 13 * px);
  text.addScreen(label, r.x + r.w / 2, r.y + (r.h - cap) / 2, cap, s);
}

}  // namespace

void RhythmGame::open(std::vector<SongChoice> songs) {
  songs_ = std::move(songs);
  if (songs_.size() > static_cast<size_t>(kMaxSongs)) songs_.resize(kMaxSongs);
  song_ = std::clamp(song_, 0, std::max(0, static_cast<int>(songs_.size()) - 1));
  state_ = State::kMenu;
  engine::audio::setMusicGain(0.15f);  // turn the jukebox down while you play
  LOG_INFO("Rhythm game opened (%zu songs).", songs_.size());
}

void RhythmGame::close() {
  engine::Track::stop();
  engine::audio::setMusicGain(1.0f);
  state_ = State::kClosed;
}

float RhythmGame::laneWidth(float W, float H) const { return std::min(W * 0.92f, H * 0.56f) / 4; }

float RhythmGame::laneX(int lane, float W, float H) const {
  const float lw = laneWidth(W, H);
  return (W - 4 * lw) / 2 + (static_cast<float>(lane) + 0.5f) * lw;
}

void RhythmGame::startSong() {
  if (songs_.empty()) return;
  engine::Track::load(songs_[static_cast<size_t>(song_)].url);
  state_ = State::kLoading;
  analyzeShown_ = false;
}

void RhythmGame::judge(Note& n, Judge j, double nowMs) {
  n.judged = true;
  ++counts_[j];
  points_ += kValue[j];
  if (j == kMiss) {
    combo_ = 0;
  } else {
    maxCombo_ = std::max(maxCombo_, ++combo_);
    bursts_.push_back({n.lane, j, nowMs});
  }
  lastJudge_ = j;
  lastJudgeMs_ = nowMs;
}

void RhythmGame::finish() {
  engine::Track::stop();
  const int judged = counts_[0] + counts_[1] + counts_[2] + counts_[3];
  const double acc = judged ? points_ / (300.0 * judged) : 0;
  const std::string key = songs_[static_cast<size_t>(song_)].title + kDifficultyName[static_cast<int>(difficulty_)];
  best_[key] = std::max(best_[key], acc);
  LOG_INFO("Rhythm game result: %.2f%% (%d perfect, %d great, %d good, %d miss), max combo %d.", acc * 100,
           counts_[0], counts_[1], counts_[2], counts_[3], maxCombo_);
  state_ = State::kResults;
}

void RhythmGame::menuPress(Vec2 p, float W, float H) {
  for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
    if (songRow(i, W, H).contains(p.x, p.y)) song_ = i;
  }
  for (int i = 0; i < 3; ++i) {
    if (difficultyButton(i, W, H).contains(p.x, p.y)) difficulty_ = static_cast<Difficulty>(i);
  }
  if (playButton(W, H).contains(p.x, p.y)) startSong();
  if (closeButton(W, H).contains(p.x, p.y)) close();
}

void RhythmGame::resultsPress(Vec2 p, float W, float H) {
  if (resultButton(0, W, H).contains(p.x, p.y)) startSong();
  if (resultButton(1, W, H).contains(p.x, p.y)) state_ = State::kMenu;
  if (resultButton(2, W, H).contains(p.x, p.y)) close();
}

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

  if (state_ == State::kMenu || state_ == State::kResults) {
    for (const engine::Input::RawPress& r : presses) {
      if (!r.down) continue;
      const Vec2 p{r.pos.x * W, r.pos.y * H};
      if (r.key >= 0 && r.key < 4 && state_ == State::kMenu) {  // keys: D/F pick a song, J/K difficulty
        if (r.key == 0) song_ = (song_ + static_cast<int>(songs_.size()) - 1) % std::max<int>(1, static_cast<int>(songs_.size()));
        if (r.key == 1) song_ = (song_ + 1) % std::max<int>(1, static_cast<int>(songs_.size()));
        if (r.key == 2) difficulty_ = static_cast<Difficulty>((static_cast<int>(difficulty_) + 1) % 3);
        if (r.key == 3) startSong();
        continue;
      }
      if (state_ == State::kMenu) menuPress(p, W, H);
      else resultsPress(p, W, H);
      if (state_ == State::kClosed) return;
    }
    return;
  }

  if (state_ == State::kLoading) {
    const engine::Track::State s = engine::Track::state();
    if (s == engine::Track::State::kFailed) state_ = State::kMenu;
    if (s == engine::Track::State::kReady) state_ = State::kAnalyzing;
    return;
  }
  if (state_ == State::kAnalyzing) {
    if (!analyzeShown_) {  // let "making the chart" show for a frame first
      analyzeShown_ = true;
      return;
    }
    float rate = 44100;
    std::vector<float> samples = engine::Track::takeSamples(rate);
    const double t0 = emscripten_get_now();
    const Chart chart = makeChart(samples.data(), samples.size(), rate, difficulty_);
    LOG_INFO("Chart made in %.0f ms.", emscripten_get_now() - t0);
    notes_.clear();
    for (const ChartNote& n : chart.notes) notes_.push_back({n.time, n.lane});
    duration_ = chart.length;
    firstOpen_ = 0;
    std::fill(std::begin(counts_), std::end(counts_), 0);
    combo_ = maxCombo_ = 0;
    points_ = 0;
    bursts_.clear();
    lastJudgeMs_ = -1e9;
    std::fill(std::begin(lanePressed_), std::end(lanePressed_), 0);
    fingerLane_.clear();
    state_ = engine::Track::play(kLeadIn) ? State::kPlaying : State::kWaitTap;
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

  // Playing.
  songTime_ = engine::Track::time();
  const float lw = laneWidth(W, H);
  const float x0 = (W - 4 * lw) / 2;
  for (const engine::Input::RawPress& r : presses) {
    const Vec2 p{r.pos.x * W, r.pos.y * H};
    if (r.down && r.key < 0 && quitButton(W, H).contains(p.x, p.y)) {
      engine::Track::stop();
      state_ = State::kMenu;
      return;
    }
    int lane = r.key >= 0 && r.key < 4 ? r.key : std::clamp(static_cast<int>(std::floor((p.x - x0) / lw)), 0, 3);
    if (!r.down) {
      const auto it = fingerLane_.find(r.id);
      if (it != fingerLane_.end()) {
        lanePressed_[it->second] = std::max(0, lanePressed_[it->second] - 1);
        fingerLane_.erase(it);
      }
      continue;
    }
    fingerLane_[r.id] = lane;
    ++lanePressed_[lane];
    if (autoplay) continue;
    // When the finger came down, in song time.
    const double t = songTime_ - (nowMs - r.ms) / 1000.0;
    Note* best = nullptr;
    for (size_t i = firstOpen_; i < notes_.size(); ++i) {
      Note& n = notes_[i];
      if (n.time - t > kMissWindow) break;
      if (n.judged || n.lane != lane || t - n.time > kMissWindow) continue;
      best = &n;
      break;
    }
    if (!best) continue;
    const double err = std::fabs(best->time - t);
    judge(*best, err <= kWindow[0] ? kPerfect : err <= kWindow[1] ? kGreat : err <= kWindow[2] ? kGood : kMiss, nowMs);
  }
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    Note& n = notes_[i];
    if (n.time > songTime_ + 0.05) break;
    if (n.judged) continue;
    if (autoplay && n.time <= songTime_) {
      judge(n, kPerfect, nowMs);
    } else if (songTime_ - n.time > kMissWindow) {
      judge(n, kMiss, nowMs);
    }
  }
  while (firstOpen_ < notes_.size() && notes_[firstOpen_].judged) ++firstOpen_;
  bursts_.erase(std::remove_if(bursts_.begin(), bursts_.end(), [nowMs](const Burst& b) { return nowMs - b.ms > 300; }),
                bursts_.end());
  const double lastNote = notes_.empty() ? 0 : notes_.back().time;
  if (songTime_ > std::max(lastNote + 2.0, std::min(duration_, lastNote + 6.0))) finish();
}

void RhythmGame::draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  if (state_ == State::kClosed) return;
  engine::TextStyle center;
  center.align = 0.5f;
  center.outlineWidth = 0.7f;
  // Dim the bar behind the game.
  text.addRect(0, 0, W, H, {0.02f, 0.01f, 0.04f, state_ == State::kPlaying ? 0.8f : 0.88f});

  if (state_ == State::kMenu) {
    engine::TextStyle title = center;
    title.color = {1.0f, 0.42f, 0.78f, 1};
    text.addScreen("4K MANIA", W / 2, H * 0.05f, 26 * px, title);
    engine::TextStyle sub = center;
    sub.color = {0.8f, 0.8f, 0.9f, 1};
    text.addScreen("Pick a song", W / 2, H * 0.115f, 10 * px, sub);
    for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
      const Rect r = songRow(i, W, H);
      const bool sel = i == song_;
      text.addRect(r.x, r.y, r.w, r.h, sel ? Vec4{0.95f, 0.4f, 0.75f, 0.9f} : Vec4{0.12f, 0.1f, 0.16f, 0.9f});
      text.addShape(engine::font::kShapeDisc, r.x + r.h * 0.5f, r.y + r.h * 0.5f, r.h * 0.22f,
                    sel ? Vec4{1, 1, 1, 1} : kLaneColor[i % 4]);
      engine::TextStyle s;
      s.color = sel ? Vec4{0.05f, 0.03f, 0.08f, 1} : Vec4{1, 1, 1, 1};
      s.outline.w = sel ? 0 : 0.6f;
      const std::string& t = songs_[static_cast<size_t>(i)].title;
      text.addScreen(t.size() > 24 ? t.substr(0, 23) + "." : t, r.x + r.h, r.y + r.h * 0.32f, 11 * px, s);
      const auto it = best_.find(t + kDifficultyName[static_cast<int>(difficulty_)]);
      if (it != best_.end()) {
        char b[32];
        std::snprintf(b, sizeof(b), "%.1f%%", it->second * 100);
        engine::TextStyle bs = s;
        bs.align = 1;
        text.addScreen(b, r.x + r.w - 8 * px, r.y + r.h * 0.36f, 8 * px, bs);
      }
    }
    if (songs_.empty()) text.addScreen("No songs", W / 2, H * 0.3f, 12 * px, center);
    for (int i = 0; i < 3; ++i) {
      button(text, difficultyButton(i, W, H), kDifficultyName[i], px, static_cast<int>(difficulty_) == i,
             i == 0 ? Vec4{0.4f, 0.9f, 0.5f, 1} : i == 1 ? Vec4{0.5f, 0.75f, 1.0f, 1} : Vec4{1.0f, 0.45f, 0.4f, 1});
    }
    button(text, playButton(W, H), "PLAY", px, true);
    button(text, closeButton(W, H), "LEAVE", px, false);
    return;
  }

  if (state_ == State::kLoading || state_ == State::kAnalyzing || state_ == State::kWaitTap) {
    const char* msg = state_ == State::kLoading ? "Listening to the song..." : state_ == State::kAnalyzing ?
                      "Making the chart..." : "Tap to start";
    text.addScreen(msg, W / 2, H * 0.45f, 13 * px, center);
    for (int k = 0; k < 8; ++k) {  // spinner
      const float a = static_cast<float>(nowMs / 160.0) + static_cast<float>(k) * 0.785f;
      text.addShape(engine::font::kShapeDisc, W / 2 + std::cos(a) * 26 * px, H * 0.38f + std::sin(a) * 26 * px,
                    (2.0f + static_cast<float>(k) * 0.5f) * px, withAlpha(kLaneColor[k % 4], 0.3f + 0.08f * static_cast<float>(k)));
    }
    return;
  }

  const int judged = counts_[0] + counts_[1] + counts_[2] + counts_[3];
  const double acc = judged ? points_ / (300.0 * judged) : 1.0;
  if (state_ == State::kResults) {
    const char* grade = acc >= 0.999 ? "SS" : acc >= 0.95 ? "S" : acc >= 0.9 ? "A" : acc >= 0.8 ? "B" : acc >= 0.7 ? "C" : "D";
    engine::TextStyle g = center;
    g.color = acc >= 0.95 ? Vec4{1.0f, 0.85f, 0.3f, 1} : acc >= 0.8 ? Vec4{0.5f, 1.0f, 0.6f, 1} : Vec4{1.0f, 0.45f, 0.45f, 1};
    text.addScreen(songs_[static_cast<size_t>(song_)].title, W / 2, H * 0.08f, 12 * px, center);
    text.addScreen(grade, W / 2, H * 0.15f, 60 * px, g);
    char line[96];
    const int total = static_cast<int>(notes_.size());
    std::snprintf(line, sizeof(line), "%07d", total ? static_cast<int>(std::lround(1e6 * points_ / (300.0 * total))) : 0);
    text.addScreen(line, W / 2, H * 0.32f, 18 * px, center);
    std::snprintf(line, sizeof(line), "%.2f%%   MAX COMBO %d", acc * 100, maxCombo_);
    text.addScreen(line, W / 2, H * 0.39f, 10 * px, center);
    for (int j = 0; j < 4; ++j) {
      engine::TextStyle s = center;
      s.color = kJudgeColor[j];
      std::snprintf(line, sizeof(line), "%s  %d", kJudgeName[j], counts_[j]);
      text.addScreen(line, W / 2, H * (0.46f + 0.05f * static_cast<float>(j)), 11 * px, s);
    }
    button(text, resultButton(0, W, H), "RETRY", px, true);
    button(text, resultButton(1, W, H), "SONGS", px, false);
    button(text, resultButton(2, W, H), "LEAVE", px, false);
    return;
  }

  // ---- Playing: circle skin ----
  const float lw = laneWidth(W, H), R = lw * 0.36f;
  const float judgeY = H * 0.82f, topY = H * 0.06f;
  const double approach = kApproach[static_cast<int>(difficulty_)];
  for (int l = 0; l < 4; ++l) {  // lane backgrounds, brighter while held
    const float x = laneX(l, W, H) - lw / 2;
    text.addRect(x + px, 0, lw - 2 * px, H, lanePressed_[l] ? withAlpha(kLaneColor[l], 0.16f) : Vec4{0.06f, 0.05f, 0.09f, 0.55f});
  }
  for (int l = 0; l <= 4; ++l) {  // thin lane dividers
    text.addRect(laneX(0, W, H) - lw / 2 + static_cast<float>(l) * lw - px, 0, 2 * px, H, {1, 1, 1, 0.08f});
  }
  for (int l = 0; l < 4; ++l) {  // receptors: rings that fill in when pressed
    const float x = laneX(l, W, H);
    if (lanePressed_[l]) text.addShape(engine::font::kShapeDisc, x, judgeY, R * 0.9f, withAlpha(kLaneColor[l], 0.35f));
    text.addShape(engine::font::kShapeRing, x, judgeY, lanePressed_[l] ? R * 0.93f : R, withAlpha(kLaneColor[l], 0.9f));
  }
  // Notes: discs with a white rim, fading in at the top.
  for (size_t i = firstOpen_; i < notes_.size(); ++i) {
    const Note& n = notes_[i];
    if (n.judged) continue;
    const double dt = n.time - songTime_;
    if (dt > approach * 1.05) break;
    const float y = judgeY - static_cast<float>(dt / approach) * (judgeY - topY);
    if (y > H + R) continue;
    const float fade = std::clamp((y - topY) / (H * 0.06f) + 0.3f, 0.0f, 1.0f);
    text.addShape(engine::font::kShapeDisc, laneX(n.lane, W, H), y, R * 0.92f, withAlpha(kLaneColor[n.lane], fade),
                  {1, 1, 1, 0.9f * fade}, 0.45f);
  }
  // Hit bursts: a ring growing out of the receptor.
  for (const Burst& b : bursts_) {
    const float t = static_cast<float>((nowMs - b.ms) / 300.0);
    text.addShape(engine::font::kShapeRing, laneX(b.lane, W, H), judgeY, R * (1.0f + 0.7f * t),
                  withAlpha(kJudgeColor[b.judge], 1.0f - t));
  }
  // Judgement, combo, score, accuracy, progress, quit button.
  const double sinceJudge = nowMs - lastJudgeMs_;
  if (sinceJudge < 450) {
    engine::TextStyle s = center;
    s.color = withAlpha(kJudgeColor[lastJudge_], static_cast<float>(1.0 - sinceJudge / 450.0));
    s.outline.w *= s.color.w;
    const float pop = 1.0f + 0.25f * static_cast<float>(std::max(0.0, 1.0 - sinceJudge / 90.0));
    text.addScreen(kJudgeName[lastJudge_], W / 2, H * 0.56f, 15 * px * pop, s);
  }
  if (combo_ >= 3) {
    engine::TextStyle s = center;
    s.color = {1, 1, 1, 0.85f};
    char c[16];
    std::snprintf(c, sizeof(c), "%d", combo_);
    text.addScreen(c, W / 2, H * 0.44f, 30 * px, s);
  }
  char line[64];
  const int total = std::max(1, static_cast<int>(notes_.size()));
  std::snprintf(line, sizeof(line), "%07d", static_cast<int>(std::lround(1e6 * points_ / (300.0 * total))));
  engine::TextStyle right;
  right.align = 1;
  right.outlineWidth = 0.7f;
  text.addScreen(line, W - 10 * px, H * 0.045f + 6 * px, 13 * px, right);
  std::snprintf(line, sizeof(line), "%.2f%%", acc * 100);
  text.addScreen(line, W - 10 * px, H * 0.045f + 26 * px, 9 * px, right);
  const float progress = duration_ > 0 ? std::clamp(static_cast<float>(songTime_ / duration_), 0.0f, 1.0f) : 0;
  text.addRect(0, 0, W * progress, 3 * px, {1.0f, 0.42f, 0.78f, 0.9f});
  engine::TextStyle left;
  left.outlineWidth = 0.7f;
  text.addRect(10 * px, H * 0.045f, 30 * px, 30 * px, {0.15f, 0.12f, 0.2f, 0.8f});
  text.addScreen("X", 25 * px - 7 * px * 5 / 7, H * 0.045f + 8 * px, 14 * px, left);
  if (songTime_ < 0) {
    std::snprintf(line, sizeof(line), "%d", static_cast<int>(std::ceil(-songTime_)));
    text.addScreen(songTime_ < -kLeadIn + 0.4 ? "READY" : line, W / 2, H * 0.4f, 28 * px, center);
  }
}

void RhythmGame::screenState(float out[5]) const {
  int judged = 0;
  for (int c : counts_) judged += c;
  out[0] = static_cast<float>(combo_);
  out[1] = judged ? static_cast<float>(points_ / (300.0 * judged)) : 1.0f;
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
    quad(u0, 0.05f, u0 + 0.19f, 0.95f, {0.06f, 0.04f, 0.1f, 1});
    // Notes falling (a pattern from the song time, not the real chart).
    if (playing) {
      for (int k = 0; k < 4; ++k) {
        const float phase = std::fmod(state[4] * 1.3f + static_cast<float>(k) * 0.25f + static_cast<float>(l) * 0.37f, 1.0f);
        if ((k + l) % 2 == 0) quad(u0 + 0.03f, phase * 0.8f, u0 + 0.16f, phase * 0.8f + 0.04f, kLaneColor[l]);
      }
    }
    const bool held = (mask >> l) & 1;
    quad(u0 + 0.01f, 0.84f, u0 + 0.18f, 0.9f, held ? kLaneColor[l] : withAlpha(kLaneColor[l], 0.35f));
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
