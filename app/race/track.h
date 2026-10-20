#pragma once
// The race circuit. A closed spline through hand-placed control points
// (position, elevation, width, banking) is sampled every meter; everything
// else comes from those samples:
// - the surface the physics drives on (asphalt, curbs, grass, gravel), via
//   engine::Ground, with a fast "where am I on the track" lookup,
// - the walls (tire barriers) at the edge of the run-off,
// - the road mesh, curbs, lines, run-off, terrain, barriers and scenery.
// Station s = meters along the centerline from the start line;
// lateral = meters to the right of the centerline.
#include <vector>

#include "app/race/mesh.h"
#include "engine/math.h"
#include "engine/vehicle.h"

namespace app::race {

class Track : public engine::Ground {
 public:
  struct Sample {
    engine::Vec3 pos;       // centerline point (road surface)
    engine::Vec3 tangent;   // along the road (with slope)
    engine::Vec3 side;      // horizontal, to the right
    engine::Vec3 up;        // road normal (with banking)
    float halfWidth = 7;
    float bankSlope = 0;    // height change per meter to the right
    float curvature = 0;    // 1 / radius, + = turning right
    float curbLeft = 0, curbRight = 0;  // curb width (0 = none)
    float wallLeft = 12, wallRight = 12;  // lateral distance of the barriers
    bool gravelLeft = false, gravelRight = false;
  };

  void build();
  float length() const { return length_; }
  const std::vector<Sample>& samples() const { return samples_; }
  // Interpolated sample at station s (wraps around).
  Sample at(float s) const;
  // Station and lateral offset of a world point. `hint` = a sample index
  // to search near (updated; -1 = search everywhere).
  void locate(engine::Vec3 p, float& s, float& lateral, int& hint) const;
  // Surface height and normal at (s, lateral), and what it's made of.
  engine::Vec3 surfacePoint(float s, float lateral, engine::Vec3* normal = nullptr,
                            engine::Surface* surface = nullptr) const;
  engine::Vec3 point(float s, float lateral) const { return surfacePoint(s, lateral); }
  // Distance to the barrier on that side at s (positive numbers).
  float wallLeft(float s) const;
  float wallRight(float s) const;

  bool cast(engine::Vec3 origin, engine::Vec3 dir, float maxDistance, engine::GroundHit& hit,
            int& hint) const override;

  // The skid pad in the infield (docs/DRIVING_REWORK.md, the MVP): a flat
  // asphalt circle with painted 30 m and 50 m rings, grass to a round tire
  // wall. Its own ground: inside the wall, the track doesn't count.
  struct Pad {
    engine::Vec3 centre;      // on the surface
    float asphalt = 60;       // radius of the asphalt
    float wall = 66;          // radius of the tire wall
  };
  const Pad& pad() const { return pad_; }
  bool onPad(engine::Vec3 p) const;  // inside the pad's wall (+ a little margin)
  // Where the car starts on the pad: on the 50 m ring, facing along it.
  void padStart(engine::Vec3& position, float& heading) const;
  // The ground under a point anywhere (track or pad): point, normal, surface.
  engine::Vec3 groundAt(engine::Vec3 p, int& hint, engine::Vec3* normal = nullptr,
                        engine::Surface* surface = nullptr) const;

  // Starting grid slot k (0 = pole): ground position and heading.
  void gridSlot(int k, engine::Vec3& position, float& heading) const;

  // Geometry, split into chunks along the track (for culling).
  struct Chunk {
    MeshBuilder mesh;
    engine::Vec3 boundsMin, boundsMax;
  };
  std::vector<Chunk> buildMeshes() const;

 private:
  void placePad();

  std::vector<Sample> samples_;
  float length_ = 0;
  Pad pad_;
};

}  // namespace app::race
