#pragma once
// The new frame's one scene shader (J116.4), written twice: WGSL for
// WebGPU and GLSL ES 3.0 for WebGL2, line for line the same maths
// (docs/plan/jobs/J116.md → The light: one formula):
//   light = Σk u_k(x) · w_k              (the room's baked light, any brightnesses)
//         + Σ4 lamp_i(x) · seen_i(x)     (small things: their four lamps, worked out per pixel)
//   colour = glow + surface · light / π, then the eye's curve (ACES) and sRGB.
// The uniform block (std140, 1168 bytes) is SceneUniforms in draw_lab.cpp.
namespace lab {

inline const char* kSceneWgsl = R"WGSL(
struct U {
  viewProj: mat4x4f,
  eye: vec4f,
  w: array<vec4f, 4>,
  lampPos: array<vec4f, 16>,
  lampCol: array<vec4f, 16>,
  lampAx: array<vec4f, 16>,
  lampAx2: array<vec4f, 16>,
};
@group(0) @binding(0) var<uniform> u: U;
@group(0) @binding(1) var tex0: texture_2d<f32>;
@group(0) @binding(2) var samp0: sampler;

struct VOut {
  @builtin(position) pos: vec4f,
  @location(0) world: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) albedo: vec4f,
  @location(4) glow: vec3f,
  @location(5) corner: vec4f,
  @location(6) @interpolate(flat) lamps: vec4u,
  @location(7) seen: vec4f,
};

@vertex fn vs_main(@location(0) p: vec3f, @location(1) n: vec3f, @location(2) uv: vec2f, @location(3) albedo: vec4f,
                   @location(4) glow: vec3f, @location(5) corner: vec4f, @location(6) lamps: vec4u, @location(7) seen: vec4f) -> VOut {
  var o: VOut;
  o.pos = u.viewProj * vec4f(p, 1.0);
  o.world = p; o.normal = n; o.uv = uv; o.albedo = albedo; o.glow = glow; o.corner = corner; o.lamps = lamps; o.seen = seen;
  return o;
}

fn lampLux(i: u32, p: vec3f, n: vec3f) -> vec3f {
  let c = u.lampPos[i].xyz;
  let shape = u.lampCol[i].w;
  let ax = u.lampAx[i].xyz;
  let ax2 = u.lampAx2[i].xyz;
  var q = c;
  var r = u.lampPos[i].w;
  if (shape > 0.5) {
    q = c + ax * clamp(dot(p - c, ax) / max(dot(ax, ax), 1e-6), -1.0, 1.0);
    r = min(r, 0.05);
  }
  if (shape > 1.5) { q = q + ax2 * clamp(dot(p - c, ax2) / max(dot(ax2, ax2), 1e-6), -1.0, 1.0); }
  let d = q - p;
  let d2 = max(dot(d, d), r * r);
  let dir = d * inverseSqrt(max(dot(d, d), 1e-8));
  let cs = max(dot(n, dir), 0.0);
  var emit = 1.0;
  if (shape > 1.5) {
    let pn = normalize(cross(ax, ax2));
    let e = dot(-dir, pn);
    emit = select(abs(e), max(e, 0.0), u.lampAx[i].w > 0.5);
  }
  return u.lampCol[i].rgb * (emit * cs / d2);
}

fn aces(x: vec3f) -> vec3f { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), vec3f(0.0), vec3f(1.0)); }
fn srgb(x: vec3f) -> vec3f { return select(1.055 * pow(x, vec3f(1.0 / 2.4)) - 0.055, 12.92 * x, x <= vec3f(0.0031308)); }

@fragment fn fs_main(v: VOut, @builtin(front_facing) front: bool) -> @location(0) vec4f {
  var n = normalize(v.normal);
  if (dot(n, u.eye.xyz - v.world) < 0.0) { n = -n; }
  var e = vec3f(0.0);
  let t = textureSampleLevel(tex0, samp0, max(v.uv, vec2f(0.0)), 0.0);
  if (v.albedo.a > 0.5) {
    e = t.x * u.w[0].rgb + t.y * u.w[1].rgb + t.z * u.w[2].rgb + t.w * u.w[3].rgb;
  } else {
    e = v.corner.x * u.w[0].rgb + v.corner.y * u.w[1].rgb + v.corner.z * u.w[2].rgb + v.corner.w * u.w[3].rgb;
    let at = v.world + n * 0.002;
    e += lampLux(v.lamps.x, at, n) * v.seen.x + lampLux(v.lamps.y, at, n) * v.seen.y;
    e += lampLux(v.lamps.z, at, n) * v.seen.z + lampLux(v.lamps.w, at, n) * v.seen.w;
  }
  let nits = v.glow + v.albedo.rgb * max(e, vec3f(0.0)) / 3.14159265;
  return vec4f(srgb(aces(nits * u.eye.w)), 1.0);
}
)WGSL";

