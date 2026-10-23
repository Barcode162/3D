// Moods and the budget meter (docs/STUDIO.md 5.6, S7.3), drawn in the
// Monitor's Look tab. A mood is a named set of light colours and brightness
// (by kind, or by thing) in the place file: "moods": {"Party": {"lights":
// {"light-sign": {"color": [..], "intensity": 1}}}}, "mood": "Party". The
// engine uses the place's mood when it builds the place; the Studio shows a
// switch at once (st.light). The meter: what the phone draws now against
// what it holds at 60 fps.
import { S, on, send } from './studio.js';
import { edit, placeFile, kindOf, thingEntry, refreshLights, setOp } from './draft.mjs';

const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
// Phone limits (60 fps on an iPhone, docs/ENGINE.md perf notes): the bar as built sits well inside.
export const LIMITS = { draws: 450, tris: 1200000 };
const L = { budget: null, listeners: [] };

// The lit things' lights now (without a mood), by kind: what a new mood starts from.
function lightsByKind() {
  const out = {};
  for (const t of S.things) {
    const own = thingEntry(t.id).json;
    const k = kindOf(t.kind);
    const l = (own && own.parts && own.parts.light) || (k && k.parts && k.parts.light);
    if (!l || !Array.isArray(l.color) || out[t.kind]) continue;
    out[t.kind] = { color: [...l.color], intensity: l.intensity ?? 1 };
  }
  return out;
}
// Presets made from the lights as they are.
function hueShift([r, g, b], deg) {
  const a = (deg * Math.PI) / 180, c = Math.cos(a), s = Math.sin(a), k = 1 / 3, q = Math.sqrt(k);
  const m = [c + (1 - c) * k, k * (1 - c) - q * s, k * (1 - c) + q * s];
  const clamp = (v) => +Math.max(0, Math.min(1, v)).toFixed(3);
  return [clamp(r * m[0] + g * m[1] + b * m[2]), clamp(r * m[2] + g * m[0] + b * m[1]), clamp(r * m[1] + g * m[2] + b * m[0])];
}
export function presets() {
  const base = lightsByKind();
  const map = (f) => Object.fromEntries(Object.entries(base).map(([k, l], i) => [k, f(l, i)]));
  return {
    // Real units (L1.2): lumens follow the brightness; Closing also warms the bulbs.
    Evening: { lights: map((l) => ({ color: l.color, intensity: l.intensity, ...(l.lumens ? { lumens: l.lumens } : {}) })) },
    Party: { lights: map((l, i) => ({ color: hueShift(l.color, 120 * (i % 3) + 60), intensity: +(l.intensity * 1.3).toFixed(2), ...(l.lumens ? { lumens: Math.round(l.lumens * 1.3) } : {}) })) },
    Closing: { lights: map((l) => ({ color: [l.color[0], +(l.color[1] * 0.8).toFixed(3), +(l.color[2] * 0.6).toFixed(3)], intensity: +(l.intensity * 0.35).toFixed(2), ...(l.lumens ? { lumens: Math.round(l.lumens * 0.35) } : {}), ...(l.kelvin ? { kelvin: Math.max(1800, l.kelvin - 400) } : {}) })) },
  };
}
export function useMood(name) {
  const place = S.draft[placeFile()];
  const ops = [];
  if (!place.moods || !place.moods[name]) {
    const p = presets()[name] || presets().Evening;
    ops.push(setOp(placeFile(), ['moods'], { ...(place.moods || {}), [name]: p }));
  }
  ops.push(setOp(placeFile(), ['mood'], name));
  edit({ op: 'group', label: `${S.place}: the ${name} mood`, ops });
  refreshLights();
  setTimeout(askBudget, 400);
}
export function askBudget() { send('st.budget'); }
on('budget', (e) => { L.budget = e; L.listeners.forEach((f) => f(e)); });

// The Look tab's moods and meter.
export function drawMoods(body) {
  const place = S.draft[placeFile()] || {};
  body.append(el('span', 'label', 'Mood'));
  const row = el('div', 'chiprow');
  const names = [...new Set(['Evening', 'Party', 'Closing', ...Object.keys(place.moods || {})])];
  for (const n of names) {
    const b = el('button', 'chip' + (place.mood === n ? ' on' : ''), n);
    b.type = 'button';
    b.dataset.mood = n;
    b.onclick = () => useMood(n);
    row.append(b);
  }
  body.append(row);
  body.append(el('p', 'dim', place.mood ? `${place.mood}: the lights of this place, as a set. Change a light in Inspect, then pick the mood again to keep it.` : 'Pick one: Evening is the lights as they are; Party and Closing start from them.'));
  // The budget meter.
  body.append(el('span', 'label', 'What the phone draws'));
  const meter = el('div', 'meter');
  const draw = (b) => {
    meter.textContent = '';
    // Lights are a cap (more than it holds are left out), not a cost: green up to it.
    const rows = [['Draws', b.draws, LIMITS.draws, 0.8], ['Triangles', b.tris, LIMITS.tris, 0.8], ['Lights', b.lights, b.maxLights, 1.0001]];
    const level = ([, v, max, warn]) => (v / max < warn ? 'ok' : v <= max && warn < 1 ? 'warn' : 'bad');
    for (const row of rows) {
      const [name, v, max] = row;
      const r = el('div', 'meterrow');
      const f = Math.min(1, v / max);
      const bar = el('div', 'bar');
      const fill = el('i', level(row));
      fill.style.width = Math.round(f * 100) + '%';
      bar.append(fill);
      r.append(el('span', '', name), bar, el('small', '', `${v.toLocaleString('en-US')} / ${max.toLocaleString('en-US')}`));
      meter.append(r);
    }
    meter.dataset.state = rows.every((r) => level(r) === 'ok') ? 'green' : rows.some((r) => level(r) === 'bad') ? 'red' : 'amber';
  };
  if (L.budget) draw(L.budget); else meter.append(el('p', 'dim', 'Measuring…'));
  L.listeners = [draw];
  body.append(meter);
  askBudget();
}
window.bar3dLook = L;
