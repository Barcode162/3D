// Life (docs/STUDIO.md 5.7, S8): the selected thing's kind as states and
// rules, with a small diagram and Test buttons (S8.1), and the Loop Rack
// (S8.2-S8.3). Changes go to the kind (every thing of it) and show at once:
// the engine swaps the kind's loops and states in place (st.kind).
import { S, on, send } from './studio.js';
import { edit, kindOf, placeFile, setOp } from './draft.mjs';

const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
const svgEl = (tag, attrs) => { const e = document.createElementNS('http://www.w3.org/2000/svg', tag); for (const [k, v] of Object.entries(attrs || {})) e.setAttribute(k, v); return e; };
export const LIFE = { state: null, kind: null, lastTest: '' };
window.bar3dLife = LIFE;

export function kindFileOf(id) {
  return (S.draft[placeFile()].kinds || []).find((f) => (S.draft[f].kinds || []).some((k) => k.id === id));
}
const own = (id) => { const f = kindFileOf(id); return f && S.draft[f].kinds.find((k) => k.id === id); };
// Ops that make the kind hold its own copy of a field it inherits, then the change.
export function kindOps(id, field, path, value) {
  const f = kindFileOf(id), k = own(id), ops = [];
  if (!f) return ops;
  if (!k[field] && field !== 'start') ops.push(setOp(f, ['kinds', { id }, field], JSON.parse(JSON.stringify((kindOf(id) || {})[field] || {}))));
  ops.push(setOp(f, ['kinds', { id }, field, ...path], value));
  return ops;
}

// ---- The diagram: states round a circle, rules as arrows ----
function diagram(k, current) {
  const names = Object.keys(k.states || {});
  const W = 320, H = 170, cx = W / 2, cy = H / 2, R = Math.min(62, 18 + names.length * 12);
  const pos = Object.fromEntries(names.map((n, i) => { const a = (i / Math.max(1, names.length)) * Math.PI * 2 - Math.PI / 2; return [n, [cx + Math.cos(a) * R * 1.6, cy + Math.sin(a) * R]]; }));
  const svg = svgEl('svg', { viewBox: `0 0 ${W} ${H}`, class: 'lifemap', role: 'img', 'aria-label': 'States and rules' });
  const defs = svgEl('defs');
  const mk = svgEl('marker', { id: 'arr', viewBox: '0 0 10 10', refX: 9, refY: 5, markerWidth: 6, markerHeight: 6, orient: 'auto-start-reverse' });
  mk.append(svgEl('path', { d: 'M0 0L10 5L0 10z', fill: 'currentColor' }));
  defs.append(mk); svg.append(defs);
  const arrow = (a, b, label, dashed) => {
    if (!pos[a] || !pos[b] || a === b) return;
    const [x1, y1] = pos[a], [x2, y2] = pos[b], dx = x2 - x1, dy = y2 - y1, d = Math.hypot(dx, dy) || 1;
    const sx = x1 + (dx / d) * 22, sy = y1 + (dy / d) * 14, ex = x2 - (dx / d) * 24, ey = y2 - (dy / d) * 16;
    const mx = (sx + ex) / 2 - (dy / d) * 14, my = (sy + ey) / 2 + (dx / d) * 14;
    svg.append(svgEl('path', { d: `M${sx} ${sy} Q${mx} ${my} ${ex} ${ey}`, class: 'arrow' + (dashed ? ' timed' : ''), 'marker-end': 'url(#arr)' }));
    const t = svgEl('text', { x: mx, y: my, class: 'rule' }); t.textContent = label; svg.append(t);
  };
  for (const [n, st] of Object.entries(k.states || {})) {
    for (const [ev, to] of Object.entries({ ...(k.on || {}), ...(st.on || {}) })) arrow(n, to, ev === 'tap' ? 'touch' : ev);
    if (st.next) arrow(n, st.next, st.afterLoops ? `${st.afterLoops} loop` : `${st.after}s`, true);
  }
  for (const n of names) {
    const [x, y] = pos[n];
    const g = svgEl('g', { class: 'node' + (n === current ? ' on' : '') + (n === k.start ? ' start' : '') });
    g.append(svgEl('rect', { x: x - 30, y: y - 12, width: 60, height: 24, rx: 7 }));
    const t = svgEl('text', { x, y: y + 4 }); t.textContent = n; g.append(t);
    svg.append(g);
  }
  return svg;
}

