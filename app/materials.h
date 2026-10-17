#pragma once
// The bar's materials: every kind of stuff in the room, defined once with
// its look, feel and sound (see engine/material.h).
#include "engine/material.h"

namespace app {

// Surface textures, one layer each in the GPU texture array (same order as
// buildBarTextures()). Only the avatars use generated textures now; the room
// is Blender models with their own textures, so the old room surfaces map to
// no texture (generating them cost ~250 ms at start-up for nothing).
enum TextureLayer : int {
  kNoTexture = -1,
  kTexFabric,  // avatar shirt and socks
  kTexDenim,   // avatar jeans
  kTexHair,    // avatar hair and stubble
  kTextureCount,
  kTexWoodPlanks = kNoTexture,
  kTexWoodGrain = kNoTexture,
  kTexBrick = kNoTexture,
  kTexLeather = kNoTexture,
  kTexPlaster = kNoTexture,
};

enum MaterialId : int {
  kOakFloor,       // worn floor planks
  kDarkWood,       // paneling, counter front, shelves
  kVarnishedWood,  // bar top and table tops: glossy clear coat
  kDoorWood,
  kBrick,
  kPlaster,
  kLeather,        // stools and booth
  kChrome,
  kBrass,
  kBlackSteel,     // powder-coated table legs, lamp shades
  kBottleGlass,    // bottles (color set per bottle) and drinking glasses
  kLacquer,        // jukebox cabinet
  kGlowGlass,      // bulbs, neon tubes, lit panels: color and glow per object
  kMaterialCount,
};

const engine::Material& material(MaterialId id);

}  // namespace app
