#pragma once
// Page options used by tests and for checking things by hand, read once at
// start-up (see CLAUDE.md → Build and test):
//   ?cam=x,y,z,yaw,pitch  start the camera there (degrees)
//   ?still                no intro text, no idle motion (screenshot diffs)
//   ?perf=1               performance overlay on
//   ?sit=N  ?emote=N  ?pianosong=N  ?rhythm=song,difficulty  ?autoplay
//   ?shift  ?darts        start a bartending shift / the darts game
//   ?garage               with ?sit= on a racing rig: open its garage
//   ?chess                play chess at the lounge's board
// (?skipintro, ?server=, ?cloud=, ?room=, ?name=, ?testerror are read by
// the page's JavaScript.) The game applies them once it's ready.
namespace app {

struct TestHooks {
  bool still = false, perf = false, autoplay = false, shift = false, darts = false;
  bool garage = false;
  bool walkcheck = false;
  float walkPose = -1;  // ?walkpose=<degrees>: frozen mid-walk at that phase (with ?walkdir=<degrees>)
  float walkDir = 0;
  float previewTurn = 0;  // ?previewturn=<degrees>: the avatar customizer's view turned (side views)  // ?walkcheck: log how much planted feet slide (app/avatar.cpp walkReport)
  bool chess = false;   // ?chess: sit down at the chessboard  // with ?sit= on a racing rig: the garage instead of straight into a race
  int sit = -1, emote = -1, pianoSong = -1;
  int rhythmSong = -1, rhythmDifficulty = 1;
  bool hasCam = false;
  float cam[5] = {};  // x, y, z, yaw, pitch (degrees)

  static TestHooks parse();
};

}  // namespace app
