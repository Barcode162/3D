#include "app/race/track.h"

#include <algorithm>
#include <cmath>

using engine::Vec3;

namespace app::race {
namespace {

// The circuit, ~2.45 km (designed top-down, see docs/RACING.md): x, z,
// elevation, half width, banking (degrees, + = right side lower). Driven in
// list order; the start line is the first point. -z is "north".
struct Control {
  float x, z, y, halfWidth, bank;
};
const Control kCircuit[] = {
    {0, 0, 0, 7, 0},       {200, 0, 0, 7, 0},     {420, 0, 0, 7, 0},          // main straight
    {500, -8, 0, 7, 0},    {530, -45, 0, 7, 0},   {515, -85, 1, 7, 0},        // T1 hairpin (left)
    {470, -100, 2, 6.5f, 0}, {400, -110, 4, 6.5f, 0},                         // short straight
    {340, -140, 7, 6.5f, 0}, {300, -200, 10, 6.5f, 0},                        // uphill right
    {290, -260, 12, 6.5f, 0}, {240, -300, 13, 6.5f, 0}, {230, -360, 13, 6.5f, 0},  // esses
    {180, -410, 12, 6.5f, 0}, {100, -430, 11, 7, 0},
    {-80, -430, 9, 7, 0},  {-230, -420, 7, 7, 0},                             // back straight
    {-320, -380, 6, 7, 2}, {-360, -310, 5, 7, 2},                             // fast sweeper
    {-350, -265, 4, 6.5f, 0}, {-318, -228, 4, 6.5f, 0}, {-322, -180, 3, 6.5f, 0},  // chicane
    {-380, -140, 2, 7, 3}, {-410, -80, 1, 7, 4}, {-390, -25, 0, 7, 3},        // carousel
    {-320, -2, 0, 7, 0},   {-200, 0, 0, 7, 0},                                // onto the straight
};
constexpr int kControls = sizeof(kCircuit) / sizeof(kCircuit[0]);

// Centripetal Catmull-Rom through four control points (t in 0..1 between
// b and c): smooth, no loops or overshoots on uneven spacing.
Control catmullRom(const Control& a, const Control& b, const Control& c, const Control& d, float t) {
  const auto knot = [](const Control& p, const Control& q, float k) {
    return k + std::sqrt(std::max(std::hypot(q.x - p.x, q.z - p.z), 1e-3f));
  };
  const float t0 = 0, t1 = knot(a, b, t0), t2 = knot(b, c, t1), t3 = knot(c, d, t2);
  const float u = t1 + (t2 - t1) * t;
  const auto lerp = [](const Control& p, const Control& q, float tp, float tq, float x) {
    const float wp = (tq - x) / (tq - tp), wq = (x - tp) / (tq - tp);
    return Control{p.x * wp + q.x * wq, p.z * wp + q.z * wq, p.y * wp + q.y * wq, p.halfWidth * wp + q.halfWidth * wq,
                   p.bank * wp + q.bank * wq};
  };
  const Control a1 = lerp(a, b, t0, t1, u), a2 = lerp(b, c, t1, t2, u), a3 = lerp(c, d, t2, t3, u);
  const Control b1 = lerp(a1, a2, t0, t2, u), b2 = lerp(a2, a3, t1, t3, u);
  return lerp(b1, b2, t1, t2, u);
}

constexpr float kCurbWidth = 1.1f;           // curb width (m)
constexpr float kCurbHeight = 0.035f;   // at the outer edge
constexpr float kRunoff = 11;           // grass to the barrier
constexpr float kGravelRunoff = 24;     // gravel traps are deeper

const engine::Surface kAsphaltSurface{1.0f, 0.012f, 0, 0};
const engine::Surface kCurbSurface{0.95f, 0.014f, 0, 0.004f};
const engine::Surface kGrassSurface{0.55f, 0.05f, 0.05f, 0.01f};
const engine::Surface kGravelSurface{0.45f, 0.12f, 0.35f, 0.012f};

float hash(float x, float y) {
  const float h = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
  return h - std::floor(h);
}

}  // namespace

void Track::build() {
  // Dense points along the spline, then resampled every ~1 m.
  std::vector<Control> dense;
  for (int i = 0; i < kControls; ++i) {
    const Control& a = kCircuit[(i + kControls - 1) % kControls];
    const Control& b = kCircuit[i];
    const Control& c = kCircuit[(i + 1) % kControls];
    const Control& d = kCircuit[(i + 2) % kControls];
    for (int k = 0; k < 64; ++k) dense.push_back(catmullRom(a, b, c, d, static_cast<float>(k) / 64));
  }
  std::vector<float> dist(dense.size() + 1, 0);
  for (size_t i = 0; i < dense.size(); ++i) {
    const Control& p = dense[i];
    const Control& q = dense[(i + 1) % dense.size()];
    dist[i + 1] = dist[i] + std::sqrt((q.x - p.x) * (q.x - p.x) + (q.z - p.z) * (q.z - p.z) + (q.y - p.y) * (q.y - p.y));
  }
  length_ = dist.back();
  const int n = static_cast<int>(std::round(length_));
  const float step = length_ / static_cast<float>(n);
  samples_.assign(static_cast<size_t>(n), {});
  size_t j = 0;
  for (int i = 0; i < n; ++i) {
    const float s = static_cast<float>(i) * step;
    while (j + 1 < dense.size() && dist[j + 1] < s) ++j;
    const Control& p = dense[j];
    const Control& q = dense[(j + 1) % dense.size()];
    const float t = (s - dist[j]) / std::max(dist[j + 1] - dist[j], 1e-4f);
    Sample& o = samples_[static_cast<size_t>(i)];
    o.pos = {p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t};
    o.halfWidth = p.halfWidth + (q.halfWidth - p.halfWidth) * t;
    o.bankSlope = std::tan(engine::radians(p.bank + (q.bank - p.bank) * t));
  }
  // Directions and curvature.
  for (int i = 0; i < n; ++i) {
    Sample& o = samples_[static_cast<size_t>(i)];
    const Vec3 a = samples_[static_cast<size_t>((i + n - 1) % n)].pos, b = samples_[static_cast<size_t>((i + 1) % n)].pos;
    o.tangent = engine::normalize(b - a);
    o.side = engine::normalize(engine::cross(o.tangent, {0, 1, 0}));
    const Vec3 flatUp = engine::normalize(engine::cross(o.side, o.tangent));
    o.up = engine::normalize(flatUp - o.side * o.bankSlope);
  }
  std::vector<float> rawCurve(static_cast<size_t>(n));
  // (Banking is given as a size; the inside of the corner is the low side.)
  for (int i = 0; i < n; ++i) {
    const Sample& a = samples_[static_cast<size_t>((i + n - 2) % n)];
    const Sample& b = samples_[static_cast<size_t>((i + 2) % n)];
    rawCurve[static_cast<size_t>(i)] = engine::dot(b.tangent - a.tangent, samples_[static_cast<size_t>(i)].side) / (4 * step);
  }
  for (int i = 0; i < n; ++i) {  // smoothed over ~20 m
    float sum = 0;
    for (int k = -10; k <= 10; ++k) sum += rawCurve[static_cast<size_t>((i + k + n) % n)];
    samples_[static_cast<size_t>(i)].curvature = sum / 21;
  }
  for (Sample& o : samples_) {
    o.bankSlope = std::fabs(o.bankSlope) * (o.curvature >= 0 ? 1.0f : -1.0f);
    o.up = engine::normalize(engine::normalize(engine::cross(o.side, o.tangent)) - o.side * o.bankSlope);
  }
  // Curbs on both sides through corners (and a little before and after);
  // gravel traps on the outside of the slow corners; barriers beyond.
  for (int i = 0; i < n; ++i) {
    float maxCurve = 0, signedAtMax = 0;
    for (int k = -18; k <= 18; ++k) {
      const float c = samples_[static_cast<size_t>((i + k + n) % n)].curvature;
      if (std::fabs(c) > maxCurve) {
        maxCurve = std::fabs(c);
        signedAtMax = c;
      }
    }
    Sample& o = samples_[static_cast<size_t>(i)];
    if (maxCurve > 1.0f / 220) o.curbLeft = o.curbRight = kCurbWidth;
    float trap = 0;
    for (int k = -40; k <= 25; ++k) trap = std::max(trap, std::fabs(samples_[static_cast<size_t>((i + k + n) % n)].curvature));
    const bool slow = trap > 1.0f / 70;
    // Turning right: the outside is the left.
    o.gravelLeft = slow && signedAtMax > 0;
    o.gravelRight = slow && signedAtMax < 0;
    o.wallLeft = o.halfWidth + o.curbLeft + (o.gravelLeft ? kGravelRunoff : kRunoff);
    o.wallRight = o.halfWidth + o.curbRight + (o.gravelRight ? kGravelRunoff : kRunoff);
  }
  // Smooth the barrier lines so they don't step.
  for (int pass = 0; pass < 3; ++pass) {
    std::vector<float> l(static_cast<size_t>(n)), r(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
      float sl = 0, sr = 0;
      for (int k = -6; k <= 6; ++k) {
        sl = std::max(sl, samples_[static_cast<size_t>((i + k + n) % n)].wallLeft);
        sr = std::max(sr, samples_[static_cast<size_t>((i + k + n) % n)].wallRight);
      }
      l[static_cast<size_t>(i)] = sl;
      r[static_cast<size_t>(i)] = sr;
    }
    for (int i = 0; i < n; ++i) {
      samples_[static_cast<size_t>(i)].wallLeft = l[static_cast<size_t>(i)];
      samples_[static_cast<size_t>(i)].wallRight = r[static_cast<size_t>(i)];
    }
  }
}

Track::Sample Track::at(float s) const {
  const int n = static_cast<int>(samples_.size());
  float u = std::fmod(s, length_);
  if (u < 0) u += length_;
  const float f = u / length_ * static_cast<float>(n);
  const int i = static_cast<int>(f) % n;
  const float t = f - std::floor(f);
  const Sample& a = samples_[static_cast<size_t>(i)];
  const Sample& b = samples_[static_cast<size_t>((i + 1) % n)];
  Sample o = a;
  const auto mix = [t](Vec3 p, Vec3 q) { return p + (q - p) * t; };
  o.pos = mix(a.pos, b.pos);
  o.tangent = engine::normalize(mix(a.tangent, b.tangent));
  o.side = engine::normalize(mix(a.side, b.side));
  o.up = engine::normalize(mix(a.up, b.up));
  o.halfWidth = a.halfWidth + (b.halfWidth - a.halfWidth) * t;
  o.bankSlope = a.bankSlope + (b.bankSlope - a.bankSlope) * t;
  o.curvature = a.curvature + (b.curvature - a.curvature) * t;
  o.wallLeft = a.wallLeft + (b.wallLeft - a.wallLeft) * t;
  o.wallRight = a.wallRight + (b.wallRight - a.wallRight) * t;
  return o;
}

void Track::locate(Vec3 p, float& s, float& lateral, int& hint) const {
  const int n = static_cast<int>(samples_.size());
  const auto d2 = [&](int i) {
    const Vec3 q = samples_[static_cast<size_t>((i % n + n) % n)].pos;
    const float dx = p.x - q.x, dz = p.z - q.z, dy = (p.y - q.y) * 0.5f;
    return dx * dx + dz * dz + dy * dy;
  };
  int best = 0;
  if (hint < 0 || hint >= n) {
    // Coarse pass every 8 samples, then refine around the winner.
    float bestD = 1e30f;
    for (int i = 0; i < n; i += 8) {
      const float d = d2(i);
      if (d < bestD) {
        bestD = d;
        best = i;
      }
    }
    hint = best;
  }
  // Walk downhill from the hint (the nearest sample moves smoothly).
  best = hint;
  float bestD = d2(best);
  for (int iter = 0; iter < 64; ++iter) {
    const float fwd = d2(best + 1), back = d2(best - 1);
    if (fwd < bestD) {
      best = (best + 1) % n;
      bestD = fwd;
    } else if (back < bestD) {
      best = (best - 1 + n) % n;
      bestD = back;
    } else {
      break;
    }
  }
  hint = best;
  const Sample& a = samples_[static_cast<size_t>(best)];
  const float step = length_ / static_cast<float>(n);
  const float along = engine::clampf(engine::dot(p - a.pos, a.tangent), -step, step);
  s = static_cast<float>(best) * step + along;
  if (s < 0) s += length_;
  if (s >= length_) s -= length_;
  const Sample c = at(s);
  const Vec3 flat{p.x - c.pos.x, 0, p.z - c.pos.z};
  lateral = engine::dot(flat, c.side);
}

Vec3 Track::surfacePoint(float s, float lateral, Vec3* normal, engine::Surface* surface) const {
  const Sample c = at(s);
  const float edge = c.halfWidth;
  const float side = lateral >= 0 ? 1.0f : -1.0f;
  const float off = std::fabs(lateral);
  const float curb = lateral >= 0 ? c.curbRight : c.curbLeft;
  const bool gravel = lateral >= 0 ? c.gravelRight : c.gravelLeft;
  // Height across the road: the banked road plane, curbs rising to the
  // outside, then run-off continuing level with the road edge.
  const float roadDrop = c.bankSlope * std::min(off, edge) * side;
  float h = -roadDrop;
  Vec3 n = c.up;
  engine::Surface surf = kAsphaltSurface;
  if (off > edge) {
    const float past = off - edge;
    if (past <= curb) {
      h += kCurbHeight * (past / curb);
      surf = kCurbSurface;
    } else {
      h += curb > 0 ? kCurbHeight * std::max(0.0f, 1 - (past - curb) / 0.3f) : 0;
      h -= std::min(past - curb, 8.0f) * 0.012f;  // run-off drains outward a little
      surf = gravel ? kGravelSurface : kGrassSurface;
      const Vec3 flatUp = engine::normalize(engine::cross(c.side, c.tangent));
      n = past - curb < 8 ? engine::normalize(flatUp + c.side * (0.012f * side)) : flatUp;
    }
  }
  if (surf.bumps > 0) {
    const float q = std::floor(s * 1.7f), w = std::floor(lateral * 1.7f);
    h += (hash(q, w) - 0.5f) * surf.bumps;
  }
  if (normal) *normal = n;
  if (surface) *surface = surf;
  return c.pos + c.side * lateral + Vec3{0, h, 0};
}

float Track::wallLeft(float s) const { return at(s).wallLeft; }
float Track::wallRight(float s) const { return at(s).wallRight; }

bool Track::cast(Vec3 origin, Vec3 dir, float maxDistance, engine::GroundHit& hit, int& hint) const {
  float s, lateral;
  locate(origin, s, lateral, hint);
  Vec3 n;
  engine::Surface surf;
  const Vec3 p = surfacePoint(s, lateral, &n, &surf);
  const float denom = engine::dot(dir, n);
  if (denom > -1e-3f) return false;
  const float t = engine::dot(p - origin, n) / denom;
  if (t < -0.5f || t > maxDistance) return false;
  hit.distance = t;
  hit.point = origin + dir * t;
  hit.normal = n;
  hit.surface = surf;
  return true;
}

void Track::gridSlot(int k, Vec3& position, float& heading) const {
  const float s = length_ - 12.0f - static_cast<float>(k) * 8.0f;
  const float lateral = (k % 2 == 0 ? -1.0f : 1.0f) * 3.0f;
  position = surfacePoint(s, lateral);
  heading = engine::Vehicle::headingOf(at(s).tangent);
}

namespace {

const Vec3 kAsphaltColor{0.055f, 0.055f, 0.06f};
const Vec3 kGrassColor{0.10f, 0.20f, 0.05f};
const Vec3 kGravelColor{0.42f, 0.37f, 0.29f};
const Vec3 kWhite{0.85f, 0.85f, 0.82f};
const Vec3 kRed{0.62f, 0.04f, 0.03f};

}  // namespace

std::vector<Track::Chunk> Track::buildMeshes() const {
  const int n = static_cast<int>(samples_.size());
  const float step = length_ / static_cast<float>(n);
  constexpr int kChunk = 100;  // samples per chunk
  std::vector<Chunk> chunks(static_cast<size_t>((n + kChunk - 1) / kChunk));
  const auto P = [&](int i, float lateral, float lift = 0) {
    return surfacePoint(static_cast<float>(i) * step, lateral) + Vec3{0, lift, 0};
  };
  const auto N = [&](int i, float lateral) {
    Vec3 nn;
    surfacePoint(static_cast<float>(i) * step, lateral, &nn);
    return nn;
  };
  // A strip between two lateral offsets (functions of the sample).
  const auto strip = [&](MeshBuilder& m, int i, float l0a, float l1a, float l0b, float l1b, Vec3 color, uint32_t mat,
                         float lift = 0) {
    const int j = (i + 1) % n;
    const uint32_t a = m.vertex(P(i, l0a, lift), N(i, l0a), color, mat);
    const uint32_t b = m.vertex(P(i, l1a, lift), N(i, l1a), color, mat);
    const uint32_t c = m.vertex(P(j, l1b, lift), N(j, l1b), color, mat);
    const uint32_t d = m.vertex(P(j, l0b, lift), N(j, l0b), color, mat);
    m.quad(a, b, c, d);
  };
  for (int i = 0; i < n; ++i) {
    MeshBuilder& m = chunks[static_cast<size_t>(i / kChunk)].mesh;
    const Sample& a = samples_[static_cast<size_t>(i)];
    const Sample& b = samples_[static_cast<size_t>((i + 1) % n)];
    // Road in four strips across (follows the banking).
    for (int k = 0; k < 4; ++k) {
      const float u0 = -1 + 0.5f * static_cast<float>(k), u1 = u0 + 0.5f;
      strip(m, i, u0 * a.halfWidth, u1 * a.halfWidth, u0 * b.halfWidth, u1 * b.halfWidth, kAsphaltColor, kAsphalt);
    }
    // White edge lines.
    for (float sd : {-1.0f, 1.0f}) {
      const float la = sd * (a.halfWidth - 0.3f), lb = sd * (b.halfWidth - 0.3f);
      if (sd < 0) strip(m, i, la - 0.08f, la + 0.08f, lb - 0.08f, lb + 0.08f, kWhite, kPaint, 0.008f);
      else strip(m, i, la - 0.08f, la + 0.08f, lb - 0.08f, lb + 0.08f, kWhite, kPaint, 0.008f);
    }
    // Curbs: red and white blocks.
    const Vec3 curbColor = (i / 3) % 2 == 0 ? kRed : kWhite;
    if (a.curbRight > 0 || b.curbRight > 0) {
      strip(m, i, a.halfWidth, a.halfWidth + kCurbWidth, b.halfWidth, b.halfWidth + kCurbWidth, curbColor, kCurb, 0.002f);
    }
    if (a.curbLeft > 0 || b.curbLeft > 0) {
      strip(m, i, -a.halfWidth - kCurbWidth, -a.halfWidth, -b.halfWidth - kCurbWidth, -b.halfWidth, curbColor, kCurb, 0.002f);
    }
    // Run-off: grass or gravel out to the barrier, then a grass skirt.
    const float ea = a.halfWidth + a.curbRight, eb = b.halfWidth + b.curbRight;
    strip(m, i, ea, a.wallRight, eb, b.wallRight, a.gravelRight ? kGravelColor : kGrassColor,
          a.gravelRight ? kGravel : kGrass);
    const float fa = a.halfWidth + a.curbLeft, fb = b.halfWidth + b.curbLeft;
    strip(m, i, -a.wallLeft, -fa, -b.wallLeft, -fb, a.gravelLeft ? kGravelColor : kGrassColor,
          a.gravelLeft ? kGravel : kGrass);
    for (float sd : {-1.0f, 1.0f}) {
      const float wa = sd > 0 ? a.wallRight : a.wallLeft, wb = sd > 0 ? b.wallRight : b.wallLeft;
      const Vec3 pa0 = P(i, sd * wa), pb0 = P((i + 1) % n, sd * wb);
      const Vec3 pa1 = pa0 + a.side * (sd * 30) + Vec3{0, -2.0f, 0}, pb1 = pb0 + b.side * (sd * 30) + Vec3{0, -2.0f, 0};
      if (sd > 0) m.flatQuad(pa0, pa1, pb1, pb0, kGrassColor, kGrass);
      else m.flatQuad(pa1, pa0, pb0, pb1, kGrassColor, kGrass);
      // Tire barrier: inner face, top and back, striped every 4 m.
      const Vec3 stripe = (i / 4) % 2 == 0 ? Vec3{0.75f, 0.75f, 0.72f} : Vec3{0.5f, 0.03f, 0.02f};
      const Vec3 up{0, 1.0f, 0};
      const Vec3 back = a.side * (sd * 0.9f), backB = b.side * (sd * 0.9f);
      if (sd > 0) {
        m.flatQuad(pb0, pb0 + up, pa0 + up, pa0, stripe, kRubber);
        m.flatQuad(pa0 + up, pb0 + up, pb0 + up + backB, pa0 + up + back, {0.03f, 0.03f, 0.03f}, kRubber);
        m.flatQuad(pa0 + back, pa0 + back + up, pb0 + backB + up, pb0 + backB, {0.03f, 0.03f, 0.03f}, kRubber);
      } else {
        m.flatQuad(pa0, pa0 + up, pb0 + up, pb0, stripe, kRubber);
        m.flatQuad(pb0 + up, pa0 + up, pa0 + up + back, pb0 + up + backB, {0.03f, 0.03f, 0.03f}, kRubber);
        m.flatQuad(pb0 + backB, pb0 + backB + up, pa0 + back + up, pa0 + back, {0.03f, 0.03f, 0.03f}, kRubber);
      }
    }
  }
  // Start/finish: a checkered band across the road at s = 0, grid slots behind.
  {
    MeshBuilder& m = chunks[0].mesh;
    const Sample& c = samples_[0];
    const int across = 14;
    for (int row = 0; row < 2; ++row) {
      for (int k = 0; k < across; ++k) {
        const float l0 = -c.halfWidth + 2 * c.halfWidth * static_cast<float>(k) / across;
        const float l1 = l0 + 2 * c.halfWidth / across;
        const float s0 = static_cast<float>(row) * 1.0f - 1.0f, s1 = s0 + 1.0f;
        const Vec3 color = (row + k) % 2 == 0 ? Vec3{0.9f, 0.9f, 0.9f} : Vec3{0.02f, 0.02f, 0.02f};
        const Vec3 p0 = surfacePoint(s0, l0) + Vec3{0, 0.01f, 0}, p1 = surfacePoint(s0, l1) + Vec3{0, 0.01f, 0};
        const Vec3 p2 = surfacePoint(s1, l1) + Vec3{0, 0.01f, 0}, p3 = surfacePoint(s1, l0) + Vec3{0, 0.01f, 0};
        m.flatQuad(p0, p1, p2, p3, color, kPaint);
      }
    }
    for (int k = 0; k < 12; ++k) {  // grid slot markings
      Vec3 pos;
      float heading;
      gridSlot(k, pos, heading);
      float s, lat;
      int hint = -1;
      locate(pos, s, lat, hint);
      MeshBuilder& g = chunks[static_cast<size_t>(std::min<int>(static_cast<int>(s / step), n - 1) / kChunk)].mesh;
      const float front = s + 2.6f;
      const Vec3 q0 = surfacePoint(front, lat - 1.3f) + Vec3{0, 0.01f, 0}, q1 = surfacePoint(front, lat + 1.3f) + Vec3{0, 0.01f, 0};
      const Vec3 q2 = surfacePoint(front + 0.25f, lat + 1.3f) + Vec3{0, 0.01f, 0}, q3 = surfacePoint(front + 0.25f, lat - 1.3f) + Vec3{0, 0.01f, 0};
      g.flatQuad(q0, q1, q2, q3, kWhite, kPaint);
    }
    // Start gantry over the line.
    const Vec3 left = surfacePoint(4, -c.halfWidth - 1.5f), right = surfacePoint(4, c.halfWidth + 1.5f);
    const Vec3 gray{0.3f, 0.3f, 0.32f};
    m.box(left + Vec3{0, 3.3f, 0}, {0.25f, 3.3f, 0.25f}, gray, kMetal);
    m.box(right + Vec3{0, 3.3f, 0}, {0.25f, 3.3f, 0.25f}, gray, kMetal);
    const Vec3 mid = (left + right) * 0.5f + Vec3{0, 6.3f, 0};
    const float span = engine::length(right - left) * 0.5f;
    const engine::Quat rot = engine::Quat::axisAngle({0, 1, 0}, engine::Vehicle::headingOf(c.tangent));
    m.box(mid, {span, 0.45f, 0.35f}, {0.08f, 0.08f, 0.09f}, kPlain, rot);
  }
  // Main straight dressing: grandstand on the left, pit building on the right.
  {
    for (int i = 40; i < 260; i += 2) {
      const Sample& c = samples_[static_cast<size_t>(i)];
      MeshBuilder& m = chunks[static_cast<size_t>(i / kChunk)].mesh;
      const engine::Quat rot = engine::Quat::axisAngle({0, 1, 0}, engine::Vehicle::headingOf(c.tangent));
      const Vec3 base = c.pos - c.side * (c.wallLeft + 3);
      for (int row = 0; row < 8; ++row) {  // stepped seating rising away from the track
        const float h = 0.6f + 0.55f * static_cast<float>(row);
        const Vec3 seat = (row + i / 2) % 3 == 0 ? Vec3{0.05f, 0.15f, 0.5f} : Vec3{0.6f, 0.6f, 0.62f};
        m.box(base - c.side * (static_cast<float>(row) * 0.9f) + Vec3{0, h * 0.5f, 0}, {0.45f, h * 0.5f, 1.05f}, seat,
              kPlain, rot);
      }
      if (i % 20 == 0) m.box(base - c.side * 7.6f + Vec3{0, 4.0f, 0}, {0.15f, 4.0f, 0.15f}, {0.3f, 0.3f, 0.3f}, kMetal, rot);
      m.box(base - c.side * 3.5f + Vec3{0, 8.2f, 0}, {4.6f, 0.12f, 1.05f}, {0.85f, 0.85f, 0.85f}, kPlain, rot);  // roof
      const Vec3 pit = c.pos + c.side * (c.wallRight + 8);
      m.box(pit + Vec3{0, 3.0f, 0}, {5.0f, 3.0f, 1.05f}, {0.75f, 0.74f, 0.7f}, kPlain, rot);
      if (i % 6 == 0) m.box(pit - c.side * 5.02f + Vec3{0, 1.6f, 0}, {0.02f, 1.5f, 0.8f}, {0.12f, 0.14f, 0.18f}, kMetal, rot);
    }
  }
  // Terrain beyond the barriers: rolling hills that settle down near the track.
  Chunk terrain;
  {
    Vec3 lo{1e9f, 0, 1e9f}, hi{-1e9f, 0, -1e9f};
    for (const Sample& c : samples_) {
      lo = {std::min(lo.x, c.pos.x), 0, std::min(lo.z, c.pos.z)};
      hi = {std::max(hi.x, c.pos.x), 0, std::max(hi.z, c.pos.z)};
    }
    const float margin = 450, cell = 20;
    const int nx = static_cast<int>((hi.x - lo.x + 2 * margin) / cell) + 1;
    const int nz = static_cast<int>((hi.z - lo.z + 2 * margin) / cell) + 1;
    std::vector<float> heights(static_cast<size_t>(nx * nz));
    int hint = -1;
    for (int zi = 0; zi < nz; ++zi) {
      for (int xi = 0; xi < nx; ++xi) {
        const float x = lo.x - margin + static_cast<float>(xi) * cell, z = lo.z - margin + static_cast<float>(zi) * cell;
        float s, lat;
        if (xi == 0) hint = -1;
        locate({x, 0, z}, s, lat, hint);
        const Sample c = at(s);
        const float wall = lat >= 0 ? c.wallRight : c.wallLeft;
        const float d = std::fabs(lat) - wall;
        const float hills = 6 + 7 * std::sin(x * 0.006f + 1.0f) * std::cos(z * 0.005f) + 3 * std::sin(x * 0.017f + z * 0.013f);
        const float t = engine::clampf((d - 25) / 140, 0, 1);
        const float near = c.pos.y - 2.5f;
        heights[static_cast<size_t>(zi * nx + xi)] = d < 25 ? near - 1.0f : near + (hills - near) * (t * t * (3 - 2 * t));
      }
    }
    MeshBuilder& m = terrain.mesh;
    const auto H = [&](int xi, int zi) { return heights[static_cast<size_t>(std::clamp(zi, 0, nz - 1) * nx + std::clamp(xi, 0, nx - 1))]; };
    for (int zi = 0; zi < nz; ++zi) {
      for (int xi = 0; xi < nx; ++xi) {
        const Vec3 p{lo.x - margin + static_cast<float>(xi) * cell, H(xi, zi), lo.z - margin + static_cast<float>(zi) * cell};
        const Vec3 nn = engine::normalize({H(xi - 1, zi) - H(xi + 1, zi), 2 * cell, H(xi, zi - 1) - H(xi, zi + 1)});
        const float shade = 0.85f + 0.3f * hash(static_cast<float>(xi), static_cast<float>(zi));
        m.vertex(p, nn, kGrassColor * shade, kGrass);
      }
    }
    for (int zi = 0; zi + 1 < nz; ++zi) {
      for (int xi = 0; xi + 1 < nx; ++xi) {
        const uint32_t a = static_cast<uint32_t>(zi * nx + xi);
        m.quad(a, a + static_cast<uint32_t>(nx), a + static_cast<uint32_t>(nx) + 1, a + 1);
      }
    }
    // Trees, scattered outside the barriers (with gaps), attached to the
    // nearest track chunk so they're culled with it.
    hint = -1;
    for (int k = 0; k < 2600; ++k) {
      const float x = lo.x - 250 + hash(static_cast<float>(k), 1.7f) * (hi.x - lo.x + 500);
      const float z = lo.z - 250 + hash(static_cast<float>(k), 9.3f) * (hi.z - lo.z + 500);
      float s, lat;
      locate({x, 0, z}, s, lat, hint);
      const Sample c = at(s);
      const float d = std::fabs(lat) - (lat >= 0 ? c.wallRight : c.wallLeft);
      if (d < 18 || d > 260) continue;
      if (std::sin(x * 0.011f) * std::cos(z * 0.013f) < -0.15f) continue;  // clearings
      const int xi = static_cast<int>((x - (lo.x - margin)) / cell), zi = static_cast<int>((z - (lo.z - margin)) / cell);
      const float fx = (x - (lo.x - margin)) / cell - static_cast<float>(xi), fz = (z - (lo.z - margin)) / cell - static_cast<float>(zi);
      const float y = H(xi, zi) * (1 - fx) * (1 - fz) + H(xi + 1, zi) * fx * (1 - fz) + H(xi, zi + 1) * (1 - fx) * fz +
                      H(xi + 1, zi + 1) * fx * fz;
      const float size = 0.7f + 0.7f * hash(static_cast<float>(k), 4.1f);
      const Vec3 base{x, y - 0.2f, z};
      const int idx = std::min(static_cast<int>(s / step), n - 1) / kChunk;
      MeshBuilder& tm = d < 120 ? chunks[static_cast<size_t>(idx)].mesh : terrain.mesh;
      tm.box(base + Vec3{0, 1.2f * size, 0}, {0.25f * size, 1.2f * size, 0.25f * size}, {0.18f, 0.11f, 0.06f}, kPlain);
      const Vec3 leaf = Vec3{0.05f, 0.13f, 0.04f} * (0.7f + 0.6f * hash(static_cast<float>(k), 2.2f));
      tm.cone(base + Vec3{0, 1.6f * size, 0}, 2.6f * size, 5.5f * size, 7, leaf, kFoliage);
      tm.cone(base + Vec3{0, 4.2f * size, 0}, 1.9f * size, 4.2f * size, 7, leaf * 1.15f, kFoliage);
    }
  }
  chunks.push_back(std::move(terrain));
  for (Chunk& c : chunks) {
    c.boundsMin = {1e9f, 1e9f, 1e9f};
    c.boundsMax = {-1e9f, -1e9f, -1e9f};
    for (const Vertex& v : c.mesh.vertices) {
      c.boundsMin = {std::min(c.boundsMin.x, v.position.x), std::min(c.boundsMin.y, v.position.y), std::min(c.boundsMin.z, v.position.z)};
      c.boundsMax = {std::max(c.boundsMax.x, v.position.x), std::max(c.boundsMax.y, v.position.y), std::max(c.boundsMax.z, v.position.z)};
    }
  }
  return chunks;
}

}  // namespace app::race
