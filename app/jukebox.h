#pragma once
// The jukebox: a playlist of smooth-jazz tracks played from the jukebox's
// spot in the room. Tap it to start; tap again for the next song; after the
// last song a tap turns it off. Songs move on by themselves when one ends.
#include <string>

#include "engine/audio.h"
#include "engine/math.h"

namespace app {

struct Track {
  const char* file;   // under assets/music/
  const char* title;
};

class Jukebox {
 public:
  void init(engine::Vec3 speakerPosition);
  void tap();
  void update();

  bool playing() const { return current_ >= 0; }
  const Track* currentTrack() const;
  // Prompt shown when you stand at the jukebox.
  std::string prompt() const;
  // Changes every time a new song starts (for the "now playing" banner).
  int songCount() const { return songsStarted_; }

 private:
  void start(int index);

  engine::MusicSource source_;
  int current_ = -1;  // -1 = silent
  int songsStarted_ = 0;
};

// Attribution required by the CC BY license (see assets/music/CREDITS.md).
constexpr const char* kMusicCredit = "Kevin MacLeod (incompetech.com)\nLicensed under CC BY 4.0";

}  // namespace app
