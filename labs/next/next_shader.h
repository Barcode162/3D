#pragma once
// The bar on the new engine (J116.6): its one scene shader, written twice
// (WGSL for WebGPU, GLSL ES 3.0 for WebGL2), the same maths line for line:
//   light   = Σk u_k(x) · w_k                 (the baked room, any brightnesses)
//           + Σ4 lamp_i(x) · seen_i(x)        (small things: their four lamps per pixel)
//   surface = colour · picture / its average  (the picture's detail, the bake's brightness)
//   colour  = glow + surface · light / π, then the eye's curve (ACES) and sRGB.
// The lamps live in a small texture (4 texels a lamp: position + radius,
// colour · candela · brightness + shape, axis + one-sided, axis2), so a
// corner can name any of 64. Each pixel's facing is its corners' facing,
// blended (J118.3: round things shade smooth), turned towards the eye; where a
// model has none, the screen-space slope of its position. The white balance
// (group.yzw, J118.3) takes part of the bulbs' orange out before the curve.
// Glass (glow.w = 1, J119.1; drawn last, mixed over what's behind): a touch
// of its tint lit like a small thing, the room mirrored in it more at a slant
// (Schlick's Fresnel, 4% head on), and each of its four lamps as a sharp
// highlight (Blinn-Phong, normalised); its colour's alpha is how much it hides.
// Textures: 0 the light texture, 1 the picture, 2 the lamps.
namespace nx {

inline const char* kWgsl = R"WGSL(
struct U {
  viewProj: mat4x4f,
  eye: vec4f,          // xyz, exposure
  w: array<vec4f, 4>,
  group: vec4f,        // x: has a picture; yzw: the white balance
  picAvg: vec4f,       // the picture's average colour
  glow: vec4f,
};
@group(0) @binding(0) var<uniform> u: U;
@group(0) @binding(1) var tex0: texture_2d<f32>;
@group(0) @binding(2) var samp0: sampler;
@group(0) @binding(3) var tex1: texture_2d<f32>;
@group(0) @binding(4) var samp1: sampler;
@group(0) @binding(5) var tex2: texture_2d<f32>;
@group(0) @binding(6) var samp2: sampler;

struct VOut {
  @builtin(position) pos: vec4f,
  @location(0) world: vec3f,
  @location(1) uv: vec2f,
  @location(2) uv2: vec2f,
  @location(3) col: vec4f,
  @location(4) corner: vec4f,
  @location(5) @interpolate(flat) lamps: vec4u,
  @location(6) seen: vec4f,
  @location(7) normal: vec3f,
};

@vertex fn vs_main(@location(0) p: vec3f, @location(1) uv: vec2f, @location(2) uv2: vec2f, @location(3) col: vec4f,
                   @location(4) corner: vec4f, @location(5) lamps: vec4u, @location(6) seen: vec4f, @location(7) nrm: vec4f) -> VOut {
  var o: VOut;
  o.pos = u.viewProj * vec4f(p, 1.0);
  o.world = p; o.uv = uv; o.uv2 = uv2; o.col = col; o.corner = corner; o.lamps = lamps; o.seen = seen; o.normal = nrm.xyz;
  return o;
}

fn lampLux(i: u32, p: vec3f, n: vec3f) -> vec3f {
  let t0 = textureLoad(tex2, vec2i(0, i32(i)), 0);
  let t1 = textureLoad(tex2, vec2i(1, i32(i)), 0);
  let t2 = textureLoad(tex2, vec2i(2, i32(i)), 0);
  let t3 = textureLoad(tex2, vec2i(3, i32(i)), 0);
  let c = t0.xyz;
  let shape = t1.w;
  let ax = t2.xyz;
  let ax2 = t3.xyz;
  var q = c;
  var r = t0.w;
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
    emit = select(abs(e), max(e, 0.0), t2.w > 0.5);
  }
  return t1.rgb * (emit * cs / d2);
}

// A lamp's sharp highlight on a smooth surface (glass): its light from the
// lamp's nearest point, bounced towards the eye.
fn lampSpec(i: u32, p: vec3f, n: vec3f, view: vec3f, shininess: f32) -> vec3f {
  let t0 = textureLoad(tex2, vec2i(0, i32(i)), 0);
  let t1 = textureLoad(tex2, vec2i(1, i32(i)), 0);
  let t2 = textureLoad(tex2, vec2i(2, i32(i)), 0);
  let t3 = textureLoad(tex2, vec2i(3, i32(i)), 0);
  var q = t0.xyz;
  if (t1.w > 0.5) { q = q + t2.xyz * clamp(dot(p - t0.xyz, t2.xyz) / max(dot(t2.xyz, t2.xyz), 1e-6), -1.0, 1.0); }
  if (t1.w > 1.5) { q = q + t3.xyz * clamp(dot(p - t0.xyz, t3.xyz) / max(dot(t3.xyz, t3.xyz), 1e-6), -1.0, 1.0); }
  let d = q - p;
  let d2 = max(dot(d, d), 0.01);
  let l = d * inverseSqrt(d2);
  let h = normalize(l + view);
  let cs = max(dot(n, l), 0.0);
  let f = 0.04 + 0.96 * pow(1.0 - max(dot(h, view), 0.0), 5.0);
  return t1.rgb * (f * (shininess + 8.0) / 25.1327 * pow(max(dot(n, h), 0.0), shininess) * cs / d2);
}

