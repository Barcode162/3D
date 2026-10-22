// Controls for one field of the world, drawn from its schema entry
// (assets/world/schema.json): numbers with a value and a fader, points as
// three numbers, switches, choices as chips, colours, text. Used by the
// Studio's Inspect (S5.4). Each control calls onSet(newValue) once per
// change; the caller turns that into an edit (one undo step).
//   control(schema, field, label, value, onSet, extra) → element
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
const dec = (step) => (step && step < 1 ? Math.min(3, String(step).split('.')[1].length) : 0);
const fmt = (v, step) => (+v).toFixed(dec(step));
const parse = (s) => parseFloat(String(s).replace(',', '.').replace('−', '-'));

function numberControl(f, label, v, onSet) {
  const step = f.step || (f.type === 'int' ? 1 : 0.01);
  const min = f.min ?? -100, max = f.max ?? 100;
  const span = Math.max(Math.abs(v) * 2, step * 20);
  const lo = Math.max(min, v - span), hi = Math.min(max, v + span);
  const input = el('input', 'field num');
  input.inputMode = 'decimal'; input.enterKeyHint = 'done'; input.value = fmt(v, step); input.setAttribute('aria-label', label);
  const fader = el('div', 'field');
  Object.assign(fader.dataset, { fader: '', min: lo, max: hi, value: v, dec: dec(step), unit: f.unit ? ' ' + f.unit : '' });
  fader.setAttribute('aria-label', label);
  const put = (nv, from) => {
    nv = +Math.max(min, Math.min(max, Math.round(nv / step) * step)).toFixed(4);
    if (from !== input) input.value = fmt(nv, step);
    if (from !== fader && fader.set) fader.set(nv);
    onSet(nv);
  };
  input.addEventListener('change', () => { const nv = parse(input.value); if (Number.isFinite(nv)) put(nv, input); else input.value = fmt(v, step); });
  input.addEventListener('keydown', (e) => { if (e.key === 'Enter') input.blur(); });
  let t = 0;
  fader.addEventListener('change', (e) => { if (e.target !== fader) return; input.value = fmt(e.detail, step); clearTimeout(t); t = setTimeout(() => put(e.detail, fader), 150); });
  const box = el('div', 'ctl num');
  box.append(el('span', 'name', label), input, el('span', 'unit', f.unit || ''), fader);
  return box;
}
function vecControl(f, label, v, onSet) {
  const box = el('div', 'ctl vec');
  box.append(el('span', 'name', label + (f.unit ? ` (${f.unit})` : '')));
  const cur = [...v];
  ['X', 'Y', 'Z'].slice(0, v.length).forEach((axis, i) => {
    const inp = el('input', 'field num');
    inp.inputMode = 'decimal'; inp.value = fmt(v[i], 0.001); inp.setAttribute('aria-label', `${label} ${axis}`);
    inp.addEventListener('change', () => { const nv = parse(inp.value); if (!Number.isFinite(nv)) return; cur[i] = +nv.toFixed(3); onSet([...cur]); });
    inp.addEventListener('keydown', (e) => { if (e.key === 'Enter') inp.blur(); });
    box.append(inp);
  });
  return box;
}
function boolControl(label, v, onSet) {
  const box = el('div', 'ctl flag');
  const s = el('button', 'switch');
  s.type = 'button'; s.setAttribute('role', 'switch'); s.setAttribute('aria-checked', String(!!v)); s.setAttribute('aria-label', label);
  s.append(el('i'));
  s.addEventListener('change', (e) => onSet(!!e.detail));
  box.append(el('span', 'name', label), s);
  return box;
}
function chipsControl(label, list, v, onSet) {
  const box = el('div', 'ctl chips');
  box.append(el('span', 'name', label));
  const row = el('div', 'chiprow');
  for (const c of list) {
    const b = el('button', 'chip' + (c === v ? ' on' : ''), c);
    b.type = 'button';
    b.onclick = () => { [...row.children].forEach((x) => x.classList.toggle('on', x === b)); onSet(c); };
    row.append(b);
  }
  box.append(row);
  return box;
}
const hex = (c) => '#' + c.map((x) => Math.round(Math.max(0, Math.min(1, x)) * 255).toString(16).padStart(2, '0')).join('');
function colorControl(label, v, onSet) {
  const box = el('div', 'ctl flag');
  const inp = el('input', 'swatch');
  inp.type = 'color'; inp.value = hex(v); inp.setAttribute('aria-label', label);
  inp.addEventListener('change', () => onSet([1, 3, 5].map((k) => +(parseInt(inp.value.slice(k, k + 2), 16) / 255).toFixed(3))));
  box.append(el('span', 'name', label), inp);
  return box;
}
function textControl(label, v, onSet) {
  const box = el('div', 'ctl text');
  const inp = el('input', 'field');
  inp.value = v ?? ''; inp.setAttribute('aria-label', label);
  inp.addEventListener('change', () => onSet(inp.value.trim() || undefined));
  inp.addEventListener('keydown', (e) => { if (e.key === 'Enter') inp.blur(); });
  box.append(el('span', 'name', label), inp);
  return box;
}

// The right control for a field, or null when it's not one you edit here
// (lists, maps, ids): those show as words.
export function control(schema, f, label, v, onSet, extra = {}) {
  const t = f.type;
  if ((t === 'number' || t === 'int' || t === 'range') && typeof v === 'number') return numberControl(f, label, v, onSet);
  if ((t === 'vec3' || t === 'rect') && Array.isArray(v) && v.every((x) => typeof x === 'number')) return vecControl(f, label, v, onSet);
  if (t === 'color' && Array.isArray(v)) return colorControl(label, v, onSet);
  if (t === 'bool') return boolControl(label, v, onSet);
  if (t === 'choice' && schema.choices[f.choices]) return chipsControl(label, schema.choices[f.choices], v, onSet);
  if (t === 'stateRef' && extra.states && extra.states.length) return chipsControl(label, extra.states, v, onSet);
  if (t === 'text') return textControl(label, v, onSet);
  return null;
}
