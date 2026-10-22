// Building (docs/STUDIO.md 5.5, S6): the Library (every kind you can add,
// with a picture the engine takes of it, its catalog name and price); a
// thing you add lands on the surface in front of you, then the world
// reloads from the draft with it selected.
import { S, on, send, select, reloadWorld } from './studio.js';
import { edit, placeFile, kindOf } from './draft.mjs';
import { add as addOp } from './ops.mjs';

const $ = (id) => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
const store = { get: (k) => { try { return localStorage.getItem(k); } catch (e) { return null; } }, set: (k, v) => { try { localStorage.setItem(k, v); } catch (e) {} } };
const B = { pics: (() => { try { return JSON.parse(store.get('studioPics') || '{}'); } catch (e) { return {}; } })(), want: null };
window.bar3dBuild = B;

// A new id no thing has: "candle-7".
export function freshId(base) {
  const taken = new Set([...(S.draft[placeFile()].things || []).map((t) => t.id), ...S.things.map((t) => t.id)]);
  const stem = base.replace(/-\d+$/, '');
  for (let n = 1; ; n++) if (!taken.has(`${stem}-${n}`)) return `${stem}-${n}`;
}
// Every kind you can add: the catalog's (names, prices), then the rest that have a model.
function library() {
  const cat = (S.draft['catalog.json'] || {}).items || [];
  const out = cat.map((c) => ({ kind: c.kind, name: c.name, price: c.price, category: c.category }));
  const seen = new Set(out.map((x) => x.kind));
  for (const f of S.draft[placeFile()].kinds || []) for (const k of S.draft[f].kinds || []) {
    if (seen.has(k.id)) continue;
    const r = kindOf(k.id) || {};
    if (!r.parts || !r.parts.model || k.id === 'room' || r.is === 'room') continue;
    out.push({ kind: k.id, name: k.id.replace(/[_-]/g, ' '), category: 'other' });
  }
  return out;
}

// ---- Pictures: the engine frames one of each kind and the page keeps a small copy ----
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
async function takePictures(items, status) {
  const canvas = S.engine && S.engine.document.getElementById('canvas');
  if (!canvas) return;
  B.busy = true;
  B.cancel = false;
  const was = S.selected;
  B.wantView = true;
  send('st.getview');
  const view = await new Promise((r) => { B.viewDone = r; setTimeout(() => r(null), 1500); });
  const c = document.createElement('canvas');
  c.width = c.height = 96;
  const g = c.getContext('2d');
  let n = 0;
  for (const it of items) {
    if (B.cancel) break;
    if (B.pics[it.kind]) continue;
    const t = S.things.find((x) => x.kind === it.kind);
    if (!t) continue;
    status(`Taking pictures… ${++n}`);
    send('st.select', t.i, 1);
    await wait(220);
    // The copy is made while the engine's frame is still on the canvas.
    await new Promise((resolve) => {
      B.snap = () => {
        B.snap = null;
        try {
          const side = Math.min(canvas.width, canvas.height) * 0.6;
          g.fillStyle = '#0D0C0B'; g.fillRect(0, 0, 96, 96);
          g.drawImage(canvas, (canvas.width - side) / 2, (canvas.height - side) / 2, side, side, 0, 0, 96, 96);
          const px = g.getImageData(48, 48, 1, 1).data;
          if (px[0] + px[1] + px[2] > 6) B.pics[it.kind] = c.toDataURL('image/jpeg', 0.72);
        } catch (e) {}
        resolve();
      };
      send('st.snap', n);
      setTimeout(() => { B.snap = null; resolve(); }, 1500);
    });
  }
  store.set('studioPics', JSON.stringify(B.pics));
  select(was, false);
  if (view) send('st.view', 0, 0, view.join(','));
  await wait(120);
  B.busy = false;
  if (B.idle) { const f = B.idle; B.idle = null; f(); }
}
on('snap', () => { if (B.snap) B.snap(); });
on('view', (e) => { if (B.wantView) { B.wantView = false; if (B.viewDone) B.viewDone(e.view); } });