fn aces(x: vec3f) -> vec3f { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), vec3f(0.0), vec3f(1.0)); }
fn srgb(x: vec3f) -> vec3f { return select(1.055 * pow(x, vec3f(1.0 / 2.4)) - 0.055, 12.92 * x, x <= vec3f(0.0031308)); }

@fragment fn fs_main(v: VOut) -> @location(0) vec4f {
  var n = normalize(cross(dpdx(v.world), dpdy(v.world)));
  if (dot(v.normal, v.normal) > 0.25) { n = normalize(v.normal); }
  if (dot(n, u.eye.xyz - v.world) < 0.0) { n = -n; }
  let t = textureSampleLevel(tex0, samp0, max(v.uv2, vec2f(0.0)), 0.0);
  let pic = textureSample(tex1, samp1, v.uv).rgb;
  var e = vec3f(0.0);
  let glass = u.glow.w > 0.5;
  if (v.col.a > 0.5 && !glass) {
    e = t.x * u.w[0].rgb + t.y * u.w[1].rgb + t.z * u.w[2].rgb + t.w * u.w[3].rgb;
  } else {
    e = v.corner.x * u.w[0].rgb + v.corner.y * u.w[1].rgb + v.corner.z * u.w[2].rgb + v.corner.w * u.w[3].rgb;
    let at = v.world + n * 0.002;
    e += lampLux(v.lamps.x, at, n) * v.seen.x + lampLux(v.lamps.y, at, n) * v.seen.y;
    e += lampLux(v.lamps.z, at, n) * v.seen.z + lampLux(v.lamps.w, at, n) * v.seen.w;
  }
  var surface = v.col.rgb;
  if (u.group.x > 0.5) { surface = surface * pic / max(u.picAvg.rgb, vec3f(0.02)); }
  if (glass) {
    let view = normalize(u.eye.xyz - v.world);
    let f = 0.04 + 0.96 * pow(1.0 - max(dot(n, view), 0.0), 5.0);
    let bounce = max(v.corner.x * u.w[0].rgb + v.corner.y * u.w[1].rgb + v.corner.z * u.w[2].rgb + v.corner.w * u.w[3].rgb, vec3f(0.0));
    let at = v.world + n * 0.002;
    var hl = lampSpec(v.lamps.x, at, n, view, 300.0) * v.seen.x + lampSpec(v.lamps.y, at, n, view, 300.0) * v.seen.y;
    hl += lampSpec(v.lamps.z, at, n, view, 300.0) * v.seen.z + lampSpec(v.lamps.w, at, n, view, 300.0) * v.seen.w;
    let lit = surface * max(e, vec3f(0.0)) / 3.14159265 * v.col.a * 0.35 + f * bounce / 3.14159265 + hl;
    let a = clamp(v.col.a * (1.0 - f) + f, 0.0, 1.0);
    return vec4f(srgb(aces(lit * u.group.yzw * u.eye.w)), a);
  }
  let nits = u.glow.rgb + surface * max(e, vec3f(0.0)) / 3.14159265;
  return vec4f(srgb(aces(nits * u.group.yzw * u.eye.w)), 1.0);
}
)WGSL";

inline const char* kGlsl = R"GLSL(#version 300 es
precision highp float;
layout(std140) uniform U {
  mat4 viewProj;
  vec4 eye;
  vec4 w[4];
  vec4 group;
  vec4 picAvg;
  vec4 glow;
};
layout(location = 0) in vec3 p;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 uv2;
layout(location = 3) in vec4 col;
layout(location = 4) in vec4 corner;
layout(location = 5) in uvec4 lamps;
layout(location = 6) in vec4 seen;
layout(location = 7) in vec4 nrm;
out vec3 vWorld; out vec2 vUv; out vec2 vUv2; out vec4 vCol; out vec4 vCorner; flat out uvec4 vLamps; out vec4 vSeen; out vec3 vNormal;
void main() {
  gl_Position = viewProj * vec4(p, 1.0);
  vWorld = p; vUv = uv; vUv2 = uv2; vCol = col; vCorner = corner; vLamps = lamps; vSeen = seen; vNormal = nrm.xyz;
}
//--- fragment
#version 300 es
precision highp float;
precision highp int;
layout(std140) uniform U {
  mat4 viewProj;
  vec4 eye;
  vec4 w[4];
  vec4 group;
  vec4 picAvg;
  vec4 glow;
};
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
in vec3 vWorld; in vec2 vUv; in vec2 vUv2; in vec4 vCol; in vec4 vCorner; flat in uvec4 vLamps; in vec4 vSeen; in vec3 vNormal;
out vec4 color;

