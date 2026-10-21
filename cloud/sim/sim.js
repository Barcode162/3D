// JavaScript side of the room's physics (room_sim.wasm, built from the bar's
// C++ by cloud/sim/build.sh). new Sim(module, files) with the compiled
// WebAssembly.Module (the Worker imports it; tests compile it from the file)
// and the bar's world files ({"places/bar.json": text, "kinds/bar.json": text},
// assets/world/), which the bar's layout is made from.
export class Sim {
  constructor(module, files = {}) {
    const noop = () => 0;
    this.inst = new WebAssembly.Instance(module, {
      env: { emscripten_notify_memory_growth: () => { this.view = null; } },
      wasi_snapshot_preview1: { fd_close: noop, fd_write: noop, fd_seek: noop },
    });
    const e = this.inst.exports;
    this.e = e;
    if (e._initialize) e._initialize();
    const enc = new TextEncoder();
    for (const [path, text] of Object.entries(files)) {
      const bytes = enc.encode(path + '\0' + text);
      const at = e.sim_text(bytes.length);
      new Uint8Array(e.memory.buffer, at, bytes.length).set(bytes);
      e.sim_file();
    }
    this.count = e.sim_init();
    this.bufPtr = e.sim_buffer();
    this.view = null;
  }
  buf() {
    if (!this.view || this.view.buffer !== this.e.memory.buffer) this.view = new Float32Array(this.e.memory.buffer, this.bufPtr, 16);
    return this.view;
  }
  step(dt) { this.e.sim_step(dt); }
  awake() { return this.e.sim_awake(); }
  // → {pos: [3], quat: [4], vel: [3], asleep, held}
  get(i) {
    if (!this.e.sim_get(i)) return null;
    const b = this.buf();
    return { pos: [b[0], b[1], b[2]], quat: [b[3], b[4], b[5], b[6]], vel: [b[7], b[8], b[9]], asleep: b[10] > 0, held: b[11] > 0 };
  }
  // Things (the bar's world): an event for thing `net` at the room's time →
  // the changes it made, [[net, state, start], ...] ([] = nothing happens).
  thingEvent(net, event, nowS) {
    const bytes = new TextEncoder().encode(event);
    const at = this.e.sim_text(bytes.length);
    new Uint8Array(this.e.memory.buffer, at, bytes.length).set(bytes);
    const n = this.e.sim_thing_event(net, nowS);
    const d = new Float64Array(this.e.memory.buffer, this.e.sim_dbuffer(), 96);
    const out = [];
    for (let k = 0; k < n; k++) out.push([d[k * 3], d[k * 3 + 1], d[k * 3 + 2]]);
    return out;
  }
  thingApply(net, state, start) { this.e.sim_thing_apply(net, state, start); }
  // Where thing `net` is ([x, y, z]) or null if there's no such thing.
  thingAt(net) {
    if (!this.e.sim_thing_at(net)) return null;
    const b = this.buf();
    return [b[0], b[1], b[2]];
  }
  hold(i, hand) { const b = this.buf(); b.set(hand, 0); this.e.sim_hold(i); }
  release(i, pos, quat, vel) { const b = this.buf(); b.set(pos, 0); b.set(quat, 3); b.set(vel, 7); this.e.sim_release(i); }
  place(i, pos, quat) { const b = this.buf(); b.set(pos, 0); b.set(quat, 3); this.e.sim_place(i); }
}
