// Main scene shader: physically based lighting (metal/roughness, like glTF)
// from several point lights, emissive surfaces (neon, lamp bulbs),
// procedural textures, distance haze and tone mapping.
//
// Textures: meshes have no texture coordinates, so textures are projected
// "triplanar": along the object's own x, y and z axes, blended by which way
// the surface faces. Boxes get one clean projection per side.

const kMaxLights = 32u;
const kMaxGridCells = 256u;

struct Light {
  position: vec4f,   // xyz = position, w = range in meters
  color: vec4f,      // rgb = color, w = intensity
  params: vec4f,     // x = radius of the glowing part
  axis: vec4f,       // xyz = half a tube's length / a panel's first side; w = shape (0 ball, 1 tube, 2 panel)
  axis2: vec4f,      // xyz = half a panel's second side
};

struct Frame {
  viewProj: mat4x4f,
  cameraPos: vec4f,
  ambient: vec4f,     // rgb = ambient light color, w = most lights per pixel (0 = all listed)
  fog: vec4f,         // rgb = haze color, w = density
  // Mirror views: drop anything behind the glass (dot(xyz, p) + w < 0).
  // The normal view uses (0, 0, 0, 1), which keeps everything.
  clipPlane: vec4f,  // unused: mirrors cut with an oblique near plane instead (no discard, keeps Apple GPUs fast)
  // Reflections (E4.4): x = 1 while capturing the cube (store light, not
  // display colour), y = 1 once the cube is ready, z = its last mip level.
  // w = 1: classic lighting (before L1), the picture leaves here tone mapped.
  env: vec4f,
  envMin: vec4f,     // the room's box: reflections are matched to it
  envMax: vec4f,
  envCenter: vec4f,  // where the cube was captured from
  // New lighting (L1.2): x = cd/m² per unit of a surface's own glow,
  // y = ambient fill scale, z = 1 / pi.
  units: vec4f,
  // Bounced light (L1.5, looped/gi.h): xyz = probe (0,0,0), w = 1 / spacing;
  // gi2: probes along x, y, z, w = how much of it to use (0 = none yet).
  gi: vec4f,
  gi2: vec4f,
};

fn classicLight() -> bool { return frame.env.w > 0.5; }

// All the lights, and a grid over the floor: each cell lists the (at most
// 8) lights that reach it, so a pixel only adds up the lights near it.
struct Lighting {
  lightCount: vec4u,  // x = number of lights
  grid: vec4f,        // x, y = where cell (0, 0) starts (world x, z), z = 1 / cell size
  gridSize: vec4u,    // x, y = cells along x and z
  lights: array<Light, kMaxLights>,
  cells: array<vec4u, kMaxGridCells>,  // x, y = 8 light indices (one per byte), z = count
};

struct Object {
  model: mat4x4f,
  color: vec4f,       // rgb = base color, a = metallic (0..1)
  material: vec4f,    // x = emissive strength, y = roughness (0..1),
                      // z = texture layer (< 0 = none), w = texture repeats per meter
};

@group(0) @binding(0) var<uniform> frame: Frame;
@group(0) @binding(1) var colorMaps: texture_2d_array<f32>;
@group(0) @binding(2) var bumpMaps: texture_2d_array<f32>;
@group(0) @binding(3) var mapSampler: sampler;
@group(0) @binding(4) var<uniform> lighting: Lighting;
@group(0) @binding(5) var envMap: texture_cube<f32>;
@group(0) @binding(6) var envSampler: sampler;
// Bounced light: each probe's light from every direction (spherical
// harmonics, one texture per colour) and which lamps it sees (bits).
@group(0) @binding(7) var giR: texture_3d<f32>;
@group(0) @binding(8) var giG: texture_3d<f32>;
@group(0) @binding(9) var giB: texture_3d<f32>;
@group(0) @binding(10) var giMask: texture_3d<u32>;
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
  gloss: f32,       // gloss multiplier (1 = unchanged, lower = rougher)
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
  var base = object.color.rgb;
  var roughness = object.material.y;
  var n = normalize(in.normal);
  if (object.material.z >= 0.0) {
    let s = texturedSurface(in.localPos, in.localNormal);
    base = base * s.color;
    // Gloss above 1 smooths, below 1 roughens (plank gaps, mortar).
    roughness = 1.0 - (1.0 - roughness) * s.gloss;
    n = normalize((object.model * vec4f(s.normal, 0.0)).xyz);
  }
  return shade(in.worldPos, n, base, roughness, object.color.a, base * object.material.x, 1.0);
}

