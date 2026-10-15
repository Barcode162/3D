#pragma once
// Small 3D math library: vectors, 4x4 matrices, quaternions.
// Conventions: right-handed world, +Y up, camera looks down -Z.
// Matrices are column-major (m[col * 4 + row]), matching WGSL mat4x4f.
#include <cmath>

namespace engine {

constexpr float kPi = 3.14159265358979f;

inline float radians(float degrees) { return degrees * kPi / 180.0f; }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct Vec2 {
  float x = 0, y = 0;
};

struct Vec3 {
  float x = 0, y = 0, z = 0;

  Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator-() const { return {-x, -y, -z}; }
  Vec3& operator+=(Vec3 o) { return *this = *this + o; }
};

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(Vec3 v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(Vec3 v) {
  const float len = length(v);
  return len > 0 ? v * (1.0f / len) : v;
}

struct Vec4 {
  float x = 0, y = 0, z = 0, w = 0;
};

struct Quat {
  float x = 0, y = 0, z = 0, w = 1;

  static Quat axisAngle(Vec3 axis, float angle) {
    const Vec3 a = normalize(axis) * std::sin(angle * 0.5f);
    return {a.x, a.y, a.z, std::cos(angle * 0.5f)};
  }
  Quat operator*(Quat o) const {
    return {w * o.x + x * o.w + y * o.z - z * o.y, w * o.y - x * o.z + y * o.w + z * o.x,
            w * o.z + x * o.y - y * o.x + z * o.w, w * o.w - x * o.x - y * o.y - z * o.z};
  }
  Vec3 rotate(Vec3 v) const {
    const Vec3 q{x, y, z};
    const Vec3 t = cross(q, v) * 2.0f;
    return v + t * w + cross(q, t);
  }
};

struct Mat4 {
  float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  float& at(int col, int row) { return m[col * 4 + row]; }
  float at(int col, int row) const { return m[col * 4 + row]; }

  Mat4 operator*(const Mat4& o) const {
    Mat4 r;
    for (int c = 0; c < 4; ++c) {
      for (int rw = 0; rw < 4; ++rw) {
        float sum = 0;
        for (int k = 0; k < 4; ++k) sum += at(k, rw) * o.at(c, k);
        r.at(c, rw) = sum;
      }
    }
    return r;
  }

  static Mat4 translation(Vec3 t) {
    Mat4 r;
    r.at(3, 0) = t.x;
    r.at(3, 1) = t.y;
    r.at(3, 2) = t.z;
    return r;
  }

  static Mat4 rotation(Quat q) {
    Mat4 r;
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    r.at(0, 0) = 1 - 2 * (yy + zz);
    r.at(0, 1) = 2 * (xy + wz);
    r.at(0, 2) = 2 * (xz - wy);
    r.at(1, 0) = 2 * (xy - wz);
    r.at(1, 1) = 1 - 2 * (xx + zz);
    r.at(1, 2) = 2 * (yz + wx);
    r.at(2, 0) = 2 * (xz + wy);
    r.at(2, 1) = 2 * (yz - wx);
    r.at(2, 2) = 1 - 2 * (xx + yy);
    return r;
  }

  // Translation, then rotation: places an object in the world.
  static Mat4 transform(Vec3 position, Quat rot = {}) { return translation(position) * rotation(rot); }

  // WebGPU clip space has depth in [0, 1].
  static Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
    Mat4 r;
    const float f = 1.0f / std::tan(fovY * 0.5f);
    r.at(0, 0) = f / aspect;
    r.at(1, 1) = f;
    r.at(2, 2) = zFar / (zNear - zFar);
    r.at(2, 3) = -1;
    r.at(3, 2) = zNear * zFar / (zNear - zFar);
    r.at(3, 3) = 0;
    return r;
  }

  static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 f = normalize(target - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);
    Mat4 r;
    r.at(0, 0) = s.x;  r.at(1, 0) = s.y;  r.at(2, 0) = s.z;
    r.at(0, 1) = u.x;  r.at(1, 1) = u.y;  r.at(2, 1) = u.z;
    r.at(0, 2) = -f.x; r.at(1, 2) = -f.y; r.at(2, 2) = -f.z;
    r.at(3, 0) = -dot(s, eye);
    r.at(3, 1) = -dot(u, eye);
    r.at(3, 2) = dot(f, eye);
    return r;
  }
};

}  // namespace engine