// ---- The Library sheet ----
async function openLibrary() {
  const sheet = $('library');
  if (!sheet.hidden) { sheet.hidden = true; return; }
  sheet.hidden = false;
  sheet.textContent = '';
  const head = el('div', 'row');
  const title = el('h2', '', 'Add');
  title.style.flex = '1';
  const close = el('button', 'key small');
  close.type = 'button';
  close.setAttribute('aria-label', 'Close the Library');
  close.innerHTML = '<svg class="ico"><use href="/ui/icons.svg#i-x"/></svg>';
  close.onclick = () => { sheet.hidden = true; };
  head.append(title, close);
  const note = el('p', 'dim', 'Tap one: it lands on the surface in the middle of your view.');
  const grid = el('div', 'libgrid');
  sheet.append(head, note, grid);
  const items = library();
  const draw = () => {
    grid.textContent = '';
    // Build a box (S6.3): a plain box on the surface in view, to push, pull and extrude.
    const bx = el('button', 'libitem');
    bx.type = 'button';
    bx.dataset.kind = 'box';
    const bp = el('div', 'pic');
    bp.innerHTML = '<svg class="ico" style="width:44px;height:44px"><use href="/ui/icons.svg#i-box"/></svg>';
    bx.append(bp, el('span', 'nm', 'A box'), el('small', '', 'build'));
    bx.onclick = () => { sheet.hidden = true; place('built'); };
    grid.append(bx);
    for (const it of items) {
      const b = el('button', 'libitem');
      b.type = 'button';
      b.dataset.kind = it.kind;
      const pic = el('div', 'pic');
      if (B.pics[it.kind]) pic.style.backgroundImage = `url(${B.pics[it.kind]})`;
      else pic.textContent = it.name.slice(0, 1).toUpperCase();
      b.append(pic, el('span', 'nm', it.name), el('small', '', it.price !== undefined ? `${it.price} credits` : it.kind));
      b.onclick = () => { sheet.hidden = true; place(it.kind); };
      grid.append(b);
    }
  };
  draw();
  if (items.some((it) => !B.pics[it.kind] && S.things.some((t) => t.kind === it.kind))) {
    await takePictures(items, (t) => { note.textContent = t; });
    note.textContent = 'Tap one: it lands on the surface in the middle of your view.';
    draw();
  }
}
// Add one: ask the engine where the middle of the view lands, put it there.
async function place(kind) {
  // Still taking pictures: stop, put the camera back, then aim.
  if (B.busy) { B.cancel = true; await new Promise((r) => { B.idle = r; }); }
  B.want = kind;
  send('st.aim', 0.5, 0.55);
}
on('aim', async (e) => {
  const kind = B.want;
  B.want = null;
  if (!kind) return;
  const id = freshId(kind);
  const at = e.at.map((v) => +(+v).toFixed(3));
  const ops = [];
  if (kind === 'built') {
    // Things built from boxes are of the kind "built" (made once, in the place's first kind file).
    const kf = (S.draft[placeFile()].kinds || [])[0];
    if (!(S.draft[placeFile()].kinds || []).some((f) => (S.draft[f].kinds || []).some((k) => k.id === 'built')))
      ops.push(addOp(kf, ['kinds'], { id: 'built', about: 'Built in the Studio from plain boxes: floors, steps, a stage.', still: 'built from boxes; it stands still' }));
    ops.push(addOp(placeFile(), ['things'], { id, kind, at, parts: { boxes: [{ at: [0, 0.05, 0], size: [1, 0.1, 1], material: 'oak-floor' }] } }));
  } else ops.push(addOp(placeFile(), ['things'], { id, kind, at }));
  edit({ op: 'group', label: `added ${id}`, ops }, { noReload: true });
  Looped.toast(`Added ${id}`);
  await reloadWorld(id);
});
$('addKey').addEventListener('click', openLibrary);

