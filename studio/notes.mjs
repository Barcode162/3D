// Notes for Claude (docs/archive/STUDIO.md S10.1): on any selection, ⋯ → Note for
// Claude. The note goes to Dev's notes (dev/api.php; Claude reads them at the
// start of every session: decision 0001) with the thing's id and kind, your
// camera view and a picture of it (kept by the hub, hub.php?shot=…).
// Until J5.4 it went to the retired hub board, which nothing read any more.
import { S, on, send } from './studio.js';
import { MENU_EXTRAS } from './build.mjs';

const HUB = '/hub/hub.php', DEV = '/dev/api.php';
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
export const NOTES = { last: null };
window.bar3dNotes = NOTES;

async function hub(body, to = HUB) {
  const r = await fetch(to, { method: 'POST', body: JSON.stringify({ ...body, token: S.token, project: 'bar3d' }), headers: { 'Content-Type': 'text/plain' } });
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
  const note = [
    `Studio: ${text}`,
    '',
    `Thing: ${t.id} (kind ${t.kind}) in ${S.place}`,
    v ? `Camera: ${v}  (open the Studio there: st.view ${v})` : '',
    picture ? `Picture: /hub/${picture}` : '',
    `From the Studio, draft based on v${S.draftBase || 0}.`,
  ].filter((x) => x !== '').join('\n');
  const r = await hub({ a: 'note', kind: 'note', text: note }, DEV);
  if (!r.ok) throw new Error(r.error || 'Dev didn\'t take it.');
  NOTES.last = { note: r.note && r.note.id, picture, thing: t.id };
  return r.note;
}

MENU_EXTRAS.push({
  label: 'Note for Claude',
  small: 'to Dev\'s notes, with a picture',
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
      try { await sendNote(t, text); Looped.toast('Sent: Claude reads it at the start of the next session'); } catch (e) { Looped.toast(e.message); }
    };
    const row = el('div', 'rowform');
    row.append(area, go);
    box.append(row);
    area.focus();
  },
});
