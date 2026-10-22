// Move / Turn / Size (docs/STUDIO.md 5.4): the tools under the selection.
// The arrows on the thing (the engine draws them, bar/studio_mode.cpp) are
// for quick drags; the strip is for exact values: axis keys, world or local,
// the number itself (with a ± key: the iPhone number pad has none), and the
// fader (drag; slide your finger up off it for ten times finer). Snapping on
// by default (5 cm, 15°, 5 %). One gesture = one undo.
import { S, on, send } from './studio.js';
import { edit, preview, thingEntry, restOf, placeFile, setOp } from './draft.mjs';
import { partsOf, partSource, partRest, partFrom, showPart } from './parts.mjs';

const $ = (id) => document.getElementById(id);
const T = { tool: 0, axis: 0, local: false, snap: true, gesture: null };
const UNITS = ['m', '°', '×'];
const SNAP = [0.05, 15, 0.05];
const PER_PX = [0.004, 0.6, 0.004];  // fader: how much a pixel moves it
const NAMES = ['X', 'Y', 'Z', 'All'];

function entry() {
  const t = S.things[S.selected];
  if (!t) return null;
  if (T.part) {
    // Part mode (S5.2): the chosen part, read fresh from the draft.
    const src = partSource(t, T.part.name);
    const p = src && Array.isArray(src.list) ? src.list[T.part.i] : null;
    if (p) return { t, part: T.part.name, json: p, src, path: [...src.path, T.part.i] };
    T.part = null;
  }
  return { t, ...thingEntry(t.id) };
}
const restOfE = (e) => (e.part ? partRest(e.part, e.json) : restOf(e.json));
// The axis a move goes along: world X/Y/Z, or turned with the thing (local).
function moveDir(rest, axis) {
  if (!T.local || T.part || axis === 1) return axis === 0 ? [1, 0, 0] : axis === 1 ? [0, 1, 0] : [0, 0, 1];
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
    const amount = by !== undefined ? by : v - (T.local && !T.part && T.axis !== 1 ? 0 : rest0.at[T.axis]);
    for (let k = 0; k < 3; k++) r.at[k] = round(rest0.at[k] + d[k] * amount);
  } else if (T.tool === 1) r.turn = round(((by !== undefined ? rest0.turn + by : v) + 540) % 360 - 180);
  else {
    const nv = Math.max(0.05, by !== undefined ? (T.axis === 3 ? rest0.scale[0] : rest0.scale[T.axis]) + by : v);
    if (T.axis === 3) r.scale = [round(nv), round(nv), round(nv)]; else r.scale[T.axis] = round(nv);
  }
  return r;
}
// The ops that put a rest into the files (only the fields that changed).
// The things a change reaches: this one, or (Every) each thing of its kind in the place.
function sameKind(e) {
  return (S.draft[placeFile()].things || []).filter((x) => x.kind === e.t.kind && !x.row);
}
function opsFor(e, r0, r) {
  if (e.part) {
    if (JSON.stringify(r) === JSON.stringify(r0)) return [];
    const item = partFrom(e.part, e.json, r);
    if (!T.every && e.src.owner !== e.t.id) {
      // This one only: the thing gets its own copy of the kind's list, with the change.
      const te = thingEntry(e.t.id);
      if (!te.json) return [];
      const list = e.src.list.map((x, i) => (i === T.part.i ? item : x));
      return [setOp(placeFile(), [...te.path, 'parts', e.part], JSON.parse(JSON.stringify(list)))];
    }
    return [setOp(e.src.file, e.path, item)];
  }
  if (T.every) {
    // Every one of its kind: the same move, turn or resize on each.
    const ops = [];
    const d = r.at.map((v, k) => v - r0.at[k]), dt = (r.turn || 0) - (r0.turn || 0);
    const k3 = r.scale.map((v, k) => (r0.scale[k] ? v / r0.scale[k] : 1));
    for (const x of sameKind(e)) {
      const xr0 = restOf(x);
      const xr = { at: xr0.at.map((v, k) => +(v + d[k]).toFixed(3)), turn: +(((xr0.turn + dt + 540) % 360) - 180).toFixed(1), scale: xr0.scale.map((v, k) => +(v * k3[k]).toFixed(3)) };
      if (xr.turn === -180) xr.turn = 180;
      ops.push(...thingOps(['things', { id: x.id }], xr0, xr));
    }
    return ops;
  }
  return thingOps(e.path, r0, r);
}
function thingOps(path, r0, r) {
  const e = { path };
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
  const r0 = restOfE(e);
  const ops = opsFor(e, r0, r);
  if (ops.length) edit({ op: 'group', ops, label: ['Move', 'Turn', 'Size'][T.tool] + ' ' + (e.part ? `${T.every ? e.src.owner : e.t.id}'s ${e.part.replace(/s$/, '')} ${T.part.i + 1}` : T.every ? 'every ' + e.t.kind : e.t.id) });
}

