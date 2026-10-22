// Move / Turn / Size (docs/STUDIO.md 5.4): the tools under the selection.
// The arrows on the thing (the engine draws them, bar/studio_mode.cpp) are
// for quick drags; the strip is for exact values: axis keys, world or local,
// the number itself (with a ± key: the iPhone number pad has none), and the
// fader (drag; slide your finger up off it for ten times finer). Snapping on
// by default (5 cm, 15°, 5 %). One gesture = one undo.
import { S, on, send } from './studio.js';
import { edit, preview, thingEntry, restOf, placeFile, setOp } from './draft.mjs';

const $ = (id) => document.getElementById(id);
const T = { tool: 0, axis: 0, local: false, snap: true, gesture: null };
const UNITS = ['m', '°', '×'];
const SNAP = [0.05, 15, 0.05];
const PER_PX = [0.004, 0.6, 0.004];  // fader: how much a pixel moves it
const NAMES = ['X', 'Y', 'Z', 'All'];

function entry() {
  const t = S.things[S.selected];
  return t ? { t, ...thingEntry(t.id) } : null;
}
// The axis a move goes along: world X/Y/Z, or turned with the thing (local).
function moveDir(rest, axis) {
  if (!T.local || axis === 1) return axis === 0 ? [1, 0, 0] : axis === 1 ? [0, 1, 0] : [0, 0, 1];
  const a = rest.turn * Math.PI / 180, c = Math.cos(a), s = Math.sin(a);
  return axis === 0 ? [c, 0, -s] : [s, 0, c];
}
// The number shown for the current tool and axis.
function value(rest) {
  if (T.tool === 0) return T.local && T.axis !== 1 ? (T.gesture ? T.gesture.along : 0) : rest.at[T.axis];
  if (T.tool === 1) return rest.turn;
  return T.axis === 3 ? rest.scale[0] : rest.scale[T.axis];
}
const round = (v) => +(+v).toFixed(T.tool === 1 ? 1 : 3);

// The rest after changing the value (or moving by `by` along the axis).
function restWith(rest0, v, by) {
  const r = { at: [...rest0.at], turn: rest0.turn, scale: [...rest0.scale] };
  if (T.tool === 0) {
    const d = moveDir(rest0, T.axis);
    const amount = by !== undefined ? by : v - (T.local && T.axis !== 1 ? 0 : rest0.at[T.axis]);
    for (let k = 0; k < 3; k++) r.at[k] = round(rest0.at[k] + d[k] * amount);
  } else if (T.tool === 1) r.turn = round(((by !== undefined ? rest0.turn + by : v) + 540) % 360 - 180);
  else {
    const nv = Math.max(0.05, by !== undefined ? (T.axis === 3 ? rest0.scale[0] : rest0.scale[T.axis]) + by : v);
    if (T.axis === 3) r.scale = [round(nv), round(nv), round(nv)]; else r.scale[T.axis] = round(nv);
  }
  return r;
}
// The ops that put a rest into the files (only the fields that changed).
function opsFor(e, r0, r) {
  const ops = [];
  if (JSON.stringify(r.at) !== JSON.stringify(r0.at)) ops.push(setOp(placeFile(), [...e.path, 'at'], r.at));
  if (r.turn !== r0.turn) ops.push(setOp(placeFile(), [...e.path, 'turn'], r.turn || undefined));
  if (JSON.stringify(r.scale) !== JSON.stringify(r0.scale)) {
    const uni = r.scale[0] === r.scale[1] && r.scale[1] === r.scale[2];
    ops.push(setOp(placeFile(), [...e.path, 'scale'], uni ? (r.scale[0] === 1 ? undefined : r.scale[0]) : r.scale));
  }
  return ops;
}
function commit(r) {
  const e = entry();
  if (!e || !e.json) return;
  const r0 = restOf(e.json);
  const ops = opsFor(e, r0, r);
  if (ops.length) edit({ op: 'group', ops, label: ['Move', 'Turn', 'Size'][T.tool] + ' ' + e.t.id });
}

