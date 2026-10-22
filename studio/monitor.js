// The Monitor (docs/STUDIO.md 5.3, 5.4): a drawer over the bottom of the
// world. World: every thing by what it sits on, searchable. Inspect: the
// selected thing in words, from the schema (assets/world/schema.json).
// Problems: what the schema and the engine's own checks find.
import { S, on, send, select, api } from './studio.js';
import { problems as schemaProblems } from './schema.mjs';
import { kindOf, thingEntry, placeFile, restore, edit, setOp } from './draft.mjs';
import { add as addOp } from './ops.mjs';
import { control } from './fields.mjs';
import { partSource } from './parts.mjs';
import { changes } from './diff.mjs';

const $ = (id) => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
const M = { tab: 'world', query: '', engineProblems: null };

// ---- The world's files: the draft (draft.mjs) ----
function thingJson(id) {
  const e = thingEntry(id);
  if (e.json) return e.json;
  const slash = id.lastIndexOf('/');  // a thing inside a kind: "<parent>/<child>"
  if (slash > 0) {
    const parent = S.byId.get(id.slice(0, slash));
    const kind = parent && kindOf(parent.kind);
    return kind && (kind.things || []).find((t) => t.id === id.slice(slash + 1));
  }
  const m = /^(.*)-(\d+)$/.exec(id);  // a row's copy ("stool-3" from "stool" × n)
  return m && (S.draft[placeFile()].things || []).find((t) => t.id === m[1] && t.row);
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
  const te = thingEntry(t.id);
  const kind = kindOf(t.kind) || {};
  const sch = S.schema;
  body.append(el('h3', '', t.id));
  if (kind.about) body.append(el('p', 'dim', kind.about));
  body.append(el('span', 'label', 'This one'));
  body.append(fieldRow('Kind', t.kind));
  const states = Object.keys(kind.states || {});
  if (te.json) {
    // Every field of this one, editable (S5.4); where/turn/size always.
    const shown = { at: [0, 0, 0], turn: 0, scale: 1, ...te.json };
    for (const [k, v] of Object.entries(shown)) {
      if (k === 'id' || k === 'kind' || k === 'parts' || k === 'loops' || k === 'parent') continue;
      const f = sch.thing.fields[k] || {};
      const c = control(sch, f, f.label || k, v, (nv) => edit({ op: 'group', ops: [setOp(placeFile(), [...te.path, k], nv)], label: `${t.id}: ${f.label || k}` }), { states });
      body.append(c || fieldRow(f.label || k, v, f.unit, f.about));
    }
  } else {
    body.append(fieldRow('Where (in the world)', t.at, 'm'));
    if (te.why) body.append(el('p', 'dim', te.why));
  }
  const own = te.json ? [...Object.keys(te.json.parts || {}).map((n) => ['parts', n]), ...Object.keys(te.json.loops || {}).map((n) => ['loops', n])] : [];
  if (own.length) {
    // What this one has of its own (S5.3): a dot each, Reset to kind, or make it a kind.
    const row = el('div', 'scope');
    const nk = el('button', 'key small', 'New kind from this');
    nk.type = 'button';
    nk.onclick = () => newKindFrom(t, te);
    row.append(el('span', 'dim', `Its own: ${own.map(([, n]) => n).join(', ')}`), nk);
    body.append(row);
  }
  const parts = { ...(kind.parts || {}), ...(tj.parts || {}) };
  for (const [name, v] of Object.entries(parts)) {
    const p = sch.parts[name] || {};
    const mine = !!(tj.parts && tj.parts[name] !== undefined);
    const head = el('span', 'label', (p.label || name) + (mine ? ' · this one' : ' · every ' + t.kind));
    if (mine) {
      head.append(el('span', 'dot'));
      if (te.json && kind.parts && kind.parts[name] !== undefined) {
        const r = el('button', 'key small', 'Reset to kind');
        r.type = 'button';
        r.style.marginLeft = '8px';
        r.onclick = () => edit({ op: 'group', ops: [setOp(placeFile(), [...te.path, 'parts', name], undefined)], label: `${t.id}: ${name} back to the kind's` });
        head.append(r);
      }
    }
    body.append(head);
    if (p.about) body.append(el('p', 'dim', p.about));
    const items = Array.isArray(v) && p.list ? v : [v];
    const src = partSource(t, name);
    items.slice(0, 6).forEach((item, i) => {
      if (item && typeof item === 'object' && !Array.isArray(item) && p.fields) {
        if (items.length > 1) body.append(el('span', 'dim', `${p.label || name} ${i + 1}${item.name ? ' · ' + item.name : ''}`));
        for (const [k, x] of Object.entries(item)) {
          const f = p.fields[k] || {};
          const path = src ? [...src.path, ...(p.list ? [i] : []), k] : null;
          const c = path && control(sch, f, f.label || k, x, (nv) => edit({ op: 'group', ops: [setOp(src.file, path, nv)], label: `${src.owner}: ${p.label || name} · ${f.label || k}` }), { states });
          if (c && mine) c.classList.add('own');
          body.append(c || fieldRow(f.label || k, x, f.unit));
        }
      } else body.append(fieldRow(p.label || name, item, p.unit));
    });
    if (items.length > 6) body.append(el('p', 'dim', `…and ${items.length - 6} more`));
  }
  const stateList = Object.entries(kind.states || {});
  if (stateList.length) {
    body.append(el('span', 'label', 'States'));
    for (const [name, st] of stateList) {
      const rules = Object.entries(st.on || {}).map(([e, to]) => `${e === 'tap' ? 'a touch' : e} → ${to}`);
      if (st.afterLoops) rules.push(`after ${st.afterLoops} loop(s) → ${st.next}`);
      if (st.after) rules.push(`after ${st.after} s → ${st.next}`);
      body.append(fieldRow(name, rules.join(' · ') || 'stays', '', st.loop ? 'plays ' + st.loop : ''));
    }
  }
  const loops = Object.entries(kind.loops || {});
  if (loops.length) {
    body.append(el('span', 'label', 'Loops'));
    for (const [name, l] of loops) {
      const src = loopSource(t, name);
      body.append(el('span', 'dim', `${name}: ${(l.pieces || []).map((p) => `${p.ch} ${p.shape}`).join(' · ')}${src ? ' · every ' + src.owner : ''}`));
      for (const k of ['period', 'blend', 'restart', 'desync']) {
        if (l[k] === undefined && k !== 'period') continue;
        const f = sch.loop.fields[k] || {};
        const c = src && control(sch, f, f.label || k, l[k], (nv) => edit({ op: 'group', ops: [setOp(src.file, [...src.path, k], nv)], label: `every ${src.owner}: ${name} loop ${f.label || k}` }));
        body.append(c || fieldRow(f.label || k, l[k], f.unit));
      }
    }
  }
}
// A new kind from a thing (S5.3): it builds on the old kind with this one's
// own parts and loops; the thing becomes one of it.
// Where a loop is written: the thing's own, else the kind (or the one it builds on).
function loopSource(t, name) {
  const te = thingEntry(t.id);
  if (te.json && te.json.loops && te.json.loops[name]) return { file: placeFile(), path: [...te.path, 'loops', name], owner: t.id };
  for (let id = t.kind, n = 0; id && n < 16; n++) {
    const f = (S.draft[placeFile()].kinds || []).find((x) => (S.draft[x].kinds || []).some((k) => k.id === id));
    const k = f && S.draft[f].kinds.find((x) => x.id === id);
    if (!k) return null;
    if (k.loops && k.loops[name]) return { file: f, path: ['kinds', { id }, 'loops', name], owner: id };
    id = k.is;
  }
  return null;
}
function newKindFrom(t, te) {
  const kindFile = (S.draft[placeFile()].kinds || []).find((f) => (S.draft[f].kinds || []).some((k) => k.id === t.kind)) || (S.draft[placeFile()].kinds || [])[0];
  const taken = new Set((S.draft[placeFile()].kinds || []).flatMap((f) => (S.draft[f].kinds || []).map((k) => k.id)));
  let id = `${t.kind}_2`;
  for (let n = 2; taken.has(id); n++) id = `${t.kind}_${n}`;
  const k = { id, is: t.kind };
  if (te.json.parts) k.parts = JSON.parse(JSON.stringify(te.json.parts));
  if (te.json.loops) k.loops = JSON.parse(JSON.stringify(te.json.loops));
  edit({ op: 'group', label: `new kind ${id} from ${t.id}`, ops: [
    addOp(kindFile, ['kinds'], k),
    setOp(placeFile(), [...te.path, 'kind'], id),
    setOp(placeFile(), [...te.path, 'parts'], undefined),
    setOp(placeFile(), [...te.path, 'loops'], undefined)] });
  t.kind = id;
  Looped.toast(`${t.id} is now a ${id} (builds on ${k.is})`);
  draw();
}
function drawProblems(body) {
  const list = [];
  for (const [path, json] of Object.entries(S.draft || {})) {
    const what = path.startsWith('places/') ? 'place' : path.startsWith('kinds/') ? 'kinds' : path === 'catalog.json' ? 'catalog' : '';
    if (what) for (const p of schemaProblems(S.schema, what, json)) list.push(`${path}: ${p}`);
  }
  for (const p of M.engineProblems || []) list.push(p);
  if (M.engineProblems === null) body.append(el('p', 'dim', 'Asking the engine…'));
  if (!list.length && M.engineProblems !== null) body.append(el('div', 'ok', 'All clear: every field is known and every thing is alive.'));
  for (const p of list.slice(0, 100)) body.append(el('div', 'problem', p));
}