// Skinned meshes: color and gloss come from each vertex; parts marked with a
// texture layer (clothes) get that texture's pattern.
@fragment
fn fs_skinned(in: VsOut) -> @location(0) vec4f {
  // Always sample (texture reads must not sit behind per-pixel branches),
  // then only use the result where the vertex asks for a texture.
  let n0 = normalize(in.localNormal);
  var w = pow(abs(n0), vec3f(4.0));
  w = w / (w.x + w.y + w.z);
  let q = in.localPos * object.material.w;
  let layer = max(in.layer, 0);
  let c = sampleColor(q.zy, layer) * w.x + sampleColor(q.xz, layer) * w.y + sampleColor(q.xy, layer) * w.z;
  var base = object.color.rgb * in.paint.rgb;
  var gloss = in.paint.a;
  if (in.layer >= 0) {
    base = base * c.rgb;
    gloss = gloss * c.a;
  }
  // Vertex gloss (0..1) maps to roughness: skin and cloth stay soft.
  let roughness = mix(0.9, 0.25, clamp(gloss, 0.0, 1.0));
  return shade(in.worldPos, normalize(in.normal), base, roughness, 0.0, base * object.material.x, 1.0);
}

// ---- Loaded models (.glb): textures come from the file, with UVs. ----

struct ModelMaterial {
  baseColor: vec4f,
  params: vec4f,    // x = metallic, y = roughness, z = normal scale, w = occlusion strength
  emissive: vec4f,  // rgb; w = the real-brightness fix (new lighting only)
};
// Group 2 is shared with the bones above, so these start at binding 1.
@group(2) @binding(1) var<uniform> modelMaterial: ModelMaterial;
@group(2) @binding(2) var baseColorMap: texture_2d<f32>;
@group(2) @binding(3) var metalRoughMap: texture_2d<f32>;  // g = roughness, b = metallic (glTF)
@group(2) @binding(4) var normalMap: texture_2d<f32>;
@group(2) @binding(5) var occlusionMap: texture_2d<f32>;    // r = occlusion
@group(2) @binding(6) var modelSampler: sampler;
@group(2) @binding(7) var emissiveMap: texture_2d<f32>;  // glow pattern (white = plain glow)

struct ModelIn {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) tangent: vec4f,
};

struct ModelOut {
  @builtin(position) clip: vec4f,
  @location(0) worldPos: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) tangent: vec4f,
};

@vertex
fn vs_model(in: ModelIn) -> ModelOut {
  let world = object.model * vec4f(in.position, 1.0);
  var out: ModelOut;
  out.clip = frame.viewProj * world;
  out.worldPos = world.xyz;
  out.normal = (object.model * vec4f(in.normal, 0.0)).xyz;
  out.uv = in.uv;
  out.tangent = vec4f((object.model * vec4f(in.tangent.xyz, 0.0)).xyz, in.tangent.w);
  return out;
}

