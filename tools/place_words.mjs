#!/usr/bin/env node
// A world file → a place in words (J118.1): writes assets/world/places/<name>.looped
// from <name>.json, one line per thing, so the words become the source:
//   node tools/place_words.mjs bar          write bar.looped, then bar.json made from it (refuses
//                                           if bar.looped exists; --force when a tool changed the JSON)
//   node tools/place_words.mjs bar --check  the words make back the same world, thing by thing
// After that the JSON is made from the words (node tools/words.mjs place bar),
// read by looped/words/place_list.cpp. The list form:
//
//   place the bar
//     about: <text>
//     kinds from kinds/bar.json
//     you come in at 0.4 1.65 5.2, facing 0
//     the air: ambient 0.07 0.05 0.045, fog 0.025 0.015 0.015, fogDensity 0.008
//     a room: id bar, min -9.31 -0.2 -7.45, max 9.45 4.2 9.2
//     an area arcade: min ..., max ...
//     a point arcade-door at 3.7 1.2 -7.4
//
//     a bar stool at -7.04 0 -4.04, turned -16.3, scaled 0.97
//     a bottle at 1 1.1 -4, made of bottle_wine.glb in bottle-glass, called bottle-1
//       its light: color 1 0.18 0.55, lumens 180, wall yes
//       a seat: name "Racing sim", hip 0 0.4 0.2
//
// `its <part>:` is one part's settings; `a <part>:` is one more of a list
// (seats, uses), as `a room:` is at the place's level.
//
// A thing's id is "<kind>-<n>" (its n-th of that kind) unless it says `called`;
// `the <kind>` (the hall) is the thing whose id is its kind.
// Values: numbers (several make a list), lists of lists joined by `to`,
// yes / no, a bare word, or "quoted text".
import { readFileSync, writeFileSync, existsSync, unlinkSync } from 'node:fs';
import { execSync } from 'node:child_process';

const [name, ...flags] = process.argv.slice(2);
if (!name) { console.log('which place? node tools/place_words.mjs bar [--check|--force]'); process.exit(2); }
const jsonPath = `assets/world/places/${name}.json`, wordsPath = `assets/world/places/${name}.looped`;

const BARE = /^[A-Za-z_][A-Za-z0-9_.\-/]*$/;
const isNums = (v) => Array.isArray(v) && v.length > 0 && v.every((x) => typeof x === 'number');
const num = (n) => String(n);
const spaced = (k) => k.replace(/([a-z])([A-Z])/g, '$1 $2').toLowerCase();
const singular = (k) => (k.endsWith('s') ? k.slice(0, -1) : k);
const article = (w) => (/^[aeiou]/.test(w) ? 'an' : 'a');

function value(v, where) {
  if (typeof v === 'number') return num(v);
  if (v === true) return 'yes';
  if (v === false) return 'no';
  if (typeof v === 'string') {
    if (BARE.test(v) && v !== 'yes' && v !== 'no' && v !== 'to') return v;
    return `"${v.replace(/[\\"]/g, (c) => '\\' + c).replace(/\n/g, '\\n')}"`;  // \" \\ \n inside
  }
  if (isNums(v)) return v.map(num).join(' ');
  if (Array.isArray(v) && v.length > 1 && v.every(isNums)) return v.map((x) => x.map(num).join(' ')).join(' to ');
  if (Array.isArray(v) && v.length > 1 && v.every((x) => typeof x === 'string' && BARE.test(x))) return v.join(' ');
  throw new Error(`${where}: a value the words can't say yet: ${JSON.stringify(v)}`);
}
const pairs = (o, where) => Object.entries(o).map(([k, v]) => {
  if (!/^[A-Za-z][A-Za-z0-9]*$/.test(k)) throw new Error(`${where}: a key the words can't say: ${k}`);
  return `${k} ${value(v, `${where}.${k}`)}`;
}).join(', ');

