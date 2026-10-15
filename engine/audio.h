#pragma once
// Sound, through the browser's Web Audio API (the small bridge lives in
// web/audio.js). Phones only allow sound after the user's first tap; the
// bridge handles that automatically.
#include <string>

#include "engine/math.h"

namespace engine {

namespace audio {
// True once sound is unlocked (after the first tap/click/key).
bool ready();
// Where your "ears" are: call every frame with the camera.
void setListener(Vec3 position, Vec3 forward, Vec3 up);
}  // namespace audio

// Long music placed in 3D space: you hear it from its position, louder when
// close, panned left/right as you turn. Streams, so songs can be long.
class MusicSource {
 public:
  // refDistance: within this many meters it plays at full volume.
  void init(float refDistance);
  void play(const std::string& url);
  void pause();
  void resume();
  bool playing() const;
  bool finished() const;  // reached the end of the song
  void setPosition(Vec3 position);

 private:
  int id_ = -1;
};

}  // namespace engine