// Shared by solid and see-through models. Returns the lit color and the
// surface's own opacity (base color alpha).
fn litModel(in: ModelOut, seeThrough: bool) -> vec4f {
  let m = modelMaterial;
  let baseSample = textureSample(baseColorMap, modelSampler, in.uv);
  let mr = textureSample(metalRoughMap, modelSampler, in.uv);
  let nt = textureSample(normalMap, modelSampler, in.uv).xyz * 2.0 - 1.0;
  let occlusion = textureSample(occlusionMap, modelSampler, in.uv).r;
  let glowPattern = textureSample(emissiveMap, modelSampler, in.uv).rgb;

  let alpha = baseSample.a * m.baseColor.a;
  // object.color: rgb tints the colour, a scales the model's own glow (its
  // loop, looped/world; 1 and 1 = as made).
  var base = baseSample.rgb * m.baseColor.rgb * object.color.rgb;
  // Real material brightness (L1.4): the new lighting lifts materials darker
  // (or lowers ones brighter) than anything real into real ranges.
  if (!classicLight()) { base = base * m.emissive.w; }
  // See-through surfaces only scatter light in proportion to their opacity;
  // their reflections stay at full strength.
  if (seeThrough) {
    base = base * alpha;
  }
  // The surface frame: tangent (along u), bitangent (along v), normal.
  let n0 = normalize(in.normal);
  let t = normalize(in.tangent.xyz - n0 * dot(n0, in.tangent.xyz));
  let b = cross(n0, t) * in.tangent.w;
  let n = normalize(t * (nt.x * m.params.z) + b * (nt.y * m.params.z) + n0 * nt.z);
  let ao = mix(1.0, occlusion, m.params.w);
  // object.material.x = highlight glow when you stand at something.
  var emission = m.emissive.rgb * glowPattern * object.color.a + base * object.material.x;
  // See-through glow (flames) only shines where the image is opaque.
  if (seeThrough) {
    emission = emission * alpha;
  }
  let lit = shade(in.worldPos, n, base, mr.g * m.params.y, mr.b * m.params.x, emission, ao);
  return vec4f(lit.rgb, alpha);
}

@fragment
fn fs_model(in: ModelOut) -> @location(0) vec4f {
  return vec4f(litModel(in, false).rgb, 1.0);
}

// See-through models (glass). Output is premultiplied: the color already
// includes its opacity. Glass gets more opaque at grazing angles (Fresnel),
// which is what makes its edges visible.
@fragment
fn fs_model_blend(in: ModelOut) -> @location(0) vec4f {
  let c = litModel(in, true);
  let v = normalize(frame.cameraPos.xyz - in.worldPos);
  let edge = pow(1.0 - abs(dot(normalize(in.normal), v)), 5.0);
  let alpha = clamp(c.a + (1.0 - c.a) * edge * 0.8, 0.0, 1.0);
  return vec4f(c.rgb, alpha);
}

const kPi = 3.14159265;

// GGX microfacet distribution: how many tiny surface facets face `h`.
fn distributionGgx(nh: f32, a: f32) -> f32 {
  let a2 = a * a;
  let d = nh * nh * (a2 - 1.0) + 1.0;
  return a2 / (kPi * d * d);
}

// Smith visibility (Schlick-GGX): facets shadowing each other. Includes the
// 1 / (4 n.l n.v) of the microfacet formula.
fn visibilitySmith(nv: f32, nl: f32, roughness: f32) -> f32 {
  let k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
  return 0.25 / ((nv * (1.0 - k) + k) * (nl * (1.0 - k) + k));
}

// Fresnel (Schlick): surfaces reflect more at grazing angles.
fn fresnel(f0: vec3f, vh: f32) -> vec3f {
  return f0 + (vec3f(1.0) - f0) * pow(1.0 - vh, 5.0);
}

// How much of the surroundings a surface reflects, averaged over its
// roughness (Karis' mobile approximation of the split-sum lookup table).
fn environmentBrdf(f0: vec3f, roughness: f32, nv: f32) -> vec3f {
  let c0 = vec4f(-1.0, -0.0275, -0.572, 0.022);
  let c1 = vec4f(1.0, 0.0425, 1.04, -0.04);
  let r = roughness * c0 + c1;
  let a004 = min(r.x * r.x, exp2(-9.28 * nv)) * r.x + r.y;
  let ab = vec2f(-1.04, 1.04) * a004 + r.zw;
  return f0 * ab.x + ab.y;
}

