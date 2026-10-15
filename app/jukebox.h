#pragma once
// The jukebox: a song list (the built-in smooth-jazz tracks plus songs you
// load from your phone) played from the jukebox's spot in the room. Tap the
// jukebox to open its menu and pick a song. When a song ends, the next one
// in the list starts.
#include <string>
#include <vector>

#include "engine/audio.h"
#include "engine/math.h"
#include "engine/text.h"

namespace app {

struct Song {
  std::string title;
  std::string url;
  bool builtIn = true;  // built-in tracks need the CC BY credit shown
};

class Jukebox {
 public:
  void init(engine::Vec3 speakerPosition);
  void play(int index);
  void stop();
  // Call every frame: moves to the next song at the end of one, and picks
  // up songs the user just loaded.
  void update();

  const std::vector<Song>& songs() const { return songs_; }
  int current() const { return current_; }  // -1 = silent
  const Song* currentSong() const { return current_ >= 0 ? &songs_[static_cast<size_t>(current_)] : nullptr; }
  // Changes every time a new song starts (for the "now playing" banner).
  int songsStarted() const { return songsStarted_; }
  std::string prompt() const;  // shown when you stand at the jukebox

 private:
  engine::MusicSource source_;
  std::vector<Song> songs_;
  int userSongsSeen_ = 0;
  int current_ = -1;
  int songsStarted_ = 0;
};

// The on-screen jukebox menu: song list with jukebox-style codes (A1, A2...),
// pages when the list is long, and Load song / Stop / Close buttons.
class JukeboxMenu {
 public:
  bool isOpen() const { return open_; }
  void open();
  void close();
  // A tap while the menu is open (screen fractions 0..1).
  void tap(engine::Vec2 tap, Jukebox& jukebox);
  // Lays out and queues the menu; also keeps the native song picker over
  // the Load song button. Sizes in device pixels.
  void draw(engine::TextRenderer& text, const Jukebox& jukebox, float screenW, float screenH, float pixelRatio);

 private:
  struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
  };
  void placePicker(const Rect& r, float pixelRatio);

  bool open_ = false;
  int page_ = 0;
  // Layout from the last draw, used to hit-test taps.
  float screenW_ = 1, screenH_ = 1;
  Rect panel_, prev_, next_, load_, stop_, close_;
  std::vector<Rect> rows_;
  int firstRow_ = 0;  // song index of rows_[0]
  int pageCount_ = 1;
  Rect pickerRect_;   // where the native picker currently is (CSS pixels)
};

// Attribution required by the CC BY license (see assets/music/CREDITS.md).
constexpr const char* kMusicCredit = "Kevin MacLeod (incompetech.com)\nLicensed under CC BY 4.0";

}  // namespace app
