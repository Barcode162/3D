// Main scene shader: lit, solid-colored meshes with several point lights,
// emissive surfaces (neon, lamp bulbs), distance haze and tone mapping.

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
};

struct Object {
  model: mat4x4f,
  color: vec4f,       // rgb = base color
  material: vec4f,    // x = emissive strength, y = shininess (0..1)
};

@group(0) @binding(0) var<uniform> frame: Frame;
@group(1) @binding(0) var<uniform> object: Object;

struct VsIn {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
};

struct VsOut {
  @builtin(position) clip: vec4f,
  @location(0) worldPos: vec3f,
  @location(1) normal: vec3f,
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
  return out;
}

// Filmic tone mapping (ACES approximation): keeps bright lights from clipping.
fn tonemap(x: vec3f) -> vec3f {
  return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), vec3f(0.0), vec3f(1.0));
}

@fragment
fn fs_main(in: VsOut) -> @location(0) vec4f {
  let n = normalize(in.normal);
  let toEye = normalize(frame.cameraPos.xyz - in.worldPos);
  let base = object.color.rgb;
  let shininess = object.material.y;

  var light = frame.ambient.rgb * (0.6 + 0.4 * n.y);  // a little brighter on top
  var specular = vec3f(0.0);
  for (var i = 0u; i < frame.lightCount.x; i++) {
    let l = frame.lights[i];
    let toLight = l.position.xyz - in.worldPos;
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

  var color = base * light + specular + base * object.material.x;

  let dist = length(frame.cameraPos.xyz - in.worldPos);
  let haze = 1.0 - exp(-dist * frame.fog.w);
  color = mix(color, frame.fog.rgb, haze);

  // The surface is a plain (non-sRGB) format, so apply gamma here.
  return vec4f(pow(tonemap(color), vec3f(1.0 / 2.2)), 1.0);
}
