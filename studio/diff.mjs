// What changed between two worlds, in plain words (docs/STUDIO.md 7.4):
// "pint_glass-4 moved 12 cm left", "every bar_stool: idle loop 2.4 s → 3 s",
// "added candle-7". Used by the Studio's Changes and History, the hub and
// Claude's news posts.
//   changes(beforeFiles, afterFiles, schema?) → [{thing?, kind?, text}]

const r = (v, n = 2) => +(+v).toFixed(n);
function dist(m) {
  const a = Math.abs(m);
  return a >= 1 ? `${r(a, 2)} m` : `${Math.round(a * 100)} cm`;
}
function moveWords(a = [0, 0, 0], b = [0, 0, 0]) {
  const d = [b[0] - a[0], b[1] - a[1], b[2] - a[2]];
  const words = [];
  if (Math.abs(d[0]) >= 0.005) words.push(`${dist(d[0])} ${d[0] > 0 ? 'right (+X)' : 'left (−X)'}`);
  if (Math.abs(d[1]) >= 0.005) words.push(`${dist(d[1])} ${d[1] > 0 ? 'up' : 'down'}`);
  if (Math.abs(d[2]) >= 0.005) words.push(`${dist(d[2])} ${d[2] > 0 ? 'back (+Z)' : 'forward (−Z)'}`);
  return words.length ? 'moved ' + words.join(', ') : '';
}
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
function show(v) {
  if (v === undefined) return 'nothing';
  if (typeof v === 'number') return String(r(v, 3));
  if (Array.isArray(v) && v.every((x) => typeof x === 'number')) return v.map((x) => r(x, 3)).join(' ');
  if (typeof v === 'object') return JSON.stringify(v).length > 40 ? 'its settings' : JSON.stringify(v);
  return String(v);
}
// Leaf differences between two values, as [path, before, after].
function leaves(a, b, path = [], out = []) {
  if (same(a, b)) return out;
  const objA = a && typeof a === 'object' && !Array.isArray(a), objB = b && typeof b === 'object' && !Array.isArray(b);
  if (objA && objB) {
    for (const k of new Set([...Object.keys(a), ...Object.keys(b)])) leaves(a[k], b[k], [...path, k], out);
  } else out.push([path, a, b]);
  return out;
}

export function changes(before, after) {
  const out = [];
  for (const file of new Set([...Object.keys(before || {}), ...Object.keys(after || {})])) {
    const A = (before || {})[file], B = (after || {})[file];
    if (!A || !B) { out.push({ text: `${B ? 'added' : 'removed'} the file ${file}` }); continue; }
    if (file.startsWith('places/')) {
      const ta = new Map((A.things || []).map((t) => [t.id, t])), tb = new Map((B.things || []).map((t) => [t.id, t]));
      for (const [id, t] of tb) if (!ta.has(id)) out.push({ thing: id, text: `added ${id} (${t.kind})` });
      for (const [id, t] of ta) if (!tb.has(id)) out.push({ thing: id, text: `removed ${id} (${t.kind})` });
      for (const [id, b] of tb) {
        const a = ta.get(id);
        if (!a || same(a, b)) continue;
        const words = [];
        const mv = moveWords(a.at, b.at);
        if (mv) words.push(mv);
        if ((a.turn || 0) !== (b.turn || 0)) words.push(`turned ${r((b.turn || 0) - (a.turn || 0), 1)}°`);
        if (!same(a.scale, b.scale)) words.push(`size ${show(a.scale ?? 1)} → ${show(b.scale ?? 1)}`);
        if ((a.parent || '') !== (b.parent || '')) words.push(b.parent ? `now sits on ${b.parent}` : 'now stands on its own');
        if (a.kind !== b.kind) words.push(`is now a ${b.kind}`);
        for (const [path, x, y] of leaves(a.parts || {}, b.parts || {})) words.push(`${path.join(' ')}: ${show(x)} → ${show(y)}`);
        for (const [path, x, y] of leaves(a.loops || {}, b.loops || {})) words.push(`${path.join(' ')} loop: ${show(x)} → ${show(y)}`);
        for (const k of ['phase', 'seed', 'state', 'row']) if (!same(a[k], b[k])) words.push(`${k}: ${show(a[k])} → ${show(b[k])}`);
        out.push({ thing: id, text: `${id} ${words.join(', ') || 'changed'}` });
      }
      for (const [path, x, y] of leaves({ ...A, things: 0 }, { ...B, things: 0 })) out.push({ text: `the place's ${path.join(' ')}: ${show(x)} → ${show(y)}` });
    } else if (file.startsWith('kinds/')) {
      const ka = new Map((A.kinds || []).map((k) => [k.id, k])), kb = new Map((B.kinds || []).map((k) => [k.id, k]));
      for (const [id] of kb) if (!ka.has(id)) out.push({ kind: id, text: `a new kind: ${id}` });
      for (const [id] of ka) if (!kb.has(id)) out.push({ kind: id, text: `removed the kind ${id}` });
      for (const [id, b] of kb) {
        const a = ka.get(id);
        if (!a || same(a, b)) continue;
        for (const [path, x, y] of leaves(a, b)) {
          const where = path[0] === 'loops' ? `${path[1]} loop ${path.slice(2).join(' ')}` : path.join(' ');
          out.push({ kind: id, text: `every ${id}: ${where.trim()} ${show(x)} → ${show(y)}` });
        }
      }
    } else {
      for (const [path, x, y] of leaves(A, B)) out.push({ text: `${file} ${path.join(' ')}: ${show(x)} → ${show(y)}` });
    }
  }
  return out;
}
