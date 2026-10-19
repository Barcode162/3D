#pragma once
// The playable 4-key rhythm game on the "4K MANIA" cabinet, made to look and
// play like osu!mania: a song select with a scrolling song wheel, four
// difficulties with star ratings, your best grade per song; then bar notes
// and hold notes falling to lit keys, MAX/300/200/100/50/MISS judgements,
// a timing-error bar, a health bar and an osu!-style results screen. Songs:
// its own (assets/music/rhythm/, see CREDITS.md) plus the jukebox's
// (including ones loaded from your phone). Each song is analyzed into a
// chart (app/rhythm_chart.*) and played with exact timing (engine::Track).
// Tap a column as a note reaches the keys (sideways, the screen is split
// into four tall zones, one per finger); hold a hold note until its end.
// On a computer: D F J K, Escape to leave. Skins change the notes (bars,
// circles, bubbles, neon rings, arrows) and the menus' colours; the song
// select plays the picked song's preview.
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
    std::string title, artist, url;
    float seconds = 0;  // length (0 = not known)
  };
  // The game's own songs (assets/music/rhythm/).
  static std::vector<SongChoice> builtInSongs();

  bool isOpen() const { return state_ != State::kClosed; }
  // Opens the song select with its own songs plus `extra` (the jukebox's).
  void open(std::vector<SongChoice> extra);
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

  // What the cabinet's screen shows others: [combo, accuracy 0..1, lanes
  // held (bits), playing (1/0), song time].
  void screenState(float out[5]) const;
  // Draws a cabinet screen from such a state, on the screen's corners
  // (bottom-left, bottom-right, top-right, top-left) in the world.
  static void drawCabinet(engine::TextRenderer& text, const engine::Vec3 corners[4], const float state[5], double nowMs);

 private:
  enum class State { kClosed, kMenu, kLoading, kAnalyzing, kWaitTap, kPlaying, kResults };
  enum Judge { kMax, k300, k200, k100, k50, kMiss, kJudgeCount };
  struct Note {
    double time, end;  // end > 0: a hold note
    int lane;
    bool judged = false;      // the head (or the tap)
    bool holding = false;     // a hold note being held now
    bool tailDone = false;    // a hold note's end judged
    Judge headJudge = kMiss;
  };
  struct Burst {
    int lane;
    Judge judge;
    double ms;
  };
  struct Best {
    double acc = 0;
    int score = 0;
  };
  struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
  };
  // The song select's layout (device pixels), portrait or landscape.
  struct MenuLayout {
    Rect info, diff[kDifficultyCount], speedDown, speedUp, play, back, wheel, autoBtn, skin;
    float card = 0;  // a wheel card's height
  };

  void startSong();
  void beginPlay(const Chart& chart);
  void judge(Note& n, Judge j, double nowMs, double errorMs, bool tail = false);
  void finish();
  void menuPress(engine::Vec2 p, float W, float H);
  void menuSwipe(float dy, float W, float H);
  void resultsPress(engine::Vec2 p, float W, float H);
  void pickSong(int index);
  MenuLayout menuLayout(float W, float H) const;
  void drawMenu(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;
  void drawPlay(engine::TextRenderer& text, float W, float H, float px, double nowMs) const;
  void drawResults(engine::TextRenderer& text, float W, float H, float px) const;
  // Playfield layout in device pixels.
  float columnWidth(float W, float H) const;
  float laneX(int lane, float W, float H) const;  // the column's middle
  double accuracy() const;
  int score() const;
  std::string bestKey(int song, Difficulty d) const;
  float stars(int song, Difficulty d, bool* known = nullptr) const;
  double approachSeconds() const { return 13.72 / scrollSpeed_; }

  struct Particle {
    float x, y, vx, vy, age, life, size, gravity;
    engine::Vec4 color;
  };
  struct TouchFx {
    float x, y;
    double ms;
    int lane;
  };
  void spawn(float x, float y, float vx, float vy, float life, float size, engine::Vec4 color, float gravity = 0);
  float rand01();  // 0..1
  void updateEffects(double nowMs, float W, float H);
  void burst(int lane, Judge j, float W, float H, double nowMs);

  State state_ = State::kClosed;
  int skin_ = 2;               // see kSkins in rhythm.cpp (kept on the phone: rhSkin)
  std::vector<Particle> particles_;
  std::vector<TouchFx> touchFx_;
  double effectsMs_ = 0;
  float spawnDebt_ = 0, bubbleDebt_ = 0;
  uint32_t rng_ = 12345;
  int previewSong_ = -1, lastPicked_ = -1;  // the song select's music
  double pickedMs_ = 0;
  int comboMilestone_ = 0;
  std::vector<SongChoice> songs_;
  int song_ = 0;
  Difficulty difficulty_ = Difficulty::kNormal;
  int scrollSpeed_ = 18;      // like osu!mania's scroll speed (higher = faster notes)
  float wheel_ = 0;           // the song wheel's scroll position (in cards), eases to song_
  double wheelMs_ = 0;
  bool analyzeShown_ = false;  // the "making the chart" text was drawn once
  std::vector<Note> notes_;
  size_t firstOpen_ = 0;       // first note that may still be unjudged
  double songTime_ = -1000;
  double duration_ = 0;
  double bpm_ = 0;
  int counts_[kJudgeCount] = {};
  int combo_ = 0, maxCombo_ = 0;
  int judgements_ = 0, totalJudgements_ = 0;
  double points_ = 0;          // sum of accuracy values (300 for MAX/300...)
  double scorePoints_ = 0;     // sum of score values (320 for MAX)
  float health_ = 1;
  int lanePressed_[4] = {};    // fingers held per lane
  double laneDownMs_[4] = {};  // when each lane was last pressed (for the light)
  std::map<int, int> fingerLane_;
  std::map<int, engine::Vec2> fingerDown_;  // menu: where each finger came down (swipes)
  std::vector<Burst> bursts_;
  struct Tick {
    double errorMs, ms;
    Judge judge;
  };
  std::vector<Tick> ticks_;    // the timing-error bar's recent hits
  Judge lastJudge_ = kMiss;
  double lastJudgeMs_ = -1e9;
  double finishMs_ = 0;
  bool newBest_ = false;
  std::map<std::string, Best> best_;            // per song + difficulty (also kept on the phone)
  std::map<std::string, Chart> charts_;         // made this session (url + difficulty)
  std::map<std::string, float> starCache_;      // star rating per song + difficulty, once charted
};

}  // namespace app
