// Bar3D room protocol v1 codec (server side; the Node test tools use it as a
// client too). The spec is docs/PROTOCOL.md; the C++ twin is
// engine/netproto.*. Both must pass proto/vectors.json (tools/proto_test.mjs).
//
// Binary (high rate): STATE and PROP from clients, TICK from the server.
// Text (low rate): one type letter + a JSON object, e.g. P{"c":12.5}.
// Decoders never throw: they return {ok:false, why} on anything malformed.

export const PROTO = 1;
export const MAX_SLOTS = 16;
export const BIN = { STATE: 0x01, PROP: 0x02, TICK: 0x81 };
export const MAX_TEXT_TO_SERVER = 4096;
export const MAX_SIGNAL_TO_SERVER = 16384;
export const MAX_TO_CLIENT = 65536;

const TAU = Math.PI * 2;
const HALF_PI = Math.PI / 2;
const SQRT1_2 = 0.70710678;
const AVATAR_BYTES = 16;
const PROP_BYTES = 18;
const MAX_ACTIVITY = 48;
const MAX_PROPS_IN = 8;
const MAX_PROPS_TICK = 32;

const q = (v, scale) => Math.floor(v * scale + 0.5);
const clamp = (v, lo, hi) => (v < lo ? lo : v > hi ? hi : v);
const fin = (v) => typeof v === 'number' && Number.isFinite(v);
const isInt = (v, lo, hi) => Number.isInteger(v) && v >= lo && v <= hi;

function fail(why) { return { ok: false, why }; }

// ---- Writer / reader over a growable byte array ----
class Out {
  constructor() { this.b = []; }
  u8(v) { this.b.push(v & 0xff); }
  u16(v) { this.b.push(v & 0xff, (v >>> 8) & 0xff); }
  i16(v) { this.u16(v & 0xffff); }
  u32(v) { this.u16(v & 0xffff); this.u16((v >>> 16) & 0xffff); }
  f64(v) { const d = new DataView(new ArrayBuffer(8)); d.setFloat64(0, v, true); for (let i = 0; i < 8; i++) this.u8(d.getUint8(i)); }
  bytes(a) { for (const x of a) this.u8(x); }
  done() { return new Uint8Array(this.b); }
}

class In {
  constructor(bytes) { this.d = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength); this.p = 0; this.n = bytes.byteLength; this.bad = false; }
  need(k) { if (this.p + k > this.n) { this.bad = true; return false; } return true; }
  u8() { if (!this.need(1)) return 0; return this.d.getUint8(this.p++); }
  u16() { if (!this.need(2)) return 0; const v = this.d.getUint16(this.p, true); this.p += 2; return v; }
  i16() { if (!this.need(2)) return 0; const v = this.d.getInt16(this.p, true); this.p += 2; return v; }
  u32() { if (!this.need(4)) return 0; const v = this.d.getUint32(this.p, true); this.p += 4; return v; }
  f64() { if (!this.need(8)) return 0; const v = this.d.getFloat64(this.p, true); this.p += 8; return v; }
  bytes(k) { if (!this.need(k)) return new Uint8Array(0); const v = new Uint8Array(this.d.buffer, this.d.byteOffset + this.p, k).slice(); this.p += k; return v; }
}

function toBytes(data) {
  if (data instanceof Uint8Array) return data;
  if (data instanceof ArrayBuffer) return new Uint8Array(data);
  if (ArrayBuffer.isView(data)) return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
  return null;
}

// ---- Avatar block, activity TLV, prop sample ----
function writeAvatar(o, a) {
  for (const k of ['x', 'y', 'z', 'yaw', 'pitch']) if (!fin(a[k])) return false;
  for (const k of ['anim', 'animTick', 'held', 'look', 'flags']) if (!Number.isInteger(a[k])) return false;
  if (a.flags < 0 || a.flags > 15) return false;
  o.i16(clamp(q(a.x, 100), -32768, 32767));
  o.i16(clamp(q(a.y, 100), -32768, 32767));
  o.i16(clamp(q(a.z, 100), -32768, 32767));
  let yaw = a.yaw % TAU;
  if (yaw < 0) yaw += TAU;
  o.u16(q(yaw / TAU, 65536) & 0xffff);
  o.i16(q(clamp(a.pitch, -HALF_PI, HALF_PI), 10000));
  o.u8(clamp(a.anim, 0, 255));
  o.u16(a.animTick & 0xffff);
  o.u8(clamp(a.held, 0, 255));
  o.u8(clamp(a.look, 0, 255));
  o.u8(a.flags);
  return true;
}

