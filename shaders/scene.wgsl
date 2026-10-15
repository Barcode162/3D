// Main scene shader: lit, solid-colored meshes with several point lights,
// emissive surfaces (neon, lamp bulbs), procedural textures, distance haze
// and tone mapping.
//
// Textures: meshes have no texture coordinates, so textures are projected
// "triplanar": along the object's own x, y and z axes, blended by which way
// the surface faces. Boxes get one clean projection per side.

const kMaxLights = 8u;

struct Light {
  position: vec4f,   // xyz = position, w = range in meters
  color: vec4f,      // rgb = color, w = intensity
};

struct Frame {
  viewProj: mat4x4f,
  cameraPos: vec4f,
  ambient: vec4f,     // rgb = ambient light color
  fog: vec4f,         // rgb = haze color, w = density
  lightCount: vec4u,  // x = number of active lights
  lights: array<Light, kMaxLights>,
  // Mirror views: drop anything behind the glass (dot(xyz, p) + w < 0).
  // The normal view uses (0, 0, 0, 1), which keeps everything.
  clipPlane: vec4f,
};

struct Object {
  model: mat4x4f,
  color: vec4f,       // rgb = base color
  material: vec4f,    // x = emissive strength, y = shininess (0..1),
                      // z = texture layer (< 0 = none), w = texture repeats per meter
};

@group(0) @binding(0) var<uniform> frame: Frame;
@group(0) @binding(1) var colorMaps: texture_2d_array<f32>;
@group(0) @binding(2) var bumpMaps: texture_2d_array<f32>;
@group(0) @binding(3) var mapSampler: sampler;
@group(1) @binding(0) var<uniform> object: Object;

// Skinned meshes (the avatar): one matrix per bone, set every frame.
const kMaxBones = 32u;
struct Bones {
  m: array<mat4x4f, kMaxBones>,
};
@group(2) @binding(0) var<uniform> bones: Bones;

struct VsIn {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
};

struct VsOut {
  @builtin(position) clip: vec4f,
  @location(0) worldPos: vec3f,
  @location(1) normal: vec3f,
  @location(2) localPos: vec3f,
  @location(3) localNormal: vec3f,
  @location(4) paint: vec4f,  // skinned only: rgb = color (linear), a = gloss
  @location(5) @interpolate(flat) layer: i32,  // skinned only: texture layer
};

@vertex
fn vs_main(in: VsIn) -> VsOut {
  let world = object.model * vec4f(in.position, 1.0);
  var out: VsOut;
  out.clip = frame.viewProj * world;
  out.worldPos = world.xyz;
  // Models only translate/rotate (sizes are baked into the mesh), so the
  // model matrix is safe for normals.
  out.normal = (object.model * vec4f(in.normal, 0.0)).xyz;
  out.localPos = in.position;
  out.localNormal = in.normal;
  out.paint = vec4f(1.0);
  out.layer = -1;
  return out;
}

struct SkinIn {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) paint: vec4f,     // rgb = color (display space), a = gloss
  @location(3) joints: vec4u,
  @location(4) weights: vec4f,
  @location(5) material: vec4u,  // x = texture layer + 1
};

@vertex
fn vs_skinned(in: SkinIn) -> VsOut {
  // Each vertex follows two bones, blended by its weights.
  let skin = bones.m[in.joints.x] * in.weights.x + bones.m[in.joints.y] * in.weights.y;
  let world = object.model * (skin * vec4f(in.position, 1.0));
  var out: VsOut;
  out.clip = frame.viewProj * world;
  out.worldPos = world.xyz;
  out.normal = (object.model * (skin * vec4f(in.normal, 0.0))).xyz;
  // Textures use the rest pose, so clothes patterns stay put as limbs move.
  out.localPos = in.position;
  out.localNormal = in.normal;
  out.paint = vec4f(pow(in.paint.rgb, vec3f(2.2)), in.paint.a);
  out.layer = i32(in.material.x) - 1;
  return out;
}

// Filmic tone mapping (ACES approximation): keeps bright lights from clipping.
fn tonemap(x: vec3f) -> vec3f {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), vec3f(0.0), vec3f(1.0));
}

struct Surface {
  color: vec3f,
  gloss: f32,       // multiplier for shininess
  normal: vec3f,    // object space
};

// Samples one projection: color (x2 so 0.5 = unchanged) and the slope.
fn sampleColor(uv: vec2f, layer: i32) -> vec4f {
  return textureSample(colorMaps, mapSampler, uv, layer) * 2.0;
}
fn sampleSlope(uv: vec2f, layer: i32) -> vec2f {
  return textureSample(bumpMaps, mapSampler, uv, layer).rg * 2.0 - 1.0;
}