inline const char* kSceneGlsl = R"GLSL(#version 300 es
precision highp float;
layout(std140) uniform U {
  mat4 viewProj;
  vec4 eye;
  vec4 w[4];
  vec4 lampPos[16];
  vec4 lampCol[16];
  vec4 lampAx[16];
  vec4 lampAx2[16];
};
layout(location = 0) in vec3 p;
layout(location = 1) in vec3 n;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 albedo;
layout(location = 4) in vec3 glow;
layout(location = 5) in vec4 corner;
layout(location = 6) in uvec4 lamps;
layout(location = 7) in vec4 seen;
out vec3 vWorld; out vec3 vNormal; out vec2 vUv; out vec4 vAlbedo; out vec3 vGlow; out vec4 vCorner;
flat out uvec4 vLamps; out vec4 vSeen;
void main() {
  gl_Position = viewProj * vec4(p, 1.0);
  vWorld = p; vNormal = n; vUv = uv; vAlbedo = albedo; vGlow = glow; vCorner = corner; vLamps = lamps; vSeen = seen;
}
//--- fragment
#version 300 es
precision highp float;
precision highp int;
layout(std140) uniform U {
  mat4 viewProj;
  vec4 eye;
  vec4 w[4];
  vec4 lampPos[16];
  vec4 lampCol[16];
  vec4 lampAx[16];
  vec4 lampAx2[16];
};
uniform sampler2D tex0;
in vec3 vWorld; in vec3 vNormal; in vec2 vUv; in vec4 vAlbedo; in vec3 vGlow; in vec4 vCorner;
flat in uvec4 vLamps; in vec4 vSeen;
out vec4 color;

vec3 lampLux(uint i, vec3 p, vec3 n) {
  vec3 c = lampPos[i].xyz;
  float shape = lampCol[i].w;
  vec3 ax = lampAx[i].xyz, ax2 = lampAx2[i].xyz;
  vec3 q = c;
  float r = lampPos[i].w;
  if (shape > 0.5) {
    q = c + ax * clamp(dot(p - c, ax) / max(dot(ax, ax), 1e-6), -1.0, 1.0);
    r = min(r, 0.05);
  }
  if (shape > 1.5) q = q + ax2 * clamp(dot(p - c, ax2) / max(dot(ax2, ax2), 1e-6), -1.0, 1.0);
  vec3 d = q - p;
  float d2 = max(dot(d, d), r * r);
  vec3 dir = d * inversesqrt(max(dot(d, d), 1e-8));
  float cs = max(dot(n, dir), 0.0);
  float emit = 1.0;
  if (shape > 1.5) {
    vec3 pn = normalize(cross(ax, ax2));
    float e = dot(-dir, pn);
    emit = lampAx[i].w > 0.5 ? max(e, 0.0) : abs(e);
  }
  return lampCol[i].rgb * (emit * cs / d2);
}
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
vec3 srgb(vec3 x) { return mix(1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055, 12.92 * x, vec3(lessThanEqual(x, vec3(0.0031308)))); }

void main() {
  vec3 nn = normalize(vNormal);
  if (dot(nn, eye.xyz - vWorld) < 0.0) nn = -nn;
  vec3 e = vec3(0.0);
  vec4 t = textureLod(tex0, max(vUv, vec2(0.0)), 0.0);
  if (vAlbedo.a > 0.5) {
    e = t.x * w[0].rgb + t.y * w[1].rgb + t.z * w[2].rgb + t.w * w[3].rgb;
  } else {
    e = vCorner.x * w[0].rgb + vCorner.y * w[1].rgb + vCorner.z * w[2].rgb + vCorner.w * w[3].rgb;
    vec3 at = vWorld + nn * 0.002;
    e += lampLux(vLamps.x, at, nn) * vSeen.x + lampLux(vLamps.y, at, nn) * vSeen.y;
    e += lampLux(vLamps.z, at, nn) * vSeen.z + lampLux(vLamps.w, at, nn) * vSeen.w;
  }
  vec3 nits = vGlow + vAlbedo.rgb * max(e, vec3(0.0)) / 3.14159265;
  color = vec4(srgb(aces(nits * eye.w)), 1.0);
}
)GLSL";

}  // namespace lab
