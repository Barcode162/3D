// Surface shading: world-space PBR detail for terrain & road (Poly Haven CC0 textures),
// wind sway for foliage, snow accumulation for the glacier biome.
import * as THREE from 'three';
import * as T from './textures.js';

const loader = new THREE.TextureLoader();
export const shared = { time: { value: 0 }, wind: { value: 1 } };

function tex(uri, srgb, aniso = 8) {
  const t = loader.load(uri);
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  t.colorSpace = srgb ? THREE.SRGBColorSpace : THREE.NoColorSpace;
  t.anisotropy = aniso;
  return t;
}
const cache = {};
function set(key) {
  if (cache[key]) return cache[key];
  const K = key.toUpperCase();
  cache[key] = { det: tex(T[K + '_DET'], true), nor: tex(T[K + '_NOR'], false), rgh: tex(T[K + '_RGH'], false) };
  return cache[key];
}

const NORMAL_WS = `
  {
    vec3 tn = texture2D(tNor, wuv).xyz * 2.0 - 1.0;
    vec3 tn2 = texture2D(tNor, wuv2).xyz * 2.0 - 1.0;
    tn = normalize(vec3((tn.xy + tn2.xy * 0.5) * uNStr, tn.z));
    vec3 wN = normalize((vec4(normal, 0.0) * viewMatrix).xyz);
    vec3 Tt = normalize(vec3(1.0, 0.0, 0.0) - wN * wN.x);
    vec3 Bt = cross(Tt, wN);
    vec3 nW = normalize(Tt * tn.x + Bt * tn.y + wN * tn.z);
    normal = normalize((viewMatrix * vec4(nW, 0.0)).xyz);
  }
`;

// terrain: vertex-colour biome palette × world-space detail albedo, detail normal + roughness
export function terrainMaterial(biomeKey, opts = {}) {
  const s = set(biomeKey);
  const m = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 1.0, metalness: 0, envMapIntensity: 0.35 });
  const scale = opts.scale || 0.22, nStr = opts.nStr || 1.0, detStr = opts.detStr ?? 0.9;
  m.onBeforeCompile = (sh) => {
    sh.uniforms.tDet = { value: s.det }; sh.uniforms.tNor = { value: s.nor }; sh.uniforms.tRgh = { value: s.rgh };
    sh.uniforms.uScale = { value: scale }; sh.uniforms.uNStr = { value: nStr }; sh.uniforms.uDetStr = { value: detStr };
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nvarying vec3 vWP;')
      .replace('#include <project_vertex>', '#include <project_vertex>\nvWP = (modelMatrix * vec4(transformed, 1.0)).xyz;');
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nvarying vec3 vWP;\nuniform sampler2D tDet, tNor, tRgh; uniform float uScale, uNStr, uDetStr;')
      .replace('#include <map_fragment>', `#include <map_fragment>
        vec2 wuv = vWP.xz * uScale;
        vec2 wuv2 = mat2(0.8, -0.6, 0.6, 0.8) * vWP.xz * uScale * 0.27 + vec2(0.31, 0.17);
        vec3 det = mix(texture2D(tDet, wuv).rgb, texture2D(tDet, wuv2).rgb, 0.38) * 2.0;
        diffuseColor.rgb *= mix(vec3(1.0), det, uDetStr);`)
      .replace('#include <roughnessmap_fragment>', `#include <roughnessmap_fragment>
        roughnessFactor = clamp(0.55 + 0.6 * texture2D(tRgh, wuv).g, 0.3, 1.0);`)
      .replace('#include <normal_fragment_maps>', '#include <normal_fragment_maps>\n' + NORMAL_WS);
  };
  m.customProgramCacheKey = () => 'terrain-' + biomeKey;
  return m;
}