// ---- Duplicate, Row, Rename, Delete (S6.2): the ⋯ key on the breadcrumb ----
import { thingEntry } from './draft.mjs';
import { remove as removeOp, set as setOp2 } from './ops.mjs';
const r3 = (v) => +(+v).toFixed(3);
function current() {
  const t = S.things[S.selected];
  if (!t) return null;
  const e = thingEntry(t.id);
  if (!e.json) { Looped.toast(e.why); return null; }
  return { t, e };
}
function duplicate() {
  const c = current();
  if (!c) return;
  const id = freshId(c.t.id);
  const copy = JSON.parse(JSON.stringify(c.e.json));
  copy.id = id;
  copy.at = (copy.at || [0, 0, 0]).map((v, k) => r3(v + (k === 0 ? 0.3 : 0)));
  edit({ op: 'group', label: `${id}: a copy of ${c.t.id}`, ops: [addOp(placeFile(), ['things'], copy)] }, { noReload: true });
  Looped.toast(`Copied: ${id} (30 cm to the right)`);
  reloadWorld(id);
}
// A row: copies of it in a line, a step apart (one row entry in the files).
function makeRow(count, step) {
  const c = current();
  if (!c) return;
  const id = freshId(`${c.t.kind}-row`);
  const copy = JSON.parse(JSON.stringify(c.e.json));
  copy.id = id;
  copy.at = (copy.at || [0, 0, 0]).map((v, k) => r3(v + step[k]));
  copy.row = { count, step: step.map(r3) };
  edit({ op: 'group', label: `a row of ${count} ${c.t.kind} after ${c.t.id}`, ops: [addOp(placeFile(), ['things'], copy)] }, { noReload: true });
  Looped.toast(`A row of ${count} more, ${Math.round(Math.hypot(...step) * 100)} cm apart`);
  reloadWorld(c.t.id);
}
function rename(to) {
  const c = current();
  if (!c) return;
  to = String(to || '').trim().toLowerCase().replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '');
  if (!to || to === c.t.id) return;
  // Names players have seen stay (the room and saves use them): only new things get renamed.
  if ((S.base[placeFile()].things || []).some((x) => x.id === c.t.id)) { Looped.toast(`${c.t.id} is in the live world: its name stays (players' saves use it)`); return; }
  if ((S.draft[placeFile()].things || []).some((x) => x.id === to) || S.things.some((x) => x.id === to)) { Looped.toast(`${to} is taken`); return; }
  const ops = [setOp2(placeFile(), [...c.e.path, 'id'], to)];
  for (const x of S.draft[placeFile()].things || []) if (x.parent === c.t.id) ops.push(setOp2(placeFile(), ['things', { id: x.id }, 'parent'], to));
  edit({ op: 'group', label: `${c.t.id} renamed ${to}`, ops }, { noReload: true });
  reloadWorld(to);
}
function del() {
  const c = current();
  if (!c) return;
  const on = (S.draft[placeFile()].things || []).filter((x) => x.parent === c.t.id).map((x) => x.id);
  if (on.length) { Looped.toast(`${on.join(', ')} sit${on.length === 1 ? 's' : ''} on it: move ${on.length === 1 ? 'it' : 'them'} first`); return; }
  edit({ op: 'group', label: `removed ${c.t.id}`, ops: [removeOp(placeFile(), ['things'], c.t.id)] }, { noReload: true });
  Looped.toast(`Removed ${c.t.id} (Undo brings it back)`);
  reloadWorld(null);
}
function thingMenu() {
  const box = $('thingMenu');
  if (!box.hidden) { box.hidden = true; return; }
  const c = current();
  if (!c) return;
  box.textContent = '';
  const item = (label, small, f) => { const b = el('button'); b.type = 'button'; b.dataset.act = label; b.append(el('span', '', label), el('small', '', small)); b.onclick = () => { box.hidden = true; f(); }; box.append(b); };
  item('Duplicate', 'a copy, 30 cm along X', duplicate);
  item('Row', 'copies in a line', () => {
    box.hidden = false; box.textContent = '';
    const count = el('input', 'field num'); count.id = 'rowCount'; count.inputMode = 'numeric'; count.value = '3'; count.setAttribute('aria-label', 'How many more');
    const step = el('input', 'field num'); step.id = 'rowStep'; step.inputMode = 'decimal'; step.value = '0.7'; step.setAttribute('aria-label', 'How far apart (m)');
    const axis = el('span', 'seg field'); axis.id = 'rowAxis';
    ['X', 'Z'].forEach((a, k) => { const b = el('button', k ? '' : 'on', a); b.type = 'button'; b.dataset.axis = a; b.onclick = () => [...axis.children].forEach((x) => x.classList.toggle('on', x === b)); axis.append(b); });
    const go = el('button', 'key go small', 'Make the row'); go.type = 'button'; go.id = 'rowGo';
    go.onclick = () => {
      box.hidden = true;
      const n = Math.max(1, Math.min(50, parseInt(count.value, 10) || 1)), d = parseFloat(String(step.value).replace(',', '.')) || 0.7;
      const ax = axis.querySelector('.on').dataset.axis;
      makeRow(n, ax === 'X' ? [d, 0, 0] : [0, 0, d]);
    };
    const row = el('div', 'rowform');
    row.append(el('span', 'dim', 'More'), count, el('span', 'dim', 'm apart'), step, axis, go);
    box.append(row);
  });
  item('Rename', c.t.id, () => {
    box.hidden = false; box.textContent = '';
    const name = el('input', 'field'); name.id = 'renameField'; name.value = c.t.id; name.setAttribute('aria-label', 'New name');
    const go = el('button', 'key go small', 'Rename'); go.type = 'button'; go.id = 'renameGo';
    go.onclick = () => { box.hidden = true; rename(name.value); };
    const row = el('div', 'rowform'); row.append(name, go); box.append(row);
  });
  item('Delete', 'Undo brings it back', del);
  box.hidden = false;
}
$('moreKey').addEventListener('click', thingMenu);
on('select', () => { $('thingMenu').hidden = true; });