// ---- A gesture (the fader or an arrow): live, then one undo step ----
function begin() {
  const e = entry();
  if (!e || !e.json) { if (e && e.why) Looped.toast(e.why); return false; }
  T.gesture = { e, rest0: restOf(e.json), total: 0, along: 0, undo: null };
  return true;
}
function move(by) {
  const g = T.gesture;
  if (!g) return;
  g.total += by;
  const step = SNAP[T.tool];
  const amount = T.snap ? Math.round(g.total / step) * step : g.total;
  if (g.amount === amount) return;
  if (T.snap && g.amount !== undefined) Looped.click(false);
  g.amount = amount;
  g.along = amount;
  if (g.undo) preview(g.undo);  // back to the start, then the new place
  const r = restWith(g.rest0, 0, amount);
  const ops = opsFor(g.e, g.rest0, r);
  g.undo = ops.length ? preview({ op: 'group', ops }) : null;
  g.r = r;
  draw();
}
function end() {
  const g = T.gesture;
  T.gesture = null;
  if (!g) return;
  if (g.undo) preview(g.undo);  // the draft back as it was: the edit goes in as one undo step
  if (g.r) commit(g.r);
  draw();
}

// ---- Drawing ----
function draw() {
  const box = $('tools');
  const e = entry();
  const show = !!e;
  box.hidden = !show;
  document.body.classList.toggle('tooling', show);
  send('st.gizmo', show && e.json ? 1 : 0, T.tool);
  if (!show) return;
  document.body.style.setProperty('--tools-h', box.offsetHeight + 'px');
  [...$('toolKeys').children].forEach((k) => k.classList.toggle('lit', +k.dataset.tool === T.tool));
  if (T.tool === 1) T.axis = 1;
  if (T.tool !== 2 && T.axis === 3) T.axis = 0;
  [...$('axisKeys').querySelectorAll('[data-axis]')].forEach((k) => {
    const a = +k.dataset.axis;
    k.classList.toggle('lit', a === T.axis);
    k.disabled = T.tool === 1 && a !== 1;
  });
  $('allKey').hidden = T.tool !== 2;
  $('spaceSeg').hidden = T.tool !== 0;
  [...$('spaceSeg').children].forEach((b) => b.classList.toggle('on', (b.dataset.space === 'local') === T.local));
  $('snapKey').classList.toggle('lit', T.snap);
  $('snapText').textContent = T.snap ? ['5 cm', '15°', '5 %'][T.tool] : 'Free';
  $('axisName').textContent = NAMES[T.axis];
  $('axisName').style.color = T.axis < 3 ? `var(--ax-${'xyz'[T.axis]})` : 'var(--accent)';
  $('unitText').textContent = UNITS[T.tool];
  const field = $('valField');
  if (!e.json) { field.value = ''; field.placeholder = 'Can\'t edit this one yet'; field.disabled = true; return; }
  field.disabled = false;
  if (document.activeElement !== field) field.value = round(value(restOf(e.json)));
}

// ---- Wiring ----
$('toolKeys').addEventListener('click', (ev) => { const k = ev.target.closest('[data-tool]'); if (!k) return;
  // Each tool keeps its own axis (Turn is always about Y).
  T.axisOf = T.axisOf || {};
  if (T.tool !== 1) T.axisOf[T.tool] = T.axis;
  T.tool = +k.dataset.tool;
  if (T.tool !== 1) T.axis = T.axisOf[T.tool] || 0;
  draw();
});
$('axisKeys').addEventListener('click', (ev) => {
  const k = ev.target.closest('[data-axis]');
  if (k && !k.disabled) { T.axis = +k.dataset.axis; draw(); }
  const sp = ev.target.closest('[data-space]');
  if (sp) { T.local = sp.dataset.space === 'local'; draw(); }
});
$('snapKey').addEventListener('click', () => { T.snap = !T.snap; draw(); });
function typed() {
  const e = entry();
  if (!e || !e.json) return;
  const v = parseFloat($('valField').value.replace(',', '.').replace('−', '-'));
  if (!Number.isFinite(v)) { draw(); return; }
  const r0 = restOf(e.json);
  commit(T.tool === 0 && T.local && T.axis !== 1 ? restWith(r0, 0, v) : restWith(r0, v));
  draw();
}
$('valField').addEventListener('change', typed);
$('valField').addEventListener('keydown', (ev) => { if (ev.key === 'Enter') { ev.preventDefault(); $('valField').blur(); } });
$('flipKey').addEventListener('click', () => {
  const f = $('valField');
  const v = parseFloat(f.value);
  if (Number.isFinite(v)) { f.value = String(-v); typed(); }
});
// The fader: a jog wheel laid flat. Drag sideways; slide up off it for fine.
const jog = $('jog'), cap = $('jogCap'), bubble = $('jogBubble');
let jogDrag = null;
jog.addEventListener('pointerdown', (ev) => {
  if (!begin()) return;
  jog.setPointerCapture(ev.pointerId);
  jogDrag = { x: ev.clientX, top: jog.getBoundingClientRect().top };
});
jog.addEventListener('pointermove', (ev) => {
  if (!jogDrag) return;
  const fine = ev.clientY < jogDrag.top - 40;
  jog.classList.toggle('drag', fine);
  bubble.textContent = 'fine ×0.1';
  move((ev.clientX - jogDrag.x) * PER_PX[T.tool] * (fine ? 0.1 : 1));
  jogDrag.x = ev.clientX;
  const r = jog.getBoundingClientRect();
  cap.style.transition = 'none';
  cap.style.left = Math.max(18, Math.min(r.width - 18, ev.clientX - r.left)) + 'px';
});
function jogUp() {
  if (!jogDrag) return;
  jogDrag = null;
  jog.classList.remove('drag');
  cap.style.transition = 'left .18s cubic-bezier(.3,1.4,.5,1)';
  cap.style.left = '50%';
  end();
}
jog.addEventListener('pointerup', jogUp);
jog.addEventListener('pointercancel', jogUp);
jog.addEventListener('keydown', (ev) => {
  const d = ev.key === 'ArrowRight' ? 1 : ev.key === 'ArrowLeft' ? -1 : 0;
  if (!d || !begin()) return;
  ev.preventDefault();
  move(d * SNAP[T.tool]);
  end();
});
// The arrows on the thing: the engine says how far along its axis the finger went.
on('drag', (ev) => {
  if (!T.gesture) {
    T.axis = ev.axis;
    if (T.tool === 1) T.tool = 0;
    if (!begin()) return;
  }
  move(T.tool === 0 ? ev.by : T.tool === 1 ? ev.by * 90 : ev.by);
});
on('dragEnd', end);

