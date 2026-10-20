// After the scene (looped/post.cpp): glow and mood.
// - Glow (bloom): the bright parts (bulbs, neon, lit signs) are picked out at
//   a quarter of the screen size, blurred down to 1/8 and 1/16 and back up,
//   and added over the picture, so light bleeds softly like in a camera.
// - Mood (color grading): a warm or cool tint, saturation, contrast, a
//   gentle darkening toward the corners (vignette).
// The scene arrives already tone mapped (display colors); the glow is worked
// out in linear light (about color^2.2) and the mood in display colors.

struct Post {
  texel: vec4f,   // xy = 1 / source size (the texture sampled), zw = 1 / target size
  params: vec4f,  // x = glow threshold (linear), y = soft knee, z = glow strength, w = vignette
  grade: vec4f,   // rgb = tint (1 = none), a = saturation (1 = none)
  grade2: vec4f,  // x = contrast (1 = none), y = lift (raises the darks), zw unused
};

@group(0) @binding(0) var<uniform> post: Post;
@group(0) @binding(1) var texA: texture_2d<f32>;  // the source
@group(0) @binding(2) var texB: texture_2d<f32>;  // composite: the glow
@group(0) @binding(3) var samp: sampler;

struct VsOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
};

// One triangle that covers the screen.
@vertex
fn vs_full(@builtin(vertex_index) i: u32) -> VsOut {
  let p = vec2f(f32((i << 1u) & 2u), f32(i & 2u));
  var o: VsOut;
  o.pos = vec4f(p * 2.0 - 1.0, 0.0, 1.0);
  o.uv = vec2f(p.x, 1.0 - p.y);
  return o;
}

fn lin(c: vec3f) -> vec3f { return pow(max(c, vec3f(0.0)), vec3f(2.2)); }

// The average of a 4 x 4 block of source texels: four bilinear taps.
fn box4(uv: vec2f) -> vec3f {
  let t = post.texel.xy;
  let a = textureSampleLevel(texA, samp, uv + t * vec2f(-1.0, -1.0), 0.0).rgb;
  let b = textureSampleLevel(texA, samp, uv + t * vec2f(1.0, -1.0), 0.0).rgb;
  let c = textureSampleLevel(texA, samp, uv + t * vec2f(-1.0, 1.0), 0.0).rgb;
  let d = textureSampleLevel(texA, samp, uv + t * vec2f(1.0, 1.0), 0.0).rgb;
  return (a + b + c + d) * 0.25;
}

// Full picture → a quarter: keep only what's brighter than the threshold,
// fading in softly over the knee (no hard edge where the glow starts).
@fragment
fn fs_prefilter(in: VsOut) -> @location(0) vec4f {
  let t = post.texel.xy;
  var c = vec3f(0.0);
  c += lin(textureSampleLevel(texA, samp, in.uv + t * vec2f(-1.0, -1.0), 0.0).rgb);
  c += lin(textureSampleLevel(texA, samp, in.uv + t * vec2f(1.0, -1.0), 0.0).rgb);
  c += lin(textureSampleLevel(texA, samp, in.uv + t * vec2f(-1.0, 1.0), 0.0).rgb);
  c += lin(textureSampleLevel(texA, samp, in.uv + t * vec2f(1.0, 1.0), 0.0).rgb);
  c *= 0.25;
  let l = max(c.r, max(c.g, c.b));
  let th = post.params.x;
  let knee = max(post.params.y, 1e-4);
  var soft = clamp(l - th + knee, 0.0, 2.0 * knee);
  soft = soft * soft / (4.0 * knee);
  let w = max(soft, l - th) / max(l, 1e-4);
  return vec4f(c * w, 1.0);
}

// One level down (half the size), averaged.
@fragment
fn fs_down(in: VsOut) -> @location(0) vec4f {
  return vec4f(box4(in.uv), 1.0);
}

// One level up: a 3 x 3 tent over the smaller level, added onto this one.
@fragment
fn fs_up(in: VsOut) -> @location(0) vec4f {
  let t = post.texel.xy;
  var c = textureSampleLevel(texA, samp, in.uv, 0.0).rgb * 4.0;
  c += (textureSampleLevel(texA, samp, in.uv + vec2f(t.x, 0.0), 0.0).rgb + textureSampleLevel(texA, samp, in.uv - vec2f(t.x, 0.0), 0.0).rgb +
        textureSampleLevel(texA, samp, in.uv + vec2f(0.0, t.y), 0.0).rgb + textureSampleLevel(texA, samp, in.uv - vec2f(0.0, t.y), 0.0).rgb) * 2.0;
  c += textureSampleLevel(texA, samp, in.uv + t, 0.0).rgb + textureSampleLevel(texA, samp, in.uv - t, 0.0).rgb +
       textureSampleLevel(texA, samp, in.uv + vec2f(t.x, -t.y), 0.0).rgb + textureSampleLevel(texA, samp, in.uv + vec2f(-t.x, t.y), 0.0).rgb;
  return vec4f(c / 16.0, 1.0);
}

// The picture + its glow, then the mood.
@fragment
fn fs_composite(in: VsOut) -> @location(0) vec4f {
  let scene = textureSampleLevel(texA, samp, in.uv, 0.0).rgb;
  let glow = textureSampleLevel(texB, samp, in.uv, 0.0).rgb;
  var lit = lin(scene) + glow * post.params.z;
  // Bright glow rolls off instead of clipping to flat white.
  lit = lit / (1.0 + max(lit - vec3f(0.85), vec3f(0.0)));
  var c = pow(lit, vec3f(1.0 / 2.2));
  // Mood.
  c = c * post.grade.rgb;
  let luma = dot(c, vec3f(0.2126, 0.7152, 0.0722));
  c = mix(vec3f(luma), c, post.grade.a);
  c = (c - 0.5) * post.grade2.x + 0.5;
  c = c + post.grade2.y * (1.0 - c);
  let d = in.uv - 0.5;
  let v = 1.0 - post.params.w * smoothstep(0.25, 0.85, dot(d, d) * 2.0);
  c = c * v;
  return vec4f(clamp(c, vec3f(0.0), vec3f(1.0)), 1.0);
}
