#include "app/jukebox.h"

#include <iterator>

#include "engine/log.h"

namespace app {
namespace {

constexpr Track kPlaylist[] = {
    {"smooth-lovin.m4a", "Smooth Lovin"},
    {"bossa-antigua.m4a", "Bossa Antigua"},
    {"night-on-the-docks-sax.m4a", "Night on the Docks - Sax"},
    {"lobby-time.m4a", "Lobby Time"},
};
constexpr int kTrackCount = static_cast<int>(std::size(kPlaylist));

}  // namespace

void Jukebox::init(engine::Vec3 speakerPosition) {
  source_.init(2.0f);  // full volume within 2 m, then fades with distance
  source_.setPosition(speakerPosition);
}

void Jukebox::start(int index) {
  current_ = index % kTrackCount;
  ++songsStarted_;
  source_.play(std::string("assets/music/") + kPlaylist[current_].file);
  LOG_INFO("Jukebox: playing \"%s\".", kPlaylist[current_].title);
}

// Tapping steps through the playlist, then turns the music off.
void Jukebox::tap() {
  if (current_ == kTrackCount - 1) {
    source_.pause();
    current_ = -1;
    LOG_INFO("Jukebox: off.");
    return;
  }
  start(current_ + 1);
}

void Jukebox::update() {
  if (current_ >= 0 && source_.finished()) start(current_ + 1);
}

const Track* Jukebox::currentTrack() const { return current_ >= 0 ? &kPlaylist[current_] : nullptr; }

std::string Jukebox::prompt() const {
  if (current_ < 0) return "Jukebox\nTap to play music";
  return std::string("Now playing: ") + kPlaylist[current_].title +
         (current_ == kTrackCount - 1 ? "\nTap to turn the music off" : "\nTap for the next song");
}

}  // namespace app
