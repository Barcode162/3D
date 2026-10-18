import * as THREE from 'three';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { CAR_GLB, PROPS_GLB } from './assets.js';
import { NATURE_GLB } from './nature_glb.js';
import { terrainMaterial, upgradeRoadMaterial, propMaterial, shared as surf } from './surfaces.js';

// ---------- Blender-modelled assets (embedded GLB) ----------
function b64ToBuf(s) { const bin = atob(s); const u = new Uint8Array(bin.length); for (let i = 0; i < bin.length; i++) u[i] = bin.charCodeAt(i); return u.buffer; }
export async function loadModels() {
  const loader = new GLTFLoader(); const out = { car: null, props: null, nature: null };
  try { out.nature = await loader.parseAsync(b64ToBuf(NATURE_GLB), ''); } catch (e) { console.warn('nature kit failed, using fallback', e); }
  try { out.car = await loader.parseAsync(b64ToBuf(CAR_GLB), ''); } catch (e) { console.warn('car model failed, using fallback', e); }
  try { out.props = await loader.parseAsync(b64ToBuf(PROPS_GLB), ''); } catch (e) { console.warn('prop models failed, using fallback', e); }
  return out;
}
// textured prop kit: geometry with material groups + per-biome (snow) material variants
function kitFromGltf(gltf, name, opts) {
  const node = gltf.scene.getObjectByName(name); if (!node) return null;
  node.updateWorldMatrix(true, true);
  const geos = [], mats = [];
  node.traverse((o) => {
    if (!o.isMesh) return;
    let g = o.geometry.clone(); g.applyMatrix4(o.matrixWorld);
    for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'uv', 'color'].includes(k)) g.deleteAttribute(k);
    if (!g.attributes.uv) g.setAttribute('uv', new THREE.BufferAttribute(new Float32Array(g.attributes.position.count * 2), 2));
    if (!g.attributes.color) { const c = new Float32Array(g.attributes.position.count * 3).fill(1); g.setAttribute('color', new THREE.BufferAttribute(c, 3)); }
    else if (g.attributes.color.itemSize === 4) {
      const s = g.attributes.color, a = new Float32Array(s.count * 3);
      for (let i = 0; i < s.count; i++) { a[i * 3] = s.getX(i); a[i * 3 + 1] = s.getY(i); a[i * 3 + 2] = s.getZ(i); }
      g.setAttribute('color', new THREE.BufferAttribute(a, 3));
    }
    if (!g.index) { const idx = []; for (let i = 0; i < g.attributes.position.count; i++) idx.push(i); g.setIndex(idx); }
    geos.push(g); mats.push(o.material);
  });
  if (!geos.length) return null;
  const geo = geos.length > 1 ? mergeGeometries(geos, true) : geos[0];
  if (geos.length === 1) { geo.clearGroups(); geo.addGroup(0, geo.index.count, 0); }
  geo.computeBoundingSphere();
  const mk = (snow) => mats.map((m) => {
    const fol = /Foliage|Grass|shrub/i.test(m.name) || (m.alphaTest > 0 || m.transparent);
    return propMaterial(m, { wind: opts.wind ? (fol ? opts.wind : opts.wind * 0.5) : 0, snow, foliage: fol });
  });
  return { geo, mats: mk(0), snowMats: mk(opts.snow ?? 0.85), shadow: opts.shadow !== false };
}
function geoFromGltf(gltf, name) {
  const node = gltf.scene.getObjectByName(name); if (!node) return null;
  node.updateWorldMatrix(true, true);
  const parts = [];
  node.traverse((o) => { if (o.isMesh) { const g = o.geometry.clone(); g.applyMatrix4(o.matrixWorld); parts.push(g); } });
  if (!parts.length) return null;
  const out = [];
  for (const g of parts) {
    const ng = new THREE.BufferGeometry();
    const gi = g.index ? g.toNonIndexed() : g;
    ng.setAttribute('position', gi.attributes.position.clone());
    const c = gi.attributes.color;
    if (c) {
      const n = c.count, a = new Float32Array(n * 3);
      for (let i = 0; i < n; i++) { a[i * 3] = c.getX(i); a[i * 3 + 1] = c.getY(i); a[i * 3 + 2] = c.getZ(i); }
      ng.setAttribute('color', new THREE.BufferAttribute(a, 3));
    }
    ng.computeVertexNormals(); out.push(ng);
  }
  return out.length > 1 ? mergeGeometries(out) : out[0];
}

import { DS, ROAD_HALF, SHOULDER, BIOMES, biomeAt, mulberry32, noise2, lerp, clamp } from './road.js';

export const CHUNK = 64; // samples per chunk (128 m)

const _c1 = new THREE.Color(), _c2 = new THREE.Color(), _c3 = new THREE.Color();
function biomeColor(key, bm, out) {
  _c1.setHex(BIOMES[bm.a][key]); _c2.setHex(BIOMES[bm.b][key]);
  return out.copy(_c1).lerp(_c2, bm.t);
}

