// The bar's layout as Looped's renderer sees it (looped/render_scene.h).
// Its own file: bar_scene.cpp stays free of GPU headers (the native tests
// build it with g++).
#include "bar/avatar.h"
#include "bar/bar_scene.h"

namespace app {

engine::RenderScene renderSceneOf(const BarScene& scene) {
  engine::RenderScene r;
  r.objects.reserve(scene.objects.size());
  for (const SceneObject& o : scene.objects) {
    const engine::Material& m = material(o.material);
    r.objects.push_back({o.mesh, o.position, o.rotation, o.color, m.metallic, m.roughness, m.texture, o.emissive,
                         o.textureScale});
  }
  for (const ModelPlacement& p : scene.models) {
    // The room shell: without it there's no bar to enter.
    r.models.push_back({p.file, p.position, p.rotation, p.scale, p.room, p.file.find("hall") != std::string::npos,
                        p.prop == Prop::kNone});
  }
  r.lights = scene.lights;
  r.mirrorCount = scene.mirrors.size();
  r.rooms = scene.rooms;
  r.doorways = scene.doorways;
  r.lightGridMin = scene.lightGridMin;
  r.lightGridMax = scene.lightGridMax;
  r.ambient = scene.ambient;
  r.fogColor = scene.fogColor;
  r.fogDensity = scene.fogDensity;
  r.beamHaze = scene.beamHaze;
  r.spawnPosition = scene.spawnPosition;
  r.makeTextures = buildBarTextures;
  r.avatarTextureScale = kAvatarTextureScale;
  // A warm bar at night: amber tint, a touch richer, soft corners; the eyes a
  // third of a stop darker than the engine's default (the user, 2026-10-26 #3:
  // "sort of blinding … a tiny bit less strong").
  r.mood.exposureComp = -1.63f;
  r.mood.tint = {1.04f, 1.0f, 0.94f};
  r.mood.saturation = 1.08f;
  r.mood.contrast = 1.04f;
  r.mood.lift = 0.01f;
  r.mood.vignette = 0.22f;
  return r;
}

}  // namespace app
