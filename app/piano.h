#pragma once
// The grand piano: plays itself (ten classical songs, or your own .mid
// files), or you play it sitting on the bench with the on-screen keyboard
// or a real MIDI keyboard (Web MIDI, on computers and Android; iPhone
// Safari has no Web MIDI). Every note sounds from the piano's spot in the
// room (synthesized in engine/synth.cpp), lights its key and makes its
// strings glow and shake. Songs and live notes are shared in multiplayer.
#include <string>
#include <vector>

#include "app/bar_scene.h"
#include "app/renderer.h"
#include "engine/audio.h"
#include "engine/input.h"
#include "engine/midi.h"
#include "engine/net.h"
#include "engine/text.h"

namespace app {

class Piano {
 public:
  struct SongInfo {
    std::string title, url;
    bool builtIn = true;  // built-in songs are shared with other players
  };

  void init(const BarScene& scene);
  const std::vector<SongInfo>& songs() const { return songs_; }
  void addUserSong(const std::string& title, const std::string& url);

  // Starts song `index`. `serverStart` = the server time it started (to
  // join someone else's song in time), 0 = now.
  void playSong(int index, double serverStart = 0, bool fromNetwork = false);
  void stopSong(bool fromNetwork = false);
  int currentSong() const { return song_; }
  std::string nowPlaying() const;

  // Live playing (the on-screen keyboard, a MIDI keyboard, other players).
  void keyDown(int key, float velocity, bool share);
  void keyUp(int key, bool share);
  void sustain(bool down);

  // Each frame.
  void update(double nowMs, std::vector<engine::net::WorldEntry>& world, std::vector<engine::net::Event>& events);
  // Lit keys and glowing strings.
  void addBoxes(std::vector<SceneRenderer::Box>& out, double nowMs) const;
  // Where the hands are (-1 = far left .. 1 = far right), for the pianist's pose.
  void hands(float& left, float& right) const;
  bool busy() const { return song_ >= 0 || liveDown_ > 0; }

  // The on-screen keyboard while you sit on the bench (Input capture mode).
  bool keyboardOpen() const { return keyboard_; }
  void openKeyboard() { keyboard_ = true; }
  void closeKeyboard();
  // Returns true when you tapped "stand up".
  bool updateKeyboard(const std::vector<engine::Input::RawPress>& presses, float W, float H);
  void drawKeyboard(engine::TextRenderer& text, float W, float H, float px) const;
  // Tapped "songs" on the keyboard (cleared when read).
  bool takeSongsRequest() {
    const bool r = songsRequest_;
    songsRequest_ = false;
    return r;
  }

 private:
  struct Active {
    int key;
    double endTime;    // sound clock seconds (when the key lifts)
    double startTime;
    float velocity;
    int voice;
  };
  struct LoadedSong {
    bool loading = false, ready = false;
    engine::midi::Song midi;
  };

  void ensureSound(int key);
  void startScheduled(double nowSound);
  engine::Vec3 world(engine::Vec3 local) const;
  int keyAt(float x, float y, float W, float H) const;

  std::vector<SongInfo> songs_;
  std::vector<LoadedSong> loaded_;
  engine::Sound sounds_[88];
  engine::Vec3 position_;
  engine::Quat rotation_;
  float yaw_ = 0;

  int song_ = -1;
  double songStartSound_ = 0;  // sound clock time the song's second 0 plays
  double songServerStart_ = 0;
  size_t nextNote_ = 0;
  double lastUpdate_ = 0;  // sound clock at the last update
  bool waitingToLoad_ = false;
  double pendingServerStart_ = 0;

  std::vector<Active> active_;   // sounding notes (for the lights)
  int liveVoice_[88] = {};       // live notes held down: their voice (0 = up)
  double liveStart_[88] = {};
  int liveDown_ = 0;
  bool pedal_ = false;
  std::vector<int> pedalHeld_;   // live keys released while the pedal is down

  // On-screen keyboard.
  bool keyboard_ = false;
  int octave_ = 4;               // lowest C shown (C4 = middle C)
  struct Finger {
    int id, key;
  };
  std::vector<Finger> fingers_;
  bool songsRequest_ = false;
};

}  // namespace app