function readAvatar(r) {
  const a = {};
  a.x = r.i16() / 100; a.y = r.i16() / 100; a.z = r.i16() / 100;
  a.yaw = r.u16() * TAU / 65536;
  a.pitch = r.i16() / 10000;
  a.anim = r.u8(); a.animTick = r.u16(); a.held = r.u8(); a.look = r.u8(); a.flags = r.u8();
  if (a.flags > 15) r.bad = true;
  if (Math.abs(a.pitch) > HALF_PI + 1e-4) r.bad = true;
  return a;
}

export function hexToBytes(hex) {
  if (typeof hex !== 'string' || hex.length % 2 || /[^0-9a-f]/i.test(hex)) return null;
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.substr(i * 2, 2), 16);
  return out;
}
export function bytesToHex(b) { return Array.from(b, (x) => x.toString(16).padStart(2, '0')).join(''); }

// TLV entries must exactly fill the bytes.
function validTlv(b) {
  let p = 0;
  while (p < b.length) {
    if (p + 2 > b.length) return false;
    p += 2 + b[p + 1];
  }
  return p === b.length;
}

function writeActivity(o, hex) {
  const b = hexToBytes(hex || '');
  if (!b || b.length > MAX_ACTIVITY || !validTlv(b)) return false;
  o.u8(b.length);
  o.bytes(b);
  return true;
}

function readActivity(r) {
  const n = r.u8();
  if (n > MAX_ACTIVITY) { r.bad = true; return ''; }
  const b = r.bytes(n);
  if (!validTlv(b)) r.bad = true;
  return bytesToHex(b);
}

function writeProp(o, s) {
  if (!Number.isInteger(s.id) || s.id < 0 || s.id > 65535) return false;
  const v = [...s.pos, ...s.quat, ...s.vel];
  if (s.pos.length !== 3 || s.quat.length !== 4 || s.vel.length !== 3 || !v.every(fin)) return false;
  o.u16(s.id);
  for (const p of s.pos) o.i16(clamp(q(p, 1000), -32768, 32767));
  const len = Math.hypot(...s.quat);
  if (!(len > 1e-6)) return false;
  const n = s.quat.map((c) => c / len);
  let big = 0;
  for (let i = 1; i < 4; i++) if (Math.abs(n[i]) > Math.abs(n[big])) big = i;
  const sign = n[big] < 0 ? -1 : 1;
  let packed = big;
  for (let i = 0; i < 4; i++) {
    if (i === big) continue;
    const c = clamp(n[i] * sign, -SQRT1_2, SQRT1_2);
    packed = packed * 1024 + clamp(q(c / SQRT1_2 * 0.5 + 0.5, 1023), 0, 1023);
  }
  o.u32(packed >>> 0);
  for (const p of s.vel) o.i16(clamp(q(p, 100), -32768, 32767));
  return true;
}

function readProp(r) {
  const s = { id: r.u16(), pos: [r.i16() / 1000, r.i16() / 1000, r.i16() / 1000] };
  const packed = r.u32();
  const big = packed >>> 30;
  const small = [(packed >>> 20) & 1023, (packed >>> 10) & 1023, packed & 1023].map((u) => (u / 1023 - 0.5) * 2 * SQRT1_2);
  const sum = small[0] * small[0] + small[1] * small[1] + small[2] * small[2];
  if (sum > 1) r.bad = true;
  const quat = [];
  let k = 0;
  for (let i = 0; i < 4; i++) quat.push(i === big ? Math.sqrt(Math.max(0, 1 - sum)) : small[k++]);
  s.quat = quat;
  s.vel = [r.i16() / 100, r.i16() / 100, r.i16() / 100];
  return s;
}

// ---- Binary messages ----
// encode*(fields) → Uint8Array, or null if a field is missing or not finite.
export function encodeState(m) {
  const o = new Out();
  o.u8(BIN.STATE);
  if (!isInt(m.seq, 0, 65535)) return null;
  o.u16(m.seq);
  if (!writeAvatar(o, m.avatar) || !writeActivity(o, m.act)) return null;
  return o.done();
}

export function encodeProp(m) {
  if (!Array.isArray(m.props) || m.props.length < 1 || m.props.length > MAX_PROPS_IN) return null;
  const o = new Out();
  o.u8(BIN.PROP);
  o.u8(m.props.length);
  for (const s of m.props) if (!writeProp(o, s)) return null;
  return o.done();
}

export function encodeTick(m) {
  if (!isInt(m.tick, 0, 0xffffffff) || !fin(m.serverMs)) return null;
  if (!Array.isArray(m.players) || m.players.length > MAX_SLOTS) return null;
  if (!Array.isArray(m.props) || m.props.length > MAX_PROPS_TICK) return null;
  const o = new Out();
  o.u8(BIN.TICK);
  o.u32(m.tick);
  o.f64(m.serverMs);
  o.u8(m.players.length);
  const seen = new Set();
  for (const p of m.players) {
    if (!isInt(p.slot, 0, MAX_SLOTS - 1) || seen.has(p.slot)) return null;
    seen.add(p.slot);
    o.u8(p.slot);
    if (!writeAvatar(o, p.avatar) || !writeActivity(o, p.act)) return null;
  }
  o.u8(m.props.length);
  for (const s of m.props) if (!writeProp(o, s)) return null;
  return o.done();
}