fn texturedSurface(p: vec3f, n0: vec3f) -> Surface {
  let n = normalize(n0);
  let layer = i32(object.material.z);
  let q = p * object.material.w;
  // Blend weights: sharpened so mostly one projection wins.
  var w = pow(abs(n), vec3f(4.0));
  w = w / (w.x + w.y + w.z);
  // Projections: X sides use (z, y), top/bottom use (x, z), Z sides use (x, y).
  // u always runs horizontally, so wood grain and bricks lie flat.
  let cx = sampleColor(q.zy, layer);
  let cy = sampleColor(q.xz, layer);
  let cz = sampleColor(q.xy, layer);
  let sx = sampleSlope(q.zy, layer);
  let sy = sampleSlope(q.xz, layer);
  let sz = sampleSlope(q.xy, layer);
  let c = cx * w.x + cy * w.y + cz * w.z;
  // Tilt the normal by the height slope along each projection's u/v axes.
  let tilt = w.x * vec3f(0.0, sx.y, sx.x) + w.y * vec3f(sy.x, 0.0, sy.y) + w.z * vec3f(sz.x, sz.y, 0.0);
  return Surface(c.rgb, c.a, normalize(n + tilt));
}

@fragment
fn fs_main(in: VsOut) -> @location(0) vec4f {
  if (dot(frame.clipPlane.xyz, in.worldPos) + frame.clipPlane.w < 0.0) {
    discard;
  }
  var base = object.color.rgb;
  var shininess = object.material.y;
  var n = normalize(in.normal);
  if (object.material.z >= 0.0) {
    let s = texturedSurface(in.localPos, in.localNormal);
    base = base * s.color;
    shininess = clamp(shininess * s.gloss, 0.0, 1.0);
    n = normalize((object.model * vec4f(s.normal, 0.0)).xyz);
  }
  return shade(in.worldPos, n, base, shininess, object.material.x);
}

// Skinned meshes: color and gloss come from each vertex; parts marked with a
// texture layer (clothes) get that texture's pattern.
@fragment
fn fs_skinned(in: VsOut) -> @location(0) vec4f {
  if (dot(frame.clipPlane.xyz, in.worldPos) + frame.clipPlane.w < 0.0) {
    discard;
  }
  // Always sample (texture reads must not sit behind per-pixel branches),
  // then only use the result where the vertex asks for a texture.
  let n0 = normalize(in.localNormal);
  var w = pow(abs(n0), vec3f(4.0));
  w = w / (w.x + w.y + w.z);
  let q = in.localPos * object.material.w;
  let layer = max(in.layer, 0);
  let c = sampleColor(q.zy, layer) * w.x + sampleColor(q.xz, layer) * w.y + sampleColor(q.xy, layer) * w.z;
  var base = object.color.rgb * in.paint.rgb;
  var shininess = in.paint.a;
  if (in.layer >= 0) {
    base = base * c.rgb;
    shininess = clamp(shininess * c.a, 0.0, 1.0);
  }
  return shade(in.worldPos, normalize(in.normal), base, shininess, object.material.x);
}

// Lighting shared by every mesh: point lights, ambient, haze, tone mapping.
fn shade(worldPos: vec3f, n: vec3f, base: vec3f, shininess: f32, emissive: f32) -> vec4f {
  let toEye = normalize(frame.cameraPos.xyz - worldPos);

  var light = frame.ambient.rgb * (0.6 + 0.4 * n.y);  // a little brighter on top
  var specular = vec3f(0.0);
  for (var i = 0u; i < frame.lightCount.x; i++) {
    let l = frame.lights[i];
    let toLight = l.position.xyz - worldPos;
    let dist = length(toLight);
    let dir = toLight / dist;
    // Inverse-square falloff that fades smoothly to zero at the light's range.
    let window = clamp(1.0 - pow(dist / l.position.w, 4.0), 0.0, 1.0);
    let atten = window * window / (dist * dist + 0.25);
    let radiance = l.color.rgb * l.color.w * atten;
    light += radiance * max(dot(n, dir), 0.0);
    let h = normalize(dir + toEye);
    let power = mix(8.0, 128.0, shininess);
    specular += radiance * pow(max(dot(n, h), 0.0), power) * shininess;
  }

  var color = base * light + specular + base * emissive;

  let dist = length(frame.cameraPos.xyz - worldPos);
  let haze = 1.0 - exp(-dist * frame.fog.w);
  color = mix(color, frame.fog.rgb, haze);

  // The surface is a plain (non-sRGB) format, so apply gamma here.
  return vec4f(pow(tonemap(color), vec3f(1.0 / 2.2)), 1.0);
}
