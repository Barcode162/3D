#pragma once
// The baked room as the labs draw it (J116.4, J116.5): reading the baker's
// scene.bin (tools/native/bake_tool.cpp writeScene), and filling the scene
// shader's uniform block (labs/draw/scene_shader.h) from the lights'
// brightnesses, the camera and the things that move. Header only; used by
// the draw lab and the physics lab.
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "looped/math.h"

namespace lab {

using engine::Quat;
using engine::Vec3;

// The uniform block, std140 (both shaders declare the same fields).
struct SceneUniforms {
  float viewProj[16];
  float eye[4];  // xyz, exposure
  float w[4][4];
  float lampPos[16][4];
  float lampCol[16][4];
  float lampAx[16][4];
  float lampAx2[16][4];
  // Up to two things that move (J116.5): turn (quaternion), shift (xyz; w
  // = 1 while it's lit live), the bounce reaching it from six ways, and how
  // much of each lamp it sees (4 lamps a vec4).
  float objTurn[2][4];
  float objShift[2][4];
  float objCube[12][4];
  float objSeen[8][4];
};
static_assert(sizeof(SceneUniforms) == 1552, "the std140 block");

struct LampInfo {
  Vec3 pos, color, axis, axis2;
  float cd = 0, radius = 0, shape = 0, oneSided = 0;
  int channel = 0;
};

struct BakedRoom {
  uint32_t vertexCount = 0;
  std::vector<uint8_t> vertices;  // 72 bytes each (the layout in draw_lab.cpp / scene_shader.h)
  std::vector<uint32_t> indices;
  int atlasW = 0, atlasH = 0;
  std::vector<uint16_t> atlas;  // RGBA16F
  int layers = 0, channels = 0;
  std::vector<float> basis;  // layers x 3·channels
  std::vector<LampInfo> lamps;
  std::vector<std::string> ids;  // per channel: its light's thing id
};

constexpr size_t kVertexBytes = 72;

template <class T>
T take(const uint8_t*& p) {
  T v;
  std::memcpy(&v, p, sizeof v);
  p += sizeof v;
  return v;
}

inline bool parseBakedRoom(const uint8_t* data, size_t size, BakedRoom& r) {
  if (size < 36 || std::memcmp(data, "LSCN", 4) != 0) return false;
  const uint8_t* p = data + 4;
  take<uint32_t>(p);  // version
  r.vertexCount = take<uint32_t>(p);
  const uint32_t ni = take<uint32_t>(p), nl = take<uint32_t>(p);
  r.layers = static_cast<int>(take<uint32_t>(p));
  r.channels = static_cast<int>(take<uint32_t>(p));
  r.atlasW = static_cast<int>(take<uint32_t>(p));
  r.atlasH = static_cast<int>(take<uint32_t>(p));
  const size_t vbytes = static_cast<size_t>(r.vertexCount) * kVertexBytes;
  const size_t need = 36 + vbytes + ni * 4ull + static_cast<size_t>(r.atlasW) * r.atlasH * 8;
  if (size < need) return false;
  r.vertices.assign(p, p + vbytes);
  p += vbytes;
  r.indices.resize(ni);
  std::memcpy(r.indices.data(), p, ni * 4ull);
  p += ni * 4ull;
  r.atlas.resize(static_cast<size_t>(r.atlasW) * r.atlasH * 4);
  std::memcpy(r.atlas.data(), p, r.atlas.size() * 2);
  p += r.atlas.size() * 2;
  r.basis.resize(static_cast<size_t>(r.layers) * 3 * r.channels);
  for (float& b : r.basis) b = take<float>(p);
  r.lamps.resize(nl);
  for (LampInfo& l : r.lamps) {
    float f[18];
    for (float& x : f) x = take<float>(p);
    l.pos = {f[0], f[1], f[2]}, l.color = {f[3], f[4], f[5]}, l.cd = f[6], l.radius = f[7], l.shape = f[8];
    l.axis = {f[9], f[10], f[11]}, l.axis2 = {f[12], f[13], f[14]}, l.oneSided = f[15], l.channel = static_cast<int>(f[16]);
  }
  for (int c = 0; c < r.channels; ++c) {
    const uint16_t n = take<uint16_t>(p);
    r.ids.emplace_back(reinterpret_cast<const char*>(p), n);
    p += n;
  }
  return static_cast<size_t>(p - data) <= size;
}

inline void perspective(float* m, float fovY, float aspect, float zn, float zf, bool zeroToOne) {
  const float f = 1 / std::tan(fovY / 2);
  std::memset(m, 0, 64);
  m[0] = f / aspect;
  m[5] = f;
  if (zeroToOne) m[10] = zf / (zn - zf), m[14] = zn * zf / (zn - zf);
  else m[10] = (zf + zn) / (zn - zf), m[14] = 2 * zf * zn / (zn - zf);
  m[11] = -1;
}

inline void mul(const float* a, const float* b, float* o) {  // column-major: o = a · b
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r) {
      float s = 0;
      for (int k = 0; k < 4; ++k) s += a[k * 4 + r] * b[c * 4 + k];
      o[c * 4 + r] = s;
    }
}

