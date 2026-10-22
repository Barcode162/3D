// The hub's Dials (docs/STUDIO.md §6, S4.1): the world's numbers as dials.
// Reads the live world and your draft from the Studio's store
// (studio/api.php), draws every field the schema knows for kinds (their
// loops and lanes) and things (where, turn, size), and puts each change into
// the same draft the Studio edits. Publish is the Studio's publish.
import { History, set as setOp, merge } from '/studio/ops.mjs';
import { changes } from '/studio/diff.mjs';

const API = '/studio/api.php', WORLD = '/bar/assets/world/';
const $ = (id) => document.getElementById(id);
const clone = (v) => JSON.parse(JSON.stringify(v));
const token = (() => { try { return localStorage.getItem('hubToken') || localStorage.getItem('bar3dToken') || ''; } catch (e) { return ''; } })();
const D = { schema: null, live: null, base: null, draft: null, draftBase: 0, history: null, tab: 'kinds', open: new Set(), q: '' };
window.bar3dDials = D;

async function api(body) {
  const r = await fetch(API, { method: 'POST', body: JSON.stringify({ ...body, token }), headers: { 'Content-Type': 'text/plain' }, cache: 'no-store' });
  let j = {};
  try { j = await r.json(); } catch (e) {}
  return { status: r.status, ...j };
}
function el(tag, attrs, kids) {
  const e = document.createElement(tag);
  for (const k in attrs || {}) {
    const v = attrs[k];
    if (v == null || v === false) continue;
    if (k === 'text') e.textContent = v;
    else if (k.startsWith('on')) e[k] = v;
    else e.setAttribute(k, v === true ? '' : v);
  }
  for (const c of kids || []) if (c != null && c !== false) e.appendChild(typeof c === 'string' ? document.createTextNode(c) : c);
  return e;
}
const placeFile = 'places/bar.json';
const kindsFiles = () => D.draft[placeFile].kinds || [];
function kindFileOf(id) { return kindsFiles().find((f) => (D.draft[f].kinds || []).some((k) => k.id === id)); }

// ---- Loading: the live world, then your draft on top ----
async function load() {
  if (!token) { $('list').replaceChildren(el('p', { class: 'empty' }, ['Sign in on the ', el('a', { href: 'index.html', text: 'hub' }), ' first.'])); return; }
  const me = await api({ a: 'me' });
  if (!me.ok) { $('list').replaceChildren(el('p', { class: 'empty', text: me.error || 'The Dials are for the bar\'s owner and admins.' })); return; }
  D.live = me.live;
  D.schema = await (await fetch(WORLD + 'schema.json')).json();
  const files = {};
  if (D.live && D.live.version > 0) Object.assign(files, (await (await fetch(API + '?bundle=' + D.live.version)).json()).files);
  if (!files[placeFile]) files[placeFile] = await (await fetch(WORLD + placeFile)).json();
  for (const k of files[placeFile].kinds || []) if (!files[k]) files[k] = await (await fetch(WORLD + k)).json();
  D.base = files;
  D.draft = clone(files);
  D.draftBase = D.live.version || 0;
  const d = await api({ a: 'draft' });
  if (d.ok && d.draft && d.draft.files && d.draft.files[placeFile]) {
    for (const [k, v] of Object.entries(d.draft.files)) D.draft[k] = v;
    D.draftBase = d.draft.base || 0;
  }
  D.history = new History(D.draft);
  $('foot').hidden = false;
  setSaved(same(D.draft, D.base) ? 'live' : 'saved');
  draw();
}
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);

