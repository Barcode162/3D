// The Monitor (docs/STUDIO.md 5.3, 5.4): a drawer over the bottom of the
// world. World: every thing by what it sits on, searchable. Inspect: the
// selected thing in words, from the schema (assets/world/schema.json).
// Problems: what the schema and the engine's own checks find.
import { S, on, send, select } from './studio.js';
import { problems as schemaProblems } from './schema.mjs';

const $ = (id) => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
const M = { tab: 'world', query: '', files: null, schema: null, kinds: new Map(), placeJson: null, engineProblems: null };

// ---- The world's files (the version the engine runs) ----
async function loadFiles() {
  const files = {};
  const base = '/bar/assets/world/';
  M.schema = await (await fetch(base + 'schema.json')).json();
  if (S.live && S.live.version > 0) {
    const b = await (await fetch('api.php?bundle=' + S.live.version)).json();
    Object.assign(files, b.files);
  }
  const placePath = `places/${S.place}.json`;
  if (!files[placePath]) files[placePath] = await (await fetch(base + placePath)).json();
  for (const k of files[placePath].kinds || []) if (!files[k]) files[k] = await (await fetch(base + k)).json();
  M.files = files;
  M.placeJson = files[placePath];
  M.kinds = new Map();
  for (const k of files[placePath].kinds || []) for (const kind of (files[k].kinds || [])) M.kinds.set(kind.id, kind);
}
// A kind with what it builds on ("is") filled in.
function kindOf(id, depth = 0) {
  const k = M.kinds.get(id);
  if (!k || !k.is || depth > 16) return k;
  const b = kindOf(k.is, depth + 1) || {};
  return { ...b, ...k, parts: { ...(b.parts || {}), ...(k.parts || {}) }, loops: { ...(b.loops || {}), ...(k.loops || {}) }, states: { ...(b.states || {}), ...(k.states || {}) } };
}
function thingJson(id) {
  const direct = (M.placeJson.things || []).find((t) => t.id === id);
  if (direct) return direct;
  const slash = id.lastIndexOf('/');  // a thing inside a kind: "<parent>/<child>"
  if (slash > 0) {
    const parent = S.byId.get(id.slice(0, slash));
    const kind = parent && kindOf(parent.kind);
    return kind && (kind.things || []).find((t) => t.id === id.slice(slash + 1));
  }
  // a row's copy ("stool-3" from "stool" × n)
  const m = /^(.*)-(\d+)$/.exec(id);
  return m && (M.placeJson.things || []).find((t) => t.id === m[1] && t.row);
}

