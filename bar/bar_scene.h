#pragma once
// The bar's layout: which shapes go where, their colors, the lights and the
// walls/furniture you can't walk through. Pure data, no GPU code. Since
// 2026-10-21 (docs/ENGINE.md E2) it's made from the bar's place file
// (assets/world/places/bar.json): every thing there with its parts (model,
// solids, blocks, seats, uses, light...) turned into these lists.
#include <memory>
#include <string>
#include <vector>

#include "looped/math.h"
#include "looped/mesh_data.h"
#include "bar/materials.h"
#include "looped/procedural.h"
#include "looped/render_scene.h"
#include "looped/world/world.h"

namespace app {

// Lights, mirrors and doorways are the engine's own (looped/render_scene.h).
using engine::Mirror;
using engine::MirrorMode;
using engine::PointLight;

// A placed object. Its material says what it's made of (look, feel, sound);
// color, glow and texture scale start from the material and can be changed
// per object (bottles come in many colors, neon glows pink or cyan).
struct SceneObject {
  engine::MeshData mesh;
  engine::Vec3 position;
  engine::Quat rotation;
  MaterialId material;
  engine::Vec3 color;
  float emissive = 0;      // glows by itself (neon, bulbs)
  float textureScale = 1;  // texture repeats per meter

  SceneObject& tint(engine::Vec3 c) {
    color = c;
    return *this;
  }
  SceneObject& glow(float e) {
    emissive = e;
    return *this;
  }
  SceneObject& scale(float repeatsPerMeter) {
    textureScale = repeatsPerMeter;
    return *this;
  }
};

// A real 3D model from a .glb file in assets/models/. The file gives its
// look; `material` says what it's made of for the other senses (weight,
// friction, sound).
// Movable things: their physics shape (see bar/props.cpp).
enum class Prop { kNone, kPintGlass, kRocksGlass };

// A chess piece's model on the lounge's board (bar/projects/chess/chess.cpp moves it).
struct ChessPiece {
  size_t model = 0;   // index into BarScene::models
  int8_t piece = 0;   // app::chess::make(kind, color)
};

struct ModelPlacement {
  std::string file;  // e.g. "bar_counter.glb"
  engine::Vec3 position;
  engine::Quat rotation;
  float scale = 1;
  MaterialId material = kDarkWood;
  Prop prop = Prop::kNone;  // kNone = fixed in place
  int room = -1;  // a room it belongs to (BarScene::rooms): loaded when you come near
};

// An invisible solid box for physics, where the visible shape is a model
// (e.g. the counter). Scene objects get boxes automatically.
struct Collider {
  engine::Vec3 center;
  engine::Vec3 half;
  engine::Quat rotation;
  MaterialId material;
};

// Axis-aligned rectangle on the floor (x/z) that blocks walking.
struct Blocker {
  float minX, minZ, maxX, maxZ;
};

// A raised floor you can walk onto (a stage, a step): inside the rectangle
// the ground is `height` meters up.
struct FloorArea {
  float minX, minZ, maxX, maxZ;
  float height;
};

// A place to sit. Anyone can sit on any free seat (one person each).
enum class SeatKind { kStool, kChair, kSofa, kPianoBench, kSim };

struct Seat {
  std::string name;      // "Bar stool", "Sofa"...
  engine::Vec3 hip;      // where the sitter's hip joint goes
  float yaw = 0;         // which way the sitter faces (camera yaw convention)
  float lowSeat = 0;     // 0 = stool height, 1 = low sofa (knees bend more)
  SeatKind kind = SeatKind::kStool;
  int model = -1;        // BarScene::models entry that lights up, or -1
  int machine = -1;      // sims: which rig (0, 1)
  engine::Vec3 boundsMin, boundsMax;  // where taps hit it
};

// Something you can use. It has a floating name tag; walking close and
// looking at it highlights it and shows a prompt; tapping it runs `action`.
enum class Action {
  kNone, kJukebox, kMirror, kMirrorMode, kRhythm,
  kDarts,        // stand at the oche and throw
  kBartender,    // the bartender's station (badge needed)
  kDrinkShelf,   // a drink waiting on the counter (value = slot)
  kBell, kGuitar, kMic, kLightSwitch, kFireplace, kDoor, kTv, kBooks, kChess, kTipJar, kTaps, kCoffee,
  kPianoSongs,   // the sheet music on the piano: pick a song to play
  kArcade,       // an arcade machine (value: 0 = the platformer), full screen
};

struct Interactable {
  std::string name;               // floating name tag
  std::string prompt;             // shown when you're at it (if no action overrides it)
  engine::Vec3 tagPosition;
  engine::Vec3 boundsMin, boundsMax;  // world box that taps hit
  size_t firstObject = 0, objectCount = 0;  // parts that light up when focused
  size_t firstModel = 0, modelCount = 0;    // models (BarScene::models) that light up
  Action action = Action::kNone;
  int mirror = -1;  // for Action::kMirror(Mode): index into BarScene::mirrors
  int value = 0;    // for Action::kMirrorMode: 0 = full, 1 = avatars only, 2 = off
  // Small things (a bell, a book): only when you look straight at them or
  // tap them, not just by standing nearby.
  bool direct = false;
};

struct BarScene {
  std::vector<SceneObject> objects;
  std::vector<ModelPlacement> models;
  std::vector<Collider> colliders;
  std::vector<PointLight> lights;
  std::vector<Blocker> blockers;
  std::vector<FloorArea> floors;
  std::vector<Interactable> interactables;
  std::vector<Mirror> mirrors;
  std::vector<Seat> seats;
  engine::Vec3 lightGridMin, lightGridMax;  // floor area the light grid covers (x, z)
  engine::Vec3 ambient;
  engine::Vec3 fogColor;
  float fogDensity;
  engine::Vec3 jukeboxSpeaker;  // where the music comes from
  engine::Vec3 arcadeSpeaker;   // the arcade's music (inside, by the machine)
  engine::Vec3 arcadeMin, arcadeMax;  // the arcade's floor (x, z): its music plays inside
  engine::Vec3 arcadeDoor;
  // The chessboard: its middle (on the surface), one square's size, the pieces.
  engine::Vec3 chessCenter;
  float chessSquare = 0.055f;
  int coinModel = -1;  // the credit coin (bar/coin_pop.h), under the floor until you earn credits
  // The things these lists came from (looped/world): the world itself, built
  // from the place file, and which thing each model and light is.
  std::shared_ptr<const engine::world::World> world;
  std::vector<int> modelThing, lightThing, useThing, seatThing;
  std::string problems;  // what was wrong in the files ("" = nothing)
  std::vector<ChessPiece> chessPieces;      // the middle of its doorway (bar side): the music leaks out here
  engine::Vec3 spawnPosition;
  float spawnYaw;
  // Rooms, for drawing (SceneRenderer): a room is drawn only when you're in
  // it or can see into it through a doorway, and then only what shows
  // through the doorway. Anything not wholly inside one room (walls between
  // rooms, things in doorways) is always considered.
  std::vector<engine::Aabb> rooms;
  using Doorway = engine::Doorway;
  std::vector<Doorway> doorways;
  // Game spots (see bar/games.*): the piano, the dartboard, the two sims,
  // the rhythm cabinet's screen, the bartender's station.
  engine::Vec3 pianoPosition;
  float pianoYaw = 0;
  engine::Vec3 dartBoard;        // center of the board's face
  engine::Vec3 dartOche;         // where the thrower stands (eyes)
  engine::Vec3 simPosition[2];   // rig origins (they face +x)
  engine::Vec3 rhythmScreen[4];  // corners: bottom-left, bottom-right, top-right, top-left
  engine::Vec3 barTopFront[8];   // drink spots on the counter, customer side
  engine::Vec3 bartenderSpot;    // standing spot behind the bar (eyes)
  int fireLight = -1;            // lights index of the fireplace glow
  std::vector<int> roomLights;   // lights the switch turns down
};

// Reads the bar's kinds and place (bar/world_files.h) and lays it out.
BarScene buildBarScene();

// What the renderer needs from the bar (looped/render_scene.h): the shapes
// with their materials' looks, the models, lights, rooms, air and textures.
engine::RenderScene renderSceneOf(const BarScene& scene);

// Generates the surface textures (one per TextureLayer, in order).
std::vector<engine::MaterialImages> buildBarTextures();

// Moves a walker (circle of `radius` on the floor) out of any blocker.
engine::Vec3 resolveCollisions(const BarScene& scene, engine::Vec3 position, float radius);

// Whether a point on the floor can be walked to from the door (a 0.1 m grid
// built the first time it's asked, for a 0.3 m walker).
bool reachableOnFoot(const BarScene& scene, engine::Vec3 position);

// Where you stand after getting up from a seat: in front of sofas and
// armchairs, behind stools, the piano bench and the racing rigs; if that
// spot is boxed in, the nearest floor you can walk away from (y = 0).
engine::Vec3 standSpot(const BarScene& scene, const Seat& seat);

// How high the floor is under a point (0, or a raised FloorArea).
float groundHeight(const BarScene& scene, engine::Vec3 position);

}  // namespace app
