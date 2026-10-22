// Sounds (docs/STUDIO.md 5.7, S8.4), in the Life tab: the sound a kind
// makes by itself (which effect, how often, how loud, how far it carries)
// and the sound when you touch it. ▶ plays it at the thing (st.sound). The
// bar's voices are made when the place loads, so a change reloads the world.
import { S, on, send, reloadWorld } from './studio.js';
import { edit } from './draft.mjs';
import { control } from './fields.mjs';
import { kindOps, LIFE } from './life.mjs';

const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
export const SOUND = { heard: '' };
window.bar3dSound = SOUND;

function hear(t, effect, gain) { send('st.sound', t.i, 0, `${effect},${gain ?? 0.6}`); }
on('heard', (e) => { SOUND.heard = e.effect; });

export function drawSound(body, t, k, redraw) {
  const sch = S.schema.parts;
  const change = (name, path, v, label) => {
    edit({ op: 'group', label: `every ${t.kind}: ${label}`, ops: kindOps(t.kind, 'parts', [name, ...path], v) });
    reloadWorld();
  };
  for (const [name, title, fresh] of [
    ['sound', 'Its own sound', { effect: 'buzz', every: [3, 6], gain: 0.3, near: 4 }],
    ['touchSound', 'When touched', { effect: 'knock', gain: 0.5 }],
  ]) {
    const p = (k.parts || {})[name];
    body.append(el('span', 'label', title));
    if (!p) {
      const add = el('button', 'key small', name === 'sound' ? '+ A sound' : '+ A touch sound'); add.type = 'button'; add.dataset.addSound = name;
      add.onclick = () => change(name, [], fresh, name === 'sound' ? `a ${fresh.effect} sound` : 'a sound when touched');
      body.append(add);
      continue;
    }
    const f = sch[name].fields;
    body.append(control(S.schema, f.effect, f.effect.label, p.effect, (v) => { change(name, ['effect'], v, `sound ${v}`); hear(t, v, p.gain); }));
    if (name === 'sound') {
      // How often: a number, or between two numbers (seconds).
      const ev = Array.isArray(p.every) ? p.every : [p.every ?? 3, p.every ?? 3];
      const row = el('div', 'ctl vec');
      row.append(el('span', 'name', 'How often (s, from–to)'));
      ev.forEach((v, i) => {
        const inp = el('input', 'field num'); inp.inputMode = 'decimal'; inp.value = v; inp.setAttribute('aria-label', i ? 'At most every' : 'At least every');
        inp.addEventListener('change', () => { const n = parseFloat(inp.value); if (!Number.isFinite(n)) return; const out = [...ev]; out[i] = Math.max(0.05, n); change(name, ['every'], out, 'how often'); });
        row.append(inp);
      });
      body.append(row);
      body.append(control(S.schema, f.near, f.near.label, p.near ?? 6, (v) => change(name, ['near'], v, 'heard from')));
    }
    body.append(control(S.schema, f.gain, f.gain.label, p.gain ?? 0.5, (v) => change(name, ['gain'], v, 'loudness')));
    const play = el('button', 'key small', '▶ Hear it'); play.type = 'button'; play.dataset.hear = name;
    play.onclick = () => hear(t, p.effect, p.gain);
    const rm = el('button', 'key small', 'Remove'); rm.type = 'button';
    rm.onclick = () => change(name, [], undefined, `no ${name === 'sound' ? 'sound' : 'touch sound'}`);
    const r = el('div', 'scope'); r.append(play, rm);
    body.append(r);
  }
}
LIFE.sound = drawSound;