// ---- Saving and publishing (the Studio's own store and rules) ----
let saveTimer = 0;
function setSaved(state) {
  const words = { edit: 'Draft · changed', saving: 'Draft · saving…', saved: 'Draft · saved', offline: 'Draft · not saved', live: 'Live · v' + ((D.live && D.live.version) || 0) };
  $('savedText').textContent = words[state] || state;
  $('steps').dataset.period = state === 'saving' ? '0.4' : state === 'edit' ? '1' : '2';
  $('publish').disabled = !D.draft || same(D.draft, D.base);
  $('undo').disabled = !D.history || !D.history.canUndo;
}
function saveSoon() { setSaved('edit'); clearTimeout(saveTimer); saveTimer = setTimeout(saveNow, 900); }
async function saveNow() {
  clearTimeout(saveTimer);
  setSaved('saving');
  const r = await api({ a: 'save', base: D.draftBase, bundle: { format: 1, files: D.draft } }).catch(() => ({}));
  setSaved(r.ok ? 'saved' : 'offline');
}
function edit(file, path, value) {
  D.history.do(setOp(file, path, value));
  saveSoon();
}
$('undo').onclick = () => { if (D.history.undo()) { saveSoon(); draw(); } };
$('publish').onclick = () => {
  const list = changes(D.base, D.draft);
  $('sheetList').replaceChildren(...(list.length ? list : [{ text: 'Nothing changed.' }]).slice(0, 40).map((c) => el('li', { text: c.text })));
  $('sheet').hidden = false;
};
$('cancel').onclick = () => { $('sheet').hidden = true; };
// Undo publish (S9.3): the version before goes live again, as a new version.
$('undoPub').onclick = async () => {
  const b = $('undoPub');
  if (b.dataset.sure !== '1') { b.dataset.sure = '1'; b.textContent = 'Sure? Undo'; return; }
  const r = await api({ a: 'restore', v: +b.dataset.v });
  if (!r.ok) { Looped.toast(r.error || 'That didn\'t work'); return; }
  await api({ a: 'discard' });
  Looped.toast(`Undone: version ${b.dataset.v} is live again`);
  setTimeout(() => location.reload(), 900);
};
$('go').onclick = async () => {
  $('go').disabled = true;
  try {
    await saveNow();
    const note = $('note').value.trim();
    let r = await api({ a: 'publish', base: D.draftBase, note, bundle: { format: 1, files: D.draft } });
    if (r.status === 409 && r.live) {
      const newer = (await (await fetch(API + '?bundle=' + r.live.version)).json()).files;
      const merged = merge(D.base, D.draft, newer);
      for (const k of Object.keys(D.draft)) delete D.draft[k];
      Object.assign(D.draft, merged);
      D.draftBase = r.live.version;
      r = await api({ a: 'publish', base: D.draftBase, note, bundle: { format: 1, files: D.draft } });
    }
    if (!r.ok) throw new Error(r.error || 'Publishing didn\'t work.');
    D.live = r.live; D.base = clone(D.draft); D.draftBase = r.live.version; D.history = new History(D.draft);
    $('sheet').hidden = true; $('note').value = '';
    setSaved('live');
    Looped.toast(`Published: version ${r.live.version}. The bar picks it up within seconds.`);
    if (r.live.version > 1) { $('undoPub').hidden = false; $('undoPub').dataset.v = r.live.version - 1; }
    draw();
  } catch (e) { Looped.toast(e.message); }
  $('go').disabled = false;
};