function toWords(place) {
  const kindIds = new Set();
  for (const f of place.kinds) for (const k of JSON.parse(readFileSync(`assets/world/${f}`, 'utf8')).kinds) kindIds.add(k.id);
  const out = [];
  out.push(`# ${place.id[0].toUpperCase() + place.id.slice(1)}, written in Looped Script's words (J118.1). This file is the`);
  out.push(`# place: assets/world/places/${place.id}.json is made from it by`);
  out.push(`#   node tools/words.mjs place ${place.id}`);
  out.push('# (never edited by hand). One line per thing: what it is, where its foot');
  out.push('# sits (x right, y up, z towards the door, in metres), how far it is turned');
  out.push('# (degrees), its size, its own id when it has one, then its parts\' settings.');
  out.push('# The kinds are in assets/world/kinds/; the reader is looped/words/place_list.cpp.');
  out.push('');
  out.push(`place the ${place.id}`);
  const known = new Set(['id', 'about', 'kinds', 'spawn', 'things']);
  out.push(`  about: ${place.about}`);
  for (const f of place.kinds) out.push(`  kinds from ${f}`);
  out.push(`  you come in at ${place.spawn.at.map(num).join(' ')}, facing ${num(place.spawn.yaw)}`);
  for (const [k, v] of Object.entries(place)) {
    if (known.has(k)) continue;
    if (Array.isArray(v) && v.every((x) => x && typeof x === 'object' && !Array.isArray(x))) {
      if (!k.endsWith('s') || k === 'areas') throw new Error(`the place's list ${k} needs a plural name (not areas)`);
      for (const o of v) out.push(`  ${article(singular(k))} ${spaced(singular(k))}: ${pairs(o, k)}`);
    } else if (k === 'points') {
      for (const [pk, pv] of Object.entries(v)) out.push(`  a point ${pk} at ${value(pv, `points.${pk}`)}`);
    } else if (k === 'areas') {
      for (const [ak, av] of Object.entries(v)) out.push(`  an area ${ak}: ${pairs(av, `areas.${ak}`)}`);
    } else if (v && typeof v === 'object' && !Array.isArray(v)) {
      out.push(`  the ${spaced(k)}: ${pairs(v, k)}`);
    } else throw new Error(`the place's ${k} can't be said in words yet`);
  }
  const count = {};
  let lastKind = null;
  for (const t of place.things) {
    count[t.kind] = (count[t.kind] || 0) + 1;
    const said = t.kind.replace(/[_-]/g, ' ');
    const back = [...new Set([t.kind.replace(/[_-]/g, '_'), t.kind.replace(/[_-]/g, '-')])].filter((k) => kindIds.has(k));
    if (!kindIds.has(t.kind)) throw new Error(`${t.id}: kind ${t.kind} isn't in the place's kind files`);
    const word = back.length === 1 && back[0] === t.kind ? said : `thing of kind ${t.kind}`;
    if (t.kind !== lastKind) out.push('');
    lastKind = t.kind;
    const the = t.id === t.kind;  // `the hall`: its id is its kind
    if (/(^| )(at|of)( |$)/.test(said) && word === said) throw new Error(`${t.id}: kind ${t.kind} has the word at or of in it`);
    let line = `  ${the ? 'the' : article(word)} ${word} at ${t.at.map(num).join(' ')}`;
    for (const k of Object.keys(t)) if (!['id', 'kind', 'at', 'turn', 'scale', 'parent', 'parts'].includes(k)) throw new Error(`${t.id}: ${k} can't be said yet`);
    if (t.turn !== undefined) line += `, turned ${num(t.turn)}`;
    if (t.scale !== undefined) line += `, scaled ${num(t.scale)}`;
    const parts = { ...(t.parts || {}) };
    const m = parts.model;
    if (m && Object.keys(m).join() === 'file,material' && BARE.test(m.file) && BARE.test(m.material)) {
      line += `, made of ${m.file} in ${m.material}`;
      delete parts.model;
    }
    if (!the && t.id !== `${t.kind}-${count[t.kind]}`) line += `, called ${t.id}`;
    if (t.parent !== undefined) line += `, held by ${t.parent}`;
    out.push(line);
    for (const [pk, pv] of Object.entries(parts)) {
      if (Array.isArray(pv)) {
        if (!pv.length || !pv.every((x) => x && typeof x === 'object')) throw new Error(`${t.id}: parts.${pk} can't be said yet`);
        if (!pk.endsWith('s')) throw new Error(`${t.id}: a list part needs a plural name: ${pk}`);
        for (const o of pv) out.push(`    ${article(singular(pk))} ${spaced(singular(pk))}: ${pairs(o, `${t.id}.${pk}`)}`);
      } else out.push(`    its ${spaced(pk)}: ${pairs(pv, `${t.id}.${pk}`)}`);
    }
  }
  return out.join('\n') + '\n';
}

