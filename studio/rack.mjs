// The Loop Rack (docs/STUDIO.md 5.7, S8.2-S8.3): the selected state's loop
// as lanes, one per piece (what moves, its shape, how much, how fast).
// Every change plays at once (the kind goes to the engine, st.kind). Pause
// and scrub hold the world's clock (st.clock). Keys: draw the curve with your
// finger. Copy a loop to paste on another kind; presets to start from.
import { S, send } from './studio.js';
import { edit, kindOf } from './draft.mjs';
import { control } from './fields.mjs';
import { kindOps, LIFE } from './life.mjs';

const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
export const RACK = { paused: false, scrub: 0 };
window.bar3dRack = RACK;
const store = { get: (k) => { try { return localStorage.getItem(k); } catch (e) { return null; } }, set: (k, v) => { try { localStorage.setItem(k, v); } catch (e) {} } };

export const PRESETS = {
  sway: { period: 6, pieces: [{ ch: 'turn.z', shape: 'wave', amp: 3 }, { ch: 'turn.x', shape: 'wave', amp: 1.5, phase: 0.25 }] },
  breathe: { period: 4, pieces: [{ ch: 'scale', shape: 'wave', amp: 0.03 }, { ch: 'glow', shape: 'wave', amp: 0.2 }] },
  bob: { period: 2, pieces: [{ ch: 'move.y', shape: 'wave', amp: 0.04 }] },
  flicker: { period: 1, pieces: [{ ch: 'light', shape: 'flicker', amp: 0.4, rate: 3 }, { ch: 'glow', shape: 'noise', amp: 0.2, rate: 2 }] },
  thump: { period: 1, beat: true, pieces: [{ ch: 'scale', shape: 'beat', amp: 0.06, decay: 8 }] },
};

// Keys drawn with a finger: a strip where you drag left to right; the curve
// becomes 9 keys from -range to +range.
function keysPad(piece, onSet) {
  const W = 300, H = 90;
  const c = el('canvas', 'keyspad');
  c.width = W; c.height = H;
  const g = c.getContext('2d');
  const keys = (piece.keys || [[0, 0], [1, 0]]).map((k) => [...k]);
  const range = Math.max(1, ...keys.map((k) => Math.abs(k[1])));
  const drawCurve = (pts) => {
    g.clearRect(0, 0, W, H);
    g.strokeStyle = '#2C2926'; g.beginPath(); g.moveTo(0, H / 2); g.lineTo(W, H / 2); g.stroke();
    g.strokeStyle = '#F28C2C'; g.lineWidth = 2; g.beginPath();
    pts.forEach(([t, v], i) => { const x = t * W, y = H / 2 - (v / range) * (H / 2 - 6); if (i) g.lineTo(x, y); else g.moveTo(x, y); });
    g.stroke();
  };
  drawCurve(keys);
  let drawn = null;
  const at = (e) => { const r = c.getBoundingClientRect(); return [Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)), Math.max(-1, Math.min(1, (r.top + r.height / 2 - e.clientY) / (r.height / 2 - 6)))]; };
  c.addEventListener('pointerdown', (e) => { c.setPointerCapture(e.pointerId); drawn = [at(e)]; });
  c.addEventListener('pointermove', (e) => { if (!drawn) return; drawn.push(at(e)); drawCurve(drawn.map(([t, v]) => [t, v * range])); });
  c.addEventListener('pointerup', () => {
    if (!drawn || drawn.length < 2) { drawn = null; return; }
    // Sample the drawing at 9 even times (the nearest point drawn), ends at 0.
    drawn.sort((a, b) => a[0] - b[0]);
    const out = [];
    for (let i = 0; i <= 8; i++) {
      const t = i / 8;
      let best = drawn[0];
      for (const p of drawn) if (Math.abs(p[0] - t) < Math.abs(best[0] - t)) best = p;
      out.push([+t.toFixed(3), i === 0 || i === 8 ? 0 : +(best[1] * range).toFixed(3)]);
    }
    drawn = null;
    onSet(out);
  });
  return c;
}