// ---- Dials from the schema ----
const fmt = (v, step) => { const dec = step && step < 1 ? Math.min(3, String(step).split('.')[1].length) : 0; return (+v).toFixed(dec); };
// A number: label, the value to type, its unit, and a fader around it.
function numberDial(f, label, v, onSet, isChanged) {
  const step = f.step || (f.type === 'int' ? 1 : 0.01);
  const min = f.min ?? -10, max = f.max ?? 10;
  // The fader spans a useful range around the value (a 120 s range is no use for 4 s).
  const span = Math.max(Math.abs(v) * 2, step * 20);
  const lo = Math.max(min, v - span), hi = Math.min(max, v + span);
  const input = el('input', { class: 'field num', inputmode: 'decimal', enterkeyhint: 'done', value: fmt(v, step), 'aria-label': label });
  const fader = el('div', { class: 'field', 'data-fader': '', 'data-min': lo, 'data-max': hi, 'data-value': v, 'data-dec': String(fmt(0, step).split('.')[1] || '').length, 'data-unit': f.unit ? ' ' + f.unit : '', 'aria-label': label });
  const box = el('div', { class: 'dial' + (isChanged ? ' changed' : '') }, [el('label', { text: label }), input, el('span', { class: 'unit', text: f.unit || '' }), fader, f.about ? el('span', { class: 'note', text: f.about }) : null]);
  const put = (nv, from) => {
    nv = Math.max(min, Math.min(max, Math.round(nv / step) * step));
    nv = +nv.toFixed(4);
    if (from !== input) input.value = fmt(nv, step);
    if (from !== fader && fader.set) fader.set(nv);
    box.classList.add('changed');
    onSet(nv);
  };
  input.addEventListener('change', () => { const nv = parseFloat(input.value.replace(',', '.').replace('−', '-')); if (Number.isFinite(nv)) put(nv, input); else input.value = fmt(v, step); });
  input.addEventListener('keydown', (e) => { if (e.key === 'Enter') input.blur(); });
  let t = 0;
  fader.addEventListener('change', (e) => { if (e.target !== fader) return; const nv = e.detail; input.value = fmt(nv, step); clearTimeout(t); t = setTimeout(() => put(nv, fader), 120); });
  return box;
}
function flag(label, on, onSet) {
  const s = el('button', { class: 'switch', type: 'button', role: 'switch', 'aria-checked': String(!!on), 'aria-label': label }, [el('i')]);
  s.addEventListener('change', (e) => onSet(!!e.detail));
  return el('div', { class: 'flag' }, [el('span', { text: label }), s]);
}
function vecDial(label, v, onSet) {
  const inputs = [0, 1, 2].map((i) => el('input', { class: 'field num', inputmode: 'decimal', value: fmt(v[i], 0.001), 'aria-label': label + ' ' + 'XYZ'[i] }));
  inputs.forEach((inp, i) => inp.addEventListener('change', () => {
    const nv = parseFloat(inp.value.replace(',', '.').replace('−', '-'));
    if (!Number.isFinite(nv)) return;
    const out = [...v]; out[i] = +nv.toFixed(3); v = out; onSet(out);
  }));
  return el('div', { class: 'vec' }, [el('span', { text: label }), ...inputs]);
}
// The fields of one object (a loop, a lane, a thing) that the schema says are
// numbers, switches or points, with what's in the draft.
function fieldsOf(type, obj, file, path, baseObj) {
  const fields = D.schema[type].fields;
  const out = [];
  for (const [key, f] of Object.entries(fields)) {
    const v = obj[key];
    const label = f.label || key;
    const changed = baseObj && !same(v, baseObj[key]);
    if ((f.type === 'number' || f.type === 'int' || (f.type === 'range' && typeof v === 'number')) && typeof v === 'number') {
      out.push(numberDial(f, label, v, (nv) => edit(file, [...path, key], nv), changed));
    } else if (f.type === 'bool' && key !== 'beat') {
      out.push(flag(label, v, (on) => edit(file, [...path, key], on || undefined)));
    } else if (f.type === 'vec3' && Array.isArray(v)) {
      out.push(vecDial(label, v, (nv) => edit(file, [...path, key], nv)));
    }
  }
  return out;
}