// road: painted markings canvas (uv) × asphalt detail, gravel on the shoulders, wet-ish specular breakup
export function upgradeRoadMaterial(m, SHOULDER, ROAD_HALF) {
  const a = set('asphalt'), g = set('gravel');
  m.onBeforeCompile = (sh) => {
    Object.assign(sh.uniforms, {
      tDet: { value: a.det }, tNor: { value: a.nor }, tRgh: { value: a.rgh },
      tGDet: { value: g.det }, tGNor: { value: g.nor },
      uScale: { value: 0.32 }, uNStr: { value: 0.8 }, uSh: { value: SHOULDER }, uRH: { value: ROAD_HALF },
    });
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nvarying vec3 vWP;')
      .replace('#include <project_vertex>', '#include <project_vertex>\nvWP = (modelMatrix * vec4(transformed, 1.0)).xyz;');
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nvarying vec3 vWP;\nuniform sampler2D tDet, tNor, tRgh, tGDet, tGNor; uniform float uScale, uNStr, uSh, uRH;')
      .replace('#include <map_fragment>', `#include <map_fragment>
        vec2 wuv = vWP.xz * uScale;
        vec2 wuv2 = mat2(0.8, -0.6, 0.6, 0.8) * vWP.xz * uScale * 0.21 + vec2(0.13, 0.41);
        float dd = abs(uSh - vMapUv.x * 2.0 * uSh);
        float gw = smoothstep(uRH - 0.05, uRH + 0.35, dd);
        vec3 adet = mix(texture2D(tDet, wuv).rgb, texture2D(tDet, wuv2).rgb, 0.35) * 2.0;
        vec3 gdet = texture2D(tGDet, wuv * 1.3).rgb * 2.0;
        diffuseColor.rgb *= mix(adet, gdet, gw);`)
      .replace('#include <roughnessmap_fragment>', `#include <roughnessmap_fragment>
        float rr = texture2D(tRgh, wuv).g;
        roughnessFactor = mix(clamp(0.5 + 0.55 * rr, 0.35, 0.98), 0.95, gw);`)
      .replace('#include <normal_fragment_maps>', `#include <normal_fragment_maps>
        {
          vec3 tn = mix(texture2D(tNor, wuv).xyz, texture2D(tGNor, wuv * 1.3).xyz, gw) * 2.0 - 1.0;
          tn = normalize(vec3(tn.xy * uNStr, tn.z));
          vec3 wN = normalize((vec4(normal, 0.0) * viewMatrix).xyz);
          vec3 Tt = normalize(vec3(1.0, 0.0, 0.0) - wN * wN.x);
          vec3 Bt = cross(Tt, wN);
          normal = normalize((viewMatrix * vec4(normalize(Tt * tn.x + Bt * tn.y + wN * tn.z), 0.0)).xyz);
        }`);
  };
  m.customProgramCacheKey = () => 'road-pbr';
  m.needsUpdate = true;
}

// props: wind sway (vertex) and optional snow (fragment) on any standard material
export function propMaterial(src, { wind = 0, snow = 0, foliage = false } = {}) {
  const m = src.clone();
  m.vertexColors = true;
  if (foliage) { m.alphaTest = 0.45; m.transparent = false; m.side = THREE.DoubleSide; m.alphaToCoverage = false; }
  m.envMapIntensity = foliage ? 0.25 : 0.4;
  m.onBeforeCompile = (sh) => {
    sh.uniforms.uTime = shared.time; sh.uniforms.uWind = { value: wind }; sh.uniforms.uSnow = { value: snow };
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nuniform float uTime, uWind;')
      .replace('#include <begin_vertex>', `#include <begin_vertex>
        if (uWind > 0.0) {
          #ifdef USE_INSTANCING
            vec3 ip = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
          #else
            vec3 ip = vec3(0.0);
          #endif
          float h = max(transformed.y, 0.0);
          float ph = ip.x * 0.11 + ip.z * 0.13;
          float sway = sin(uTime * 1.1 + ph) * 0.6 + sin(uTime * 2.3 + ph * 1.7 + transformed.x * 0.6) * 0.3;
          float flutter = sin(uTime * 7.0 + transformed.x * 3.0 + transformed.z * 3.0 + ph) * 0.15;
          transformed.x += (sway + flutter) * h * h * uWind;
          transformed.z += (sway * 0.6 - flutter) * h * h * uWind;
        }`);
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nuniform float uSnow;')
      .replace('#include <normal_fragment_maps>', `#include <normal_fragment_maps>
        if (uSnow > 0.0) {
          vec3 wN = normalize((vec4(normal, 0.0) * viewMatrix).xyz);
          float up = smoothstep(0.25, 0.75, wN.y);
          diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.86, 0.9, 0.97), up * uSnow);
        }`);
  };
  m.customProgramCacheKey = () => `prop-${m.name}-${wind}-${snow}-${foliage}`;
  return m;
}
