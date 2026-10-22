#pragma once
#include <string>
// Page options used by tests and for checking things by hand, read once at
// start-up (see CLAUDE.md → Build and test):
//   ?cam=x,y,z,yaw,pitch  start the camera there (degrees)
//   ?still                no intro text, no idle motion (screenshot diffs)
//   ?reflect=0            no reflection cube (E4.4): shiny things as before
//   ?place=home           another place (assets/world/places/<name>.json, E7)
//   ?perf=1               performance overlay on
//   ?sit=N  ?emote=N  ?pianosong=N  ?rhythm=song,difficulty  ?autoplay
//   ?shift  ?darts        start a bartending shift / the darts game
//   ?garage               with ?sit= on a racing rig: open its garage
//   ?chess                play chess at the lounge's board
//   ?pad                  with ?sit= on a racing rig: the skid pad (new physics)
//   ?telemetry=1          the race's telemetry overlay
//   ?coin                 the credit coin pops out once (screenshots)
//   ?post=0|1             glow and mood after the scene off / on (looped/post.h)
//   ?light=classic        the old lighting (docs/manual/light.md); New is the default
//   ?<feature>=0          one feature of the Alive plan off, e.g. ?bounce=0
//                         (every feature's switch: docs/manual/)
// (?skipintro, ?server=, ?cloud=, ?room=, ?name=, ?testerror are read by
// the page's JavaScript.) The game applies them once it's ready.
namespace app {

struct TestHooks {
  bool noReflect = false;
  bool studio = false;  // ?studio=1: the Studio's view of the world (bar/studio_mode.*)
  std::string place = "bar";
  bool still = false, perf = false, autoplay = false, shift = false, darts = false;
  bool garage = false;
  bool pad = false, telemetry = false;
  bool walkcheck = false;
  float walkPose = -1;  // ?walkpose=<degrees>: frozen mid-walk at that phase (with ?walkdir=<degrees>)
  float walkDir = 0;
  float previewTurn = 0;  // ?previewturn=<degrees>: the avatar customizer's view turned (side views)  // ?walkcheck: log how much planted feet slide (bar/avatar.cpp walkReport)
  int mirrorQ = -1;     // ?mirrorq=0|1: Settings → Graphics → Mirror forced (full, simple)
  int quality = -1;     // ?quality=auto|high|medium|low: Settings → Graphics forced (-1: the setting)
  bool coin = false;    // ?coin: the credit coin pops once after you enter
  int post = -1;        // ?post=0|1: glow and mood forced off / on (-1: by quality)
  bool chess = false;   // ?chess: sit down at the chessboard  // with ?sit= on a racing rig: the garage instead of straight into a race
  int sit = -1, emote = -1, pianoSong = -1;
  int rhythmSong = -1, rhythmDifficulty = 1;
  bool hasCam = false;
  float cam[5] = {};  // x, y, z, yaw, pitch (degrees)

  // ?light=classic: the lighting before L1 (Settings → Graphics → Lighting
  // also switches it; this forces it). -1 = the setting, 0 = new, 1 = classic.
  int lightClassic = -1;
  // ?<name>=0 for any feature switch (docs/manual/): false = turned off.
  bool feature(const char* name) const;
  std::string query;

  static TestHooks parse();
};

}  // namespace app
