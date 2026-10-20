// Generative score + synthesized engine/tyres/wind. Everything is procedural Web Audio.
const mtof = (m) => 440 * Math.pow(2, (m - 69) / 12);

export class AudioEngine {
  constructor() {
    this.ready = false;
    this.musicVol = 0.7; this.sfxVol = 0.7; this.muted = false;
    this.flow = 0; this.speed = 0;
  }
  init() {
    if (this.ready) { this.ctx.resume(); return; }
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    const ctx = this.ctx = new AC();
    this.master = ctx.createGain(); this.master.gain.value = 0.9;
    const comp = ctx.createDynamicsCompressor();
    comp.threshold.value = -16; comp.ratio.value = 3; comp.attack.value = 0.01; comp.release.value = 0.3;
    this.master.connect(comp).connect(ctx.destination);

    this.musicBus = ctx.createGain(); this.musicBus.gain.value = this.musicVol * 0.55;
    this.sfxBus = ctx.createGain(); this.sfxBus.gain.value = this.sfxVol;
    this.musicBus.connect(this.master); this.sfxBus.connect(this.master);

    // reverb
    this.reverb = ctx.createConvolver();
    this.reverb.buffer = this._impulse(4.2, 2.4);
    this.revSend = ctx.createGain(); this.revSend.gain.value = 0.55;
    this.revSend.connect(this.reverb).connect(this.musicBus);

    // delay (dotted eighth)
    this.delay = ctx.createDelay(2); this.delay.delayTime.value = (60 / 84) * 0.75;
    const fb = ctx.createGain(); fb.gain.value = 0.42;
    const dlp = ctx.createBiquadFilter(); dlp.type = 'lowpass'; dlp.frequency.value = 2400;
    this.delay.connect(dlp).connect(fb).connect(this.delay);
    this.delaySend = ctx.createGain(); this.delaySend.gain.value = 0.35;
    this.delaySend.connect(this.delay); dlp.connect(this.musicBus); dlp.connect(this.revSend);

    // music layers
    const mk = (g) => { const n = ctx.createGain(); n.gain.value = g; n.connect(this.musicBus); n.connect(this.revSend); return n; };
    this.padFilter = ctx.createBiquadFilter(); this.padFilter.type = 'lowpass'; this.padFilter.frequency.value = 700; this.padFilter.Q.value = 0.6;
    this.layers = { pad: mk(0.0), bass: mk(0), arp: mk(0), drums: mk(0), bell: mk(0) };
    this.padFilter.connect(this.layers.pad);
    this.layers.arp.connect(this.delaySend);
    this.layers.bell.connect(this.delaySend);
    this.layers.pad.gain.setTargetAtTime(0.5, ctx.currentTime, 2.5);

    this._noise = this._noiseBuffer();
    this._initEngine();
    this._initWorld();

    // sequencer
    this.bpm = 84; this.step = 0; this.nextTime = ctx.currentTime + 0.15;
    this.prog = [
      { root: 38, pad: [53, 57, 60, 64], arp: [62, 65, 69, 72, 76] },   // Dm9
      { root: 34, pad: [50, 53, 57, 60], arp: [58, 62, 65, 69, 72] },   // Bbmaj9
      { root: 41, pad: [52, 57, 60, 67], arp: [60, 64, 67, 69, 72] },   // Fmaj7add9
      { root: 33, pad: [52, 55, 60, 64], arp: [57, 60, 64, 67, 71] },   // Am7 / C
    ];
    this.penta = [62, 64, 65, 69, 72, 74, 76, 77, 81];
    this.timer = setInterval(() => this._schedule(), 40);
    this.ready = true;
  }
  _impulse(sec, decay) {
    const ctx = this.ctx, rate = ctx.sampleRate, len = Math.floor(rate * sec);
    const buf = ctx.createBuffer(2, len, rate);
    for (let c = 0; c < 2; c++) {
      const d = buf.getChannelData(c);
      for (let i = 0; i < len; i++) d[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / len, decay);
    }
    return buf;
  }
  _noiseBuffer() {
    const ctx = this.ctx, len = ctx.sampleRate * 2;
    const buf = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = buf.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) { const w = Math.random() * 2 - 1; last = (last + 0.02 * w) / 1.02; d[i] = w * 0.6 + last * 3; }
    return buf;
  }
  _noiseSrc() { const s = this.ctx.createBufferSource(); s.buffer = this._noise; s.loop = true; s.start(); return s; }

  _initEngine() {
    const ctx = this.ctx;
    this.eng = {};
    const out = ctx.createGain(); out.gain.value = 0; this.eng.out = out;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 400; lp.Q.value = 2.2; this.eng.lp = lp;
    const shaper = ctx.createWaveShaper();
    const curve = new Float32Array(1024);
    for (let i = 0; i < 1024; i++) { const x = i / 512 - 1; curve[i] = Math.tanh(x * 2.2); }
    shaper.curve = curve;
    lp.connect(shaper).connect(out).connect(this.sfxBus);
    const o1 = ctx.createOscillator(); o1.type = 'sawtooth';
    const o2 = ctx.createOscillator(); o2.type = 'square';
    const o3 = ctx.createOscillator(); o3.type = 'sine';
    const g1 = ctx.createGain(); g1.gain.value = 0.32;
    const g2 = ctx.createGain(); g2.gain.value = 0.16;
    const g3 = ctx.createGain(); g3.gain.value = 0.55;
    o1.connect(g1).connect(lp); o2.connect(g2).connect(lp); o3.connect(g3).connect(lp);
    o1.start(); o2.start(); o3.start();
    this.eng.o1 = o1; this.eng.o2 = o2; this.eng.o3 = o3;
    // intake noise
    const n = this._noiseSrc(); const nbp = ctx.createBiquadFilter(); nbp.type = 'bandpass'; nbp.frequency.value = 600; nbp.Q.value = 1.2;
    const ng = ctx.createGain(); ng.gain.value = 0; n.connect(nbp).connect(ng).connect(this.sfxBus);
    this.eng.intake = ng; this.eng.intakeBp = nbp;
  }
  _initWorld() {
    const ctx = this.ctx;
    const chain = (type, f, q) => { const src = this._noiseSrc(); const flt = ctx.createBiquadFilter(); flt.type = type; flt.frequency.value = f; flt.Q.value = q; const g = ctx.createGain(); g.gain.value = 0; src.connect(flt).connect(g).connect(this.sfxBus); return { flt, g }; };
    this.wind = chain('lowpass', 500, 0.4);
    this.roadN = chain('lowpass', 220, 0.8);
    this.gravel = chain('bandpass', 1400, 0.7);
    this.squeal = chain('bandpass', 1150, 9);
    // squeal tone
    const so = ctx.createOscillator(); so.type = 'triangle'; so.frequency.value = 820;
    const sg = ctx.createGain(); sg.gain.value = 0; so.connect(sg).connect(this.sfxBus); so.start();
    this.squealTone = { o: so, g: sg };
  }

  setVolumes(music, sfx) {
    this.musicVol = music; this.sfxVol = sfx;
    if (!this.ready) return;
    const t = this.ctx.currentTime;
    this.musicBus.gain.setTargetAtTime(this.muted ? 0 : music * 0.55, t, 0.2);
    this.sfxBus.gain.setTargetAtTime(sfx, t, 0.2);
  }
  toggleMute() { this.muted = !this.muted; this.setVolumes(this.musicVol, this.sfxVol); return this.muted; }
  setPaused(p) {
    if (!this.ready) return;
    const t = this.ctx.currentTime;
    this.sfxBus.gain.setTargetAtTime(p ? 0 : this.sfxVol, t, 0.15);
    this.padFilter.frequency.setTargetAtTime(p ? 350 : 700, t, 0.5);
  }

  // called every frame
  update(st) {
    if (!this.ready) return;
    const t = this.ctx.currentTime, k = 0.06;
    this.flow = st.flow; this.speed = st.speed;
    // engine
    const f = st.rpm / 60 * 2; // 4-cyl firing frequency
    this.eng.o1.frequency.setTargetAtTime(f, t, 0.03);
    this.eng.o2.frequency.setTargetAtTime(f * 0.5, t, 0.03);
    this.eng.o3.frequency.setTargetAtTime(f * 0.5, t, 0.03);
    const load = st.throttle;
    this.eng.lp.frequency.setTargetAtTime(180 + load * 900 + st.rpm * 0.12, t, k);
    this.eng.out.gain.setTargetAtTime(st.active ? 0.05 + load * 0.09 + st.rpm / 7200 * 0.03 : 0.02, t, k);
    this.eng.intake.gain.setTargetAtTime(load * st.rpm / 7200 * 0.05, t, k);
    this.eng.intakeBp.frequency.setTargetAtTime(400 + st.rpm * 0.25, t, k);
    // world
    const v = st.speed;
    this.wind.g.gain.setTargetAtTime(Math.min(0.32, v * v * 0.00012), t, 0.2);
    this.wind.flt.frequency.setTargetAtTime(300 + v * 14, t, 0.2);
    this.roadN.g.gain.setTargetAtTime(Math.min(0.35, v * 0.006) * (st.offroad ? 0.4 : 1), t, 0.1);
    this.gravel.g.gain.setTargetAtTime(st.offroad ? Math.min(0.5, v * 0.025) : st.shoulder ? Math.min(0.2, v * 0.008) : 0, t, 0.06);
    const sq = st.offroad ? 0 : Math.max(0, st.slip - 0.07) * 1.6;
    this.squeal.g.gain.setTargetAtTime(Math.min(0.12, sq * 0.5), t, 0.05);
    this.squealTone.g.gain.setTargetAtTime(Math.min(0.025, sq * 0.08), t, 0.05);
    this.squealTone.o.frequency.setTargetAtTime(700 + Math.min(1, sq) * 260, t, 0.08);

    // music layers follow flow
    const fl = st.flow;
    const L = this.layers;
    const on = (x, a, b) => Math.max(0, Math.min(1, (x - a) / (b - a)));
    L.bass.gain.setTargetAtTime(0.32 * on(fl, 0.12, 0.25), t, 1.8);
    L.arp.gain.setTargetAtTime(0.16 * on(fl, 0.3, 0.45), t, 1.8);
    L.drums.gain.setTargetAtTime(0.42 * on(fl, 0.5, 0.65), t, 1.6);
    L.bell.gain.setTargetAtTime(0.12 * on(fl, 0.78, 0.9), t, 2.0);
    if (!st.paused) this.padFilter.frequency.setTargetAtTime(520 + fl * 1700, t, 1.5);
  }

  // ---- sequencer ----
  _schedule() {
    const ctx = this.ctx;
    const sixteenth = 60 / this.bpm / 4;
    while (this.nextTime < ctx.currentTime + 0.25) {
      this._playStep(this.step, this.nextTime, sixteenth);
      this.nextTime += sixteenth;
      this.step++;
    }
  }
  _playStep(step, time, dur16) {
    const bar = Math.floor(step / 16);
    const inBar = step % 16;
    const chordIdx = Math.floor(bar / 2) % this.prog.length;
    const ch = this.prog[chordIdx];
    // pad: on chord change
    if (step % 32 === 0) {
      ch.pad.forEach((n, i) => this._pad(mtof(n), time + i * 0.04, dur16 * 32 + 0.5));
    }
    // bass
    if (inBar === 0 || inBar === 10) this._bass(mtof(ch.root + (inBar === 10 ? 12 : 0)), time, inBar === 0 ? dur16 * 9 : dur16 * 5);
    // arpeggio (8ths with a gentle pattern)
    if (step % 2 === 0) {
      const pat = [0, 2, 1, 3, 4, 2, 3, 1];
      const n = ch.arp[pat[(step / 2) % 8]];
      const swing = (step % 4 === 2) ? dur16 * 0.18 : 0;
      this._pluck(mtof(n), time + swing, 0.9);
    }
    // drums
    if (inBar === 0 || inBar === 7 || inBar === 10) this._kick(time, inBar === 0 ? 1 : 0.6);
    if (inBar === 4 || inBar === 12) this._rim(time);
    if (step % 2 === 0) this._hat(time + ((step % 4 === 2) ? dur16 * 0.18 : 0), step % 4 === 2 ? 0.55 : 0.32);
    // bells: sparse pentatonic
    if (step % 4 === 0 && Math.random() < 0.28) {
      const n = this.penta[Math.floor(Math.random() * this.penta.length)] + 12;
      this._bell(mtof(n), time);
    }
  }
  _env(g, t, a, peak, d) {
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(peak, t + a);
    g.gain.exponentialRampToValueAtTime(0.0001, t + a + d);
  }
  _pad(freq, t, len) {
    const ctx = this.ctx;
    const g = ctx.createGain(); g.gain.value = 0;
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(0.07, t + 2.2);
    g.gain.setValueAtTime(0.07, t + len - 2.4);
    g.gain.linearRampToValueAtTime(0, t + len + 1.5);
    g.connect(this.padFilter);
    [-7, 6].forEach((det) => {
      const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = freq; o.detune.value = det;
      o.connect(g); o.start(t); o.stop(t + len + 1.6);
    });
    const s = ctx.createOscillator(); s.type = 'sine'; s.frequency.value = freq * 0.5;
    const sg = ctx.createGain(); sg.gain.value = 0.6; s.connect(sg).connect(g); s.start(t); s.stop(t + len + 1.6);
  }
  _bass(freq, t, len) {
    const ctx = this.ctx;
    const o = ctx.createOscillator(); o.type = 'sine'; o.frequency.value = freq;
    const o2 = ctx.createOscillator(); o2.type = 'triangle'; o2.frequency.value = freq * 2;
    const g = ctx.createGain(); const g2 = ctx.createGain(); g2.gain.value = 0.15;
    g.gain.setValueAtTime(0.0001, t); g.gain.exponentialRampToValueAtTime(0.5, t + 0.04);
    g.gain.exponentialRampToValueAtTime(0.25, t + len * 0.6); g.gain.exponentialRampToValueAtTime(0.0001, t + len);
    o.connect(g); o2.connect(g2).connect(g); g.connect(this.layers.bass);
    o.start(t); o2.start(t); o.stop(t + len + 0.05); o2.stop(t + len + 0.05);
  }
  _pluck(freq, t, vel) {
    const ctx = this.ctx;
    const o = ctx.createOscillator(); o.type = 'triangle'; o.frequency.value = freq;
    const f = ctx.createBiquadFilter(); f.type = 'lowpass'; f.frequency.setValueAtTime(3200, t); f.frequency.exponentialRampToValueAtTime(500, t + 0.35);
    const g = ctx.createGain(); this._env(g, t, 0.006, 0.22 * vel, 0.55);
    o.connect(f).connect(g).connect(this.layers.arp);
    o.start(t); o.stop(t + 0.7);
  }
  _bell(freq, t, dest) {
    const ctx = this.ctx;
    const o = ctx.createOscillator(); o.type = 'sine'; o.frequency.value = freq;
    const m = ctx.createOscillator(); m.type = 'sine'; m.frequency.value = freq * 3.5;
    const mg = ctx.createGain(); mg.gain.setValueAtTime(freq * 0.8, t); mg.gain.exponentialRampToValueAtTime(1, t + 1.2);
    m.connect(mg).connect(o.frequency);
    const g = ctx.createGain(); this._env(g, t, 0.01, 0.2, 2.6);
    o.connect(g).connect(dest || this.layers.bell);
    o.start(t); m.start(t); o.stop(t + 2.8); m.stop(t + 2.8);
  }
  _kick(t, vel) {
    const ctx = this.ctx;
    const o = ctx.createOscillator(); o.type = 'sine';
    o.frequency.setValueAtTime(110, t); o.frequency.exponentialRampToValueAtTime(42, t + 0.18);
    const g = ctx.createGain(); this._env(g, t, 0.004, 0.75 * vel, 0.32);
    o.connect(g).connect(this.layers.drums); o.start(t); o.stop(t + 0.4);
  }
  _hat(t, vel) {
    const ctx = this.ctx;
    const s = ctx.createBufferSource(); s.buffer = this._noise;
    const f = ctx.createBiquadFilter(); f.type = 'highpass'; f.frequency.value = 7000;
    const g = ctx.createGain(); this._env(g, t, 0.002, 0.09 * vel, 0.05);
    s.connect(f).connect(g).connect(this.layers.drums); s.start(t, Math.random()); s.stop(t + 0.1);
  }
  _rim(t) {
    const ctx = this.ctx;
    const s = ctx.createBufferSource(); s.buffer = this._noise;
    const f = ctx.createBiquadFilter(); f.type = 'bandpass'; f.frequency.value = 1800; f.Q.value = 3;
    const g = ctx.createGain(); this._env(g, t, 0.002, 0.22, 0.12);
    s.connect(f).connect(g).connect(this.layers.drums); s.start(t, Math.random()); s.stop(t + 0.2);
  }
  chime(good = true) {
    if (!this.ready) return;
    const t = this.ctx.currentTime + 0.02;
    const notes = good ? [74, 81] : [62];
    if (!this.chimeBus) { this.chimeBus = this.ctx.createGain(); this.chimeBus.gain.value = 0.35; this.chimeBus.connect(this.musicBus); this.chimeBus.connect(this.delaySend); }
    notes.forEach((n, i) => this._bell(mtof(n), t + i * 0.09, this.chimeBus));
  }
}
