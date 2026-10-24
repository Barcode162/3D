// The draft (docs/archive/STUDIO.md 5.8): the world's files as the Studio edits
// them, with the undo history. Loads the files the engine runs (the live
// version, else the ones built into the bar), keeps a copy to edit, and
// tells the engine about every change it can show at once (positions, sizes,
// what sits on what). Emits 'world' once loaded and 'changed' after edits.
import { S, on, send, emitEvent, api, setSaved, reloadWorld } from './studio.js';
import { History, apply, get, set as setOp, merge } from './ops.mjs';

const clone = (v) => JSON.parse(JSON.stringify(v));
export const placeFile = () => `places/${S.place}.json`;

export async function loadWorld() {
  const base = '/bar/assets/world/';
  S.schema = await (await fetch(base + 'schema.json')).json();
  const files = {};
  if (S.live && S.live.version > 0) Object.assign(files, (await (await fetch('api.php?bundle=' + S.live.version)).json()).files);
  const pf = placeFile();
  if (!files[pf]) files[pf] = await (await fetch(base + pf)).json();
  for (const k of files[pf].kinds || []) if (!files[k]) files[k] = await (await fetch(base + k)).json();
  if (!files['catalog.json']) { try { files['catalog.json'] = await (await fetch(base + 'catalog.json')).json(); } catch (e) {} }
  if (!files['materials.json']) { try { files['materials.json'] = await (await fetch(base + 'materials.json')).json(); } catch (e) {} }
  // Your draft from last time (the server's, else this phone's copy), on top
  // of the world; nothing sees the draft until it's whole.
  const draft = clone(files);
  let saved = null;
  try { const r = await api({ a: 'draft' }); if (r.ok && r.draft) saved = r.draft; } catch (e) {}
  if (!saved) { try { saved = JSON.parse(localStorage.getItem(backupKey()) || 'null'); } catch (e) {} }
  if (saved && saved.files && saved.files[placeFile()]) {
    if ((saved.base || 0) !== (S.live.version || 0)) Looped.toast(`Your draft started from version ${saved.base || 0}; the world is now version ${S.live.version}. Publishing will ask you to update it.`);
    for (const [k, v] of Object.entries(saved.files)) draft[k] = v;
    S.draftBase = saved.base || 0;
  } else S.draftBase = S.live.version || 0;
  S.base = files;
  S.draft = draft;
  S.history = new History(S.draft);
  emitEvent('world');
  // A draft with things the engine doesn't have (added, removed, a new kind): show it.
  const ids = new Set((S.draft[placeFile()].things || []).map((t) => t.id));
  const baseIds = new Set((S.base[placeFile()].things || []).map((t) => t.id));
  const kindsChanged = (S.draft[placeFile()].things || []).some((t) => { const b = (S.base[placeFile()].things || []).find((x) => x.id === t.id); return b && (b.kind !== t.kind || JSON.stringify(b.row) !== JSON.stringify(t.row) || JSON.stringify(b.parts) !== JSON.stringify(t.parts)); });
  if (kindsChanged || ids.size !== baseIds.size || [...ids].some((id) => !baseIds.has(id))) { await reloadWorld(); return; }
  // Show what the draft already changed.
  for (const t of S.things) { const e = thingEntry(t.id); if (e.json && JSON.stringify(e.json) !== JSON.stringify(((S.base[placeFile()].things) || []).find((x) => x.id === t.id))) sync({ op: 'set', file: placeFile(), path: ['things', { id: t.id }, 'at'] }); }
}

// ---- Saving (S3.4): to the server a moment after each change, and to this phone at once ----
const backupKey = () => 'studioDraft-' + S.place;
let saveTimer = 0;
export function saveSoon() {
  setSaveState('edit');
  try { localStorage.setItem(backupKey(), JSON.stringify({ base: S.draftBase, files: S.draft })); } catch (e) {}
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveNow, 1200);
}
export async function saveNow() {
  clearTimeout(saveTimer);
  setSaveState('saving');
  try {
    const r = await api({ a: 'save', base: S.draftBase, bundle: { format: 1, files: S.draft } });
    setSaveState(r.ok ? 'saved' : 'offline', r.error);
  } catch (e) { setSaveState('offline'); }
}
export function setSaveState(state, why) {
  S.saveState = state;
  const words = { edit: 'Draft · changed', saving: 'Draft · saving…', saved: 'Draft · saved', offline: 'Draft · on this phone only', live: 'Live' };
  setSaved(words[state] || state);
  const steps = document.querySelector('#saved .steps');
  if (steps) steps.dataset.period = state === 'saving' ? '0.4' : state === 'edit' ? '1' : '2';
  if (why && state === 'offline') console.warn('Studio: save failed:', why);
  emitEvent('saveState', state);
}
on('changed', (op) => { if (op) saveSoon(); });