// What the room around a surface looks like in direction `r`, very roughly:
// the warm, lamp-lit upper half is brighter than the dark floor. Stands in
// for real reflections until the engine has environment maps.
fn roomGlow(r: vec3f) -> vec3f {
  if (!classicLight()) { return frame.ambient.rgb * frame.units.y * frame.units.z * (1.0 + 2.5 * clamp(r.y, -0.3, 1.0)); }
  return frame.ambient.rgb * (2.0 + 5.0 * clamp(r.y, -0.3, 1.0));
}

// What a shiny surface at `p` sees in direction `r` (E4.4): the room captured
// into a cube, matched to the room's box (the ray from p is followed to the
// wall it meets, then looked up from where the cube was captured), blurrier
// for rougher surfaces. Outside the room, or before the capture, the old glow.
fn environment(p: vec3f, r: vec3f, roughness: f32) -> vec3f {
  let lo = frame.envMin.xyz;
  let hi = frame.envMax.xyz;
  if (frame.env.y < 0.5 || any(p < lo) || any(p > hi)) { return roomGlow(r); }
  let rr = select(r, vec3f(1e-5), abs(r) < vec3f(1e-5));
  let far = max((hi - p) / rr, (lo - p) / rr);
  let dist = min(min(far.x, far.y), far.z);
  let dir = p + r * dist - frame.envCenter.xyz;
  return textureSampleLevel(envMap, envSampler, dir, roughness * frame.env.z).rgb;  // linear light
}

// Bounced light at a surface: the light arriving from all around (lux,
// facing `n`) from the 8 probes around it, and which lamps it can see.
// Probes inside walls are left out, and probes behind the surface count
// little (so light doesn't leak through a wall from the other side).
struct GiSample {
  irradiance: vec3f,
  masks: array<u32, 8>,
  weights: array<f32, 8>,
  total: f32,
};

fn giSample(p: vec3f, n: vec3f, v: vec3f) -> GiSample {
  var out: GiSample;
  out.irradiance = vec3f(0.0);
  out.total = 0.0;
  let spacing = 1.0 / frame.gi.w;
  // A little off the surface (toward the normal and the eye) so the surface
  // itself doesn't shadow its probes.
  let q = (p + n * (0.3 * spacing) + v * (0.1 * spacing) - frame.gi.xyz) * frame.gi.w;
  let dims = vec3i(frame.gi2.xyz);
  let base = vec3i(floor(q));
  let f = q - vec3f(base);
  var r = vec4f(0.0);
  var g = vec4f(0.0);
  var b = vec4f(0.0);
  for (var c = 0; c < 8; c++) {
    let o = vec3i(c & 1, (c >> 1) & 1, c >> 2);
    let cell = clamp(base + o, vec3i(0), dims - 1);
    let mask = textureLoad(giMask, cell, 0).r;
    let tri = mix(1.0 - f, f, vec3f(o));
    var w = tri.x * tri.y * tri.z;
    let probePos = frame.gi.xyz + vec3f(cell) * spacing;
    let toProbe = normalize(probePos - p + n * 1e-3);
    let facing = (dot(toProbe, n) + 1.0) * 0.5;
    w = w * (facing * facing + 0.02) * f32(mask >> 31u);
    out.masks[c] = mask;
    out.weights[c] = w;
    out.total += w;
    r += w * textureLoad(giR, cell, 0);
    g += w * textureLoad(giG, cell, 0);
    b += w * textureLoad(giB, cell, 0);
  }
  if (out.total > 1e-5) {
    r /= out.total;
    g /= out.total;
    b /= out.total;
    // Irradiance from order-1 spherical harmonics (cosine lobe: pi, 2pi/3).
    let y = vec4f(3.14159265 * 0.282095, 2.0943951 * 0.488603 * n.y, 2.0943951 * 0.488603 * n.z,
                  2.0943951 * 0.488603 * n.x);
    out.irradiance = max(vec3f(dot(r, y), dot(g, y), dot(b, y)), vec3f(0.0));
  }
  return out;
}

