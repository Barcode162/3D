#pragma once
// Sound, through the browser's Web Audio API (the small bridge lives in
// web/audio.js). Phones only allow sound after the user's first tap; the
// bridge handles that automatically.
#include <string>
#include <vector>

#include "engine/math.h"

namespace engine {

namespace audio {
// True once sound is unlocked (after the first tap/click/key).
bool ready();
// Where your "ears" are: call every frame with the camera.
void setListener(Vec3 position, Vec3 forward, Vec3 up);

// Songs the user picked from their phone (Files app). Local only.
// Places the invisible native "pick a file" button over a drawn button
// (CSS pixels); pass w = 0 to hide it.
void setSongPickerRect(float x, float y, float w, float h);
int userSongCount();
std::string userSongName(int index);
std::string userSongUrl(int index);

// Turns all music sources (the jukebox) up or down, 0..1.
void setMusicGain(float gain);
}  // namespace audio

// A whole song decoded into memory, for when timing must be exact (the
// rhythm game): it plays with sample accuracy and reports where it is.
// One at a time; loading a new one replaces the old.
class Track {
 public:
  enum class State { kNone, kLoading, kReady, kFailed };
  static void load(const std::string& url);
  static State state();
  // The song as mono samples (call once when ready; the copy is freed in
  // the browser afterwards). `rate` = samples per second.
  static std::vector<float> takeSamples(float& rate);
  static bool play(double delaySeconds);  // false if sound isn't unlocked yet
  static double time();       // seconds into the song as heard (< 0 before it starts)
  static double duration();
  static void stop();
};

// Long music placed in 3D space: you hear it from its position, louder when
// close, panned left/right as you turn. Streams, so songs can be long.
class MusicSource {
 public:
  // refDistance: within this many meters it plays at full volume.
  void init(float refDistance);
  // Starts `url`, `offsetSeconds` into it.
  void play(const std::string& url, double offsetSeconds = 0);
  void pause();
  void resume();
  bool playing() const;
  bool finished() const;  // reached the end of the song
  void setPosition(Vec3 position);

 private:
  int id_ = -1;
};

}  // namespace engine