// decodeBinary(bytes) → {ok:true, type:'state'|'prop'|'tick', msg} or {ok:false, why}.
export function decodeBinary(data) {
  const b = toBytes(data);
  if (!b) return fail('not bytes');
  if (b.length < 1) return fail('empty');
  if (b.length > MAX_TO_CLIENT) return fail('too big');
  const r = new In(b);
  const type = r.u8();
  let out;
  if (type === BIN.STATE) {
    const seq = r.u16();
    const avatar = readAvatar(r);
    out = { type: 'state', msg: { seq, avatar, act: readActivity(r) } };
  } else if (type === BIN.PROP) {
    const n = r.u8();
    if (n < 1 || n > MAX_PROPS_IN) return fail('bad prop count');
    const props = [];
    for (let i = 0; i < n && !r.bad; i++) props.push(readProp(r));
    out = { type: 'prop', msg: { props } };
  } else if (type === BIN.TICK) {
    const tick = r.u32();
    const serverMs = r.f64();
    if (!r.bad && !Number.isFinite(serverMs)) return fail('serverMs not finite');
    const n = r.u8();
    if (n > MAX_SLOTS) return fail('bad player count');
    const players = [];
    const seen = new Set();
    for (let i = 0; i < n && !r.bad; i++) {
      const slot = r.u8();
      if (slot >= MAX_SLOTS || seen.has(slot)) return fail('bad slot');
      seen.add(slot);
      const avatar = readAvatar(r);
      players.push({ slot, avatar, act: readActivity(r) });
    }
    const m = r.u8();
    if (m > MAX_PROPS_TICK) return fail('bad prop count');
    const props = [];
    for (let i = 0; i < m && !r.bad; i++) props.push(readProp(r));
    out = { type: 'tick', msg: { tick, serverMs, players, props } };
  } else {
    return fail('unknown type');
  }
  if (r.bad) return fail('malformed');
  if (r.p !== b.length) return fail('trailing bytes');
  return { ok: true, ...out };
}

// ---- Text messages ----
function utf8Len(s) {
  let n = 0;
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    if (c < 0x80) n += 1;
    else if (c < 0x800) n += 2;
    else if (c >= 0xd800 && c <= 0xdbff) { n += 4; i++; }
    else n += 3;
  }
  return n;
}

// Field checks. Each returns true if the value is valid.
const V = {
  int: (lo, hi) => (v) => isInt(v, lo, hi),
  num: () => (v) => fin(v),
  str: (lo, hi, re) => (v) => typeof v === 'string' && utf8Len(v) >= lo && utf8Len(v) <= hi && (!re || re.test(v)),
  bit: () => (v) => v === 0 || v === 1,
  slot: () => (v) => isInt(v, 0, MAX_SLOTS - 1),
  slotOrServer: () => (v) => isInt(v, -1, MAX_SLOTS - 1),
  any: () => (v) => v !== undefined,
  nums: (max) => (v) => Array.isArray(v) && v.length <= max && v.every(fin),
  list: (max, item) => (v) => Array.isArray(v) && v.length <= max && v.every(item),
  tuple: (...items) => (v) => Array.isArray(v) && v.length === items.length && items.every((f, i) => f(v[i])),
  object: (spec) => (v) => checkObject(v, spec),
  map: (max, keyRe, item) => (v) => isPlain(v) && Object.keys(v).length <= max && Object.entries(v).every(([k, x]) => keyRe.test(k) && item(x)),
};

const HEX32 = /^[0-9a-f]{32}$/;
const KV_KEY = /^[A-Za-z0-9_]{1,16}$/;
const CLAIM_KEY = /^[A-Za-z0-9_.-]{1,48}$/;
const MAX_INT = 2147483647;

function isPlain(v) { return v !== null && typeof v === 'object' && !Array.isArray(v); }

// spec: {key: [check, required]}
function checkObject(v, spec) {
  if (!isPlain(v)) return false;
  for (const [k, [check, required]] of Object.entries(spec)) {
    if (!(k in v)) { if (required) return false; continue; }
    if (!check(v[k])) return false;
  }
  return true;
}

// Every number anywhere in a body must be finite (JSON.parse turns 1e999 into Infinity).
function allFinite(v, depth = 0) {
  if (depth > 32) return false;
  if (typeof v === 'number') return Number.isFinite(v);
  if (Array.isArray(v)) return v.every((x) => allFinite(x, depth + 1));
  if (isPlain(v)) return Object.values(v).every((x) => allFinite(x, depth + 1));
  return true;
}