export function drawRack(body, t, k, stateName, redraw) {
  const st = (k.states || {})[stateName];
  if (!st || !st.loop) return;
  const ln = st.loop, loop = (k.loops || {})[ln];
  if (!loop) return;
  const set = (path, v, label) => edit({ op: 'group', label: `every ${t.kind}: ${ln} loop ${label}`, ops: kindOps(t.kind, 'loops', [ln, ...path], v) });
  const fields = S.schema.loop.fields, pf = S.schema.piece.fields;
  const rack = el('div', 'rack');
  rack.append(el('span', 'label', `Loop Rack · ${ln}`));
  const top = el('div', 'racktop');
  const period = control(S.schema, fields.period, loop.beat ? 'Length (beats)' : 'Length', loop.period, (v) => set(['period'], v, 'length'));
  const beat = control(S.schema, fields.beat, 'Locked to the music', !!loop.beat, (v) => set(['beat'], v || undefined, 'on the beat'));
  top.append(period, beat);
  // Pause and scrub: hold the clock, slide through one loop.
  const pause = el('button', 'key small' + (RACK.paused ? ' lit' : ''), RACK.paused ? 'Play' : 'Pause');
  pause.type = 'button'; pause.id = 'rackPause';
  pause.onclick = () => { RACK.paused = !RACK.paused; RACK.scrub = 0; send('st.clock', RACK.paused ? 1 : 0); redraw(); };
  const scrub = el('div', 'field');
  Object.assign(scrub.dataset, { fader: '', min: 0, max: loop.period * (loop.beat ? 0.5 : 1), value: RACK.scrub, dec: 2, unit: ' s' });
  scrub.setAttribute('aria-label', 'Scrub through the loop');
  scrub.id = 'rackScrub';
  scrub.addEventListener('change', (e) => { if (e.target !== scrub) return; RACK.scrub = e.detail; if (!RACK.paused) { RACK.paused = true; pause.textContent = 'Play'; pause.classList.add('lit'); } send('st.clock', 2, e.detail); });
  const pr = el('div', 'scope'); pr.append(pause, el('span', 'dim', 'scrub'));
  top.append(pr, scrub);
  rack.append(top);
  // Lanes
  (loop.pieces || []).forEach((p, i) => {
    const lane = el('div', 'lane2');
    const head = el('div', 'scope');
    head.append(el('b', '', `${p.ch} · ${p.shape}`));
    const x = el('button', 'key small', '✕'); x.type = 'button'; x.setAttribute('aria-label', `Remove lane ${i + 1}`);
    x.onclick = () => { const pieces = loop.pieces.filter((_, j) => j !== i); set(['pieces'], pieces, `without lane ${i + 1}`); };
    head.append(x);
    lane.append(head);
    lane.append(control(S.schema, pf.ch, 'What moves', p.ch, (v) => set(['pieces', i, 'ch'], v, `lane ${i + 1} moves ${v}`)));
    lane.append(control(S.schema, pf.shape, 'Shape', p.shape, (v) => set(['pieces', i, 'shape'], v, `lane ${i + 1} shape ${v}`)));
    for (const key of ['amp', 'value', 'rate', 'phase', 'every', 'decay']) {
      if (typeof p[key] !== 'number' && !(key === 'amp' && p.shape !== 'keys' && p.shape !== 'hold')) continue;
      const v = typeof p[key] === 'number' ? p[key] : 1;
      const c = control(S.schema, pf[key], pf[key].label || key, v, (nv) => set(['pieces', i, key], nv, `lane ${i + 1} ${pf[key].label || key}`));
      if (c) lane.append(c);
    }
    if (p.shape === 'keys') {
      lane.append(el('span', 'dim', 'Keys: draw the curve with your finger'));
      lane.append(keysPad(p, (keys) => set(['pieces', i, 'keys'], keys, `lane ${i + 1} keys drawn`)));
    }
    rack.append(lane);
  });
  const add = el('button', 'key small', '+ Lane'); add.type = 'button'; add.id = 'rackAdd';
  add.onclick = () => set(['pieces'], [...(loop.pieces || []), { ch: 'turn.y', shape: 'wave', amp: 2 }], 'a new lane');
  // Copy, paste, presets (S8.3)
  const copy = el('button', 'key small', 'Copy loop'); copy.type = 'button';
  copy.onclick = () => { store.set('studioLoopCopy', JSON.stringify(loop)); Looped.toast(`Copied ${t.kind}'s ${ln} loop`); };
  const paste = el('button', 'key small', 'Paste'); paste.type = 'button'; paste.id = 'rackPaste';
  paste.onclick = () => { const v = store.get('studioLoopCopy'); if (!v) { Looped.toast('Copy a loop first'); return; } set([], JSON.parse(v), 'pasted'); };
  const tools = el('div', 'scope'); tools.append(add, copy, paste);
  rack.append(tools);
  const pre = el('div', 'chiprow');
  for (const name of Object.keys(PRESETS)) {
    const b = el('button', 'chip', name); b.type = 'button'; b.dataset.preset = name;
    b.onclick = () => set([], JSON.parse(JSON.stringify(PRESETS[name])), `from the ${name} preset`);
    pre.append(b);
  }
  rack.append(el('span', 'dim', 'Start from'), pre);
  body.append(rack);
}
LIFE.rack = drawRack;
