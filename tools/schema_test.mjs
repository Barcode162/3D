// The world schema is complete (docs/STUDIO.md 7.3): every field every world
// file uses is described in assets/world/schema.json, every choice used is in
// its list, and the lists match what the engine knows (bar/scene_names.cpp,
// looped/world/loop.cpp). Usage: node tools/schema_test.mjs
import { readFileSync, readdirSync } from 'node:fs';
import { problems } from '../studio/schema.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const read = (f) => JSON.parse(readFileSync(f, 'utf8'));
const schema = read('assets/world/schema.json');

const files = [
  ...readdirSync('assets/world/places').map((f) => ['place', `assets/world/places/${f}`]),
  ...readdirSync('assets/world/kinds').map((f) => ['kinds', `assets/world/kinds/${f}`]),
  ['catalog', 'assets/world/catalog.json'], ['songs', 'assets/world/songs.json'],
];
for (const [what, f] of files) {
  const p = problems(schema, what, read(f));
  check(p.length === 0, `${f}: every field is in the schema` + (p.length ? '\n  ' + p.slice(0, 20).join('\n  ') : ''));
}

// The lists match the engine's.
const names = readFileSync('bar/scene_names.cpp', 'utf8');
const list = (src, name) => {
  const m = new RegExp(name + String.raw`\[\]\s*=\s*\{([^}]*)\}`).exec(src);
  return m ? [...m[1].matchAll(/"([^"]*)"/g)].map((x) => x[1]) : null;
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
check(same(list(names, 'kMaterials'), schema.choices.material), 'materials match bar/scene_names.cpp');
check(same(list(names, 'kActions'), schema.choices.action), 'actions match bar/scene_names.cpp');
check(same(list(names, 'kSeatKinds'), schema.choices.seatKind), 'seat kinds match bar/scene_names.cpp');
check(same(list(names, 'kProps'), schema.choices.prop), 'props match bar/scene_names.cpp');
check(same(list(names, 'kEffects'), schema.choices.effect), 'sound effects match bar/scene_names.cpp');
const loop = readFileSync('looped/world/loop.cpp', 'utf8');
const channels = ['scale', ...[...loop.matchAll(/\{"([a-z.]+)", k[A-Z][A-Za-z]*, \d\}/g)].map((m) => m[1])];
check(same([...channels].sort(), [...schema.choices.channel].sort()), `channels match looped/world/loop.cpp (${channels.length})`);
const shapes = [...loop.matchAll(/\{"([a-z]+)", Shape::k/g)].map((m) => m[1]);
check(same(shapes, schema.choices.shape), 'loop shapes match looped/world/loop.cpp');

// Every part the engine reads has a description.
const parts = new Set();
for (const f of ['bar/bar_scene.cpp', 'bar/world_bar.cpp']) for (const m of readFileSync(f, 'utf8').matchAll(/part\([^,]*,\s*"([a-zA-Z]+)"\)/g)) parts.add(m[1]);
const missing = [...parts].filter((p) => !schema.parts[p]);
check(missing.length === 0, `every part the game reads is described (${parts.size})` + (missing.length ? ': ' + missing.join(', ') : ''));

console.log(failed ? `SCHEMA TEST FAILED (${failed})` : 'SCHEMA TEST PASSED');
process.exit(failed ? 1 : 0);
