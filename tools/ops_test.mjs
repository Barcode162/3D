// The Studio's edits (studio/ops.mjs) and their words (studio/diff.mjs) on
// the real world files: every op undone gives back exactly the files before;
// a history undoes and redoes; the words say what happened.
// Usage: node tools/ops_test.mjs
import { readFileSync } from 'node:fs';
import { apply, set, add, remove, group, get, History } from '../studio/ops.mjs';
import { changes } from '../studio/diff.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const read = (f) => JSON.parse(readFileSync(f, 'utf8'));
const fresh = () => ({ 'places/bar.json': read('assets/world/places/bar.json'), 'kinds/bar.json': read('assets/world/kinds/bar.json') });
const text = (f) => JSON.stringify(f);
const P = 'places/bar.json', K = 'kinds/bar.json';

const files = fresh();
const original = text(files);
const table = files[P].things.find((t) => t.kind === 'round_table');
const at = table.at;

// set + undo
const undo = apply(files, set(P, ['things', { id: table.id }, 'at'], [at[0] - 0.12, at[1], at[2]]));
check(get(files, P, ['things', { id: table.id }, 'at'])[0] === at[0] - 0.12, 'set moves a thing');
const words = changes(fresh(), files);
check(words.length === 1 && words[0].thing === table.id && /moved 12 cm left/.test(words[0].text), `the words: "${words[0] && words[0].text}"`);
apply(files, undo);
check(text(files) === original, 'its inverse gives back exactly the files before');

// a new field and its undo (deleted again, not left as null)
const u2 = apply(files, set(P, ['things', { id: table.id }, 'turn'], 45));
check(/turned 45°/.test(changes(fresh(), files)[0].text), 'turning says "turned 45°"');
apply(files, u2);
check(text(files) === original, 'undoing a new field removes it');

// add / remove
const candle = { id: 'candle-new', kind: 'candle', at: [0, 1.1, 0] };
const u3 = apply(files, add(P, ['things'], candle));
check(files[P].things.some((t) => t.id === 'candle-new') && /added candle-new \(candle\)/.test(changes(fresh(), files)[0].text), 'add puts a thing in, and says so');
let threw = false;
try { apply(files, add(P, ['things'], candle)); } catch (e) { threw = true; }
check(threw, 'the same id twice is refused');
apply(files, u3);
check(text(files) === original, 'undo of add removes it');
const u4 = apply(files, remove(P, ['things'], table.id));
check(!files[P].things.some((t) => t.id === table.id) && /removed/.test(changes(fresh(), files)[0].text), 'remove takes it out');
apply(files, u4);
check(text(files) === original, 'undo of remove puts it back in the same place in the list');

// kinds: every one of them
const stool = files[K].kinds.find((k) => k.id === 'bar_stool' || (k.loops && Object.keys(k.loops).length));
const loopName = Object.keys(stool.loops || {})[0];
if (loopName) {
  const before = stool.loops[loopName].period;
  const u5 = apply(files, set(K, ['kinds', { id: stool.id }, 'loops', loopName, 'period'], before + 1));
  const w = changes(fresh(), files);
  check(w.length === 1 && w[0].kind === stool.id && new RegExp(`every ${stool.id}: ${loopName} loop period ${before} → ${before + 1}`).test(w[0].text), `kind words: "${w[0] && w[0].text}"`);
  apply(files, u5);
}

// groups and history
const h = new History(files);
h.do(group([set(P, ['things', { id: table.id }, 'at'], [0, 0, 0]), set(P, ['things', { id: table.id }, 'turn'], 90)], 'move'));
h.do(set(P, ['things', { id: table.id }, 'phase'], 0.5));
h.undo(); h.undo();
check(text(files) === original && h.canRedo && !h.canUndo, 'a history undoes a group in one step');
h.redo();
check(get(files, P, ['things', { id: table.id }, 'turn']) === 90 && h.canUndo, 'and redoes it');
h.undo();
check(text(files) === original, 'undo after redo: back to the start');

console.log(failed ? `OPS TEST FAILED (${failed})` : 'OPS TEST PASSED');
process.exit(failed ? 1 : 0);