// Two worlds the same, thing by thing (key order aside): the first difference, or null.
function differ(a, b, path = '') {
  if (typeof a !== typeof b || Array.isArray(a) !== Array.isArray(b) || (a === null) !== (b === null)) return `${path}: ${JSON.stringify(a)} vs ${JSON.stringify(b)}`;
  if (Array.isArray(a)) {
    if (a.length !== b.length) return `${path}: ${a.length} vs ${b.length} items`;
    for (let i = 0; i < a.length; ++i) { const d = differ(a[i], b[i], `${path}[${a[i]?.id ?? i}]`); if (d) return d; }
    return null;
  }
  if (a && typeof a === 'object') {
    const ka = Object.keys(a).sort(), kb = Object.keys(b).sort();
    if (ka.join() !== kb.join()) return `${path}: keys ${ka.join(',')} vs ${kb.join(',')}`;
    for (const k of ka) { const d = differ(a[k], b[k], `${path}.${k}`); if (d) return d; }
    return null;
  }
  return a === b ? null : `${path}: ${JSON.stringify(a)} vs ${JSON.stringify(b)}`;
}

if (flags.includes('--check')) {
  // The words, read by the engine's own reader, make the world file again: equal to the one in git?
  const made = JSON.parse(execSync(`node tools/words.mjs print ${name}`, { encoding: 'utf8', maxBuffer: 64 << 20 }));
  const was = JSON.parse(execSync(`git show HEAD:${jsonPath}`, { encoding: 'utf8', maxBuffer: 64 << 20 }));
  const d = differ(was, made);
  if (d) { console.log(`✗ ${name}: the words make a different world at ${d}`); process.exit(1); }
  console.log(`ok  ${name}: the words make the same world (${made.things.length} things, thing by thing)`);
  process.exit(0);
}
const isList = (t) => /^\s*kinds from /m.test(t);
if (existsSync(wordsPath) && !isList(readFileSync(wordsPath, 'utf8'))) { console.log(`${wordsPath} is written in place words (sentences), not a list: it stays as it is`); process.exit(1); }
if (existsSync(wordsPath) && !flags.includes('--force')) { console.log(`${wordsPath} exists: it's the source now (--force to write it again from the JSON, as a tool that changed the JSON does)`); process.exit(1); }
const before = existsSync(wordsPath) ? readFileSync(wordsPath, 'utf8') : null;
const world = JSON.parse(readFileSync(jsonPath, 'utf8'));
const text = toWords(world);
writeFileSync(wordsPath, text);
// The words must make the same world before they replace the JSON's text.
let made = null;
try { made = JSON.parse(execSync(`node tools/words.mjs print ${name}`, { encoding: 'utf8', maxBuffer: 64 << 20 })); } catch {}
const d = made ? differ(world, made) : 'the words could not be read';
if (d) {
  if (before === null) unlinkSync(wordsPath); else writeFileSync(wordsPath, before);
  console.log(`✗ not written: the words would make a different world at ${d}`);
  process.exit(1);
}
execSync(`node tools/words.mjs place ${name}`, { stdio: 'ignore' });
console.log(`wrote ${wordsPath} (${text.split('\n').length} lines) and made ${jsonPath} from it (${made.things.length} things, the same world)`);