// A kind with what it builds on ("is") filled in.
// Another place's files, read in when you go there (S6.4).
export async function ensurePlace(id) {
  const base = '/bar/assets/world/', pf = `places/${id}.json`;
  if (!S.draft[pf]) {
    const j = await (await fetch(base + pf)).json();
    S.draft[pf] = j; S.base[pf] = JSON.parse(JSON.stringify(j));
  }
  for (const k of S.draft[pf].kinds || []) if (!S.draft[k]) {
    const j = await (await fetch(base + k)).json();
    S.draft[k] = j; S.base[k] = JSON.parse(JSON.stringify(j));
  }
}
export function kindOf(id, depth = 0) {
  if (!S.draft) return undefined;
  let k;
  for (const f of S.draft[placeFile()].kinds || []) {
    const found = (S.draft[f] && S.draft[f].kinds || []).find((x) => x.id === id);
    if (found) { k = found; break; }
  }
  if (!k || !k.is || depth > 16) return k;
  const b = kindOf(k.is, depth + 1) || {};
  return { ...b, ...k, parts: { ...(b.parts || {}), ...(k.parts || {}) }, loops: { ...(b.loops || {}), ...(k.loops || {}) }, states: { ...(b.states || {}), ...(k.states || {}) } };
}

// A thing's own entry in the place file, if it has one you can edit
// ({json, path}) or why not ({why}).
export function thingEntry(id) {
  const place = S.draft && S.draft[placeFile()];
  if (!place) return { why: 'The world is still loading.' };
  const json = (place.things || []).find((t) => t.id === id);
  if (json) return { json, path: ['things', { id }] };
  if (id.includes('/')) return { why: `${id} comes with its kind (${id.split('/')[0]}): change the kind to change it.` };
  return { why: `${id} is one of a row: editing a row's copies comes later.` };
}

// The rest a thing has in the files: at, turn (Y, degrees), scale [x, y, z].
export function restOf(json) {
  const turn = typeof json.turn === 'number' ? json.turn : Array.isArray(json.turn) ? json.turn[1] : 0;
  const sc = json.scale === undefined ? [1, 1, 1] : typeof json.scale === 'number' ? [json.scale, json.scale, json.scale] : json.scale;
  return { at: json.at ? [...json.at] : [0, 0, 0], turn, scale: [...sc] };
}
export function poseEngine(id) {
  const t = S.byId.get(id), e = thingEntry(id);
  if (!t || !e.json) return;
  const r = restOf(e.json);
  send('st.pose', t.i, 0, [...r.at, r.turn, ...r.scale].map((v) => +(+v).toFixed(5)).join(','));
}

// Which things an op touched (for the engine and the Changes list).
function touched(op, out = new Set()) {
  if (op.op === 'group') op.ops.forEach((o) => touched(o, out));
  else if (op.file === placeFile() && op.path && op.path[0] === 'things' && op.path[1] && op.path[1].id) out.add(op.path[1].id);
  return out;
}
function sync(op) {
  for (const id of touched(op)) {
    // What it sits on first (the engine keeps it in place), then where.
    const t = S.byId.get(id), e = thingEntry(id);
    if (t && e.json) {
      const want = e.json.parent && S.byId.get(e.json.parent) ? S.byId.get(e.json.parent).i : -1;
      if (want !== t.parent) { send('st.attach', t.i, want); t.parent = want; }
    }
    poseEngine(id);
  }
}

