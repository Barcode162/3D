// Proposals (docs/STUDIO.md S10.4): a world someone else (Claude) suggests,
// waiting for you. Preview shows it in the Studio's frame (your draft is
// untouched); Apply puts its changes on top of your draft as one undoable
// edit; Discard throws it away. The same screen an AI inside the Studio
// would use later.
import { S, on, api, reloadWorld } from './studio.js';
import { edit } from './draft.mjs';
import { merge, file as fileOp } from './ops.mjs';
import { changes } from './diff.mjs';

const $ = (id) => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
export const PROP = { list: [], previewing: null };
window.bar3dProposals = PROP;

const key = (() => {
  const b = el('button', 'key small', 'Proposals');
  b.id = 'propKey'; b.type = 'button'; b.hidden = true;
  $('publish').before(b);
  return b;
})();
const sheet = (() => {
  const s = el('section', 'panel over');
  s.id = 'propSheet'; s.hidden = true; s.setAttribute('aria-label', 'Proposals');
  $('app').append(s);
  return s;
})();
const bar = (() => {
  const b = el('div', 'panel over');
  b.id = 'previewBar'; b.hidden = true;
  $('app').append(b);
  return b;
})();

export async function refresh() {
  const r = await api({ a: 'proposals' }).catch(() => ({}));
  PROP.list = (r.ok && r.proposals) || [];
  key.hidden = !PROP.list.length;
  key.textContent = `Proposals ${PROP.list.length}`;
  key.classList.toggle('lit', PROP.list.length > 0);
}
async function load(id) { const r = await api({ a: 'proposal', id }); if (!r.ok) throw new Error(r.error || 'It\'s gone.'); return r.proposal; }

async function preview(p) {
  const full = await load(p.id);
  PROP.previewing = full;
  S.previewFiles = { ...S.draft, ...full.files };
  sheet.hidden = true;
  bar.hidden = false;
  bar.textContent = '';
  const back = el('button', 'key small', 'Back to my draft'); back.type = 'button'; back.id = 'previewBack';
  back.onclick = () => endPreview();
  const apply = el('button', 'key go small', 'Apply'); apply.type = 'button';
  apply.onclick = () => applyIt(full);
  bar.append(el('span', '', `Previewing ${full.by}'s proposal`), back, apply);
  await reloadWorld();
}
async function endPreview() {
  S.previewFiles = null;
  PROP.previewing = null;
  bar.hidden = true;
  await reloadWorld();
}
async function applyIt(full) {
  // Its changes (from the world it was made on) go on top of your draft.
  const merged = merge(S.base, full.files, S.draft);
  const ops = Object.keys(merged).filter((f) => JSON.stringify(merged[f]) !== JSON.stringify(S.draft[f])).map((f) => fileOp(f, merged[f]));
  S.previewFiles = null;
  PROP.previewing = null;
  bar.hidden = true;
  if (ops.length) edit({ op: 'group', label: `${full.by}'s proposal: ${full.note || 'applied'}`, ops });
  await api({ a: 'drop', id: full.id });
  Looped.toast(`Applied ${full.by}'s proposal (${ops.length} file${ops.length === 1 ? '' : 's'}): Undo takes it back`);
  await refresh();
  if (!ops.length) await reloadWorld();
}
async function discard(p) {
  await api({ a: 'drop', id: p.id });
  Looped.toast('Discarded');
  await refresh();
  drawSheet();
}
async function drawSheet() {
  sheet.textContent = '';
  const head = el('div', 'row');
  const h = el('h2', '', 'Proposals'); h.style.flex = '1';
  const x = el('button', 'key small', '✕'); x.type = 'button'; x.setAttribute('aria-label', 'Close'); x.onclick = () => { sheet.hidden = true; };
  head.append(h, x);
  sheet.append(head);
  if (!PROP.list.length) sheet.append(el('p', 'dim', 'Nothing waiting.'));
  for (const p of PROP.list) {
    const card = el('div', 'hist');
    card.dataset.prop = p.id;
    card.append(el('span', '', `${p.role === 'claude' ? 'Claude: ' : ''}${p.note || 'a proposal'}`), el('small', '', `${p.by} · on v${p.base}`));
    const what = el('div', 'histwhat');
    load(p.id).then((full) => { const list = changes(S.base, { ...S.base, ...full.files }); what.textContent = ''; list.slice(0, 12).forEach((c) => what.append(el('div', 'dim', c.text))); if (!list.length) what.append(el('div', 'dim', 'No changes to the world.')); }).catch(() => {});
    const row = el('div', 'row');
    const pv = el('button', 'key small', 'Preview'); pv.type = 'button'; pv.dataset.act = 'preview'; pv.onclick = () => preview(p);
    const ap = el('button', 'key go small', 'Apply'); ap.type = 'button'; ap.dataset.act = 'apply'; ap.onclick = async () => applyIt(await load(p.id));
    const ds = el('button', 'key small', 'Discard'); ds.type = 'button'; ds.dataset.act = 'discard'; ds.onclick = () => discard(p);
    row.append(pv, ap, ds);
    sheet.append(card, what, row);
  }
  sheet.hidden = false;
}
key.addEventListener('click', drawSheet);
on('world', refresh);