// How much lamp `k` is in view from here (0..1), from the probes around it.
fn giLampVisible(s: GiSample, k: u32) -> f32 {
  if (s.total <= 1e-5 || k >= 31u) { return 1.0; }
  var seen = 0.0;
  for (var c = 0; c < 8; c++) {
    seen += s.weights[c] * f32((s.masks[c] >> k) & 1u);
  }
  return seen / s.total;
}

// Lights with size (L1.6): the light a tube or a panel gives a surface.
// Diffuse: the lamp as a few points along it (a tube: 3; a panel: 4, each
// side lit, brighter head-on). Highlights: the point on the lamp closest to
// the reflection (a "representative point"), widened by the lamp's size,
// so a neon tube reflects as a streak and a panel as a soft rectangle.
struct AreaLight {
  diffuse: vec3f,   // lux (times nl), before visibility
  specular: vec3f,  // as the point lights' specular sum
};

fn areaLight(l: Light, p: vec3f, n: vec3f, v: vec3f, a: f32, roughness: f32, nv: f32, f0: vec3f) -> AreaLight {
  var out: AreaLight;
  out.diffuse = vec3f(0.0);
  out.specular = vec3f(0.0);
  let shape = l.axis.w;
  let c = l.position.xyz;
  let ha = l.axis.xyz;
  let hb = l.axis2.xyz;
  let reach2 = l.position.w * l.position.w;
  let r2 = max(l.params.x * l.params.x, 1e-4);
  let panel = shape > 1.5;
  let pn = normalize(cross(ha, hb) + vec3f(0.0, 1e-6, 0.0));
  // Diffuse from points along the lamp.
  let count = select(3, 4, panel);
  for (var i = 0; i < count; i++) {
    var q = c;
    if (panel) {
      q = c + ha * select(-0.5, 0.5, (i & 1) == 1) + hb * select(-0.5, 0.5, (i & 2) == 2);
    } else {
      q = c + ha * (f32(i - 1) * 0.6667);
    }
    let d = q - p;
    let d2 = dot(d, d);
    if (d2 >= reach2) { continue; }
    let dir = d * inverseSqrt(max(d2, 1e-8));
    let nl = dot(n, dir);
    if (nl <= 0.0) { continue; }
    let qq = d2 / reach2;
    let w = clamp(1.0 - qq * qq, 0.0, 1.0);
    let emit = select(1.0, abs(dot(pn, dir)), panel);  // a panel is brightest head-on
    out.diffuse += l.color.rgb * (l.color.w / f32(count)) * emit * nl * w * w / max(d2, r2);
  }
  // The highlight: the lamp's point nearest the reflected ray.
  let r = reflect(-v, n);
  var s: vec3f;
  if (panel) {
    let denom = dot(r, pn);
    var hit = c;
    if (abs(denom) > 1e-4) {
      let t = dot(c - p, pn) / denom;
      if (t > 0.0) { hit = p + r * t; }
    }
    let la = length(ha);
    let lb = length(hb);
    let u = clamp(dot(hit - c, ha / max(la, 1e-6)), -la, la);
    let w2 = clamp(dot(hit - c, hb / max(lb, 1e-6)), -lb, lb);
    s = c + ha / max(la, 1e-6) * u + hb / max(lb, 1e-6) * w2 - p;
  } else {
    let p0 = c - ha - p;
    let ld = 2.0 * ha;
    let rl = dot(r, ld);
    let t = clamp((dot(r, p0) * rl - dot(p0, ld)) / max(dot(ld, ld) - rl * rl, 1e-6), 0.0, 1.0);
    s = p0 + ld * t;
  }
  let dist = max(length(s), 1e-4);
  let dir = s / dist;
  let nl = dot(n, dir);
  if (nl > 0.0 && dist * dist < reach2) {
    // Wider, softer highlight for a bigger lamp, the same energy.
    let size = length(ha) + length(hb);
    let aSphere = clamp(a + l.params.x / (2.0 * dist), 0.0, 1.0);
    let aSize = clamp(a + size / (2.0 * dist), 0.0, 1.0);
    let h = normalize(dir + v);
    let d = distributionGgx(max(dot(n, h), 0.0), aSphere) * (a * a) / (aSphere * aSphere) * (a / aSize);
    let spec = d * visibilitySmith(nv, nl, roughness) * fresnel(f0, max(dot(v, h), 0.0));
    let emit = select(1.0, abs(dot(pn, dir)), panel);
    let qq = dist * dist / reach2;
    let w = clamp(1.0 - qq * qq, 0.0, 1.0);
    out.specular = l.color.rgb * l.color.w * emit * w * w / max(dist * dist, r2) * spec * (nl * kPi);
  }
  return out;
}

