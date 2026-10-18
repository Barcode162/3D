// Checks cloud/proto.js against the golden vectors in proto/vectors.json
// (the C++ codec runs the same vectors in tools/native/proto_test.cpp).
// Usage: node tools/proto_test.mjs   (exit 1 on any failure)
import { readFileSync } from 'node:fs';
import * as P from '../cloud/proto.js';

const vectors = JSON.parse(readFileSync(new URL('../proto/vectors.json', import.meta.url), 'utf8'));
let failures = 0;
function check(ok, what) {
  if (!ok) { failures++; console.log('FAIL ' + what); }
}

const TAU = Math.PI * 2;
const near = (a, b, tol) => Math.abs(a - b) <= tol;
function angleNear(a, b, tol) {
  let d = (a - b) % TAU;
  if (d > Math.PI) d -= TAU;
  if (d < -Math.PI) d += TAU;
  return Math.abs(d) <= tol;
}
const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));
const sat = (v, scale) => clamp(v, -32768 / scale, 32767 / scale);

function avatarNear(got, want) {
  return near(got.x, sat(want.x, 100), 0.0051) && near(got.y, sat(want.y, 100), 0.0051) && near(got.z, sat(want.z, 100), 0.0051) &&
    angleNear(got.yaw, want.yaw, 1e-4) && near(got.pitch, clamp(want.pitch, -Math.PI / 2, Math.PI / 2), 6e-5) &&
    ['anim', 'animTick', 'held', 'look', 'flags'].every((k) => got[k] === want[k]);
}
function propNear(got, want) {
  const len = Math.hypot(...want.quat);
  const dot = got.quat.reduce((s, c, i) => s + c * want.quat[i] / len, 0);
  return got.id === want.id && got.pos.every((p, i) => near(p, sat(want.pos[i], 1000), 0.00051)) &&
    Math.abs(dot) > 0.99999 && got.vel.every((v, i) => near(v, sat(want.vel[i], 100), 0.0051));
}

const encoders = { state: P.encodeState, prop: P.encodeProp, tick: P.encodeTick };
for (const v of vectors.binary) {
  const bytes = encoders[v.kind](v.fields);
  check(bytes && P.bytesToHex(bytes) === v.hex, `${v.name}: encode gave ${bytes && P.bytesToHex(bytes)}`);
  const d = P.decodeBinary(P.hexToBytes(v.hex));
  check(d.ok && d.type === v.kind, `${v.name}: decode (${d.why})`);
  if (!d.ok) continue;
  const m = d.msg, f = v.fields;
  if (v.kind === 'state') check(m.seq === f.seq && avatarNear(m.avatar, f.avatar) && m.act === f.act, `${v.name}: decoded fields`);
  if (v.kind === 'prop') check(m.props.length === f.props.length && m.props.every((p, i) => propNear(p, f.props[i])), `${v.name}: decoded fields`);
  if (v.kind === 'tick') {
    check(m.tick === f.tick && m.serverMs === f.serverMs, `${v.name}: tick/serverMs`);
    check(m.players.length === f.players.length && m.players.every((p, i) => p.slot === f.players[i].slot &&
      avatarNear(p.avatar, f.players[i].avatar) && p.act === f.players[i].act), `${v.name}: players`);
    check(m.props.length === f.props.length && m.props.every((p, i) => propNear(p, f.props[i])), `${v.name}: props`);
  }
}

function deepEqual(a, b) {
  if (Array.isArray(a)) return Array.isArray(b) && a.length === b.length && a.every((x, i) => deepEqual(x, b[i]));
  if (a && typeof a === 'object') {
    if (!b || typeof b !== 'object' || Array.isArray(b)) return false;
    const ka = Object.keys(a), kb = Object.keys(b);
    return ka.length === kb.length && ka.every((k) => deepEqual(a[k], b[k]));
  }
  return a === b;
}

for (const v of vectors.text) {
  const d = P.decodeText(v.text, v.dir);
  check(d.ok && d.letter === v.letter && deepEqual(d.body, v.body), `${v.name}: decode (${d.why})`);
  const t = P.encodeText(v.letter, v.body, v.dir);
  check(t !== null && deepEqual(JSON.parse(t.slice(1)), v.body) && t[0] === v.letter, `${v.name}: encode`);
}

for (const v of vectors.bad) {
  let r;
  try {
    r = v.kind === 'binary' ? P.decodeBinary(P.hexToBytes(v.hex) || new Uint8Array(0)) : P.decodeText(v.text, v.dir);
  } catch (e) {
    r = { ok: true, why: 'threw ' + e.message };
  }
  check(!r.ok, `${v.name}: should be rejected`);
}

// Encoders refuse what they can't send.
check(P.encodeState({ seq: 1, avatar: { x: NaN, y: 0, z: 0, yaw: 0, pitch: 0, anim: 0, animTick: 0, held: 0, look: 0, flags: 0 }, act: '' }) === null, 'encode NaN position');
check(P.encodeProp({ props: [{ id: 1, pos: [0, 0, 0], quat: [0, 0, 0, 0], vel: [0, 0, 0] }] }) === null, 'encode zero quaternion');
check(P.encodeText('P', { c: Infinity }, 'toServer') === null, 'encode infinite ping');

// Random garbage never throws.
let seed = 12345;
const rnd = () => (seed = (seed * 1103515245 + 12345) >>> 0) / 4294967296;
for (let i = 0; i < 20000; i++) {
  const b = new Uint8Array(Math.floor(rnd() * 80));
  for (let j = 0; j < b.length; j++) b[j] = Math.floor(rnd() * 256);
  if (b.length && rnd() < 0.5) b[0] = [1, 2, 0x81][Math.floor(rnd() * 3)];
  try { P.decodeBinary(b); P.decodeText(String.fromCharCode(...b), 'toServer'); } catch (e) { check(false, 'garbage threw: ' + e.message); break; }
}

const total = vectors.binary.length + vectors.text.length + vectors.bad.length;
if (failures) { console.log(`PROTO TEST FAILED (${failures})`); process.exit(1); }
console.log(`PROTO TEST PASSED (${total} vectors + 20000 random frames)`);