// ---- A gesture (the fader or an arrow): live, then one undo step ----
function begin() {
  const e = entry();
  if (!e || !e.json) { if (e && e.why) Looped.toast(e.why); return false; }
  T.gesture = { e, rest0: restOfE(e), total: 0, along: 0, undo: null };
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
  $('allKey').hidden = T.tool !== 2 || !!e.part;
  if (e.part && T.axis === 3) T.axis = 0;
  $('spaceSeg').hidden = T.tool !== 0 || !!e.part;
  $('partKey').classList.toggle('lit', !!e.part);
  // This one | Every <kind> (S5.3).
  const n = sameKind(e).length;
  $('scopeRow').hidden = n < 2 && !(e.part && e.src.owner !== e.t.id);
  $('kindName').textContent = e.part && e.src.owner !== e.t.kind && e.src.owner !== e.t.id ? e.src.owner : e.t.kind;
  $('kindCount').textContent = n > 1 ? ` (${n})` : '';
  [...$('scopeSeg').children].forEach((b) => b.classList.toggle('on', (b.dataset.every === '1') === T.every));
  $('partText').textContent = e.part ? (partsOf(e.t).find((x) => x.name === e.part && x.i === T.part.i) || { label: 'Part' }).label : 'Part';
  $('dropKey').disabled = $('putOnKey').disabled = !!e.part;
  [...$('toolKeys').querySelectorAll('[data-tool="1"]')].forEach((k) => { k.disabled = e.part === 'uses'; });
  if (e.part === 'uses' && T.tool === 1) T.tool = 0;
  if (e.part && e.json) showPart(e.t, e.part, restOfE(e));
  [...$('spaceSeg').children].forEach((b) => b.classList.toggle('on', (b.dataset.space === 'local') === T.local));
  $('snapKey').classList.toggle('lit', T.snap);
  $('snapText').textContent = T.snap ? ['5 cm', '15°', e.part ? '5 cm' : '5 %'][T.tool] : 'Free';
  $('axisName').textContent = NAMES[T.axis];
  $('axisName').style.color = T.axis < 3 ? `var(--ax-${'xyz'[T.axis]})` : 'var(--accent)';
  $('unitText').textContent = e.part && T.tool === 2 ? 'm' : UNITS[T.tool];
  const field = $('valField');
  if (!e.json) { field.value = ''; field.placeholder = 'Can\'t edit this one yet'; field.disabled = true; return; }
  field.disabled = false;
  if (document.activeElement !== field) field.value = round(value(restOfE(e)));
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
  const r0 = restOfE(e);
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
  const r0 = restOfE(e);
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
  const r0 = restOfE(e);
  const r = { ...r0, at: ev.at.map((v) => +(+v).toFixed(3)), turn: +(+ev.turn).toFixed(1) };
  const ops = opsFor(e, r0, r);
  const parentId = ev.parent >= 0 && S.things[ev.parent] ? S.things[ev.parent].id : undefined;
  ops.unshift(setOp(placeFile(), [...e.path, 'parent'], parentId));
  edit({ op: 'group', ops, label: `${e.t.id} now sits on ${parentId || 'the floor'}` });
  S.things[ev.thing].parent = ev.parent;
  Looped.toast(parentId ? `${e.t.id} now sits on ${parentId}` : `${e.t.id} stands on its own`);
});
// Part mode (S5.2): the Part key lists the thing's parts; pick one to edit it.
$('partKey').addEventListener('click', () => {
  const e = entry();
  if (!e) return;
  const list = $('partList');
  if (!list.hidden) { list.hidden = true; return; }
  const parts = partsOf(e.t);
  const pick = (part) => { T.part = part; T.every = !!part; list.hidden = true; if (!part) showPart(null); draw(); };
  list.replaceChildren(
    ...[{ label: e.t.id, small: 'the thing itself', part: null }, ...parts.map((x) => ({ label: x.label, small: x.src.owner === e.t.id ? 'its own' : 'every ' + x.src.owner, part: { name: x.name, i: x.i } }))]
      .map((x) => { const b = document.createElement('button'); b.type = 'button'; b.innerHTML = '<span></span><small></small>'; b.firstChild.textContent = x.label; b.lastChild.textContent = x.small; b.onclick = () => pick(x.part); return b; }));
  if (!parts.length) Looped.toast(`${e.t.id} has no shapes, tap areas or seats of its own to move`);
  list.hidden = false;
});
on('select', () => { if (T.part) { T.part = null; showPart(null); } T.every = false; $('partList').hidden = true; draw(); });
$('scopeSeg').addEventListener('click', (ev) => { const b = ev.target.closest('[data-every]'); if (b) { T.every = b.dataset.every === '1'; draw(); } });
on('changed', draw);
on('world', draw);
window.bar3dTools = T;
