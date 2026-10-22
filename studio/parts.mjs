// Part mode (docs/STUDIO.md 5.3, S5.2): a thing's parts you can see and
// move: its shapes you bump into (solids), tap areas (uses) and seats. A part
// lives where it's written: the thing's own parts if it has them, else the
// kind (or the kind it builds on) that defines it, so editing one changes
// every thing of that kind. The engine draws the chosen part as a glowing box
// (st.part); the bar uses the change after Publish.
import { S, send } from './studio.js';
import { placeFile, thingEntry } from './draft.mjs';

export const PART_NAMES = { boxes: 'Box', solids: 'Shape', uses: 'Tap area', seats: 'Seat' };
const r3 = (v) => +(+v).toFixed(3);

function kindJson(id) {
  for (const f of S.draft[placeFile()].kinds || []) {
    const k = (S.draft[f].kinds || []).find((x) => x.id === id);
    if (k) return { k, f };
  }
  return null;
}
// Where a part list is written: {file, path, list, owner}.
export function partSource(t, name) {
  const e = thingEntry(t.id);
  if (e.json && e.json.parts && e.json.parts[name] !== undefined) return { file: placeFile(), path: [...e.path, 'parts', name], list: e.json.parts[name], owner: t.id };
  for (let id = t.kind, n = 0; id && n < 16; n++) {
    const kf = kindJson(id);
    if (!kf) break;
    if (kf.k.parts && kf.k.parts[name] !== undefined) return { file: kf.f, path: ['kinds', { id }, 'parts', name], list: kf.k.parts[name], owner: id };
    id = kf.k.is;
  }
  return null;
}
// Every editable part of a thing: [{name, i, p, src, label}].
export function partsOf(t) {
  const out = [];
  if (!t || !S.draft) return out;
  for (const name of Object.keys(PART_NAMES)) {
    const src = partSource(t, name);
    if (!src || !Array.isArray(src.list)) continue;
    src.list.forEach((p, i) => out.push({ name, i, p, src, label: `${PART_NAMES[name]} ${i + 1}` + (p.name ? ` · ${p.name}` : p.prompt ? ` · ${p.prompt}` : '') }));
  }
  return out;
}
// A part as a rest the tools understand: at (its middle), turn, scale (its size in metres).
export function partRest(name, p) {
  if (name === 'uses') {
    const [a, b] = p.box || [[0, 0, 0], [0.2, 0.2, 0.2]];
    return { at: [0, 1, 2].map((k) => r3((a[k] + b[k]) / 2)), turn: 0, scale: [0, 1, 2].map((k) => r3(Math.abs(b[k] - a[k]))) };
  }
  if (name === 'seats') return { at: [...(p.hip || [0, 0, 0])], turn: p.facing || 0, scale: [1, 1, 1] };
  if (name === 'boxes') return { at: [...(p.at || [0, 0, 0])], turn: 0, scale: [...(p.size || [1, 1, 1])] };
  return { at: [...(p.at || [0, 0, 0])], turn: p.turn || 0, scale: (p.half || [0.1, 0.1, 0.1]).map((h) => r3(h * 2)) };
}
// The part with a new rest.
export function partFrom(name, p, r) {
  if (name === 'uses') return { ...p, box: [r.at.map((c, k) => r3(c - r.scale[k] / 2)), r.at.map((c, k) => r3(c + r.scale[k] / 2))] };
  if (name === 'seats') { const q = { ...p, hip: r.at.map(r3) }; if (r.turn) q.facing = r.turn; else delete q.facing; return q; }
  if (name === 'boxes') return { ...p, at: r.at.map(r3), size: r.scale.map((v) => r3(Math.max(0.01, v))) };
  const q = { ...p, at: r.at.map(r3), half: r.scale.map((s) => r3(Math.max(0.01, s / 2))) };
  if (r.turn) q.turn = r.turn; else delete q.turn;
  return q;
}
// Show a part in the world (or none).
export function showPart(t, name, r) {
  if (!t || !name) { send('st.part', -1, 0, ''); return; }
  const half = name === 'seats' ? [0.18, 0.12, 0.18] : r.scale.map((s) => s / 2);
  send('st.part', t.i, 0, [name === 'uses' ? 1 : 0, ...r.at, ...half, r.turn || 0].map((v) => +(+v).toFixed(4)).join(','));
}
