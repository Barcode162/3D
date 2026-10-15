// First pipeline test: a spinning triangle whose colors come from a
// uniform buffer written by C++ every frame.

struct Frame {
  time: f32,
  aspect: f32,      // width / height, keeps the triangle from stretching
  _pad: vec2f,
  tint: vec4f,
};

@group(0) @binding(0) var<uniform> frame: Frame;

struct VsOut {
  @builtin(position) position: vec4f,
  @location(0) color: vec3f,
};

@vertex
fn vs_main(@builtin(vertex_index) i: u32) -> VsOut {
  // Equilateral triangle corners, generated from the vertex index.
  let angle = f32(i) * 2.0943951 + frame.time * 0.8;
  // Size relative to the narrower screen side so it fits portrait and landscape.
  var p = vec2f(sin(angle), cos(angle)) * 0.6 * min(frame.aspect, 1.0);
  p.x /= frame.aspect;

  let corners = array<vec3f, 3>(
    vec3f(1.0, 0.55, 0.15),   // amber
    vec3f(1.0, 0.2, 0.6),     // neon pink
    vec3f(0.2, 0.9, 0.9),     // teal
  );

  var out: VsOut;
  out.position = vec4f(p, 0.0, 1.0);
  out.color = mix(corners[i], frame.tint.rgb, 0.35);
  return out;
}

@fragment
fn fs_main(in: VsOut) -> @location(0) vec4f {
  return vec4f(in.color, 1.0);
}