vec3 lampLux(uint i, vec3 p, vec3 n) {
  vec4 t0 = texelFetch(tex2, ivec2(0, int(i)), 0);
  vec4 t1 = texelFetch(tex2, ivec2(1, int(i)), 0);
  vec4 t2 = texelFetch(tex2, ivec2(2, int(i)), 0);
  vec4 t3 = texelFetch(tex2, ivec2(3, int(i)), 0);
  vec3 c = t0.xyz;
  float shape = t1.w;
  vec3 ax = t2.xyz, ax2 = t3.xyz;
  vec3 q = c;
  float r = t0.w;
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
    emit = t2.w > 0.5 ? max(e, 0.0) : abs(e);
  }
  return t1.rgb * (emit * cs / d2);
}
vec3 lampSpec(uint i, vec3 p, vec3 n, vec3 view, float shininess) {
  vec4 t0 = texelFetch(tex2, ivec2(0, int(i)), 0);
  vec4 t1 = texelFetch(tex2, ivec2(1, int(i)), 0);
  vec4 t2 = texelFetch(tex2, ivec2(2, int(i)), 0);
  vec4 t3 = texelFetch(tex2, ivec2(3, int(i)), 0);
  vec3 q = t0.xyz;
  if (t1.w > 0.5) q = q + t2.xyz * clamp(dot(p - t0.xyz, t2.xyz) / max(dot(t2.xyz, t2.xyz), 1e-6), -1.0, 1.0);
  if (t1.w > 1.5) q = q + t3.xyz * clamp(dot(p - t0.xyz, t3.xyz) / max(dot(t3.xyz, t3.xyz), 1e-6), -1.0, 1.0);
  vec3 d = q - p;
  float d2 = max(dot(d, d), 0.01);
  vec3 l = d * inversesqrt(d2);
  vec3 h = normalize(l + view);
  float cs = max(dot(n, l), 0.0);
  float f = 0.04 + 0.96 * pow(1.0 - max(dot(h, view), 0.0), 5.0);
  return t1.rgb * (f * (shininess + 8.0) / 25.1327 * pow(max(dot(n, h), 0.0), shininess) * cs / d2);
}
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
vec3 srgb(vec3 x) { return mix(1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055, 12.92 * x, vec3(lessThanEqual(x, vec3(0.0031308)))); }

void main() {
  vec3 n = normalize(cross(dFdx(vWorld), dFdy(vWorld)));
  if (dot(vNormal, vNormal) > 0.25) n = normalize(vNormal);
  if (dot(n, eye.xyz - vWorld) < 0.0) n = -n;
  vec4 t = textureLod(tex0, max(vUv2, vec2(0.0)), 0.0);
  vec3 pic = texture(tex1, vUv).rgb;
  vec3 e = vec3(0.0);
  bool glass = glow.w > 0.5;
  if (vCol.a > 0.5 && !glass) {
    e = t.x * w[0].rgb + t.y * w[1].rgb + t.z * w[2].rgb + t.w * w[3].rgb;
  } else {
    e = vCorner.x * w[0].rgb + vCorner.y * w[1].rgb + vCorner.z * w[2].rgb + vCorner.w * w[3].rgb;
    vec3 at = vWorld + n * 0.002;
    e += lampLux(vLamps.x, at, n) * vSeen.x + lampLux(vLamps.y, at, n) * vSeen.y;
    e += lampLux(vLamps.z, at, n) * vSeen.z + lampLux(vLamps.w, at, n) * vSeen.w;
  }
  vec3 surface = vCol.rgb;
  if (group.x > 0.5) surface = surface * pic / max(picAvg.rgb, vec3(0.02));
  if (glass) {
    vec3 view = normalize(eye.xyz - vWorld);
    float f = 0.04 + 0.96 * pow(1.0 - max(dot(n, view), 0.0), 5.0);
    vec3 bounce = max(vCorner.x * w[0].rgb + vCorner.y * w[1].rgb + vCorner.z * w[2].rgb + vCorner.w * w[3].rgb, vec3(0.0));
    vec3 at = vWorld + n * 0.002;
    vec3 hl = lampSpec(vLamps.x, at, n, view, 300.0) * vSeen.x + lampSpec(vLamps.y, at, n, view, 300.0) * vSeen.y;
    hl += lampSpec(vLamps.z, at, n, view, 300.0) * vSeen.z + lampSpec(vLamps.w, at, n, view, 300.0) * vSeen.w;
    vec3 lit = surface * max(e, vec3(0.0)) / 3.14159265 * vCol.a * 0.35 + f * bounce / 3.14159265 + hl;
    float a = clamp(vCol.a * (1.0 - f) + f, 0.0, 1.0);
    color = vec4(srgb(aces(lit * group.yzw * eye.w)), a);
    return;
  }
  vec3 nits = glow.rgb + surface * max(e, vec3(0.0)) / 3.14159265;
  color = vec4(srgb(aces(nits * group.yzw * eye.w)), 1.0);
}
)GLSL";

}  // namespace nx
