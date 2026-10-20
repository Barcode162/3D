// Procedural road: an endless, clothoid-like ribbon with gentle elevation and banking.
// Coordinate system: world X/Z plane, Y up. Road progresses roughly toward +Z.
// Heading h: forward = (sin h, cos h). Left normal = (cos h, -sin h). Lateral offset d > 0 is left.

export const DS = 2;            // metres between centerline samples
export const LANE = 2.5;        // lane centre offset (right lane is d = -LANE)
export const ROAD_HALF = 5.0;   // asphalt half width
export const SHOULDER = 6.6;    // outer edge of paved shoulder
export const BIOME_LEN = 2800;  // metres per biome
export const BIOME_BLEND = 600; // metres of blending into the next biome

export function mulberry32(a) {
  return function () {
    a |= 0; a = (a + 0x6d2b79f5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
export const lerp = (a, b, t) => a + (b - a) * t;
export const smoothstep = (a, b, x) => { const t = clamp((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); };

// ---- noise ----
function hash2(ix, iz) {
  let h = (ix * 374761393 + iz * 668265263) | 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
}
export function noise2(x, z) {
  const ix = Math.floor(x), iz = Math.floor(z);
  const fx = x - ix, fz = z - iz;
  const ux = fx * fx * (3 - 2 * fx), uz = fz * fz * (3 - 2 * fz);
  const a = hash2(ix, iz), b = hash2(ix + 1, iz), c = hash2(ix, iz + 1), d = hash2(ix + 1, iz + 1);
  return lerp(lerp(a, b, ux), lerp(c, d, ux), uz) * 2 - 1;
}
export function fbm2(x, z) {
  return noise2(x, z) * 0.6 + noise2(x * 2.03 + 17.1, z * 2.03 - 9.4) * 0.28 + noise2(x * 4.1 - 3.3, z * 4.1 + 5.7) * 0.12;
}
function noise1(x, seed) { return noise2(x, seed * 13.37); }

// ---- biomes ----
// Each biome is a time of day + landscape. Values blend smoothly as you drive.
export const BIOMES = [
  {
    name: 'Mojave', sub: 'Golden hour',
    skyTop: 0x2c2560, skyMid: 0x9a5f86, skyHorizon: 0xffb27a, fog: 0xd98f74, fogNear: 70, fogFar: 760,
    disc: 0xffd2a0, discDir: [-0.35, 0.09, 0.93], discSize: 1.0, stars: 0.0,
    sun: 0xffc48f, sunI: 2.6, hemiSky: 0xffc9a8, hemiGround: 0x6a3a3a, hemiI: 0.9,
    ground: 0xc07448, ground2: 0x8c4936, dirt: 0x9e5c3e, mount: 0x7a4a62, mount2: 0xb26f72,
    headI: 0.0, exposure: 1.0, bloom: 0.45, hill: 34,
    props: { joshua: 0.34, rock: 0.4, shrub: 0.9 }, density: 70, grass: 120, grassTint: 0xfff0d0,
    particle: { color: 0xffd6a8, size: 0.16, fall: -0.05, drift: 0.6, opacity: 0.5 },
  },
  {
    name: 'Sierra', sub: 'Blue hour',
    skyTop: 0x0d1430, skyMid: 0x3d4a7c, skyHorizon: 0xd99aa0, fog: 0x56607e, fogNear: 50, fogFar: 600,
    disc: 0xffa98a, discDir: [0.5, -0.02, 0.86], discSize: 1.3, stars: 0.35,
    sun: 0xa8b6e6, sunI: 1.25, hemiSky: 0x8a9ccc, hemiGround: 0x2a3440, hemiI: 1.0,
    ground: 0x34503e, ground2: 0x24382e, dirt: 0x4a4a40, mount: 0x26304e, mount2: 0x46507a,
    headI: 0.55, exposure: 1.05, bloom: 0.6, hill: 42,
    props: { pine: 1.0, rock: 0.25 }, density: 170, grass: 170, grassTint: 0x9fc890,
    particle: { color: 0xfff0a0, size: 0.13, fall: 0.0, drift: 0.9, opacity: 0.85, glow: true },
  },
  {
    name: 'Glacier Pass', sub: 'Moonlit night',
    skyTop: 0x03050e, skyMid: 0x0d1630, skyHorizon: 0x2a3b62, fog: 0x1b2742, fogNear: 40, fogFar: 520,
    disc: 0xdde8ff, discDir: [-0.55, 0.32, 0.77], discSize: 0.55, stars: 1.0,
    sun: 0x9fb8ff, sunI: 0.8, hemiSky: 0x5a6e9e, hemiGround: 0x18203a, hemiI: 0.7,
    ground: 0xd6e2f2, ground2: 0xa9bbd6, dirt: 0x6a7488, mount: 0x182440, mount2: 0x2c3c66,
    headI: 1.0, exposure: 1.12, bloom: 0.75, hill: 48,
    props: { snowpine: 0.9, rock: 0.4 }, density: 120, grass: 40, grassTint: 0xc8d0e0,
    particle: { color: 0xffffff, size: 0.11, fall: -2.2, drift: 0.7, opacity: 0.9 },
  },
  {
    name: 'Coast Road', sub: 'First light',
    skyTop: 0x2d4278, skyMid: 0x8f86b4, skyHorizon: 0xffc6a8, fog: 0xc9a2ac, fogNear: 60, fogFar: 700,
    disc: 0xffe3c0, discDir: [0.62, 0.05, 0.78], discSize: 1.1, stars: 0.08,
    sun: 0xffd2b4, sunI: 1.9, hemiSky: 0xc8c4e8, hemiGround: 0x3c4a3a, hemiI: 0.85,
    ground: 0x7d9460, ground2: 0x5b7048, dirt: 0x8a7a5a, mount: 0x5a5f86, mount2: 0x9a8fb0,
    headI: 0.2, exposure: 1.0, bloom: 0.5, hill: 26,
    props: { cypress: 0.55, shrub: 0.8, rock: 0.3 }, density: 90, grass: 220, grassTint: 0xa8d890,
    particle: { color: 0xfff4e8, size: 0.12, fall: 0.05, drift: 0.4, opacity: 0.45 },
  },
];

export function biomeAt(s) {
  const n = BIOMES.length;
  const k = Math.max(0, s) / BIOME_LEN;
  const i = Math.floor(k);
  const local = (k - i) * BIOME_LEN;
  const t = smoothstep(BIOME_LEN - BIOME_BLEND, BIOME_LEN, local);
  return { a: i % n, b: (i + 1) % n, t, index: i };
}

// ---- road ----
export class Road {
  constructor(seed = 7) {
    this.rng = mulberry32(seed);
    this.seed = seed;
    this.samples = [];
    this.heading = 0; this.k = 0; this.kTarget = 0; this.segLeft = 300; // gentle warm-up straight
    this.x = 0; this.z = 0; this.s = 0;
    this.y0 = this._elev(0);
    this.ensure(2000);
  }
  _elev(s) {
    const base = noise1(s / 700, this.seed + 1) * 26 + noise1(s / 210, this.seed + 2) * 7 + noise1(s / 90, this.seed + 3) * 1.2;
    return base * smoothstep(0, 400, s);
  }
  _pickSegment() {
    const r = this.rng();
    const h = this.heading;
    let len, kt;
    if (r < 0.28) {
      kt = (this.rng() - 0.5) * 0.0016;
      len = 60 + this.rng() * 200;
    } else {
      const R = 65 + Math.pow(this.rng(), 1.6) * 420;
      let pNeg = clamp(0.5 + h * 0.55, 0.08, 0.92);
      let sign = this.rng() < pNeg ? -1 : 1;
      if (h > 1.0) sign = -1; if (h < -1.0) sign = 1;
      kt = sign / R;
      const maxTurn = 0.6 + this.rng() * 0.9; // radians of turn per segment
      len = Math.min(40 + this.rng() * 240, maxTurn * R);
      // keep heading bounded so the road never loops back on itself
      const projected = h + kt * len;
      if (Math.abs(projected) > 1.25) len = Math.max(20, (Math.sign(kt) * 1.25 - h) / kt);
    }
    this.kTarget = kt; this.segLeft = len;
  }
  _step() {
    if (this.segLeft <= 0) this._pickSegment();
    const KR = 0.00016; // curvature change rate per metre (clothoid transitions)
    const dk = clamp(this.kTarget - this.k, -KR * DS, KR * DS);
    this.k += dk;
    // hard safety on heading
    if (this.heading > 1.3 && this.k > 0) this.k -= KR * DS * 2;
    if (this.heading < -1.3 && this.k < 0) this.k += KR * DS * 2;
    this.heading += this.k * DS;
    this.x += Math.sin(this.heading) * DS;
    this.z += Math.cos(this.heading) * DS;
    this.s += DS;
    this.segLeft -= DS;
    const y = this._elev(this.s) - this.y0;
    const bank = clamp(this.k * 6.5, -0.075, 0.075);
    const h = this.heading;
    this.samples.push({ x: this.x, y, z: this.z, h, k: this.k, bank, s: this.s, sx: Math.sin(h), cz: Math.cos(h) });
  }
  ensure(n) { while (this.samples.length < n) this._step(); }
  get(i) { this.ensure(i + 2); return this.samples[Math.max(0, i)]; }

  // Find nearest sample index to (x,z) starting from a hint, returns {i, d, t}
  locate(x, z, hint = 0) {
    let i = Math.max(0, hint | 0);
    for (let iter = 0; iter < 400; iter++) {
      const p = this.get(i);
      const t = (x - p.x) * p.sx + (z - p.z) * p.cz;
      if (t > DS * 0.5) i++;
      else if (t < -DS * 0.5 && i > 0) i--;
      else break;
    }
    const p = this.get(i);
    const t = (x - p.x) * p.sx + (z - p.z) * p.cz;
    const d = (x - p.x) * p.cz - (z - p.z) * p.sx;
    return { i, d, t, p };
  }

  // Continuous ground height: blends the two samples bracketing (x,z) so the car glides instead of stepping every DS metres.
  groundY(x, z, hint = 0) {
    const l = this.locate(x, z, hint);
    const y0 = this.heightAt(l.p, l.d, x, z);
    const j = l.t >= 0 ? l.i + 1 : Math.max(0, l.i - 1);
    if (j === l.i) return y0;
    const q = this.get(j);
    const dq = (x - q.x) * q.cz - (z - q.z) * q.sx;
    const y1 = this.heightAt(q, dq, x, z);
    const a = Math.min(1, Math.abs(l.t) / DS);
    return y0 + (y1 - y0) * a;
  }
  // Smoothly varying slope (dy/ds) and bank at a located point
  slopeBank(l) {
    const i = l.i, p = l.p, pP = this.get(Math.max(0, i - 1)), pN = this.get(i + 1);
    const sA = (p.y - pP.y) / DS, sB = (pN.y - p.y) / DS;
    const f = Math.min(1, Math.max(0, 0.5 + l.t / DS));
    const q = l.t >= 0 ? pN : pP, a = Math.min(1, Math.abs(l.t) / DS);
    return { slope: sA + (sB - sA) * f, bank: p.bank + (q.bank - p.bank) * a };
  }

  // Ground height given a sample, a lateral offset, and world xz (for noise).
  heightAt(p, d, x, z) {
    const ad = Math.abs(d);
    const tb = Math.tan(p.bank);
    if (ad <= SHOULDER) {
      let y = p.y - d * tb;
      if (ad > ROAD_HALF + 0.2) y -= (ad - ROAD_HALF - 0.2) * 0.05;
      return y;
    }
    const sgn = Math.sign(d);
    const edgeY = p.y - sgn * SHOULDER * tb - 0.07;
    const bm = biomeAt(p.s);
    const amp = lerp(BIOMES[bm.a].hill, BIOMES[bm.b].hill, bm.t);
    const ditch = -0.55 * smoothstep(SHOULDER, SHOULDER + 2.2, ad) * (1 - smoothstep(9.5, 15, ad));
    const hills = fbm2(x * 0.0085, z * 0.0085) * amp * smoothstep(10, 85, ad) + Math.max(0, ad - 200) * -0.04;
    const detail = noise2(x * 0.09, z * 0.09) * 0.35 * smoothstep(7, 11, ad);
    return edgeY + ditch + hills + detail;
  }

  // Map a nominal lateral offset to a non-overlapping one on the inside of curves
  compressD(p, d) {
    const ad = Math.abs(d);
    const D0 = 20;
    if (ad <= D0 || Math.sign(d) !== Math.sign(p.k) || Math.abs(p.k) < 1e-5) return d;
    const R = 1 / Math.abs(p.k);
    const maxD = 420;
    const room = Math.max(4, 0.82 * R - D0);
    const f = Math.min(1, room / (maxD - D0));
    return Math.sign(d) * (D0 + (ad - D0) * f);
  }

  // Max |curvature| over a distance window ahead (for autopilot & advisory)
  maxCurvatureAhead(i, metres) {
    let m = 0;
    const n = Math.ceil(metres / DS);
    for (let j = 0; j < n; j += 2) m = Math.max(m, Math.abs(this.get(i + j).k));
    return m;
  }
}