// ---- The lists ----
function counts() {
  const n = {};
  for (const t of D.draft[placeFile].things || []) n[t.kind] = (n[t.kind] || 0) + (t.row ? t.row.count : 1);
  return n;
}
function kindRow(k, file, n) {
  const open = D.open.has('k:' + k.id);
  const head = el('button', { class: 'head', type: 'button', 'aria-expanded': String(open), onclick: () => { D.open[open ? 'delete' : 'add']('k:' + k.id); draw(); } },
    [el('span', { class: 'id', text: k.id }), el('small', { text: n ? `${n} in the bar` : (k.is ? 'builds on ' + k.is : 'kind') })]);
  const row = el('div', { class: 'row panel', 'data-kind': k.id }, [head]);
  if (!open) return row;
  const more = el('div', { class: 'more' });
  if (k.about) more.appendChild(el('p', { class: 'about', text: k.about }));
  const baseK = ((D.base[file] || {}).kinds || []).find((x) => x.id === k.id) || {};
  const loops = Object.entries(k.loops || {});
  if (!loops.length) more.appendChild(el('p', { class: 'about', text: k.still ? 'Stands still: ' + k.still : 'No loops of its own' + (k.is ? ` (it moves like ${k.is}).` : '.') }));
  for (const [name, loop] of loops) {
    const bl = (baseK.loops || {})[name] || {};
    more.appendChild(el('h3', { text: 'Loop: ' + name }));
    const path = ['kinds', { id: k.id }, 'loops', name];
    fieldsOf('loop', loop, file, path, bl).forEach((d) => more.appendChild(d));
    (loop.pieces || []).forEach((p, i) => {
      const lane = el('div', { class: 'lane' }, [el('b', { text: `${p.ch} · ${p.shape}` })]);
      fieldsOf('piece', p, file, [...path, 'pieces', i], (bl.pieces || [])[i]).forEach((d) => lane.appendChild(d));
      if (lane.children.length > 1) more.appendChild(lane);
    });
  }
  const one = (D.draft[placeFile].things || []).find((t) => t.kind === k.id);
  if (one) more.appendChild(el('a', { class: 'key small', href: '/studio/#thing=' + encodeURIComponent(one.id) }, ['Open ' + one.id + ' in the Studio']));
  row.appendChild(more);
  return row;
}
function thingRow(t) {
  const open = D.open.has('t:' + t.id);
  const head = el('button', { class: 'head', type: 'button', 'aria-expanded': String(open), onclick: () => { D.open[open ? 'delete' : 'add']('t:' + t.id); draw(); } },
    [el('span', { class: 'id', text: t.id }), el('small', { text: t.kind + (t.parent ? ' · on ' + t.parent : '') })]);
  const row = el('div', { class: 'row panel', 'data-thing': t.id }, [head]);
  if (!open) return row;
  const more = el('div', { class: 'more' });
  const baseT = (D.base[placeFile].things || []).find((x) => x.id === t.id) || {};
  const path = ['things', { id: t.id }];
  const f = D.schema.thing.fields;
  more.appendChild(vecDial(f.at.label, t.at || [0, 0, 0], (nv) => edit(placeFile, [...path, 'at'], nv)));
  const turn = typeof t.turn === 'number' ? t.turn : 0;
  more.appendChild(numberDial({ ...f.turn, step: 1 }, f.turn.label, turn, (nv) => edit(placeFile, [...path, 'turn'], nv || undefined), !same(t.turn, baseT.turn)));
  if (typeof t.scale !== 'object') more.appendChild(numberDial(f.scale, f.scale.label, t.scale ?? 1, (nv) => edit(placeFile, [...path, 'scale'], nv === 1 ? undefined : nv), !same(t.scale, baseT.scale)));
  more.appendChild(el('a', { class: 'key small', href: '/studio/#thing=' + encodeURIComponent(t.id) }, ['Open in the Studio']));
  row.appendChild(more);
  return row;
}
function draw() {
  if (!D.draft) return;
  const q = D.q.toLowerCase();
  const list = $('list');
  const rows = [];
  if (D.tab === 'kinds') {
    const n = counts();
    for (const file of kindsFiles()) for (const k of D.draft[file].kinds || []) {
      if (q && !(k.id.includes(q) || (k.about || '').toLowerCase().includes(q))) continue;
      rows.push([n[k.id] || 0, k.id, kindRow(k, file, n[k.id])]);
    }
    rows.sort((a, b) => b[0] - a[0] || a[1].localeCompare(b[1]));
  } else {
    for (const t of D.draft[placeFile].things || []) {
      if (q && !(t.id.includes(q) || t.kind.includes(q))) continue;
      rows.push([0, t.id, thingRow(t)]);
    }
  }
  list.replaceChildren(...(rows.length ? rows.map((r) => r[2]) : [el('p', { class: 'empty', text: 'Nothing matches.' })]));
  Looped.scan();
  $('undo').disabled = !D.history.canUndo;
  $('publish').disabled = same(D.draft, D.base);
}
$('tabs').addEventListener('click', (e) => {
  const b = e.target.closest('[data-tab]');
  if (!b) return;
  D.tab = b.dataset.tab;
  [...$('tabs').children].forEach((x) => x.classList.toggle('on', x === b));
  draw();
});
$('search').addEventListener('input', () => { D.q = $('search').value.trim(); draw(); });
// The hub links here as dials.html#kind=<id> or #thing=<id>: open that row.
function fromLink() {
  const m = location.hash.match(/(kind|thing)=([^&]+)/);
  if (!m) return;
  const id = decodeURIComponent(m[2]);
  D.tab = m[1] === 'kind' ? 'kinds' : 'things';
  [...$('tabs').children].forEach((x) => x.classList.toggle('on', x.dataset.tab === D.tab));
  D.open.add((m[1] === 'kind' ? 'k:' : 't:') + id);
  draw();
  const row = document.querySelector(`[data-${m[1]}="${CSS.escape(id)}"]`);
  if (row) row.scrollIntoView({ block: 'start' });
}
window.addEventListener('hashchange', fromLink);
load().then(fromLink).catch((e) => { $('list').replaceChildren(el('p', { class: 'empty', text: 'Couldn\'t read the world: ' + e.message })); });
