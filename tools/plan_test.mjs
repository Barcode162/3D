// The plan keeper (tools/plan.mjs) on a copy of PLAN.md: the real file keeps
// its shape; add, park, tick, start and done work; anything that would break
// the shape is refused. No browser.  Usage: node tools/plan_test.mjs
import { mkdtempSync, writeFileSync, readFileSync, rmSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { problems } from './plan.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const real = readFileSync('PLAN.md', 'utf8');
check(problems(real).length === 0, 'PLAN.md keeps its shape' + (problems(real).length ? ': ' + problems(real).join(' | ') : ''));

const dir = mkdtempSync(join(tmpdir(), 'plan-')), tool = resolve('tools/plan.mjs');
// The copy gets a known Now (one open piece), so the test doesn't depend on the real step.
writeFileSync(join(dir, 'PLAN.md'), real.replace(/(## 4\. Now\n)[\s\S]*?(\n## 5\.)/, '$1**X1 · A test step** (the tools). For the test.\n- [ ] **X1.1** The only piece.\n$2'));
const plan = (...a) => spawnSync('node', [tool, ...a], { cwd: dir, encoding: 'utf8' });
const text = () => readFileSync(join(dir, 'PLAN.md'), 'utf8');
try {
  check(plan('add', 'world', 'W99', 'A test road item', '--serves', 'N3').status === 0 && /\*\*W99\*\* A test road item · N3/.test(text()), 'add puts a road item in its chapter');
  check(plan('add', 'world', 'W99', 'Again', '--serves', 'N3').status === 1, 'the same tag twice is refused');
  check(plan('add', 'world', 'W98', 'Serves nothing').status === 1, 'an item that serves no North star line is refused');
  check(plan('add', 'world', 'W97', 'Bad star', '--serves', 'N9').status === 1, 'a North star line that does not exist is refused');
  check(plan('add', 'kitchen', 'W96', 'No such chapter', '--serves', 'N1').status === 1, 'an unknown chapter is refused');
  check(plan('park', 'site', 'A parked test idea').status === 0 && /### The site\n(- .*\n)*- A parked test idea\n/.test(text().split('## 6. Parked')[1]), 'park adds a line under its chapter in Parked');
  check(plan('start', 'W99', '--piece', 'W99.1|One|look').status === 1, 'start is refused while Now has open pieces');
  check(plan('tick', 'X1.1').status === 0 && /- \[x\] \*\*X1\.1\*\*/.test(text()), 'tick ticks a piece in Now');
  check(plan('start', 'W99', '--piece', 'W99.1|The first piece|stand by the bar', '--piece', 'W99.2|The second').status === 0, 'start moves a road item into Now');
  const t = text();
  check(/\*\*W99 · A test road item\*\* \(the world\)/.test(t.split('## 5.')[0]) && !/\*\*W99\*\*/.test(t.split('## 5. The road')[1].split('## 6.')[0]), 'Now holds W99 and the road no longer does');
  check(/- \[ \] \*\*W99\.1\*\* The first piece \*Check:\* stand by the bar/.test(t), 'the pieces carry their phone checks');
  check(plan('done', '--sha', 'abc1234', '--what', 'W99.1-W99.2: a test', '--range', 'a..b').status === 0 && /## 7\. Done\n.*\n- \(this push\) W99\.1-W99\.2: a test → a\.\.b\n/.test(text()), 'done puts a line on top of Done');
  check(plan('done', '--sha', 'def5678', '--what', 'next').status === 0 && /- `def5678` W99\.1-W99\.2: a test/.test(text()), 'the next done gives the last line its build code');
  check(plan('add', 'tools', 'X2', 'Next test', '--serves', 'N1').status === 0 && plan('next', 'X2 → R5').status === 0 && /Up next, in order:\*\* X2 → R5/.test(text()), 'next sets the Up next line');
  check(plan('check').status === 0, 'after all that the copy still keeps its shape');
  const broken = text().replace('## 6. Parked', '## 6. Ideas');
  check(problems(broken).some((p) => /parts must be exactly/.test(p)), 'a renamed part is caught');
  check(problems(text().replace('### The backbone', '### The money')).length > 0, 'a renamed chapter is caught');
  check(problems(text().replace('· N4\n', '\n')).length > 0, 'a road item without its North star line is caught');
} finally { rmSync(dir, { recursive: true, force: true }); }
console.log(failed ? `PLAN TEST: ${failed} FAILED` : 'PLAN TEST PASSED');
process.exit(failed ? 1 : 0);