// ---------- textures ----------
function roadTextures() {
  const W = 256, H = 512;
  const cv = document.createElement('canvas'); cv.width = W; cv.height = H;
  const g = cv.getContext('2d');
  const ev = document.createElement('canvas'); ev.width = W; ev.height = H;
  const e = ev.getContext('2d');
  e.fillStyle = '#000'; e.fillRect(0, 0, W, H);
  const pxPerM = W / (SHOULDER * 2);
  const X = (d) => (SHOULDER - d) * pxPerM;
  // asphalt
  g.fillStyle = '#3a3a40'; g.fillRect(0, 0, W, H);
  const rng = mulberry32(99);
  for (let i = 0; i < 26000; i++) {
    const v = 40 + rng() * 40 | 0; g.fillStyle = `rgba(${v},${v},${v + 4},${0.35 + rng() * 0.4})`;
    g.fillRect(rng() * W, rng() * H, 1 + rng() * 1.5, 1 + rng() * 1.5);
  }
  // wheel-worn darker tracks
  g.fillStyle = 'rgba(20,20,26,0.22)';
  [1.6, 3.4, -1.6, -3.4].forEach((d) => g.fillRect(X(d) - 8, 0, 16, H));
  // shoulders
  g.fillStyle = 'rgba(120,112,100,0.38)';
  g.fillRect(0, 0, X(ROAD_HALF), H); g.fillRect(X(-ROAD_HALF), 0, W - X(-ROAD_HALF), H);
  // edge lines
  const line = (ctx, d, w, col, y0 = 0, y1 = H) => { ctx.fillStyle = col; ctx.fillRect(X(d) - w * pxPerM / 2, y0, w * pxPerM, y1 - y0); };
  [ROAD_HALF - 0.18, -(ROAD_HALF - 0.18)].forEach((d) => { line(g, d, 0.15, '#e9e6de'); line(e, d, 0.15, '#cfcac0'); });
  // center dashed (yellow)
  line(g, 0, 0.13, '#e8b23a', 0, H * 0.3); line(e, 0, 0.13, '#e8a830', 0, H * 0.3);
  const tex = new THREE.CanvasTexture(cv); tex.wrapS = THREE.ClampToEdgeWrapping; tex.wrapT = THREE.RepeatWrapping; tex.colorSpace = THREE.SRGBColorSpace; tex.anisotropy = 8;
  const em = new THREE.CanvasTexture(ev); em.wrapT = THREE.RepeatWrapping; em.colorSpace = THREE.SRGBColorSpace; em.anisotropy = 8;
  return { tex, em };
}
function chevronTexture() {
  const cv = document.createElement('canvas'); cv.width = 128; cv.height = 128;
  const g = cv.getContext('2d');
  g.fillStyle = '#16120c'; g.fillRect(0, 0, 128, 128);
  g.fillStyle = '#ffc04a';
  g.beginPath(); g.moveTo(30, 64); g.lineTo(72, 18); g.lineTo(98, 18); g.lineTo(56, 64); g.lineTo(98, 110); g.lineTo(72, 110); g.closePath(); g.fill();
  g.strokeStyle = '#ffc04a'; g.lineWidth = 6; g.strokeRect(3, 3, 122, 122);
  const t = new THREE.CanvasTexture(cv); t.colorSpace = THREE.SRGBColorSpace; return t;
}
function dotTexture() {
  const cv = document.createElement('canvas'); cv.width = 64; cv.height = 64;
  const g = cv.getContext('2d');
  const gr = g.createRadialGradient(32, 32, 0, 32, 32, 32);
  gr.addColorStop(0, 'rgba(255,255,255,1)'); gr.addColorStop(0.25, 'rgba(255,255,255,0.7)'); gr.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = gr; g.fillRect(0, 0, 64, 64);
  return new THREE.CanvasTexture(cv);
}
function shadowTexture() {
  const cv = document.createElement('canvas'); cv.width = 128; cv.height = 256;
  const g = cv.getContext('2d');
  const gr = g.createRadialGradient(64, 128, 10, 64, 128, 120);
  gr.addColorStop(0, 'rgba(0,0,0,0.75)'); gr.addColorStop(0.55, 'rgba(0,0,0,0.4)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
  g.save(); g.scale(1, 1); g.fillStyle = gr; g.fillRect(0, 0, 128, 256); g.restore();
  return new THREE.CanvasTexture(cv);
}

// ---------- prop geometries (vertex coloured, low poly) ----------
function colorize(geo, hex) {
  const c = new THREE.Color(hex);
  const n = geo.attributes.position.count;
  const arr = new Float32Array(n * 3);
  for (let i = 0; i < n; i++) { arr[i * 3] = c.r; arr[i * 3 + 1] = c.g; arr[i * 3 + 2] = c.b; }
  geo.setAttribute('color', new THREE.BufferAttribute(arr, 3));
  return geo;
}
function prep(g) { const ng = g.index ? g.toNonIndexed() : g; ng.deleteAttribute('uv'); return ng; }
function pineGeo(snow) {
  const parts = [];
  const trunk = prep(new THREE.CylinderGeometry(0.18, 0.28, 2.2, 5)); trunk.translate(0, 1.1, 0); parts.push(colorize(trunk, 0x4a3426));
  const tiers = [[2.6, 1.8, 3.4], [2.0, 3.6, 3.0], [1.4, 5.2, 2.6], [0.8, 6.6, 2.0]];
  tiers.forEach(([r, y, h], i) => {
    const c = prep(new THREE.ConeGeometry(r, h, 6)); c.rotateY(i * 0.5); c.translate(0, y + h / 2, 0);
    parts.push(colorize(c, snow ? (i % 2 ? 0xe8eef8 : 0x2c4a44) : (i % 2 ? 0x2f5a3c : 0x264c34)));
  });
  return mergeGeometries(parts);
}
function cypressGeo() {
  const parts = [];
  const trunk = prep(new THREE.CylinderGeometry(0.15, 0.22, 1.6, 5)); trunk.translate(0, 0.8, 0); parts.push(colorize(trunk, 0x4a3a2a));
  const body = prep(new THREE.IcosahedronGeometry(1, 0)); body.scale(1.3, 4.2, 1.3); body.translate(0, 5, 0); parts.push(colorize(body, 0x3d5a36));
  const top = prep(new THREE.IcosahedronGeometry(1, 0)); top.scale(0.9, 2.2, 0.9); top.translate(0.2, 8.4, 0); parts.push(colorize(top, 0x46663c));
  return mergeGeometries(parts);
}
function joshuaGeo() {
  const parts = [];
  const trunkCol = 0x6e5a44, tuft = 0x5f6e3a;
  const t = prep(new THREE.CylinderGeometry(0.22, 0.32, 3, 5)); t.translate(0, 1.5, 0); parts.push(colorize(t, trunkCol));
  const arms = [[0.7, 0.6, 1.8], [-0.8, -0.3, 1.6], [0.2, -0.9, 1.4]];
  arms.forEach(([ax, az, len]) => {
    const a = prep(new THREE.CylinderGeometry(0.13, 0.18, len, 5));
    a.translate(0, len / 2, 0); a.rotateZ(-ax * 0.6); a.rotateX(az * 0.6); a.translate(0, 2.8, 0); parts.push(colorize(a, trunkCol));
    const tip = new THREE.Vector3(0, len, 0).applyEuler(new THREE.Euler(az * 0.6, 0, -ax * 0.6)).add(new THREE.Vector3(0, 2.8, 0));
    const tf = prep(new THREE.OctahedronGeometry(0.55, 0)); tf.scale(1, 1.2, 1); tf.translate(tip.x, tip.y + 0.2, tip.z); parts.push(colorize(tf, tuft));
  });
  const top = prep(new THREE.OctahedronGeometry(0.6, 0)); top.translate(0, 3.2, 0); parts.push(colorize(top, tuft));
  return mergeGeometries(parts);
}
function rockGeo() {
  const g = new THREE.IcosahedronGeometry(1, 0);
  const pos = g.attributes.position; const rng = mulberry32(5);
  for (let i = 0; i < pos.count; i++) { pos.setXYZ(i, pos.getX(i) * (0.75 + rng() * 0.5), pos.getY(i) * (0.55 + rng() * 0.3), pos.getZ(i) * (0.75 + rng() * 0.5)); }
  g.computeVertexNormals();
  return colorize(prep(g), 0x8a8078);
}
function shrubGeo() {
  const parts = [];
  [[0, 0.35, 0, 0.6], [0.45, 0.25, 0.2, 0.42], [-0.35, 0.28, -0.25, 0.45]].forEach(([x, y, z, r]) => {
    const s = prep(new THREE.IcosahedronGeometry(r, 0)); s.scale(1, 0.75, 1); s.translate(x, y, z); parts.push(colorize(s, 0x7a7a52));
  });
  return mergeGeometries(parts);
}

// ---------- sky ----------
const skyVert = `varying vec3 vDir; void main(){ vDir = normalize(position); vec4 p = modelViewMatrix * vec4(position,1.0); gl_Position = projectionMatrix * p; gl_Position.z = gl_Position.w; }`;
const skyFrag = `
uniform vec3 top; uniform vec3 mid; uniform vec3 horizon; uniform vec3 fogCol; uniform vec3 discCol; uniform vec3 discDir; uniform float discSize; uniform float stars; uniform float time;
varying vec3 vDir;
float hash(vec3 p){ p = fract(p*0.3183099+.1); p*=17.0; return fract(p.x*p.y*p.z*(p.x+p.y+p.z)); }
void main(){
  vec3 d = normalize(vDir); float h = d.y;
  vec3 col = mix(horizon, mid, smoothstep(0.0, 0.22, h));
  col = mix(col, top, smoothstep(0.18, 0.75, h));
  col = mix(col, fogCol, smoothstep(0.02, -0.12, h));
  float sd = max(dot(d, normalize(discDir)), 0.0);
  float disc = smoothstep(1.0 - 0.00035*discSize*discSize, 1.0 - 0.0002*discSize*discSize, sd);
  col += discCol * (disc * 3.0 + pow(sd, 14.0/discSize) * 0.35 + pow(sd, 3.0) * 0.12);
  // stars
  vec3 q = floor(d * 380.0);
  float n = hash(q);
  float tw = 0.6 + 0.4*sin(time*2.0 + n*60.0);
  col += vec3(0.85,0.9,1.0) * step(0.9975, n) * stars * tw * smoothstep(0.03, 0.25, h) * 1.4;
  gl_FragColor = vec4(col, 1.0);
}`;

// ---------- grade / vignette pass ----------
export const GradeShader = {
  uniforms: { tDiffuse: { value: null }, time: { value: 0 }, vig: { value: 0.32 }, grain: { value: 0.035 }, ab: { value: 0.0 } },
  vertexShader: `varying vec2 vUv; void main(){ vUv = uv; gl_Position = projectionMatrix*modelViewMatrix*vec4(position,1.0); }`,
  fragmentShader: `uniform sampler2D tDiffuse; uniform float time; uniform float vig; uniform float grain; uniform float ab; varying vec2 vUv;
  float rand(vec2 c){ return fract(sin(dot(c, vec2(12.9898,78.233)))*43758.5453); }
  void main(){
    vec2 c = vUv - 0.5; float r = length(c);
    vec2 off = c * ab * r;
    vec3 col; col.r = texture2D(tDiffuse, vUv - off).r; col.g = texture2D(tDiffuse, vUv).g; col.b = texture2D(tDiffuse, vUv + off).b;
    col *= 1.0 - vig * smoothstep(0.25, 0.85, r);
    col += (rand(vUv*vec2(1920.0,1080.0) + fract(time)*100.0) - 0.5) * grain;
    gl_FragColor = vec4(col, 1.0);
  }`,
};

// ---------- World ----------
export class World {
  constructor(renderer, road, models = {}) {
    this.renderer = renderer; this.road = road;
    const scene = this.scene = new THREE.Scene();
    scene.fog = new THREE.Fog(0xd98f74, 70, 760);

    // sky
    this.skyMat = new THREE.ShaderMaterial({
      vertexShader: skyVert, fragmentShader: skyFrag, side: THREE.BackSide, depthWrite: false, fog: false,
      uniforms: { top: { value: new THREE.Color() }, mid: { value: new THREE.Color() }, horizon: { value: new THREE.Color() }, fogCol: { value: new THREE.Color() }, discCol: { value: new THREE.Color() }, discDir: { value: new THREE.Vector3(0, 0.1, 1) }, discSize: { value: 1 }, stars: { value: 0 }, time: { value: 0 } },
    });
    this.sky = new THREE.Mesh(new THREE.SphereGeometry(2000, 32, 16), this.skyMat);
    this.sky.renderOrder = -10; this.sky.frustumCulled = false;
    scene.add(this.sky);
    this.envScene = new THREE.Scene();
    this.envScene.add(new THREE.Mesh(new THREE.SphereGeometry(50, 32, 16), this.skyMat));
    this.pmrem = new THREE.PMREMGenerator(renderer);
    this.envTimer = 0; this.envRT = null;

    // lights
    this.hemi = new THREE.HemisphereLight(0xffffff, 0x444444, 1); scene.add(this.hemi);
    this.sun = new THREE.DirectionalLight(0xffffff, 2);
    this.sun.castShadow = true;
    this.sun.shadow.mapSize.set(2048, 2048);
    const sc = this.sun.shadow.camera; sc.left = -45; sc.right = 45; sc.top = 45; sc.bottom = -45; sc.near = 1; sc.far = 260;
    this.sun.shadow.bias = -0.0004; this.sun.shadow.normalBias = 0.04;
    scene.add(this.sun); scene.add(this.sun.target);

    // mountains
    this.mountains = [];
    [[1500, 0.0, 240, 17], [1150, 0.5, 170, 31]].forEach(([R, ph, H, seed], li) => {
      const N = 220; const pos = [];
      const rng = mulberry32(seed);
      for (let i = 0; i <= N; i++) {
        const a = (i / N) * Math.PI * 2;
        const h = 40 + H * (0.5 + 0.5 * noise2(i * 0.09 + ph * 10, seed)) * (0.55 + 0.45 * noise2(i * 0.025, seed + 3)) + rng() * 10;
        pos.push([Math.cos(a) * R, h, Math.sin(a) * R]);
      }
      const verts = [], idx = [];
      pos.forEach(([x, h, z]) => { verts.push(x, -120, z, x, h, z); });
      for (let i = 0; i < N; i++) { const a = i * 2; idx.push(a, a + 2, a + 1, a + 1, a + 2, a + 3); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3)); g.setIndex(idx);
      const m = new THREE.Mesh(g, new THREE.MeshBasicMaterial({ color: 0x555577, fog: false, side: THREE.DoubleSide }));
      m.frustumCulled = false; m.renderOrder = -5 + li;
      scene.add(m); this.mountains.push(m);
    });
    // distant ground
    this.groundDisc = new THREE.Mesh(new THREE.CircleGeometry(2600, 48), new THREE.MeshLambertMaterial({ color: 0x553322 }));
    this.groundDisc.rotation.x = -Math.PI / 2; this.groundDisc.renderOrder = -4;
    scene.add(this.groundDisc);
    // horizon for reflections: ground + scaled mountain silhouettes inside the env-capture scene
    this.envGround = new THREE.Mesh(new THREE.CircleGeometry(49, 32), new THREE.MeshBasicMaterial({ color: 0x553322, side: THREE.DoubleSide }));
    this.envGround.rotation.x = -Math.PI / 2; this.envGround.position.y = -0.6; this.envScene.add(this.envGround);
    this.envMounts = this.mountains.map((m) => {
      const c = new THREE.Mesh(m.geometry, new THREE.MeshBasicMaterial({ color: 0x555577, side: THREE.DoubleSide }));
      const s = 44 / 1500; c.scale.setScalar(s); c.position.y = -25 * s; this.envScene.add(c); return c;
    });

    // materials
    const rt = roadTextures();
    this.roadMat = new THREE.MeshStandardMaterial({ map: rt.tex, emissiveMap: rt.em, emissive: 0xffffff, emissiveIntensity: 0.0, roughness: 0.78, metalness: 0.0, envMapIntensity: 0.6 });
    this.terrainMat = new THREE.MeshStandardMaterial({ vertexColors: true, flatShading: true, roughness: 0.96, metalness: 0, envMapIntensity: 0.4 });
    try {
      upgradeRoadMaterial(this.roadMat, SHOULDER, ROAD_HALF);
      this.terrainMats = [terrainMaterial('mojave', { scale: 0.2 }), terrainMaterial('sierra', { scale: 0.22 }), terrainMaterial('glacier', { scale: 0.12, nStr: 0.7 }), terrainMaterial('coast', { scale: 0.24 })];
    } catch (e) { console.warn('surface textures failed', e); this.terrainMats = null; }
    this.propMat = new THREE.MeshStandardMaterial({ vertexColors: true, flatShading: true, roughness: 0.9, envMapIntensity: 0.4 });
    this.postMat = new THREE.MeshStandardMaterial({ color: 0xd8d4cc, roughness: 0.6 });
    this.reflMat = new THREE.MeshStandardMaterial({ color: 0x331a00, emissive: 0xffb04a, emissiveIntensity: 1.2, roughness: 0.4 });
    this.reflMatR = new THREE.MeshStandardMaterial({ color: 0x220000, emissive: 0xff5a3a, emissiveIntensity: 1.0, roughness: 0.4 });
    const chev = chevronTexture();
    this.chevMat = new THREE.MeshStandardMaterial({ map: chev, emissiveMap: chev, emissive: 0xffffff, emissiveIntensity: 0.55, side: THREE.DoubleSide, roughness: 0.5 });

    this.geos = {
      pine: pineGeo(false), snowpine: pineGeo(true), cypress: cypressGeo(), joshua: joshuaGeo(), rock: rockGeo(), shrub: shrubGeo(),
      post: new THREE.BoxGeometry(0.1, 1.05, 0.1).translate(0, 0.52, 0),
      refl: new THREE.BoxGeometry(0.115, 0.17, 0.03).translate(0, 0.9, 0),
      chev: new THREE.PlaneGeometry(0.85, 0.85).translate(0, 1.55, 0),
      chevPost: new THREE.BoxGeometry(0.07, 1.2, 0.07).translate(0, 0.6, 0),
    };
    this.kit = {};
    if (models.nature) {
      const spec = { Pine: { wind: 0.0016 }, Pine2: { wind: 0.002 }, Cypress: { wind: 0.002 }, Joshua: { wind: 0.0006 },
        Rock: {}, Rock2: {}, Rock3: {}, Shrub: { wind: 0.025, shadow: false }, Grass: { wind: 0.12, shadow: false, snow: 0.5 } };
      for (const [n, o] of Object.entries(spec)) {
        try { const k = kitFromGltf(models.nature, n, o); if (k) this.kit[n] = k; } catch (e) { console.warn('kit', n, e); }
      }
    }
    if (models.props) {
      const map = { Pine: 'pine', SnowPine: 'snowpine', Joshua: 'joshua', Cypress: 'cypress', Rock: 'rock', Rock2: 'rock2', Shrub: 'shrub', Grass: 'grass', Delineator: 'post' };
      for (const [n, k] of Object.entries(map)) {
        try { const g = geoFromGltf(models.props, n); if (g) { if (k === 'joshua') g.scale(0.72, 0.72, 0.72); if (k === 'post') g.deleteAttribute('color'); g.computeBoundingSphere(); this.geos[k] = g; } }
        catch (e) { console.warn('prop', n, e); }
      }
      this.modelProps = true;
    }
    this.chunks = new Map();
    this.buildQueue = [];

    this._buildParticles();
    this.dotTex = dotTexture();
    this.shadowTex = shadowTexture();
    this.cur = {};
    this.applyBiome(biomeAt(0), true);
  }

  // ---------- chunk building ----------
  buildChunk(ci) {
    const road = this.road;
    const i0 = ci * CHUNK, i1 = i0 + CHUNK;
    road.ensure(i1 + 200);
    const group = new THREE.Group();
    const rng = mulberry32(ci * 7919 + 13);

    // road ribbon
    const cols = [SHOULDER, ROAD_HALF, ROAD_HALF - 0.3, 2.5, 0, -2.5, -(ROAD_HALF - 0.3), -ROAD_HALF, -SHOULDER];
    {
      const pos = [], uv = [], idx = [];
      for (let i = i0; i <= i1; i++) {
        const p = road.get(i);
        for (const d of cols) {
          const x = p.x + p.cz * d, z = p.z - p.sx * d;
          pos.push(x, road.heightAt(p, d, x, z) + 0.015, z);
          uv.push((SHOULDER - d) / (SHOULDER * 2), p.s / 12);
        }
      }
      const C = cols.length;
      for (let r = 0; r < CHUNK; r++) for (let c = 0; c < C - 1; c++) {
        const a = r * C + c, b = a + 1, d = a + C, e = d + 1;
        idx.push(a, b, d, b, e, d);
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
      g.setIndex(idx); g.computeVertexNormals();
      const m = new THREE.Mesh(g, this.roadMat); m.receiveShadow = true;
      group.add(m);
    }

    // terrain strips
    const dl = [SHOULDER, 7.6, 9.2, 11.5, 14.5, 18.5, 24, 31, 40, 52, 68, 90, 120, 160, 220, 300, 420];
    for (const side of [1, -1]) {
      const pos = [], col = [], idx = [];
      const step = 2;
      const rows = CHUNK / step;
      const cA = new THREE.Color(), cB = new THREE.Color(), cD = new THREE.Color(), cc = new THREE.Color();
      for (let r = 0; r <= rows; r++) {
        const p = road.get(i0 + r * step);
        const bm = biomeAt(p.s);
        biomeColor('ground', bm, cA); biomeColor('ground2', bm, cB); biomeColor('dirt', bm, cD);
        for (const d0 of dl) {
          const dn = side * d0;
          const d = road.compressD(p, dn);
          const x = p.x + p.cz * d, z = p.z - p.sx * d;
          const y = road.heightAt(p, d, x, z);
          pos.push(x, y, z);
          const n = noise2(x * 0.03, z * 0.03) * 0.5 + 0.5;
          const n2 = noise2(x * 0.15 + 4, z * 0.15) * 0.5 + 0.5;
          cc.copy(cA).lerp(cB, clamp(n * 0.9 + n2 * 0.25, 0, 1));
          const near = clamp(1 - (d0 - SHOULDER) / 4, 0, 1);
          cc.lerp(cD, near * 0.85);
          col.push(cc.r, cc.g, cc.b);
        }
      }
      const C = dl.length;
      for (let r = 0; r < rows; r++) for (let c = 0; c < C - 1; c++) {
        const a = r * C + c, b = a + 1, d = a + C, e = d + 1;
        if (side > 0) idx.push(a, d, b, b, d, e); else idx.push(a, b, d, b, e, d);
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
      g.setIndex(idx); g.computeVertexNormals();
      const tb = biomeAt(road.get(i0 + CHUNK / 2).s); const ti = tb.t < 0.5 ? tb.a : tb.b;
      const m = new THREE.Mesh(g, this.terrainMats ? this.terrainMats[ti] : this.terrainMat); m.receiveShadow = true;
      group.add(m);
    }

    // props
    const pm = biomeAt(road.get(i0 + CHUNK / 2).s);
    const A = BIOMES[pm.a], B = BIOMES[pm.b];
    const density = lerp(A.density, B.density, pm.t);
    const types = {};
    for (const [k, w] of Object.entries(A.props)) types[k] = (types[k] || 0) + w * (1 - pm.t);
    for (const [k, w] of Object.entries(B.props)) types[k] = (types[k] || 0) + w * pm.t;
    const tot = Object.values(types).reduce((a, b) => a + b, 0);
    const lists = {}; Object.keys(types).forEach((k) => (lists[k] = [])); lists.rock2 = [];
    const n = Math.round(density * (0.8 + rng() * 0.4));
    const mtx = new THREE.Matrix4(), q = new THREE.Quaternion(), sc = new THREE.Vector3(), ps = new THREE.Vector3(), up = new THREE.Vector3(0, 1, 0);
    for (let k = 0; k < n; k++) {
      let pick = rng() * tot, type = null;
      for (const [t, w] of Object.entries(types)) { pick -= w; if (pick <= 0) { type = t; break; } }
      if (!type) type = Object.keys(types)[0];
      if (type === 'rock' && this.geos.rock2 && rng() < 0.5) type = 'rock2';
      const ii = i0 + Math.floor(rng() * CHUNK);
      const p = road.get(ii);
      const side = rng() < 0.5 ? -1 : 1;
      const minD = type === 'shrub' || type === 'rock' || type === 'rock2' ? 9 : 11;
      let d0 = minD + Math.pow(rng(), 1.8) * 190;
      let d = road.compressD(p, side * d0);
      const x = p.x + p.cz * d, z = p.z - p.sx * d;
      // reject if near any other part of the road
      let ok = true;
      for (let j = -50; j <= 50; j += 3) {
        const o = road.get(ii + j);
        if ((o.x - x) ** 2 + (o.z - z) ** 2 < (minD - 1) ** 2) { ok = false; break; }
      }
      if (!ok) continue;
      const y = road.heightAt(p, d, x, z);
      let s = 1;
      if (type === 'pine' || type === 'snowpine') s = 0.8 + rng() * 1.1;
      else if (type === 'cypress') s = 0.8 + rng() * 0.6;
      else if (type === 'joshua') s = 0.8 + rng() * 0.8;
      else if (type === 'rock' || type === 'rock2') s = 0.5 + Math.pow(rng(), 2) * 4.5;
      else if (type === 'shrub') s = 0.6 + rng() * 0.9;
      lists[type].push({ x, y: y - 0.15, z, s, rot: rng() * Math.PI * 2, tint: 0.85 + rng() * 0.3 });
    }
    const domI = pm.t < 0.5 ? pm.a : pm.b;
    const snowy = BIOMES[domI].name.startsWith('Glacier');
    const kitName = (type, r) => {
      if (type === 'pine' || type === 'snowpine') return r < 0.55 ? 'Pine' : 'Pine2';
      if (type === 'cypress') return 'Cypress';
      if (type === 'joshua') return 'Joshua';
      if (type === 'shrub') return 'Shrub';
      if (type === 'rock' || type === 'rock2') return domI === 0 ? (r < 0.6 ? 'Rock' : 'Rock3') : (r < 0.55 ? 'Rock2' : 'Rock3');
      return null;
    };
    const kitLists = {};
    if (Object.keys(this.kit).length) {
      for (const [type, arr] of Object.entries(lists)) {
        for (const o of [...arr]) {
          const kn = kitName(type, rng());
          if (kn && this.kit[kn]) { arr.splice(arr.indexOf(o), 1); if (kn.startsWith('Pine') && rng() < 0.4) continue; (kitLists[kn] = kitLists[kn] || []).push(o); }
        }
      }
    }
    for (const [kn, arr] of Object.entries(kitLists)) {
      const k = this.kit[kn];
      const im = new THREE.InstancedMesh(k.geo, snowy ? k.snowMats : k.mats, arr.length);
      if (snowy) im.material = k.snowMats;
      const tc = new THREE.Color();
      const isRock = kn.startsWith('Rock');
      arr.forEach((o, j) => {
        q.setFromAxisAngle(up, o.rot);
        let s = o.s;
        if (isRock) s = 0.45 + Math.pow(rng(), 2.2) * 2.6;
        if (kn === 'Joshua') s = 0.75 + rng() * 0.6;
        sc.set(s, s * (isRock ? 0.6 + rng() * 0.5 : 1), s);
        ps.set(o.x, o.y + (isRock ? -0.12 * s : 0.05), o.z);
        mtx.compose(ps, q, sc); im.setMatrixAt(j, mtx);
        const v = isRock ? 0.85 + rng() * 0.25 : o.tint;
        if (isRock) tc.setRGB(v * (0.97 + rng() * 0.06), v, v * (0.95 + rng() * 0.06));
        else { const f = 0.9 + rng() * 0.2; tc.setRGB(f * 0.88, f, f * 0.92); }
        im.setColorAt(j, tc);
      });
      im.castShadow = k.shadow; im.receiveShadow = true;
      im.computeBoundingSphere();
      group.add(im);
    }
    for (const [type, arr] of Object.entries(lists)) {
      if (!arr.length) continue;
      const im = new THREE.InstancedMesh(this.geos[type], this.propMat, arr.length);
      const tc = new THREE.Color();
      arr.forEach((o, j) => {
        q.setFromAxisAngle(up, o.rot);
        sc.set(o.s, o.s * (type === 'rock' || type === 'rock2' ? 0.8 : 1), o.s);
        ps.set(o.x, o.y, o.z);
        mtx.compose(ps, q, sc); im.setMatrixAt(j, mtx);
        tc.setScalar(o.tint); im.setColorAt(j, tc);
      });
      im.castShadow = type !== 'shrub'; im.receiveShadow = true;
      im.computeBoundingSphere();
      group.add(im);
    }

    // grass tufts hugging the shoulders
    if (this.geos.grass) {
      const gN = Math.round(lerp(A.grass || 0, B.grass || 0, pm.t));
      const tintA = new THREE.Color(A.grassTint || 0xffffff), tintB = new THREE.Color(B.grassTint || 0xffffff);
      const tint = tintA.clone().lerp(tintB, pm.t);
      if (gN > 0) {
        const gk = this.kit.Grass;
        const im = new THREE.InstancedMesh(gk ? gk.geo : this.geos.grass, gk ? (snowy ? gk.snowMats : gk.mats) : this.propMat, gN);
        const tc = new THREE.Color();
        let used = 0;
        for (let k = 0; k < gN; k++) {
          const ii = i0 + Math.floor(rng() * CHUNK);
          const p = road.get(ii);
          const side = rng() < 0.5 ? -1 : 1;
          const d0 = 7.7 + Math.pow(rng(), 1.6) * 36;
          const d = road.compressD(p, side * d0);
          const x = p.x + p.cz * d, z = p.z - p.sx * d;
          const y = road.heightAt(p, d, x, z);
          const s = gk ? 0.8 + rng() * 0.9 : 0.7 + rng() * 0.9;
          q.setFromAxisAngle(up, rng() * Math.PI * 2); sc.set(s, s * (0.8 + rng() * 0.5), s); ps.set(x, y - 0.05, z);
          mtx.compose(ps, q, sc); im.setMatrixAt(used, mtx);
          tc.copy(tint).multiplyScalar(0.8 + rng() * 0.35); im.setColorAt(used, tc);
          used++;
        }
        im.count = used; im.computeBoundingSphere(); im.receiveShadow = true;
        group.add(im);
      }
    }

    // reflector posts every 24 m, both sides
    {
      const spots = [];
      for (let i = i0; i < i1; i += 12) {
        const p = road.get(i);
        for (const side of [1, -1]) {
          const d = side * 7.3;
          const x = p.x + p.cz * d, z = p.z - p.sx * d;
          spots.push({ x, y: road.heightAt(p, d, x, z), z, h: p.h, side });
        }
      }
      const posts = new THREE.InstancedMesh(this.geos.post, this.postMat, spots.length);
      const L = spots.filter((s) => s.side > 0), R = spots.filter((s) => s.side < 0);
      const reflL = new THREE.InstancedMesh(this.geos.refl, this.reflMat, L.length);
      const reflR = new THREE.InstancedMesh(this.geos.refl, this.reflMatR, R.length);
      let li = 0, ri = 0;
      spots.forEach((s, j) => {
        q.setFromAxisAngle(up, s.h); sc.set(1, 1, 1); ps.set(s.x, s.y, s.z);
        mtx.compose(ps, q, sc); posts.setMatrixAt(j, mtx);
        if (s.side > 0) reflL.setMatrixAt(li++, mtx); else reflR.setMatrixAt(ri++, mtx);
      });
      [posts, reflL, reflR].forEach((m) => { m.computeBoundingSphere(); group.add(m); });
      posts.castShadow = true;
    }

    // curve chevrons on the outside of tighter bends
    {
      const spots = [];
      for (let i = i0; i < i1; i++) {
        const p = road.get(i);
        if (Math.abs(p.k) > 1 / 160 && i % 9 === 0) {
          const d = -Math.sign(p.k) * 8.6;
          const x = p.x + p.cz * d, z = p.z - p.sx * d;
          spots.push({ x, y: road.heightAt(p, d, x, z), z, h: p.h + Math.PI, flip: p.k < 0 });
        }
      }
      if (spots.length) {
        const ch = new THREE.InstancedMesh(this.geos.chev, this.chevMat, spots.length);
        const cp = new THREE.InstancedMesh(this.geos.chevPost, this.postMat, spots.length);
        spots.forEach((s, j) => {
          q.setFromAxisAngle(up, s.h); ps.set(s.x, s.y, s.z);
          sc.set(s.flip ? -1 : 1, 1, 1); mtx.compose(ps, q, sc); ch.setMatrixAt(j, mtx);
          sc.set(1, 1, 1); mtx.compose(ps, q, sc); cp.setMatrixAt(j, mtx);
        });
        ch.computeBoundingSphere(); cp.computeBoundingSphere();
        group.add(ch); group.add(cp);
      }
    }

    this.scene.add(group);
    this.chunks.set(ci, group);
  }
  disposeChunk(ci) {
    const g = this.chunks.get(ci); if (!g) return;
    g.traverse((o) => {
      if (o.isInstancedMesh) { o.dispose(); }
      else if (o.isMesh) o.geometry.dispose();
    });
    this.scene.remove(g); this.chunks.delete(ci);
  }
  updateChunks(carIndex, sync = false) {
    const cc = Math.floor(carIndex / CHUNK);
    const lo = Math.max(0, cc - 2), hi = cc + 6;
    for (const k of [...this.chunks.keys()]) if (k < lo || k > hi) this.disposeChunk(k);
    const need = [];
    for (let k = lo; k <= hi; k++) if (!this.chunks.has(k)) need.push(k);
    need.sort((a, b) => Math.abs(a - cc) - Math.abs(b - cc));
    if (sync) need.forEach((k) => this.buildChunk(k));
    else if (need.length) this.buildChunk(need[0]); // one per frame to avoid hitches
  }
  resetChunks() { for (const k of [...this.chunks.keys()]) this.disposeChunk(k); }

  // ---------- particles ----------
  _buildParticles() {
    const N = 900;
    const g = new THREE.BufferGeometry();
    this.pPos = new Float32Array(N * 3);
    this.pSeed = new Float32Array(N);
    for (let i = 0; i < N; i++) {
      this.pPos[i * 3] = (Math.random() - 0.5) * 120; this.pPos[i * 3 + 1] = Math.random() * 40 - 8; this.pPos[i * 3 + 2] = (Math.random() - 0.5) * 120;
      this.pSeed[i] = Math.random() * 100;
    }
    g.setAttribute('position', new THREE.BufferAttribute(this.pPos, 3));
    this.pMat = new THREE.PointsMaterial({ size: 0.15, transparent: true, opacity: 0.6, depthWrite: false, blending: THREE.AdditiveBlending, color: 0xffffff });
    this.points = new THREE.Points(g, this.pMat); this.points.frustumCulled = false;
    this.scene.add(this.points);
  }
  updateParticles(dt, cam, carVel, time) {
    if (!this.pMat.map) { this.pMat.map = this.dotTex; this.pMat.needsUpdate = true; }
    const P = this.cur.particle; if (!P) return;
    const N = this.pSeed.length, pos = this.pPos;
    const S = 120, H = 40;
    for (let i = 0; i < N; i++) {
      const s = this.pSeed[i];
      let x = pos[i * 3], y = pos[i * 3 + 1], z = pos[i * 3 + 2];
      x += (Math.sin(time * 0.5 + s) * P.drift) * dt;
      z += (Math.cos(time * 0.4 + s * 1.3) * P.drift) * dt;
      y += (P.fall + Math.sin(time * 0.8 + s) * 0.15) * dt;
      // wrap around camera
      x = cam.x + ((((x - cam.x) + S / 2) % S + S) % S) - S / 2;
      z = cam.z + ((((z - cam.z) + S / 2) % S + S) % S) - S / 2;
      y = cam.y + ((((y - cam.y) + 10) % H + H) % H) - 10;
      pos[i * 3] = x; pos[i * 3 + 1] = y; pos[i * 3 + 2] = z;
    }
    this.points.geometry.attributes.position.needsUpdate = true;
  }

  // ---------- biome blending ----------
  applyBiome(bm, force = false) {
    const A = BIOMES[bm.a], B = BIOMES[bm.b], t = bm.t;
    const L = (k) => lerp(A[k], B[k], t);
    const C = (k, out) => biomeColor(k, bm, out);
    const u = this.skyMat.uniforms;
    C('skyTop', u.top.value); C('skyMid', u.mid.value); C('skyHorizon', u.horizon.value); C('fog', u.fogCol.value); C('disc', u.discCol.value);
    u.discDir.value.set(lerp(A.discDir[0], B.discDir[0], t), lerp(A.discDir[1], B.discDir[1], t), lerp(A.discDir[2], B.discDir[2], t)).normalize();
    u.discSize.value = L('discSize'); u.stars.value = L('stars');
    C('fog', this.scene.fog.color); this.scene.fog.near = L('fogNear'); this.scene.fog.far = L('fogFar');
    C('sun', this.sun.color); this.sun.intensity = L('sunI');
    C('hemiSky', this.hemi.color); C('hemiGround', this.hemi.groundColor); this.hemi.intensity = L('hemiI');
    C('mount', this.mountains[1].material.color); C('mount2', this.mountains[0].material.color);
    // blend mountains toward fog for aerial perspective
    this.mountains[0].material.color.lerp(this.scene.fog.color, 0.45);
    this.mountains[1].material.color.lerp(this.scene.fog.color, 0.25);
    C('ground2', this.groundDisc.material.color);
    if (this.envGround) {
      this.envGround.material.color.copy(this.groundDisc.material.color).multiply(this.hemi.color).multiplyScalar(0.55 * this.hemi.intensity + 0.15 * this.sun.intensity);
      this.envMounts.forEach((c, i) => c.material.color.copy(this.mountains[i].material.color));
    }
    this.cur.headI = L('headI'); this.cur.exposure = L('exposure'); this.cur.bloom = L('bloom');
    const PA = A.particle, PB = B.particle;
    this.cur.particle = { fall: lerp(PA.fall, PB.fall, t), drift: lerp(PA.drift, PB.drift, t) };
    this.pMat.color.setHex(PA.color).lerp(_c3.setHex(PB.color), t);
    this.pMat.size = lerp(PA.size, PB.size, t);
    this.pMat.opacity = lerp(PA.opacity, PB.opacity, t);
    this.cur.bm = bm;
    if (force) this.refreshEnv();
  }
  refreshEnv() {
    if (this.envRT) this.envRT.dispose();
    const dc = this.skyMat.uniforms.discCol.value, keep = dc.clone();
    dc.multiplyScalar(0.18); // the sun disc is HDR-bright: keep it out of the paint's reflections
    this.envRT = this.pmrem.fromScene(this.envScene, 0, 0.1, 200);
    dc.copy(keep);
    this.scene.environment = this.envRT.texture;
  }
  followCamera(cam, carPos, sunOffsetDir, dt) {
    surf.time.value += dt;
    this.sky.position.copy(cam.position);
    this.mountains.forEach((m) => { m.position.set(cam.position.x, carPos.y - 25, cam.position.z); });
    this.groundDisc.position.set(cam.position.x, carPos.y - 34, cam.position.z);
    // sun follows car for shadows
    const dir = this.skyMat.uniforms.discDir.value;
    const elev = Math.max(dir.y, 0.25);
    _v.set(dir.x, elev, dir.z).normalize();
    this.sun.position.set(carPos.x + _v.x * 120, carPos.y + _v.y * 120, carPos.z + _v.z * 120);
    this.sun.target.position.copy(carPos);
    this.envTimer -= dt;
    if (this.envTimer <= 0 && this.cur.bm && this.cur.bm.t > 0 && this.cur.bm.t < 1) { this.refreshEnv(); this.envTimer = 1.5; }
  }
}
const _v = new THREE.Vector3();

// ---------- the car ----------
function tumble(geo, y0, y1, f) {
  const p = geo.attributes.position;
  for (let i = 0; i < p.count; i++) {
    const y = p.getY(i); const t = clamp((y - y0) / (y1 - y0), 0, 1);
    p.setX(i, p.getX(i) * lerp(1, f, t));
  }
  geo.computeVertexNormals();
}
function buildProcCar() {
  const root = new THREE.Group();
  const body = new THREE.Group(); root.add(body);
  const paint = new THREE.MeshPhysicalMaterial({ color: 0xe6e0d4, roughness: 0.32, metalness: 0.15, clearcoat: 1, clearcoatRoughness: 0.08, envMapIntensity: 1.2 });
  const dark = new THREE.MeshStandardMaterial({ color: 0x0c0d10, roughness: 0.6, metalness: 0.2 });
  const glass = new THREE.MeshPhysicalMaterial({ color: 0x0a0c12, roughness: 0.05, metalness: 0.4, clearcoat: 1, envMapIntensity: 1.6 });

  const s = new THREE.Shape();
  const arch = (cx) => { s.lineTo(cx - 0.42, 0.3); s.absarc(cx, 0.34, 0.42, Math.PI, 0, true); s.lineTo(cx + 0.42, 0.3); };
  s.moveTo(-2.18, 0.3);
  arch(-1.36);
  arch(1.24);
  s.lineTo(2.1, 0.3); s.lineTo(2.25, 0.44); s.lineTo(2.23, 0.62); s.lineTo(1.9, 0.72); s.lineTo(0.95, 0.88);
  s.lineTo(-1.6, 0.94); s.lineTo(-2.14, 0.92); s.lineTo(-2.26, 0.82); s.lineTo(-2.26, 0.42); s.lineTo(-2.18, 0.3);
  const bg = new THREE.ExtrudeGeometry(s, { depth: 1.7, bevelEnabled: true, bevelThickness: 0.07, bevelSize: 0.05, bevelSegments: 3, curveSegments: 10 });
  bg.rotateY(-Math.PI / 2); bg.translate(0.85, 0, 0);
  tumble(bg, 0.72, 0.98, 0.92);
  const bodyMesh = new THREE.Mesh(bg, paint); bodyMesh.castShadow = true; body.add(bodyMesh);

  const cs = new THREE.Shape();
  cs.moveTo(-1.62, 0.93); cs.lineTo(0.92, 0.89); cs.lineTo(0.05, 1.29); cs.lineTo(-0.88, 1.31); cs.lineTo(-1.62, 0.93);
  const cg = new THREE.ExtrudeGeometry(cs, { depth: 1.36, bevelEnabled: true, bevelThickness: 0.06, bevelSize: 0.05, bevelSegments: 3 });
  cg.rotateY(-Math.PI / 2); cg.translate(0.68, 0, 0);
  tumble(cg, 0.9, 1.36, 0.72);
  const cabin = new THREE.Mesh(cg, glass); cabin.castShadow = true; body.add(cabin);
  // roof panel in paint
  const rs = new THREE.Shape();
  rs.moveTo(-0.95, 1.29); rs.lineTo(0.12, 1.27); rs.lineTo(0.06, 1.33); rs.lineTo(-0.86, 1.35); rs.lineTo(-0.95, 1.29);
  const rg = new THREE.ExtrudeGeometry(rs, { depth: 1.28, bevelEnabled: true, bevelThickness: 0.04, bevelSize: 0.04, bevelSegments: 2 });
  rg.rotateY(-Math.PI / 2); rg.translate(0.64, 0, 0);
  tumble(rg, 0.9, 1.36, 0.72);
  // ducktail spoiler
  const sp = new THREE.Mesh(new THREE.BoxGeometry(1.55, 0.05, 0.28), paint); sp.position.set(0, 0.97, -2.0); sp.rotation.x = 0.12; body.add(sp);
  body.add(new THREE.Mesh(rg, paint));

  // lower trim / diffuser
  const trim = new THREE.Mesh(new THREE.BoxGeometry(1.78, 0.12, 4.3), dark); trim.position.set(0, 0.3, -0.02); body.add(trim);
  const diff = new THREE.Mesh(new THREE.BoxGeometry(1.5, 0.16, 0.1), dark); diff.position.set(0, 0.42, -2.3); body.add(diff);
  const grille = new THREE.Mesh(new THREE.BoxGeometry(1.2, 0.1, 0.06), dark); grille.position.set(0, 0.45, 2.3); body.add(grille);
  // lights
  const tailMat = new THREE.MeshStandardMaterial({ color: 0x300000, emissive: 0xff1e30, emissiveIntensity: 2.2 });
  const tail = new THREE.Mesh(new THREE.BoxGeometry(1.62, 0.07, 0.04), tailMat); tail.position.set(0, 0.8, -2.32); body.add(tail);
  const headMat = new THREE.MeshStandardMaterial({ color: 0x222222, emissive: 0xfff2dc, emissiveIntensity: 3.0 });
  [-0.62, 0.62].forEach((x) => { const h = new THREE.Mesh(new THREE.BoxGeometry(0.42, 0.055, 0.05), headMat); h.position.set(x, 0.63, 2.27); body.add(h); });
  // mirrors
  [-0.86, 0.86].forEach((x) => { const m = new THREE.Mesh(new THREE.BoxGeometry(0.16, 0.08, 0.12), paint); m.position.set(x * 1.0, 1.0, 0.72); body.add(m); });

  // wheels
  const tyre = new THREE.MeshStandardMaterial({ color: 0x141414, roughness: 0.85 });
  const rim = new THREE.MeshStandardMaterial({ color: 0x8a8a90, roughness: 0.3, metalness: 0.9 });
  const tg = new THREE.CylinderGeometry(0.33, 0.33, 0.25, 22); tg.rotateZ(Math.PI / 2);
  const rgeo = new THREE.CylinderGeometry(0.22, 0.22, 0.26, 10); rgeo.rotateZ(Math.PI / 2);
  const spoke = new THREE.BoxGeometry(0.27, 0.36, 0.05);
  const wheels = [];
  [[0.79, 1.24], [-0.79, 1.24], [0.79, -1.36], [-0.79, -1.36]].forEach(([x, z], i) => {
    const pivot = new THREE.Group(); pivot.position.set(x, 0.33, z);
    const spin = new THREE.Group(); pivot.add(spin);
    const t = new THREE.Mesh(tg, tyre); t.castShadow = true; spin.add(t);
    spin.add(new THREE.Mesh(rgeo, rim));
    for (let k = 0; k < 3; k++) { const sp = new THREE.Mesh(spoke, rim); sp.rotation.x = k * Math.PI / 3; spin.add(sp); }
    root.add(pivot); wheels.push({ pivot, spin, front: i < 2 });
  });

  // contact shadow
  const sh = new THREE.Mesh(new THREE.PlaneGeometry(2.6, 5.2), new THREE.MeshBasicMaterial({ map: shadowTexture(), transparent: true, depthWrite: false, opacity: 0.85 }));
  sh.rotation.x = -Math.PI / 2; sh.position.y = 0.03; sh.renderOrder = 1;
  root.add(sh);

  // headlight
  const spot = new THREE.SpotLight(0xfff0dc, 0, 160, 0.48, 0.65, 1.15);
  spot.position.set(0, 0.7, 2.0);
  spot.target.position.set(0, -0.6, 22);
  body.add(spot); body.add(spot.target);
  // beam haze
  const beamMat = new THREE.ShaderMaterial({
    transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide,
    uniforms: { strength: { value: 0 } },
    vertexShader: `varying float vY; void main(){ vY = uv.y; gl_Position = projectionMatrix*modelViewMatrix*vec4(position,1.0); }`,
    fragmentShader: `uniform float strength; varying float vY; void main(){ float a = pow(vY, 2.2) * strength; gl_FragColor = vec4(vec3(1.0,0.93,0.82)*a, 1.0); }`,
  });
  const beamG = new THREE.ConeGeometry(4.2, 26, 24, 1, true); beamG.translate(0, -13, 0); beamG.rotateX(-Math.PI / 2);
  const beam = new THREE.Mesh(beamG, beamMat); beam.position.set(0, 0.68, 2.2); beam.rotation.x = 0.04; body.add(beam);
  const tailGlow = new THREE.PointLight(0xff2030, 0, 9, 2); tailGlow.position.set(0, 0.8, -2.8); body.add(tailGlow);

  return { root, body, wheels, tailMat, headMat, spot, beamMat, tailGlow, shadow: sh };
}

export const PAINTS = { pearl: 0xcdc7bb, graphite: 0x2a2c30, midnight: 0x14213d, ember: 0xb2381c };
export function buildCarMesh(gltf, paint = 'pearl') {
  const rig = buildProcCar();
  if (!gltf) return rig;
  try {
    // strip procedural body + wheels, keep lights/shadow rig
    const keep = new Set([rig.spot, rig.spot.target, rig.tailGlow]);
    rig.body.children.slice().forEach((c) => { if (!keep.has(c) && c.material !== rig.beamMat) rig.body.remove(c); });
    rig.wheels.forEach((w) => rig.root.remove(w.pivot));
    const paintMat = new THREE.MeshPhysicalMaterial({ color: PAINTS[paint] || PAINTS.pearl, roughness: 0.38, metalness: 0.15, clearcoat: 1, clearcoatRoughness: 0.13, envMapIntensity: 1.0, vertexColors: true });
    const glass = new THREE.MeshPhysicalMaterial({ color: 0x080a10, roughness: 0.16, metalness: 0.2, clearcoat: 0.6, clearcoatRoughness: 0.14, transparent: true, opacity: 0.85, envMapIntensity: 1.2 });
    const tailMat = new THREE.MeshStandardMaterial({ color: 0x300000, emissive: 0xff1e30, emissiveIntensity: 2.2, roughness: 0.3 });
    const headMat = new THREE.MeshStandardMaterial({ color: 0x222222, emissive: 0xfff2dc, emissiveIntensity: 2.2, roughness: 0.2 });
    const swap = (o) => {
      if (!o.isMesh) return;
      const n = o.material && o.material.name;
      if (n === 'Paint') o.material = paintMat;
      else if (n === 'Glass') { o.material = glass; o.renderOrder = 2; }
      else if (n === 'Tail') o.material = tailMat;
      else if (n === 'Head') o.material = headMat;
      else if (o.material) { o.material.envMapIntensity = 1.0; if (n === 'Chrome' || n === 'Rim') { o.material.envMapIntensity = 1.0; o.material.roughness = Math.max(o.material.roughness, 0.22); o.material.color.multiplyScalar(0.72); } }
      o.castShadow = n !== 'Glass'; o.receiveShadow = true;
    };
    const flip = new THREE.Group(); flip.rotation.y = Math.PI;
    const bodySrc = gltf.scene.getObjectByName('Body');
    const bodyM = bodySrc.clone(true); bodyM.position.set(0, 0, 0); bodyM.traverse(swap);
    flip.add(bodyM); rig.body.add(flip);
    const wheelSrc = gltf.scene.getObjectByName('Wheel'), calSrc = gltf.scene.getObjectByName('Caliper');
    const wheels = [];
    [[0.79, 1.24], [-0.79, 1.24], [0.80, -1.36], [-0.80, -1.36]].forEach(([x, z], i) => {
      const pivot = new THREE.Group(); pivot.position.set(x, 0.33, z);
      const spin = new THREE.Group(); pivot.add(spin);
      const w = wheelSrc.clone(true); w.position.set(0, 0, 0); w.traverse(swap);
      if (x < 0) w.scale.x = -1;
      spin.add(w);
      if (calSrc) { const c = calSrc.clone(true); c.position.set(0, 0, 0); c.traverse(swap); if (x < 0) c.scale.x = -1; pivot.add(c); }
      rig.root.add(pivot); wheels.push({ pivot, spin, front: i < 2 });
    });
    rig.wheels = wheels; rig.tailMat = tailMat; rig.headMat = headMat; rig.paintMat = paintMat;
    rig.paintHex = PAINTS[paint] || PAINTS.pearl;
    rig.setPaint = (k) => { rig.paintHex = PAINTS[k] || PAINTS.pearl; paintMat.color.setHex(rig.paintHex); };
    rig.spot.position.set(0, 0.62, 2.45);
    rig.modeled = true;
    return rig;
  } catch (e) {
    console.warn('car model failed, using procedural car', e);
    return buildProcCar();
  }
}

// ---------- light trails ----------
export class Trail {
  constructor(scene, color, n = 70) {
    this.n = n; this.pts = [];
    const g = new THREE.BufferGeometry();
    this.pos = new Float32Array(n * 2 * 3); this.col = new Float32Array(n * 2 * 3);
    g.setAttribute('position', new THREE.BufferAttribute(this.pos, 3));
    g.setAttribute('color', new THREE.BufferAttribute(this.col, 3));
    const idx = []; for (let i = 0; i < n - 1; i++) { const a = i * 2; idx.push(a, a + 1, a + 2, a + 1, a + 3, a + 2); }
    g.setIndex(idx);
    this.mesh = new THREE.Mesh(g, new THREE.MeshBasicMaterial({ vertexColors: true, transparent: true, blending: THREE.AdditiveBlending, depthWrite: false, side: THREE.DoubleSide, fog: false }));
    this.mesh.frustumCulled = false;
    this.color = new THREE.Color(color);
    scene.add(this.mesh);
    this.intensity = 0;
  }
  push(p) { this.pts.unshift(p.clone()); if (this.pts.length > this.n) this.pts.pop(); }
  clear() { this.pts.length = 0; }
  update() {
    const n = this.n, P = this.pts;
    for (let i = 0; i < n; i++) {
      const p = P[Math.min(i, P.length - 1)];
      const f = i < P.length ? Math.pow(1 - i / n, 2.4) * this.intensity * (i < 2 ? i / 2 : 1) : 0;
      const w = 0.02 + 0.015 * (1 - i / n);
      if (p) { this.pos.set([p.x, p.y + w, p.z, p.x, p.y - w, p.z], i * 6); }
      const c = this.color;
      this.col.set([c.r * f, c.g * f, c.b * f, c.r * f, c.g * f, c.b * f], i * 6);
    }
    this.mesh.geometry.attributes.position.needsUpdate = true;
    this.mesh.geometry.attributes.color.needsUpdate = true;
    this.mesh.visible = P.length > 2 && this.intensity > 0.01;
  }
}