// ---- Slide on surfaces, Drop, Put on… (S3.3) ----
on('slide', (ev) => {
  if (!T.gesture) { T.tool = 0; if (!begin()) return; }
  const g = T.gesture;
  if (g.undo) preview(g.undo);
  const r = { ...g.rest0, at: ev.rest.map((v) => +(+v).toFixed(3)) };
  const ops = opsFor(g.e, g.rest0, r);
  g.undo = ops.length ? preview({ op: 'group', ops }) : null;
  g.r = r;
  draw();
});
on('slideEnd', end);
$('dropKey').addEventListener('click', () => {
  const e = entry();
  if (!e || !e.json) { if (e && e.why) Looped.toast(e.why); return; }
  send('st.drop', e.t.i);
});
on('drop', (ev) => {
  const e = entry();
  if (!e || !e.json || ev.thing !== e.t.i) return;
  const r0 = restOf(e.json);
  commit({ ...r0, at: ev.rest.map((v) => +(+v).toFixed(3)) });
  Looped.toast('Dropped');
});
// Put on…: the next thing you tap becomes what it sits on (tap empty space: on its own).
$('putOnKey').addEventListener('click', () => {
  const e = entry();
  if (!e || !e.json) { if (e && e.why) Looped.toast(e.why); return; }
  T.putOn = !T.putOn;
  T.putOnFor = T.putOn ? e : null;
  $('putOnKey').classList.toggle('lit', T.putOn);
  if (T.putOn) Looped.toast(`Tap what ${e.t.id} should sit on`);
});
on('pick', (ev) => {
  if (!T.putOn) return;
  T.putOn = false;
  $('putOnKey').classList.remove('lit');
  const e = T.putOnFor;
  if (!e || !e.json) return;
  const target = (ev.hits || []).map((h) => h[0]).find((i) => i !== e.t.i);
  send('st.attach', e.t.i, target === undefined ? -1 : target);
});
on('attached', (ev) => {
  const e = T.putOnFor;
  T.putOnFor = null;
  if (!e || ev.thing !== e.t.i) return;
  const r0 = restOf(e.json);
  const r = { ...r0, at: ev.at.map((v) => +(+v).toFixed(3)), turn: +(+ev.turn).toFixed(1) };
  const ops = opsFor(e, r0, r);
  const parentId = ev.parent >= 0 && S.things[ev.parent] ? S.things[ev.parent].id : undefined;
  ops.unshift(setOp(placeFile(), [...e.path, 'parent'], parentId));
  edit({ op: 'group', ops, label: `${e.t.id} now sits on ${parentId || 'the floor'}` });
  S.things[ev.thing].parent = ev.parent;
  Looped.toast(parentId ? `${e.t.id} now sits on ${parentId}` : `${e.t.id} stands on its own`);
});
on('select', draw);
on('changed', draw);
on('world', draw);
window.bar3dTools = T;
