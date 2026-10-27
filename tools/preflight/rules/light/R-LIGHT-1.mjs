// R-LIGHT-1 · Only real objects give light.
import { readFileSync } from 'node:fs';
import { worldFiles } from '../world/_world.mjs';

// Where a model glows: the triangles of its emissive parts, in the model's
// own space (read from the .glb: positions, indices, the node's move and scale).
const glowCache = new Map();
function glowTris(path) {
  if (glowCache.has(path)) return glowCache.get(path);
  const tris = [];
  try {
    const d = readFileSync(path);
    const jlen = d.readUInt32LE(12);
    const j = JSON.parse(d.subarray(20, 20 + jlen).toString('utf8'));
    const binStart = 20 + jlen + 8;
    const view = (acc) => {
      const bv = j.bufferViews[acc.bufferView];
      return { off: binStart + (bv.byteOffset || 0) + (acc.byteOffset || 0), stride: bv.byteStride || 0 };
    };
    const nodeOf = new Map((j.nodes || []).filter((n) => 'mesh' in n).map((n) => [n.mesh, n]));
    (j.meshes || []).forEach((m, mi) => {
      const n = nodeOf.get(mi) || {}, t = n.translation || [0, 0, 0], s = n.scale || [1, 1, 1];
      for (const p of m.primitives) {
        const mat = (j.materials || [])[p.material ?? 0] || {};
        const e = mat.emissiveFactor || [0, 0, 0];
        const k = mat.extensions?.KHR_materials_emissive_strength?.emissiveStrength ?? 1;
        if (Math.max(...e) * k < 0.2) continue;
        const pa = j.accessors[p.attributes.POSITION], pv = view(pa), ps = pv.stride || 12;
        const pos = (i) => [0, 1, 2].map((c) => d.readFloatLE(pv.off + i * ps + c * 4) * s[c] + t[c]);
        let idx;
        if (p.indices !== undefined) {
          const ia = j.accessors[p.indices], iv = view(ia);
          const size = ia.componentType === 5125 ? 4 : ia.componentType === 5123 ? 2 : 1;
          const read = size === 4 ? (o) => d.readUInt32LE(o) : size === 2 ? (o) => d.readUInt16LE(o) : (o) => d.readUInt8(o);
          idx = Array.from({ length: ia.count }, (_, i) => read(iv.off + i * (iv.stride || size)));
        } else idx = Array.from({ length: pa.count }, (_, i) => i);
        for (let i = 0; i + 2 < idx.length; i += 3) tris.push([pos(idx[i]), pos(idx[i + 1]), pos(idx[i + 2])]);
      }
    });
  } catch { /* not a readable .glb: no glow */ }
  glowCache.set(path, tris);
  return tris;
}
// Distance from point p to triangle (a, b, c) (Ericson, Real-Time Collision Detection 5.1.5).
const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const lerp = (a, v, t) => [a[0] + v[0] * t, a[1] + v[1] * t, a[2] + v[2] * t];
function triGap(p, [a, b, c]) {
  const ab = sub(b, a), ac = sub(c, a), ap = sub(p, a);
  const d1 = dot(ab, ap), d2 = dot(ac, ap);
  let q;
  if (d1 <= 0 && d2 <= 0) q = a;
  else {
    const bp = sub(p, b), d3 = dot(ab, bp), d4 = dot(ac, bp);
    const cp = sub(p, c), d5 = dot(ab, cp), d6 = dot(ac, cp);
    const vc = d1 * d4 - d3 * d2, vb = d5 * d2 - d1 * d6, va = d3 * d6 - d5 * d4;
    if (d3 >= 0 && d4 <= d3) q = b;
    else if (d6 >= 0 && d5 <= d6) q = c;
    else if (vc <= 0 && d1 >= 0 && d3 <= 0) q = lerp(a, ab, d1 / (d1 - d3));
    else if (vb <= 0 && d2 >= 0 && d6 <= 0) q = lerp(a, ac, d2 / (d2 - d6));
    else if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) q = lerp(b, sub(c, b), (d4 - d3) / ((d4 - d3) + (d5 - d6)));
    else { const den = 1 / (va + vb + vc); q = [0, 1, 2].map((i) => a[i] + ab[i] * vb * den + ac[i] * vc * den); }
  }
  return Math.hypot(...sub(p, q));
}
// Turn (degrees about up) applied to a local point, as the engine does.
const turn = (v, deg) => { const a = (deg || 0) * Math.PI / 180, c = Math.cos(a), s = Math.sin(a); return [v[0] * c + v[2] * s, v[1], -v[0] * s + v[2] * c]; };
const add = (a, b) => a.map((v, i) => v + b[i]);
const PLACES = ['bar'];

export default {
  id: 'R-LIGHT-1',
  area: 'light',
  rule: 'Every light sits on something that visibly glows (a bulb, a sign, a screen, a flame): only real objects give light.',
  why: 'The user, request 2026-10-27 #3: lights where nothing glows lit the room from nowhere and drew square patches.',
  when: ['assets/world/**', 'assets/models/**'],
  check(ctx) {
    const w = worldFiles(ctx);
    const spots = [];
    const kMargin = 0.6;  // metres a light may sit from the glowing part (inside a shade, in front of a sign)
    // The bar for now; home.json and showroom.json still have 9 lights from
    // before (parked: make them real, then add them here).
    for (const place of w.places.filter((p) => PLACES.some((n) => p.endsWith(`/${n}.json`)))) {
      const things = ctx.json(place).things || [];
      const glows = [];
      for (const t of things) {
        const k = w.resolve(t.kind) || {};
        const file = t.parts?.model?.file || k.parts?.model?.file;
        if (!file || !t.at) continue;
        const tris = glowTris(`assets/models/${file}`);
        if (tris.length) glows.push({ id: t.id, at: t.at, turn: t.turn || 0, tris });
      }
      for (const t of things) {
        const k = w.resolve(t.kind) || {};
        const l = t.parts?.light || k.parts?.light;
        if (!l || !l.color || !t.at) continue;
        const at = add(t.at, turn(l.center || [0, 0, 0], t.turn));
        let best = null;
        for (const g of glows) {
          const local = turn(at.map((v, i) => v - g.at[i]), -g.turn);  // the light in the model's own space
          for (const tri of g.tris) {
            const gap = triGap(local, tri);
            if (!best || gap < best.gap) best = { gap, id: g.id };
          }
        }
        if (!best || best.gap > kMargin)
          spots.push({ where: `${place} → ${t.id}`, fix: `nothing glows within ${kMargin} m (nearest: ${best ? `${best.id}, ${best.gap.toFixed(1)} m` : 'none'}): put it on its lamp, sign or screen, or remove it` });
      }
    }
    return spots;
  },
};