// The camera (yaw/pitch in degrees, as looped/camera.h: yaw 0 looks down -Z).
inline void camera(SceneUniforms& u, Vec3 eye, float yawDeg, float pitchDeg, float aspect, bool zeroToOne, float exposure) {
  const float yr = yawDeg * 0.017453292f, pr = pitchDeg * 0.017453292f;
  const Vec3 f{std::sin(yr) * std::cos(pr), std::sin(pr), -std::cos(yr) * std::cos(pr)};
  const Vec3 right = engine::normalize(engine::cross(f, {0, 1, 0})), up = engine::cross(right, f);
  float view[16] = {right.x, up.x, -f.x, 0, right.y, up.y, -f.y, 0, right.z, up.z, -f.z, 0,
                    -engine::dot(right, eye), -engine::dot(up, eye), engine::dot(f, eye), 1};
  float proj[16];
  perspective(proj, 70 * 0.017453292f, aspect, 0.05f, 50, zeroToOne);
  mul(proj, view, u.viewProj);
  u.eye[0] = eye.x, u.eye[1] = eye.y, u.eye[2] = eye.z, u.eye[3] = exposure;
}

// The lights at brightnesses `s` (one per channel): w_k = B_k · s, and each lamp's colour times its candela times s.
inline void lights(SceneUniforms& u, const BakedRoom& r, const std::vector<float>& s) {
  for (int k = 0; k < 4; ++k)
    for (int e = 0; e < 4; ++e) u.w[k][e] = 0;
  for (int k = 0; k < r.layers && k < 4; ++k)
    for (int c = 0; c < r.channels; ++c)
      for (int e = 0; e < 3; ++e) u.w[k][e] += r.basis[static_cast<size_t>(k * 3 * r.channels + 3 * c + e)] * s[static_cast<size_t>(c)];
  for (size_t i = 0; i < r.lamps.size() && i < 16; ++i) {
    const LampInfo& l = r.lamps[i];
    const float b = l.cd * s[static_cast<size_t>(l.channel)];
    u.lampPos[i][0] = l.pos.x, u.lampPos[i][1] = l.pos.y, u.lampPos[i][2] = l.pos.z, u.lampPos[i][3] = l.radius;
    u.lampCol[i][0] = l.color.x * b, u.lampCol[i][1] = l.color.y * b, u.lampCol[i][2] = l.color.z * b, u.lampCol[i][3] = l.shape;
    u.lampAx[i][0] = l.axis.x, u.lampAx[i][1] = l.axis.y, u.lampAx[i][2] = l.axis.z, u.lampAx[i][3] = l.oneSided;
    u.lampAx2[i][0] = l.axis2.x, u.lampAx2[i][1] = l.axis2.y, u.lampAx2[i][2] = l.axis2.z, u.lampAx2[i][3] = 0;
  }
}

}  // namespace lab
