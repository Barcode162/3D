#pragma once
// Page options used by tests and for checking things by hand, read once at
// start-up (see CLAUDE.md → Build and test):
//   ?cam=x,y,z,yaw,pitch  start the camera there (degrees)
//   ?still                no intro text, no idle motion (screenshot diffs)
//   ?perf=1               performance overlay on
//   ?sit=N  ?emote=N  ?pianosong=N  ?rhythm=song,difficulty  ?autoplay
//   ?shift  ?darts        start a bartending shift / the darts game
// (?skipintro, ?server=, ?cloud=, ?room=, ?name=, ?testerror are read by
// the page's JavaScript.) The game applies them once it's ready.
namespace app {

struct TestHooks {
  bool still = false, perf = false, autoplay = false, shift = false, darts = false;
  int sit = -1, emote = -1, pianoSong = -1;
  int rhythmSong = -1, rhythmDifficulty = 1;
  bool hasCam = false;
  float cam[5] = {};  // x, y, z, yaw, pitch (degrees)

  static TestHooks parse();
};

}  // namespace app
