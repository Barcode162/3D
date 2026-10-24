// The plan keeper (tools/plan.mjs) on a throwaway copy of the plan's data
// (docs/plan/plan.json, docs/charter/rules.json): the real plan converts to
// PLAN.md and back without losing a word; PLAN.md, the Charter's rules block
// and CLAUDE.md's "Never" list match the data; every command works on the
// copy and anything that would break the shape is refused. No browser.
// Usage: node tools/plan_test.mjs
import { mkdtempSync, mkdirSync, writeFileSync, readFileSync, copyFileSync, rmSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { problems, stale } from './plan.mjs';
import { load, render, parse } from './lib/plandata.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };

// The real plan.
const data = load(), md = render(data);
check(problems(md).length === 0, 'the plan keeps its shape' + (problems(md).length ? ': ' + problems(md).join(' | ') : ''));
check(render(parse(md)) === md && JSON.stringify(parse(md).plan) === JSON.stringify(data.plan) && JSON.stringify(parse(md).rules) === JSON.stringify(data.rules), 'the plan turns into PLAN.md and back without losing a word');
const st = stale(data);
check(st.length === 0, 'PLAN.md, the Charter\'s rules block and CLAUDE.md\'s "Never" list match the data' + (st.length ? ': ' + st.join(' | ') : ''));

// A copy with a known Now (one open piece), so the test doesn't depend on the real job.
const dir = mkdtempSync(join(tmpdir(), 'plan-')), tool = resolve('tools/plan.mjs');
mkdirSync(join(dir, 'docs/plan'), { recursive: true }); mkdirSync(join(dir, 'docs/charter'), { recursive: true });
for (const f of ['docs/charter/rules.json', 'docs/charter/charter.md', 'CLAUDE.md', 'PLAN.md']) copyFileSync(f, join(dir, f));
const copy = structuredClone(data.plan);
copy.now = { id: 'X1', title: 'A test step', chapter: 'the tools', about: 'For the test.', serves: ['N1'], pieces: [{ id: 'X1.1', text: 'The only piece.', check: '', done: false }] };
writeFileSync(join(dir, 'docs/plan/plan.json'), JSON.stringify(copy));
const plan = (...a) => spawnSync('node', [tool, ...a], { cwd: dir, encoding: 'utf8' });
const text = () => readFileSync(join(dir, 'PLAN.md'), 'utf8');
const json = () => JSON.parse(readFileSync(join(dir, 'docs/plan/plan.json'), 'utf8'));
try {
  check(plan('render').status === 0 && /\*\*X1 · A test step\*\*/.test(text()), 'render writes PLAN.md from the data');
  check(plan('add', 'world', 'W99', 'A test road item', '--serves', 'N3').status === 0 && /\*\*W99\*\* A test road item · N3/.test(text()) && json().road.world.some((r) => r.id === 'W99'), 'add puts a road item in its chapter (data and PLAN.md)');
  check(plan('add', 'world', 'W99', 'Again', '--serves', 'N3').status === 1, 'the same id twice is refused');
  check(plan('add', 'world', 'W98', 'Serves nothing').status === 1, 'an item that serves no North star line is refused');
  check(plan('add', 'world', 'W97', 'Bad star', '--serves', 'N9').status === 1, 'a North star line that does not exist is refused');
  check(plan('add', 'kitchen', 'W96', 'No such chapter', '--serves', 'N1').status === 1, 'an unknown chapter is refused');
  check(plan('park', 'site', 'A parked test idea').status === 0 && /### The site\n(- .*\n)*- A parked test idea\n/.test(text().split('## 6. Parked')[1]), 'park adds a line under its chapter in Parked');
  check(plan('start', 'W99', '--piece', 'W99.1|One|look').status === 1, 'start is refused while Now has open pieces');
  check(plan('drop', 'X1.9').status === 1, 'drop refuses a piece that is not in Now');
  check(plan('tick', 'X1.1').status === 0 && /- \[x\] \*\*X1\.1\*\*/.test(text()), 'tick ticks a piece in Now');
  check(plan('start', 'W99', '--piece', 'W99.1|The first piece|stand by the bar', '--piece', 'W99.2|The second').status === 0, 'start moves a road item into Now');
  const t = text();
  check(/\*\*W99 · A test road item\*\* \(the world\)/.test(t.split('## 5.')[0]) && !/\*\*W99\*\*/.test(t.split('## 5. The road')[1].split('## 6.')[0]), 'Now holds W99 and the road no longer does');
  check(/- \[ \] \*\*W99\.1\*\* The first piece \*Check:\* stand by the bar/.test(t), 'the pieces carry their phone checks');
  check(plan('done', '--sha', 'abc1234', '--what', 'W99.1-W99.2: a test', '--range', 'a..b').status === 0 && /## 7\. Done\n.*\n- \(this push\) W99\.1-W99\.2: a test → a\.\.b\n/.test(text()), 'done puts a line on top of Done');
  check(plan('done', '--sha', 'def5678', '--what', 'next').status === 0 && /- `def5678` W99\.1-W99\.2: a test/.test(text()), 'the next done gives the last line its build code');
  check(plan('seal', 'bbb2222').status === 0 && /- `bbb2222` next/.test(text()) && !/\(this push\)/.test(text()), 'seal gives the "(this push)" line its build code');
  check(plan('seal', 'ccc3333').status === 1, 'seal refuses when nothing is waiting for a build code');
  check(plan('rule', '**An old test rule:** before').status === 0 && plan('rule', '**A test rule:** one', '--replaces', 'An old test rule').status === 0 && !/\*\*An old test rule/.test(text()) && /\*\*A test rule:\*\* one/.test(text()), 'rule --replaces swaps a rule for a new one');
  check(plan('rule', 'x', '--replaces', 'No such rule').status === 1, 'rule --replaces refuses a rule that is not there');
  check(/\*\*A test rule:\*\* one/.test(readFileSync(join(dir, 'docs/charter/charter.md'), 'utf8')), 'a new rule reaches the Charter\'s block by itself');
  check(plan('chapter', 'site', 'A test now line.').status === 0 && /- \*\*The site:\*\* .* \*Now:\* A test now line\./.test(text()), 'chapter sets a chapter\'s Now line');
  check(plan('add', 'tools', 'X2', 'Next test', '--serves', 'N1').status === 0 && plan('next', 'X2 → W1').status === 0 && /Up next, in order:\*\* X2 → W1/.test(text()), 'next sets the Up next line');
  check(plan('check').status === 0, 'after all that the copy still keeps its shape and its files agree');
  writeFileSync(join(dir, 'PLAN.md'), text().replace('A test road item', 'Edited by hand'));
  check(plan('check').status === 1, 'a hand edit of PLAN.md is caught');
  check(problems(text().replace('## 6. Parked', '## 6. Ideas')).some((p) => /parts must be exactly/.test(p)), 'a renamed part is caught');
  check(problems(text().replace('### The backbone', '### The money')).length > 0, 'a renamed chapter is caught');
} finally { rmSync(dir, { recursive: true, force: true }); }
console.log(failed ? `PLAN TEST: ${failed} FAILED` : 'PLAN TEST PASSED');
process.exit(failed ? 1 : 0);