// ---- The Life tab ----
export function drawLife(body, redraw) {
  const t = S.things[S.selected];
  if (!t) { body.append(el('p', 'dim', 'Select something to see what it does.')); return; }
  const k = kindOf(t.kind);
  if (!k || !kindFileOf(t.kind)) { body.append(el('p', 'dim', `${t.id}'s kind can't be changed here.`)); return; }
  if (LIFE.kind !== t.kind) { LIFE.kind = t.kind; LIFE.state = k.start || Object.keys(k.states || {})[0] || null; }
  const states = Object.keys(k.states || {});
  if (!states.includes(LIFE.state)) LIFE.state = states[0] || null;
  body.append(el('span', 'label', `What every ${t.kind} does`));
  if (k.still) body.append(el('p', 'dim', 'Stands still: ' + k.still));
  body.append(diagram(k, LIFE.state));
  // States
  const chips = el('div', 'chiprow');
  for (const n of states) {
    const b = el('button', 'chip' + (n === LIFE.state ? ' on' : ''), n + (n === k.start ? ' ●' : ''));
    b.type = 'button'; b.dataset.state = n;
    b.onclick = () => { LIFE.state = n; redraw(); };
    chips.append(b);
  }
  const name = el('input', 'field'); name.id = 'newState'; name.placeholder = 'wobble'; name.setAttribute('aria-label', 'A new state');
  const add = el('button', 'key small', '+ State'); add.type = 'button'; add.id = 'newStateGo';
  add.onclick = () => {
    const n = name.value.trim().toLowerCase().replace(/[^a-z0-9_-]+/g, '-');
    if (!n || states.includes(n)) return;
    // It starts as a copy of the selected state's loop, and goes back after one loop.
    const from = k.states[LIFE.state] || {};
    const loopName = n;
    const ops = [...kindOps(t.kind, 'loops', [loopName], JSON.parse(JSON.stringify((k.loops || {})[from.loop] || { period: 2, pieces: [{ ch: 'turn.z', shape: 'wave', amp: 4 }] })))];
    ops.push(...kindOps(t.kind, 'states', [n], { loop: loopName, afterLoops: 1, next: k.start || LIFE.state }));
    edit({ op: 'group', label: `every ${t.kind}: a new state ${n}`, ops });
    LIFE.state = n;
    redraw();
  };
  const row = el('div', 'scope'); row.append(name, add);
  body.append(el('span', 'label', 'States'), chips, row);
  const st = k.states[LIFE.state];
  if (!st) return;
  // The selected state: its loop, its rules, when it moves on.
  body.append(el('span', 'label', `In ${LIFE.state}`));
  const loops = el('div', 'chiprow');
  for (const ln of Object.keys(k.loops || {})) {
    const b = el('button', 'chip' + (ln === st.loop ? ' on' : ''), ln); b.type = 'button';
    b.onclick = () => edit({ op: 'group', label: `every ${t.kind}: ${LIFE.state} plays ${ln}`, ops: kindOps(t.kind, 'states', [LIFE.state, 'loop'], ln) });
    loops.append(b);
  }
  body.append(el('span', 'dim', 'Plays'), loops);
  const rules = el('div', 'rules');
  for (const [ev, to] of Object.entries(st.on || {})) {
    const r = el('div', 'rulerow');
    const x = el('button', 'key small', '✕'); x.type = 'button'; x.setAttribute('aria-label', `Remove: ${ev} → ${to}`);
    x.onclick = () => edit({ op: 'group', label: `every ${t.kind}: no more ${ev} → ${to}`, ops: kindOps(t.kind, 'states', [LIFE.state, 'on', ev], undefined) });
    r.append(el('span', '', `When ${ev === 'tap' ? 'touched' : `“${ev}”`} → ${to}`), x);
    rules.append(r);
  }
  // A new rule: an event, then the state it leads to.
  const evIn = el('input', 'field'); evIn.id = 'ruleEvent'; evIn.value = 'tap'; evIn.setAttribute('aria-label', 'When (tap, music, hit...)');
  const to = el('div', 'chiprow');
  for (const n of states) {
    const b = el('button', 'chip', '→ ' + n); b.type = 'button'; b.dataset.to = n;
    b.onclick = () => { const ev = evIn.value.trim() || 'tap'; edit({ op: 'group', label: `every ${t.kind}: ${LIFE.state}, ${ev} → ${n}`, ops: kindOps(t.kind, 'states', [LIFE.state, 'on', ev], n) }); };
    to.append(b);
  }
  body.append(el('span', 'dim', 'Rules'), rules, el('div', 'scope', ''), evIn, to);
  if (st.next) body.append(el('p', 'dim', `After ${st.afterLoops ? `${st.afterLoops} loop${st.afterLoops === 1 ? '' : 's'}` : `${st.after} s`} → ${st.next}`));
  if (LIFE.state !== k.start) {
    const del = el('button', 'key small', `Remove ${LIFE.state}`); del.type = 'button';
    del.onclick = () => {
      const ops = kindOps(t.kind, 'states', [LIFE.state], undefined);
      for (const [n, s2] of Object.entries(k.states)) {
        for (const [ev, dst] of Object.entries(s2.on || {})) if (dst === LIFE.state) ops.push(...kindOps(t.kind, 'states', [n, 'on', ev], undefined));
        if (s2.next === LIFE.state && n !== LIFE.state) ops.push(...kindOps(t.kind, 'states', [n, 'next'], k.start), ...kindOps(t.kind, 'states', [n, 'afterLoops'], s2.afterLoops || 1));
      }
      edit({ op: 'group', label: `every ${t.kind}: no more ${LIFE.state}`, ops });
      LIFE.state = k.start;
      redraw();
    };
    body.append(del);
  }
  // Test: send an event to this one and see where it goes.
  const evs = [...new Set(['tap', ...Object.values(k.states).flatMap((s2) => Object.keys(s2.on || {})), ...Object.keys(k.on || {})])];
  const test = el('div', 'chiprow');
  for (const ev of evs) {
    const b = el('button', 'key small', `Test: ${ev === 'tap' ? 'touch' : ev}`); b.type = 'button'; b.dataset.test = ev;
    b.onclick = () => send('st.event', t.i, 0, ev);
    test.append(b);
  }
  body.append(el('span', 'label', 'Test'), test, el('p', 'dim lifetest', LIFE.lastTest));
}
on('state', (e) => {
  const t = S.things[e.thing];
  LIFE.lastTest = `${t ? t.id : 'it'} is now ${e.state}${e.reacted ? '' : ' (nothing reacts to that)'}`;
  const p = document.querySelector('#monitor .lifetest');
  if (p) p.textContent = LIFE.lastTest;
});
