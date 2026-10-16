#pragma once
// The playable 4-key rhythm game on the "4K MANIA" cabinet (osu!mania /
// Quaver style, with a circle skin). Tap the cabinet to open it: pick a
// song (the jukebox's songs, including ones loaded from your phone) and a
// difficulty; the song is analyzed into a chart (app/rhythm_chart.*) and
// played with exact timing (engine::Track). Notes fall in four lanes;
// tap the lane's column (anywhere on screen) as a note reaches its ring.
// On a computer: D F J K, Escape to leave.
#include <map>
#include <string>
#include <vector>

#include "app/rhythm_chart.h"
#include "engine/input.h"
#include "engine/text.h"

namespace app {

class RhythmGame {
 public:
  struct SongChoice {
    std::string title, url;
  };

  bool isOpen() const { return state_ != State::kClosed; }
  void open(std::vector<SongChoice> songs);
  void close();
  // Starts a song directly (tests: ?rhythm=song,difficulty).
  void start(int song, Difficulty difficulty) {
    song_ = song;
    difficulty_ = difficulty;
    startSong();
  }
  // Each frame while open: handles the presses (Input capture mode).
  // W, H = screen size in device pixels, px = device pixels per CSS pixel.
  void update(double nowMs, const std::vector<engine::Input::RawPress>& presses, float W, float H);
  void draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;

  bool autoplay = false;  // the game plays itself (for tests: ?autoplay)

 private:
  enum class State { kClosed, kMenu, kLoading, kAnalyzing, kWaitTap, kPlaying, kResults };
  enum Judge { kPerfect, kGreat, kGood, kMiss, kJudgeCount };
  struct Note {
    double time;
    int lane;
    bool judged = false;
  };
  struct Burst {
    int lane;
    Judge judge;
    double ms;
  };

  void startSong();
  void judge(Note& n, Judge j, double nowMs);
  void finish();
  void menuPress(engine::Vec2 p, float W, float H);
  void resultsPress(engine::Vec2 p, float W, float H);
  // Playfield layout in device pixels.
  float laneX(int lane, float W, float H) const;
  float laneWidth(float W, float H) const;

  State state_ = State::kClosed;
  std::vector<SongChoice> songs_;
  int song_ = 0;
  Difficulty difficulty_ = Difficulty::kNormal;
  bool analyzeShown_ = false;  // the "making the chart" text was drawn once
  std::vector<Note> notes_;
  size_t firstOpen_ = 0;       // first note that may still be unjudged
  double songTime_ = -1000;
  double duration_ = 0;
  int counts_[kJudgeCount] = {};
  int combo_ = 0, maxCombo_ = 0;
  double points_ = 0;          // sum of judgement values
  int lanePressed_[4] = {};    // fingers held per lane
  std::map<int, int> fingerLane_;
  std::vector<Burst> bursts_;
  Judge lastJudge_ = kMiss;
  double lastJudgeMs_ = -1e9;
  double finishMs_ = 0;
  std::map<std::string, double> best_;  // best accuracy per song + difficulty
};

}  // namespace app