function drawChanges(body) {
  const list = changes(S.base, S.draft);
  if (!list.length) { body.append(el('p', 'dim', 'No changes yet: this is the live world.')); return; }
  body.append(el('span', 'label', `${list.length} change${list.length === 1 ? '' : 's'} in your draft`));
  for (const c of list) {
    const b = el('button', 'change');
    b.textContent = c.text;
    const t = c.thing && S.byId.get(c.thing);
    if (t) b.onclick = () => select(t.i, true);
    body.append(b);
  }
}
async function drawHistory(body) {
  const r = await api({ a: 'history' }).catch(() => ({}));
  if (!r.ok) { body.append(el('p', 'dim', 'History needs the server (it isn\'t here).')); return; }
  if (!r.versions.length) { body.append(el('p', 'dim', 'Nothing published yet: the world is the one built into the bar.')); return; }
  for (const v of r.versions) {
    const row = el('div', 'hist');
    const when = new Date(v.at * 1000).toLocaleString([], { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' });
    row.append(el('span', '', `v${v.version} · ${v.note || 'no note'}`), el('small', '', `${v.author} · ${when}`));
    if (v.version === r.live.version) row.append(el('span', 'chip on', 'Live'));
    else {
      const b = el('button', 'key small', 'Restore');
      b.onclick = async () => {
        if (b.dataset.sure !== '1') { b.dataset.sure = '1'; b.textContent = 'Sure?'; return; }
        b.disabled = true;
        try { const live = await restore(v.version); Looped.toast(`Version ${v.version} is live again (as v${live.version})`); setTimeout(() => location.reload(), 900); }
        catch (e) { Looped.toast(e.message); b.disabled = false; }
      };
      row.append(b);
    }
    body.append(row);
  }
}
function draw() {
  const box = $('monitor');
  if (box.hidden) return;
  const old = box.querySelector('.body'), scroll = old ? old.scrollTop : 0, sameTab = M.drawnTab === M.tab;
  M.drawnTab = M.tab;
  box.textContent = '';
  const grip = el('button', 'grip');
  grip.setAttribute('aria-label', 'Make the Monitor bigger or smaller');
  grip.onclick = () => box.classList.toggle('full');
  const tabs = el('div', 'seg field tabs');
  for (const [id, label] of [['world', 'World'], ['inspect', 'Inspect'], ['changes', 'Changes'], ['problems', 'Problems'], ['history', 'History']]) {
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
  if (!S.draft) { body.append(el('p', 'dim', 'Reading the world…')); return; }
  if (M.tab === 'world') drawWorld(body);
  else if (M.tab === 'inspect') drawInspect(body);
  else if (M.tab === 'changes') drawChanges(body);
  else if (M.tab === 'history') drawHistory(body);
  else drawProblems(body);
  Looped.scan();  // the faders in Inspect
  if (sameTab) body.scrollTop = scroll;  // an edit redraws; you stay where you were
}

on('world', draw);
on('changed', () => { if (M.tab !== 'world') draw(); });
on('select', () => { if (M.tab !== 'world') draw(); else { const sel = document.querySelector('#tree .sel'); if (sel) sel.classList.remove('sel'); } });
on('check', (e) => { M.engineProblems = e.problems || []; if (M.tab === 'problems') draw(); });
$('monitorKey').addEventListener('click', () => {
  const box = $('monitor');
  box.hidden = !box.hidden;
  if (!box.hidden && S.selected >= 0 && M.tab === 'world') M.tab = 'inspect';
  draw();
});
window.bar3dMonitor = M;

// ---- Publish (S3.5): only with changes and no problems ----
function draftProblems() {
  const list = [];
  for (const [path, json] of Object.entries(S.draft || {})) {
    const what = path.startsWith('places/') ? 'place' : path.startsWith('kinds/') ? 'kinds' : path === 'catalog.json' ? 'catalog' : '';
    if (what) for (const p of schemaProblems(S.schema, what, json)) list.push(`${path}: ${p}`);
  }
  return list;
}
function updatePublishKey() {
  const n = S.draft ? changes(S.base, S.draft).length : 0;
  const k = document.getElementById('publish');
  k.disabled = n === 0;
  k.textContent = n ? `Publish ${n}` : 'Publish';
}
on('changed', updatePublishKey);
on('world', updatePublishKey);
document.getElementById('publish').addEventListener('click', () => {
  const sheet = document.getElementById('publishSheet');
  const list = changes(S.base, S.draft);
  const probs = draftProblems();
  document.getElementById('pubSummary').textContent = `${list.length} change${list.length === 1 ? '' : 's'} become version ${(S.live.version || 0) + 1}, live in the bar the next time it opens.`;
  const pp = document.getElementById('pubProblems');
  pp.textContent = '';
  probs.slice(0, 6).forEach((p) => pp.append(el('div', 'problem', p)));
  document.getElementById('pubGo').disabled = probs.length > 0;
  sheet.hidden = false;
});
document.getElementById('pubCancel').addEventListener('click', () => { document.getElementById('publishSheet').hidden = true; });
document.getElementById('pubGo').addEventListener('click', async () => {
  const go = document.getElementById('pubGo');
  go.disabled = true;
  try {
    const { publish } = await import('./draft.mjs');
    const live = await publish(document.getElementById('pubNote').value.trim());
    document.getElementById('publishSheet').hidden = true;
    document.getElementById('pubNote').value = '';
    Looped.toast(`Published: version ${live.version} is live`);
  } catch (e) { Looped.toast(e.message); }
  go.disabled = false;
});