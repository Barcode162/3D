#pragma once
// The grand piano: plays itself (ten classical songs, or your own .mid
// files), or you play it sitting on the bench with the on-screen keyboard
// or a real MIDI keyboard (Web MIDI, on computers and Android; iPhone
// Safari has no Web MIDI). Every note sounds from the piano's spot in the
// room (synthesized in engine/synth.cpp), lights its key and makes its
// strings glow and shake. Songs and live notes are shared in multiplayer.
#include <map>
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
  // Each player has their own held keys and pedal; kYou = this phone.
  static constexpr int kYou = -1;
  void keyDown(int key, float velocity, bool share, int player = kYou);
  void keyUp(int key, bool share, int player = kYou);
  // The sustain pedal (on-screen button or a MIDI keyboard's pedal).
  void setPedal(bool down, bool share, int player = kYou);
  // Starts making the note samples in background jobs (call once you've
  // entered; a key played before its sample is ready makes it right away).
  void prepareSamples();

  // Each frame.
  void update(double nowMs, std::vector<engine::net::WorldEntry>& world, std::vector<engine::net::Event>& events);
  // Lit keys and glowing strings.
  void addBoxes(std::vector<SceneRenderer::Box>& out, double nowMs) const;
  // Where the hands are (-1 = far left .. 1 = far right), for the pianist's pose.
  void hands(float& left, float& right) const;

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

  // Samples: one for every 3rd key; the two keys around it play it a
  // semitone up or down (playbackRate).
  static constexpr int kSamples = 30;
  static int sampleOf(int i) { return i / 3; }               // i = key index 0..87
  static int sampleKey(int s) { return 21 + 3 * s + 1; }     // the key a sample was made at
  static float rateFor(int i);
  bool sampleReady(int i) const { return samples_[sampleOf(i)].valid(); }
  void ensureSample(int i);
  int playKey(int i, float gain, double when);
  // One player's hands on the keyboard.
  struct Hands {
    int voice[88] = {};       // held (or pedal-held) keys: their voice (0 = up)
    double start[88] = {};
    bool pedal = false;
    std::vector<int> pedalHeld;  // keys released while the pedal is down
    double lastMs = 0;           // last time they did anything (silent 10 s = released)
  };
  void releaseAll(Hands& h);
  bool keyLit(int i, double& since) const;
  void startScheduled(double nowSound);
  engine::Vec3 world(engine::Vec3 local) const;
  int keyAt(float x, float y, float W, float H) const;

  std::vector<SongInfo> songs_;
  std::vector<LoadedSong> loaded_;
  engine::Sound samples_[kSamples];
  bool preparing_ = false;
  engine::Vec3 position_;
  engine::Quat rotation_;
  float yaw_ = 0;

  int song_ = -1;
  double songStartSound_ = 0;  // sound clock time the song's second 0 plays
  double songServerStart_ = 0;
  size_t nextNote_ = 0;
  bool owner_ = false;  // you started the current song (you clear it for everyone when it ends)
  double lastUpdate_ = 0;  // sound clock at the last update
  bool waitingToLoad_ = false;
  double pendingServerStart_ = 0;

  std::vector<Active> active_;   // sounding notes (for the lights)
  std::map<int, Hands> hands_;   // by player (kYou = this phone)

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