// Changes the engine can't show by moving things (a thing added or taken
// away, a new kind, a row, its own parts): the world reloads from the draft.
function needsReload(op) {
  if (op.op === 'group') return op.ops.some(needsReload);
  if (op.op === 'add' || op.op === 'remove' || op.op === 'file') return true;
  if (op.file === 'materials.json') return true;  // boxes show the new paint after a reload
  const p = op.path || [];
  // (a box's shape is what you see: a box edit reloads too)
  return (op.file === placeFile() && p[0] === 'things' && ['kind', 'row', 'id'].includes(p[2])) || p.includes('boxes');
}
// A change to a light shows at once (S7.2): every lit thing gets its light again.
function touchesLight(op) {
  if (op.op === 'group') return op.ops.some(touchesLight);
  const p = op.path || [];
  return p.includes('light') || p[0] === 'mood' || p[0] === 'moods';
}
export function refreshLights() {
  for (const t of S.things) {
    const own = thingEntry(t.id).json;
    const k = kindOf(t.kind);
    const l = (own && own.parts && own.parts.light) || (k && k.parts && k.parts.light);
    if (!l || !Array.isArray(l.color)) continue;
    // The place's mood (S7.3) over it, by thing or by kind.
    const place = S.draft[placeFile()] || {};
    const mood = (((place.moods || {})[place.mood] || {}).lights) || {};
    const mo = mood[t.id] || mood[t.kind] || {};
    const color = mo.color || l.color, intensity = mo.intensity ?? l.intensity ?? 1;
    // Real units (L1.2): lumens and Kelvin; shape and size (L1.6).
    const lumens = mo.lumens ?? l.lumens ?? 0, kelvin = mo.kelvin ?? l.kelvin ?? 0;
    const shape = { tube: 1, panel: 2 }[l.shape] ?? 0;
    const v = (a, d) => (Array.isArray(a) && a.length === 3 ? a : d);
    const nums = [...color, intensity, l.range ?? 5, l.radius ?? 0.1, lumens, kelvin, shape, l.length ?? 1, l.width ?? 1,
                  ...v(l.axis, [1, 0, 0]), ...v(l.axis2, [0, 0, 1]), ...v(l.center, [0, 0, 0])];
    send('st.light', t.i, 0, nums.map((x) => +(+x).toFixed(4)).join(','));
  }
}
// A kind's loops or states changed (S8): the engine swaps them in place,
// for it and every kind built on it.
function lifeKinds(op, out = new Set()) {
  if (op.op === 'group') { op.ops.forEach((o) => lifeKinds(o, out)); return out; }
  const p = op.path || [];
  if (op.file && op.file.startsWith('kinds/') && p[0] === 'kinds' && p[1] && p[1].id && ['loops', 'states', 'on', 'start', 'pivot'].includes(p[2])) out.add(p[1].id);
  return out;
}
export function pushKinds(ids) {
  if (!ids.size) return;
  const all = (S.draft[placeFile()].kinds || []).flatMap((f) => (S.draft[f].kinds || []).map((k) => k.id));
  const builtOn = (id, root) => { for (let k = id, n = 0; k && n < 16; n++) { if (k === root) return true; const own = all.includes(k) && (S.draft[(S.draft[placeFile()].kinds || []).find((f) => S.draft[f].kinds.some((x) => x.id === k))].kinds.find((x) => x.id === k)); k = own && own.is; } return false; };
  for (const id of all) {
    if (![...ids].some((root) => builtOn(id, root))) continue;
    const k = JSON.parse(JSON.stringify(kindOf(id)));
    delete k.is;
    delete k.things;  // (the things inside a kind stay as built)
    send('st.kind', 0, 0, JSON.stringify(k));
  }
}
// The one way the Studio changes the world.
export function edit(op, opts = {}) {
  const inv = S.history.do(op);
  sync(op);
  if (touchesLight(op)) refreshLights();
  pushKinds(lifeKinds(op));
  emitEvent('changed', op);
  if (!opts.noReload && needsReload(op)) reloadWorld();
  return inv;
}
export function undo() { const e = S.history.undo(); if (e) { sync(e.inv); if (touchesLight(e.inv)) refreshLights(); pushKinds(lifeKinds(e.inv)); emitEvent('changed', e.inv); if (needsReload(e.inv)) reloadWorld(); } return e; }
export function redo() { const e = S.history.redo(); if (e) { sync(e.op); if (touchesLight(e.op)) refreshLights(); pushKinds(lifeKinds(e.op)); emitEvent('changed', e.op); if (needsReload(e.op)) reloadWorld(); } return e; }
// Live preview while a finger drags (no history): set, show, and say how to undo it.
export function preview(op) { const inv = apply(S.draft, op); sync(op); return inv; }
export { get, setOp, merge };

on('ready', () => { if (!S.draft) loadWorld().catch((e) => console.error('Studio: could not read the world', e)); });

// ---- Publishing (S3.5) ----
export async function publish(note) {
  await saveNow();
  let r = await api({ a: 'publish', base: S.draftBase, note, bundle: { format: 1, files: S.draft } });
  if (r.status === 409 && r.live) {
    // The world moved on (someone published, or Claude): put your changes on top of it.
    const newer = (await (await fetch('api.php?bundle=' + r.live.version)).json()).files;
    const merged = merge(S.base, S.draft, newer);
    for (const k of Object.keys(S.draft)) delete S.draft[k];
    Object.assign(S.draft, merged);
    S.base = clone(newer);
    S.draftBase = r.live.version;
    r = await api({ a: 'publish', base: S.draftBase, note, bundle: { format: 1, files: S.draft } });
    if (r.ok) Looped.toast(`The world had changed (v${r.live.version - 1}); your changes went on top of it`);
  }
  if (!r.ok) throw new Error(r.error || 'Publishing didn\'t work.');
  S.live = r.live;
  S.base = clone(S.draft);
  S.draftBase = r.live.version;
  S.history = new History(S.draft);
  try { localStorage.removeItem(backupKey()); } catch (e) {}
  setSaveState('live');
  emitEvent('published', r.live);
  emitEvent('changed', null);
  return r.live;
}
export async function restore(v) {
  const r = await api({ a: 'restore', v });
  if (!r.ok) throw new Error(r.error || 'That didn\'t work.');
  S.live = r.live;
  await api({ a: 'discard' });
  try { localStorage.removeItem(backupKey()); } catch (e) {}
  emitEvent('restored', r.live);
  return r.live;
}
// Start over: throw your draft away (the server's copy and this phone's) and
// open the live world again, as everyone sees it.
export async function discardDraft() {
  clearTimeout(saveTimer);
  const r = await api({ a: 'discard' });
  if (!r.ok) throw new Error(r.error || 'That didn\'t work.');
  try { for (let i = localStorage.length - 1; i >= 0; i--) { const k = localStorage.key(i); if (k && k.startsWith('studioDraft-')) localStorage.removeItem(k); } } catch (e) {}
  location.reload();
}
