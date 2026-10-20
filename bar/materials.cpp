#include "bar/materials.h"

namespace app {
namespace {

using engine::Material;

// Rough real-world values. Sound: pitch is the main ring when struck,
// decay how long it rings, absorption how much echo the surface soaks up.
Material make(const char* name, engine::Vec3 color, float roughness, float metallic, int texture,
              float textureScale, float density, float friction, float restitution, float pitch,
              float decay, float absorption) {
  Material m;
  m.name = name;
  m.color = color;
  m.roughness = roughness;
  m.metallic = metallic;
  m.texture = texture;
  m.textureScale = textureScale;
  m.density = density;
  m.friction = friction;
  m.restitution = restitution;
  m.impactPitch = pitch;
  m.impactDecay = decay;
  m.absorption = absorption;
  return m;
}

const Material kMaterials[kMaterialCount] = {
    //   name             color                        rough  metal texture        scale  kg/m3  fric  bounce  Hz     decay  absorb
    make("oak floor",     {0.16f, 0.09f, 0.05f},       0.55f, 0,    kTexWoodPlanks, 0.8f, 700,   0.5f, 0.30f, 180,   0.08f, 0.10f),
    make("dark wood",     {0.20f, 0.10f, 0.05f},       0.60f, 0,    kTexWoodGrain,  1.0f, 650,   0.5f, 0.30f, 220,   0.07f, 0.10f),
    make("varnished wood",{0.38f, 0.18f, 0.08f},       0.18f, 0,    kTexWoodGrain,  1.0f, 700,   0.4f, 0.35f, 260,   0.09f, 0.05f),
    make("door wood",     {0.12f, 0.06f, 0.03f},       0.45f, 0,    kTexWoodGrain,  1.2f, 600,   0.5f, 0.30f, 150,   0.10f, 0.10f),
    // Brick colors come from the texture; mid-gray keeps them as they are.
    make("brick",         {0.50f, 0.50f, 0.50f},       0.90f, 0,    kTexBrick,      1.0f / 0.9f, 1900, 0.7f, 0.10f, 450, 0.03f, 0.03f),
    make("plaster",       {0.07f, 0.055f, 0.05f},      0.95f, 0,    kTexPlaster,    0.4f, 1100,  0.6f, 0.10f, 350,   0.03f, 0.05f),
    make("leather",       {0.45f, 0.06f, 0.05f},       0.50f, 0,    kTexLeather,    4.0f, 900,   0.8f, 0.20f, 120,   0.03f, 0.40f),
    make("chrome",        {0.75f, 0.76f, 0.78f},       0.12f, 1,    kNoTexture,     1.0f, 7800,  0.3f, 0.45f, 2500,  0.60f, 0.02f),
    make("brass",         {0.91f, 0.70f, 0.38f},       0.28f, 1,    kNoTexture,     1.0f, 8500,  0.35f, 0.40f, 1800, 1.00f, 0.02f),
    make("black steel",   {0.05f, 0.05f, 0.05f},       0.45f, 0,    kNoTexture,     1.0f, 7800,  0.4f, 0.35f, 900,   0.40f, 0.02f),
    make("bottle glass",  {0.50f, 0.50f, 0.50f},       0.05f, 0,    kNoTexture,     1.0f, 2500,  0.4f, 0.35f, 3000,  0.40f, 0.02f),
    make("lacquer",       {0.32f, 0.06f, 0.08f},       0.15f, 0,    kNoTexture,     1.0f, 700,   0.4f, 0.35f, 300,   0.12f, 0.05f),
    make("glow glass",    {1.00f, 1.00f, 1.00f},       0.10f, 0,    kNoTexture,     1.0f, 2500,  0.4f, 0.35f, 3200,  0.30f, 0.02f),
};

}  // namespace

const Material& material(MaterialId id) { return kMaterials[id]; }

}  // namespace app
