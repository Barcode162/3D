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
  edit({ op: 'group', label: `added ${id}`, ops: [addOp(placeFile(), ['things'], { id, kind, at: e.at.map((v) => +(+v).toFixed(3)) })] }, { noReload: true });
  Looped.toast(`Added ${id}`);
  await reloadWorld(id);
});
$('addKey').addEventListener('click', openLibrary);
