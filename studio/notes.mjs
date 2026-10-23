// Notes for Claude (docs/STUDIO.md S10.1): on any selection, ⋯ → Note for
// Claude. The note goes to the hub as a card (Claude's inbox at the next
// "go") with the thing's id and kind, your camera view and a picture of it.
import { S, on, send } from './studio.js';
import { MENU_EXTRAS } from './build.mjs';

const HUB = '/hub/hub.php';
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
export const NOTES = { last: null };
window.bar3dNotes = NOTES;

async function hub(body) {
  const r = await fetch(HUB, { method: 'POST', body: JSON.stringify({ ...body, token: S.token, project: 'bar3d' }), headers: { 'Content-Type': 'text/plain' } });
  let j = {};
  try { j = await r.json(); } catch (e) {}
  return j;
}
// The view and a picture, taken inside the engine's frame (like the Library's pictures).
function capture() {
  return new Promise((resolve) => {
    const out = { view: null, image: null };
    let n = 0;
    const done = () => { if (++n === 2) resolve(out); };
    const onView = (e) => { if (NOTES.wantView) { NOTES.wantView = false; out.view = e.view; done(); } };
    const onSnap = () => {
      if (!NOTES.wantSnap) return;
      NOTES.wantSnap = false;
      try {
        const src = S.engine.document.getElementById('canvas');
        const w = 480, h = Math.round((src.height / src.width) * w);
        const c = document.createElement('canvas');
        c.width = w; c.height = h;
        c.getContext('2d').drawImage(src, 0, 0, w, h);
        out.image = c.toDataURL('image/jpeg', 0.72);
      } catch (err) {}
      done();
    };
    NOTES.onView = onView; NOTES.onSnap = onSnap;
    NOTES.wantView = true; NOTES.wantSnap = true;
    send('st.getview');
    send('st.snap', 99);
    setTimeout(() => { if (n < 2) { NOTES.wantView = NOTES.wantSnap = false; resolve(out); } }, 8000);  // (a slow GPU can take seconds per frame)
  });
}
on('view', (e) => { if (NOTES.onView) NOTES.onView(e); });
on('snap', () => { if (NOTES.onSnap) NOTES.onSnap(); });

async function sendNote(t, text) {
  const cap = await capture();
  let picture = '';
  if (cap.image) {
    const shot = await hub({ a: 'shot', build: '', name: `note-${Date.now()}`, image: cap.image });
    if (shot.ok) picture = shot.url;
  }
  const v = cap.view ? cap.view.map((x) => +(+x).toFixed(2)).join(',') : '';
  const notes = [
    text,
    '',
    `Thing: ${t.id} (kind ${t.kind}) in ${S.place}`,
    v ? `Camera: ${v}  (open the Studio there: st.view ${v})` : '',
    picture ? `Picture: /hub/${picture}` : '',
    `From the Studio, draft based on v${S.draftBase || 0}.`,
  ].filter((x) => x !== '').join('\n');
  const r = await hub({ a: 'save', card: { section: 'studio', title: `Studio: ${t.id} — ${text.slice(0, 60)}`, stage: 'idea', notes } });
  if (!r.ok) throw new Error(r.error || 'The hub didn\'t take it.');
  NOTES.last = { card: r.card && r.card.id, picture, thing: t.id };
  return r.card;
}

MENU_EXTRAS.push({
  label: 'Note for Claude',
  small: 'to the hub, with a picture',
  run(box, t) {
    box.hidden = false;
    box.textContent = '';
    const area = el('textarea', 'field');
    area.id = 'noteText';
    area.rows = 3;
    area.placeholder = `What about ${t.id}? ("make it glow warmer", "this should be bigger")`;
    const go = el('button', 'key go small', 'Send to Claude');
    go.type = 'button'; go.id = 'noteGo';
    go.onclick = async () => {
      const text = area.value.trim();
      if (!text) { Looped.toast('Write the note first'); return; }
      box.hidden = true;
      try { await sendNote(t, text); Looped.toast('Sent: it\'s in the hub for Claude'); } catch (e) { Looped.toast(e.message); }
    };
    const row = el('div', 'rowform');
    row.append(area, go);
    box.append(row);
    area.focus();
  },
});