// ---- Drawing ----
function fmt(v) {
  if (typeof v === 'number') return Math.abs(v) >= 100 ? v.toFixed(0) : String(+v.toFixed(3));
  if (Array.isArray(v)) return v.length <= 4 && v.every((x) => typeof x === 'number') ? v.map(fmt).join('  ') : `${v.length} items`;
  if (v && typeof v === 'object') return Object.keys(v).length + ' entries';
  if (typeof v === 'boolean') return v ? 'yes' : 'no';
  return String(v);
}
function fieldRow(label, value, unit, about) {
  const r = el('div', 'fieldrow');
  r.append(el('span', 'label', label), el('span', 'v', fmt(value) + (unit && typeof value === 'number' ? ' ' + unit : '')));
  if (about) r.append(el('span', 'about', about));
  return r;
}
const LAYERS = [[1, 'Tap areas'], [2, 'Shapes'], [4, 'Seats'], [8, 'Lights'], [16, 'Spawn']];
function drawLayers(body) {
  const row = el('div', 'row xray');
  row.append(el('span', 'label', 'X-ray'));
  for (const [bit, name] of LAYERS) {
    const k = el('button', 'key small' + (S.layers & bit ? ' lit' : ''));
    k.dataset.layer = bit;
    k.append(el('span', 'kled'), name);
    k.onclick = () => { S.layers ^= bit; k.classList.toggle('lit', !!(S.layers & bit)); send('st.layers', S.layers); };
    row.append(k);
  }
  body.append(row);
}
function drawWorld(body) {
  drawLayers(body);
  const search = el('input', 'field');
  search.placeholder = `Search ${S.things.length} things`;
  search.setAttribute('aria-label', 'Search things');
  search.value = M.query;
  search.oninput = () => { M.query = search.value; drawList(); };
  const list = el('div', 'tree');
  list.id = 'tree';
  body.append(search, list);
  function drawList() {
    list.textContent = '';
    const q = M.query.trim().toLowerCase();
    const depth = (t) => { let d = 0; for (let p = S.things[t.parent]; p && d < 8; p = S.things[p.parent]) d++; return d; };
    // Parents first, each followed by what sits on it.
    const kids = new Map();
    S.things.forEach((t) => { const k = kids.get(t.parent) || []; k.push(t); kids.set(t.parent, k); });
    const order = [];
    const walk = (p) => (kids.get(p) || []).forEach((t) => { order.push(t); walk(t.i); });
    walk(-1);
    let shown = 0;
    for (const t of order) {
      if (q && !(t.id.toLowerCase().includes(q) || t.kind.toLowerCase().includes(q))) continue;
      if (++shown > 400) break;
      const b = el('button', t.i === S.selected ? 'sel' : '');
      b.style.paddingLeft = 10 + (q ? 0 : depth(t) * 14) + 'px';
      b.append(el('span', 'id', t.id), el('small', '', t.kind));
      b.onclick = () => { select(t.i, true); drawList(); };
      list.append(b);
    }
    if (!shown) list.append(el('p', 'dim', 'Nothing by that name.'));
  }
  drawList();
}
function drawInspect(body) {
  const t = S.things[S.selected];
  if (!t) { body.append(el('p', 'dim', 'Select something: tap it in the world, or pick it in World.')); return; }
  const tj = thingJson(t.id) || {};
  const kind = kindOf(t.kind) || {};
  const sch = M.schema;
  body.append(el('h3', '', t.id));
  if (kind.about) body.append(el('p', 'dim', kind.about));
  body.append(el('span', 'label', 'This one'));
  body.append(fieldRow('Kind', t.kind), fieldRow('Where (in the world)', t.at, 'm'));
  for (const [k, v] of Object.entries(tj)) {
    if (k === 'id' || k === 'kind' || k === 'parts' || k === 'loops') continue;
    const f = sch.thing.fields[k] || {};
    body.append(fieldRow(f.label || k, v, f.unit, f.about));
  }
  const parts = { ...(kind.parts || {}), ...(tj.parts || {}) };
  for (const [name, v] of Object.entries(parts)) {
    const p = sch.parts[name] || {};
    body.append(el('span', 'label', (p.label || name) + (tj.parts && tj.parts[name] ? ' · this one' : ' · every ' + t.kind)));
    if (p.about) body.append(el('p', 'dim', p.about));
    const items = Array.isArray(v) && p.list ? v : [v];
    items.slice(0, 6).forEach((item) => {
      if (item && typeof item === 'object' && !Array.isArray(item) && p.fields) {
        for (const [k, x] of Object.entries(item)) { const f = p.fields[k] || {}; body.append(fieldRow(f.label || k, x, f.unit)); }
      } else body.append(fieldRow(p.label || name, item, p.unit));
    });
    if (items.length > 6) body.append(el('p', 'dim', `…and ${items.length - 6} more`));
  }
  const states = Object.entries(kind.states || {});
  if (states.length) {
    body.append(el('span', 'label', 'States'));
    for (const [name, st] of states) {
      const rules = Object.entries(st.on || {}).map(([e, to]) => `${e === 'tap' ? 'a touch' : e} → ${to}`);
      if (st.afterLoops) rules.push(`after ${st.afterLoops} loop(s) → ${st.next}`);
      if (st.after) rules.push(`after ${st.after} s → ${st.next}`);
      body.append(fieldRow(name, rules.join(' · ') || 'stays', '', st.loop ? 'plays ' + st.loop : ''));
    }
  }
  const loops = Object.entries(kind.loops || {});
  if (loops.length) {
    body.append(el('span', 'label', 'Loops'));
    for (const [name, l] of loops) body.append(fieldRow(name, `${l.period}${l.beat ? ' beats' : ' s'}`, '', (l.pieces || []).map((p) => `${p.ch} ${p.shape}`).join(' · ')));
  }
}
function drawProblems(body) {
  const list = [];
  for (const [path, json] of Object.entries(M.files || {})) {
    const what = path.startsWith('places/') ? 'place' : path.startsWith('kinds/') ? 'kinds' : path === 'catalog.json' ? 'catalog' : '';
    if (what) for (const p of schemaProblems(M.schema, what, json)) list.push(`${path}: ${p}`);
  }
  for (const p of M.engineProblems || []) list.push(p);
  if (M.engineProblems === null) body.append(el('p', 'dim', 'Asking the engine…'));
  if (!list.length && M.engineProblems !== null) body.append(el('div', 'ok', 'All clear: every field is known and every thing is alive.'));
  for (const p of list.slice(0, 100)) body.append(el('div', 'problem', p));
}

function draw() {
  const box = $('monitor');
  if (box.hidden) return;
  box.textContent = '';
  const grip = el('button', 'grip');
  grip.setAttribute('aria-label', 'Make the Monitor bigger or smaller');
  grip.onclick = () => box.classList.toggle('full');
  const tabs = el('div', 'seg field tabs');
  for (const [id, label] of [['world', 'World'], ['inspect', 'Inspect'], ['problems', 'Problems']]) {
    const b = el('button', id === M.tab ? 'on' : '', label);
    b.dataset.tab = id;
    b.onclick = () => { M.tab = id; if (id === 'problems') { M.engineProblems = null; send('st.check'); } draw(); };
    tabs.append(b);
  }
  const close = el('button', 'key small');
  close.innerHTML = '<svg class="ico"><use href="/ui/icons.svg#i-x"/></svg>';
  close.setAttribute('aria-label', 'Close the Monitor');
  close.onclick = () => { box.hidden = true; };
  const head = el('div', 'row');
  tabs.style.flex = '1';
  head.append(tabs, close);
  const body = el('div', 'body');
  box.append(grip, head, body);
  if (!M.files) { body.append(el('p', 'dim', 'Reading the world…')); return; }
  if (M.tab === 'world') drawWorld(body);
  else if (M.tab === 'inspect') drawInspect(body);
  else drawProblems(body);
}

on('ready', async () => { try { await loadFiles(); } catch (e) { console.error(e); } draw(); });
on('select', () => { if (M.tab !== 'world') draw(); else { const sel = document.querySelector('#tree .sel'); if (sel) sel.classList.remove('sel'); } });
on('check', (e) => { M.engineProblems = e.problems || []; if (M.tab === 'problems') draw(); });
$('monitorKey').addEventListener('click', () => {
  const box = $('monitor');
  box.hidden = !box.hidden;
  if (!box.hidden && S.selected >= 0 && M.tab === 'world') M.tab = 'inspect';
  draw();
});
window.bar3dMonitor = M;
