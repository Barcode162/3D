// JavaScript side of the room's physics (room_sim.wasm, built from the bar's
// C++ by cloud/sim/build.sh). new Sim(module) with the compiled
// WebAssembly.Module (the Worker imports it; tests compile it from the file).
export class Sim {
  constructor(module) {
    const noop = () => 0;
    this.inst = new WebAssembly.Instance(module, {
      env: { emscripten_notify_memory_growth: () => { this.view = null; } },
      wasi_snapshot_preview1: { fd_close: noop, fd_write: noop, fd_seek: noop },
    });
    const e = this.inst.exports;
    this.e = e;
    if (e._initialize) e._initialize();
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
  hold(i, hand) { const b = this.buf(); b.set(hand, 0); this.e.sim_hold(i); }
  release(i, pos, quat, vel) { const b = this.buf(); b.set(pos, 0); b.set(quat, 3); b.set(vel, 7); this.e.sim_release(i); }
  place(i, pos, quat) { const b = this.buf(); b.set(pos, 0); b.set(quat, 3); this.e.sim_place(i); }
}