const R = true, O = false;
const PLAYER = {
  slot: [V.slot(), R], name: [V.str(1, 24), R], look: [V.int(0, 255), R],
  acct: [V.str(0, 32), O], av: [V.int(0, MAX_INT), O], away: [V.bit(), O],
};
const KV_ENTRY = V.tuple(V.int(0, Number.MAX_SAFE_INTEGER), V.str(1, 16, KV_KEY), V.slotOrServer(), V.nums(20));

export const TO_SERVER = {
  H: { name: 'hello', spec: { proto: [V.int(1, 255), R], build: [V.str(0, 16), R], name: [V.str(1, 24), R], look: [V.int(0, 255), R], resume: [V.str(32, 32, HEX32), O], ticket: [V.str(0, 2048), O] } },
  I: { name: 'intent', spec: { i: [V.int(0, MAX_INT), R], k: [V.str(1, 32), R] } },
  E: { name: 'events', spec: { e: [V.list(32, V.tuple(V.str(1, 16), V.nums(16))), R] } },
  K: { name: 'kv', spec: { w: [V.map(32, KV_KEY, V.nums(20)), R] } },
  G: { name: 'signal', spec: { to: [V.slot(), R], data: [V.any(), R] } },
  P: { name: 'ping', spec: { c: [V.num(), R] } },
  R: { name: 'reauth', spec: { ticket: [V.str(1, 2048), R] } },
  A: { name: 'away', spec: { on: [V.bit(), R] } },
};

export const TO_CLIENT = {
  W: { name: 'welcome', spec: {
    proto: [V.int(1, 255), R], slot: [V.slot(), R], resume: [V.str(32, 32, HEX32), R], serverMs: [V.num(), R],
    tick: [V.int(0, 0xffffffff), R], tickHz: [V.int(1, 60), R], room: [V.str(1, 32), R],
    roster: [V.list(MAX_SLOTS, V.object(PLAYER)), R], claims: [V.map(256, CLAIM_KEY, V.slot()), R],
    sessions: [(v) => isPlain(v), R], kv: [V.list(256, KV_ENTRY), O] } },
  D: { name: 'delta', spec: { d: [V.list(64, V.tuple(V.int(0, Number.MAX_SAFE_INTEGER), V.str(1, 48), V.any(), V.slotOrServer())), R] } },
  R: { name: 'result', spec: { r: [V.int(0, MAX_INT), R], ok: [V.bit(), R], seq: [V.int(0, Number.MAX_SAFE_INTEGER), O], why: [V.str(0, 64), O], by: [V.slot(), O] } },
  E: { name: 'events', spec: { e: [V.list(64, V.tuple(V.int(0, 0xffffffff), V.slot(), V.int(0, Number.MAX_SAFE_INTEGER), V.str(1, 16), V.nums(16))), R] } },
  K: { name: 'kv', spec: { w: [V.list(64, KV_ENTRY), R] } },
  J: { name: 'join', spec: PLAYER },
  L: { name: 'leave', spec: { slot: [V.slot(), R], why: [V.str(0, 64), O] } },
  F: { name: 'profile', spec: { slot: [V.slot(), R], name: [V.str(1, 24), R], acct: [V.str(0, 32), O], av: [V.int(0, MAX_INT), O], badges: [V.list(16, V.str(1, 16)), O], role: [V.str(0, 16), O] } },
  G: { name: 'signal', spec: { from: [V.slot(), R], data: [V.any(), R] } },
  P: { name: 'pong', spec: { c: [V.num(), R], s: [V.num(), R] } },
};

// dir: 'toServer' or 'toClient'. Returns {ok:true, letter, name, body} or {ok:false, why}.
export function decodeText(text, dir) {
  if (typeof text !== 'string' || text.length < 3) return fail('too short');
  const table = dir === 'toServer' ? TO_SERVER : TO_CLIENT;
  const letter = text[0];
  const def = Object.prototype.hasOwnProperty.call(table, letter) ? table[letter] : null;
  if (!def) return fail('unknown type');
  const max = dir === 'toServer' ? (letter === 'G' ? MAX_SIGNAL_TO_SERVER : MAX_TEXT_TO_SERVER) : MAX_TO_CLIENT;
  if (utf8Len(text) > max) return fail('too big');
  let body;
  try { body = JSON.parse(text.slice(1)); } catch { return fail('bad json'); }
  if (!isPlain(body)) return fail('not an object');
  if (!allFinite(body)) return fail('number not finite');
  if (!checkObject(body, def.spec)) return fail('bad field');
  return { ok: true, letter, name: def.name, body };
}

// encodeText('P', {c: 1}, 'toServer') → 'P{"c":1}', or null if the body isn't valid.
export function encodeText(letter, body, dir) {
  const text = letter + JSON.stringify(body);
  return decodeText(text, dir).ok ? text : null;
}