// ---- Places (S6.4): the place key at the top: go to another place, make a
// new one (a floor and a light to start from), copy this one, and set where
// people appear. Each place is a file (places/<id>.json); the bar's menu lists
// them once published. ----
import { ensurePlace } from './draft.mjs';
import { file as fileOp, set as setOp3 } from './ops.mjs';
function placesInDraft() {
  const ids = new Set(['bar', 'home']);
  for (const f of Object.keys(S.draft || {})) { const m = /^places\/([a-z0-9-]+)\.json$/.exec(f); if (m) ids.add(m[1]); }
  return [...ids];
}
async function goPlace(id) {
  await ensurePlace(id);
  S.wantPlace = id;
  $('placeMenu').hidden = true;
  await reloadWorld(null);
}
function newPlace(id, from) {
  id = String(id || '').trim().toLowerCase().replace(/[^a-z0-9-]+/g, '-').replace(/^-+|-+$/g, '').slice(0, 32);
  if (!id) return;
  if (placesInDraft().includes(id)) { Looped.toast(`There's already a place called ${id}`); return; }
  let place;
  if (from) {
    place = JSON.parse(JSON.stringify(S.draft[placeFile()]));
    place.id = id;
    place.about = `A copy of ${S.place}.`;
  } else {
    place = {
      id, about: 'Made in the Studio.', kinds: ['kinds/bar.json'],
      air: { ambient: [0.16, 0.13, 0.11], fog: [0.02, 0.015, 0.015], fogDensity: 0.006 },
      spawn: { at: [0, 1.65, 4], yaw: 0 },
      lightGrid: { min: [-8, 0, -8], max: [8, 0, 8] },
      rooms: [{ id, min: [-8, -0.2, -8], max: [8, 6, 8] }],
      doorways: [], areas: { arcade: { min: [0, 0, 0], max: [0, 0, 0] } }, points: { 'arcade-door': [0, 0, 0] },
      things: [
        { id: 'floor', kind: 'built', at: [0, 0, 0], parts: { boxes: [{ at: [0, -0.05, 0], size: [12, 0.1, 12], material: 'oak-floor' }] } },
        { id: 'light-1', kind: 'light-bulb', at: [0, 3.2, 0], parts: { light: { color: [1, 0.78, 0.55], intensity: 3, range: 10, radius: 0.1 } } },
      ],
    };
  }
  const ops = [fileOp(`places/${id}.json`, place)];
  const kf = 'kinds/bar.json';
  if (!from && !(S.draft[kf].kinds || []).some((k) => k.id === 'built'))
    ops.push(addOp(kf, ['kinds'], { id: 'built', about: 'Built in the Studio from plain boxes: floors, steps, a stage.', still: 'built from boxes; it stands still' }));
  edit({ op: 'group', label: from ? `new place ${id}, a copy of ${S.place}` : `new place ${id}`, ops }, { noReload: true });
  Looped.toast(`New place: ${id}`);
  goPlace(id);
}
// People appear where the middle of your view lands, facing the way you look.
function spawnHere() {
  B.wantSpawn = true;
  send('st.aim', 0.5, 0.5);
  $('placeMenu').hidden = true;
}
on('aim', (e) => {
  if (!B.wantSpawn) return;
  B.wantSpawn = false;
  B.wantView = true;
  B.viewDone = (view) => {
    const yaw = view ? Math.round(((view[3] * 180 / Math.PI) + 540) % 360 - 180) : 0;
    const at = [+e.at[0].toFixed(2), +(e.at[1] + 1.65).toFixed(2), +e.at[2].toFixed(2)];
    edit({ op: 'group', label: `${S.place}: people appear at ${at.join(', ')}`, ops: [setOp3(placeFile(), ['spawn'], { at, yaw })] });
    Looped.toast('People will appear here (Publish to use it)');
  };
  send('st.getview');
});
function placeMenu() {
  const box = $('placeMenu');
  if (!box.hidden) { box.hidden = true; return; }
  box.textContent = '';
  for (const id of placesInDraft()) {
    const b = el('button'); b.type = 'button'; b.dataset.place = id;
    b.append(el('span', '', id === 'bar' ? 'Bar' : id === 'home' ? 'Test home' : id), el('small', '', id === S.place ? 'here' : 'go'));
    b.onclick = () => { if (id !== S.place) goPlace(id); else box.hidden = true; };
    box.append(b);
  }
  const form = (label, go, from) => {
    const b = el('button'); b.type = 'button'; b.dataset.act = label;
    b.append(el('span', '', label), el('small', '', '+'));
    b.onclick = () => {
      box.textContent = '';
      const name = el('input', 'field'); name.id = 'placeName2'; name.placeholder = 'rooftop'; name.setAttribute('aria-label', 'Its name');
      const ok = el('button', 'key go small', 'Make it'); ok.type = 'button'; ok.id = 'placeGo';
      ok.onclick = () => { box.hidden = true; newPlace(name.value, from); };
      const row = el('div', 'rowform'); row.append(name, ok); box.append(row);
      name.focus();
    };
    box.append(b);
  };
  form('New place', null, false);
  form('Copy this place', null, true);
  const sp = el('button'); sp.type = 'button'; sp.dataset.act = 'spawn';
  sp.append(el('span', '', 'People appear here'), el('small', '', 'the middle of your view'));
  sp.onclick = spawnHere;
  box.append(sp);
  box.hidden = false;
}
$('placeKey').addEventListener('click', placeMenu);