// Lighting shared by every mesh: physically based point lights, ambient,
// haze, tone mapping. Light units: diffuse leaves out the 1/pi, so the
// specular term is multiplied by pi to match.
// `emission` is light the surface gives off itself; `ao` (0..1) darkens the
// ambient light in creases (ambient occlusion).
fn shade(worldPos: vec3f, n: vec3f, base: vec3f, roughnessIn: f32, metallic: f32, emission: vec3f,
         ao: f32) -> vec4f {
  let v = normalize(frame.cameraPos.xyz - worldPos);
  let roughness = clamp(roughnessIn, 0.04, 1.0);
  let a = roughness * roughness;
  let nv = max(dot(n, v), 1e-4);
  // Metals have no diffuse color; their base color tints the reflection.
  // Non-metals reflect about 4% head-on, untinted.
  let diffuseColor = base * (1.0 - metallic);
  let f0 = mix(vec3f(0.04), base, metallic);

  let classic = classicLight();
  // Classic: lights in "intensity" with a soft window; new (L1.2): lux from
  // candela, true inverse square, fading only over the last part of its reach.
  let ambientScale = select(frame.units.y, 1.0, classic);
  var diffuse = frame.ambient.rgb * ambientScale * (0.6 + 0.4 * n.y) * ao;  // a little brighter on top
  // Bounced light replaces the flat fill (light only from light sources),
  // fading in the first time it's worked out.
  let giWeight = select(frame.gi2.w, 0.0, classic);
  var gi: GiSample;
  if (giWeight > 0.0) {
    gi = giSample(worldPos, n, v);
    diffuse = mix(diffuse, gi.irradiance * ao, giWeight);
  }
  var specular = vec3f(0.0);
  // This pixel's cell in the light grid.
  let g = lighting.grid;
  let cx = u32(clamp(floor((worldPos.x - g.x) * g.z), 0.0, f32(lighting.gridSize.x - 1u)));
  let cz = u32(clamp(floor((worldPos.z - g.y) * g.z), 0.0, f32(lighting.gridSize.y - 1u)));
  let cell = lighting.cells[cz * lighting.gridSize.x + cx];
  // Each cell's lights are listed brightest first, so a view can take only
  // the strongest few (the mirror: its reflection doesn't need them all).
  let lightCount = select(cell.z, min(cell.z, u32(frame.ambient.w)), frame.ambient.w > 0.5);
  for (var k = 0u; k < lightCount; k++) {
    let packed = select(cell.y, cell.x, k < 4u);
    let lightIndex = (packed >> (8u * (k % 4u))) & 0xffu;
    let l = lighting.lights[lightIndex];
    if (!classic && l.axis.w > 0.5) {  // a tube or a panel (L1.6)
      let al = areaLight(l, worldPos, n, v, a, roughness, nv, f0);
      var seen = 1.0;
      if (giWeight > 0.0) { seen = mix(1.0, giLampVisible(gi, lightIndex), giWeight); }
      diffuse += al.diffuse * seen;
      specular += al.specular * seen;
      continue;
    }
    let toLight = l.position.xyz - worldPos;
    // A cell lists every light that reaches any part of it, so many of them
    // don't reach this pixel, or light it from behind: those add nothing,
    // so skip the math (same picture, less work).
    let d2 = dot(toLight, toLight);
    if (d2 >= l.position.w * l.position.w) { continue; }
    let dist = sqrt(d2);
    let dir = toLight / dist;
    let nl = dot(n, dir);
    if (nl <= 0.0) { continue; }
    // Inverse-square falloff that fades smoothly to zero at the light's range.
    let q = d2 / (l.position.w * l.position.w);
    let window = clamp(1.0 - q * q, 0.0, 1.0);
    let r2 = l.params.x * l.params.x;
    let atten = window * window / select(max(d2, r2), d2 + 0.25, classic);
    var radiance = l.color.rgb * l.color.w * atten;
    // No lamp light through walls: what the probes around here can see of it.
    if (giWeight > 0.0) { radiance = radiance * mix(1.0, giLampVisible(gi, lightIndex), giWeight); }
    diffuse += radiance * nl;

    // Bulbs aren't points: a bigger glowing part makes a wider, softer
    // highlight (Karis' sphere-light trick), keeping the same total energy.
    let aLight = clamp(a + l.params.x / (2.0 * dist), 0.0, 1.0);
    let h = normalize(dir + v);
    let nh = max(dot(n, h), 0.0);
    let vh = max(dot(v, h), 0.0);
    let d = distributionGgx(nh, aLight) * (a * a) / (aLight * aLight);
    let spec = d * visibilitySmith(nv, nl, roughness) * fresnel(f0, vh);
    specular += radiance * spec * (nl * kPi);
  }
  // New lighting: diffuse and highlights in proper units (the 1 / pi the
  // classic sums leave out); surfaces' own glow in cd/m².
  // Debug views (?giview=): 1 = the bounced light alone, 2 = probes usable
  // around here (green) or not (red), 3 = how much of the nearest lamp is seen.
  if (frame.units.w > 0.5 && giWeight > 0.0) {
    if (frame.units.w < 1.5) { return vec4f(gi.irradiance * frame.units.z, 1.0); }
    if (frame.units.w < 2.5) { return vec4f(select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), gi.total > 1e-4), 1.0); }
    let first = lighting.cells[cz * lighting.gridSize.x + cx].x & 0xffu;
    return vec4f(vec3f(giLampVisible(gi, first)), 1.0);
  }
  var color: vec3f;
  if (classic) {
    color = diffuseColor * diffuse + specular + emission;
  } else {
    color = (diffuseColor * diffuse + specular) * frame.units.z + emission * frame.units.x;
  }
  let r = reflect(-v, n);
  if (frame.env.x < 0.5) {  // (no reflections of reflections while capturing)
    color += environment(worldPos, r, roughness) * environmentBrdf(f0, roughness, nv) * ao;
  } else {
    color += roomGlow(r) * environmentBrdf(f0, roughness, nv) * ao;
  }

  let dist = length(frame.cameraPos.xyz - worldPos);
  let haze = 1.0 - exp(-dist * frame.fog.w);
  color = mix(color, frame.fog.rgb, haze);
  // The reflection cube and the new lighting keep the light itself (looped/post.h
  // does the eyes and the curve); classic lighting leaves here tone mapped.
  if (frame.env.x > 0.5 || frame.env.w < 0.5) { return vec4f(color, 1.0); }
  return vec4f(pow(tonemap(color), vec3f(1.0 / 2.2)), 1.0);
}
