// Glasses, run by the room with the bar's real physics (cloud/sim/: the same
// C++ the phones use). A phone asks to grab, put down or throw a glass; the
// room checks it (in reach, not someone else's, a sane throw), moves the
// glass in its own simulation and sends everyone the poses (TICK props)
// until it comes to rest; the resting spot is saved as world key g<n>.
//
// Hand position = the holder's eyes + 0.5 m forward + 0.12 m right - 0.28 m
// down, from their latest STATE (as app/main.cpp updatePhysics does).

export const REACH_M = 3.0;       // grab/put down within this of your eyes
export const MAX_THROW = 12;      // m/s
const MAX_DT = 0.1;               // a late tick doesn't simulate more than this

// The avatar block's eyes and look direction (PROTOCOL §3.1), from the raw
// STATE bytes the room keeps.
export function eyesOf(raw) {
  if (!raw || raw.length < 16) return null;
  const d = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  return {
    x: d.getInt16(0, true) / 100, y: d.getInt16(2, true) / 100, z: d.getInt16(4, true) / 100,
    yaw: d.getUint16(6, true) * 2 * Math.PI / 65536, pitch: d.getInt16(8, true) / 10000,
  };
}

export function handOf(e) {
  const fx = Math.sin(e.yaw) * Math.cos(e.pitch), fy = Math.sin(e.pitch), fz = -Math.cos(e.yaw) * Math.cos(e.pitch);
  const rx = Math.cos(e.yaw), rz = Math.sin(e.yaw);
  return [e.x + fx * 0.5 + rx * 0.12, e.y + fy * 0.5 - 0.28, e.z + fz * 0.5 + rz * 0.12];
}

const dist = (a, b) => Math.hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
const fin = (v) => Array.isArray(v) && v.every(Number.isFinite);

export class Glasses {
  constructor(sim) {
    this.sim = sim;
    this.holder = new Map();   // glass → slot
    this.moving = new Set();   // glasses flying/settling (not held)
  }

  get count() { return this.sim.count; }
  valid(g) { return Number.isInteger(g) && g >= 0 && g < this.sim.count; }

  // Puts glasses where the room last saw them rest (world keys g<n>).
  restore(kv) {
    for (const [key, e] of kv) {
      const m = /^g(\d+)$/.exec(key);
      if (!m || !this.valid(+m[1]) || !e || e.values.length < 7) continue;
      this.sim.place(+m[1], e.values.slice(0, 3), e.values.slice(3, 7));
    }
  }

  grab(g, slot, eyes) {
    if (!this.valid(g)) return { ok: 0, why: 'no such glass' };
    const h = this.holder.get(g);
    if (h !== undefined && h !== slot) return { ok: 0, why: 'taken', by: h };
    const b = this.sim.get(g);
    if (eyes && dist([eyes.x, eyes.y, eyes.z], b.pos) > REACH_M) return { ok: 0, why: 'too far' };
    for (const [other, s] of this.holder) if (s === slot && other !== g) return { ok: 0, why: 'one glass at a time' };
    this.holder.set(g, slot);
    this.moving.delete(g);
    this.sim.hold(g, eyes ? handOf(eyes) : b.pos);
    return { ok: 1 };
  }

  // Put down at `pos` (+ `quat`), or thrown with `vel` from where it is.
  letGo(g, slot, eyes, args) {
    if (!this.valid(g)) return { ok: 0, why: 'no such glass' };
    if (this.holder.get(g) !== slot) return { ok: 0, why: 'not in your hand' };
    const b = this.sim.get(g);
    let pos = b.pos, quat = b.quat, vel = [0, 0, 0];
    if (args.pos !== undefined) {
      if (!fin(args.pos) || args.pos.length !== 3) return { ok: 0, why: 'bad spot' };
      if (eyes && dist([eyes.x, eyes.y, eyes.z], args.pos) > REACH_M) return { ok: 0, why: 'too far' };
      pos = args.pos;
      quat = [0, 0, 0, 1];  // stood upright on the spot, like the phone does
    } else {
      if (!fin(args.vel) || args.vel.length !== 3) return { ok: 0, why: 'bad throw' };
      const speed = Math.hypot(...args.vel);
      vel = speed > MAX_THROW ? args.vel.map((v) => v * MAX_THROW / speed) : args.vel;
    }
    this.holder.delete(g);
    this.moving.add(g);
    this.sim.release(g, pos, quat, vel);
    return { ok: 1 };
  }

  // Someone left (or lost the glass): it drops where it is.
  drop(slot) {
    const out = [];
    for (const [g, s] of [...this.holder]) {
      if (s !== slot) continue;
      const b = this.sim.get(g);
      this.holder.delete(g);
      this.moving.add(g);
      this.sim.release(g, b.pos, b.quat, [0, 0, 0]);
      out.push(g);
    }
    return out;
  }

  busy() { return this.holder.size > 0 || this.moving.size > 0; }

  // One room tick: hands follow their holders, the world steps. Returns the
  // poses to send (held and moving glasses) and the glasses that just
  // came to rest ({g, values} for world key g<n>, state 2).
  tick(dt, eyesOfSlot) {
    if (!this.busy()) return { samples: [], rested: [] };
    for (const [g, slot] of this.holder) {
      const e = eyesOfSlot(slot);
      if (e) this.sim.hold(g, handOf(e));
    }
    this.sim.step(Math.min(MAX_DT, Math.max(0, dt)));
    const samples = [], rested = [];
    for (const g of [...this.holder.keys(), ...this.moving]) {
      const b = this.sim.get(g);
      samples.push({ id: g, pos: b.pos, quat: b.quat, vel: b.vel, held: this.holder.has(g) });
      if (!this.holder.has(g) && b.asleep) {
        this.moving.delete(g);
        rested.push({ g, values: [...b.pos.map(r4), ...b.quat.map(r4), 0, 0, 0, 2] });
      }
    }
    return { samples, rested };
  }
}

const r4 = (v) => Math.round(v * 10000) / 10000;
