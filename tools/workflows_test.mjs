#!/usr/bin/env node
// Keeps GitHub's runs honest (restructure J2.2), no browser, a second:
// - every check in tools/lib/checks.mjs runs in exactly one workflow
//   (.github/workflows/deploy.yml or hub.yml), unless NOT_ON_CI says why not;
// - every test file in tools/ is in that list (or in NOT_LISTED, with why);
// - every test step runs even when an earlier one failed, and has a time limit;
// - a test allowed to fail names its open known issue in its step name
//   ("… (allowed to fail: KI-24)", docs/records/known-issues.md);
// - every step a deploy report reads has an id;
// - one deployer per folder (J5.3, docs/handbook/deploy-map.json): every
//   tracked file has its entry, and each workflow uploads only its own folders
//   and starts on changes to them.
// Usage: node tools/workflows_test.mjs
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { execSync } from 'node:child_process';
import { CHECKS, NOT_ON_CI, NOT_LISTED } from './lib/checks.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const WORKFLOWS = ['.github/workflows/deploy.yml', '.github/workflows/hub.yml'];
// A test script: what a step or a check runs to test something.
const TEST = /^tools\/(?:\w+_test\.mjs|smoke\.mjs|wgsl_check\.mjs|lint_emasm\.mjs|uikit\.mjs|native\/run\.sh)$/;
const scriptsIn = (text) => [...String(text).matchAll(/\btools\/[\w/]+\.(?:mjs|sh)\b/g)].map((m) => m[0]);

// The steps of a workflow: each "      - " item under a job's steps, with its keys.
function parseSteps(file) {
  const lines = readFileSync(file, 'utf8').split('\n'), steps = [];
  let cur = null, inRun = false;
  lines.forEach((line, i) => {
    if (/^ {6}- /.test(line)) {
      cur = { file, line: i + 1, name: '', id: '', if: '', timeout: '', coe: false, run: '' };
      steps.push(cur); inRun = false;
      line = '        ' + line.slice(8);  // "      - name: x" reads like "        name: x"
    } else if (cur && /^ {0,5}\S/.test(line)) { cur = null; inRun = false; return; }
    if (!cur) return;
    const key = /^ {8}([\w-]+):\s?(.*)$/.exec(line);
    if (key) {
      inRun = false;
      const [, k, v] = key, val = v.replace(/^"(.*)"$/, '$1');
      if (k === 'name') cur.name = val;
      else if (k === 'id') cur.id = val;
      else if (k === 'if') cur.if = val;
      else if (k === 'timeout-minutes') cur.timeout = val;
      else if (k === 'continue-on-error') cur.coe = val === 'true';
      else if (k === 'run') { cur.run = v === '|' ? '' : v; inRun = v === '|'; }
    } else if (inRun && (/^ {10}/.test(line) || line.trim() === '')) cur.run += line.trim() + '\n';
  });
  return steps;
}

const steps = WORKFLOWS.flatMap(parseSteps);
check(steps.length > 20, `read ${steps.length} steps from ${WORKFLOWS.length} workflows`);

// 1. Every check runs on GitHub exactly once (or says why not).
const listed = new Set(Object.values(CHECKS).flat().flatMap(([, cmd]) => scriptsIn(cmd)).filter((s) => TEST.test(s)));
const where = new Map();
for (const st of steps) for (const s of scriptsIn(st.run).filter((x) => TEST.test(x))) {
  if (!where.has(s)) where.set(s, new Set());
  where.get(s).add(st.file.replace(/^.*\//, ''));
}
for (const s of [...listed].sort()) {
  const files = [...(where.get(s) || [])];
  if (NOT_ON_CI[s]) check(files.length === 0, `${s} is not on GitHub (${NOT_ON_CI[s]})`);
  else check(files.length === 1, `${s} runs in exactly one workflow (${files.join(', ') || 'none'})`);
}
for (const s of where.keys()) check(listed.has(s), `${s}, run on GitHub, is in tools/lib/checks.mjs`);
for (const s of Object.keys(NOT_ON_CI)) check(listed.has(s) && NOT_ON_CI[s].length > 10, `NOT_ON_CI ${s} is a check, with a reason`);

// 2. Every test file is a check (or says why not).
const files = readdirSync('tools').filter((f) => /_test\.mjs$|^smoke\.mjs$|^wgsl_check\.mjs$/.test(f)).map((f) => 'tools/' + f);
for (const f of files) {
  if (NOT_LISTED[f]) check(!listed.has(f), `${f} is left out on purpose (${NOT_LISTED[f]})`);
  else check(listed.has(f), `${f} is in tools/lib/checks.mjs`);
}

// 3. Test steps: each runs even after another failed, within a time limit.
const deploy = steps.filter((s) => s.file.endsWith('deploy.yml')), hub = steps.filter((s) => s.file.endsWith('hub.yml'));
const upload = deploy.findIndex((s) => s.id === 'upload');
check(upload > 0, 'deploy.yml: the IONOS upload has id "upload"');
const isTest = (s) => scriptsIn(s.run).some((x) => TEST.test(x)) || /^Install Playwright/.test(s.name);
for (const s of deploy.slice(upload + 1).filter(isTest)) {
  check(/!cancelled\(\)/.test(s.if) && /^\d+$/.test(s.timeout), `deploy.yml "${s.name}": runs after any failure, time limit ${s.timeout || 'none'} min`);
}
for (const s of hub.filter(isTest)) {
  check(/!cancelled\(\)/.test(s.if) && /^\d+$/.test(s.timeout), `hub.yml "${s.name}": runs after any failure, time limit ${s.timeout || 'none'} min`);
}

// 4. A test allowed to fail names its open known issue.
const known = readFileSync('docs/records/known-issues.md', 'utf8');
const open = new Set([...known.split('## Closed')[0].matchAll(/^\| (KI-\d+) \|/gm)].map((m) => m[1]));
for (const s of steps.filter((x) => x.coe && isTest(x))) {
  const ki = /\(allowed to fail: (KI-\d+)\)/.exec(s.name);
  check(!!ki && open.has(ki[1]), `"${s.name}" may fail only for an open known issue (${ki ? ki[1] : 'none named'})`);
}

// 5. The deploy reports (J2.3) read steps by id: every test step has one.
for (const s of [...deploy, ...hub].filter(isTest)) check(/^[a-z][\w-]*$/.test(s.id), `"${s.name}" has an id (${s.id || 'none'})`);

// 6. One deployer per folder (J5.3).
const map = JSON.parse(readFileSync('docs/handbook/deploy-map.json', 'utf8')).entries;
const paths = map.map((e) => e.path);
check(new Set(paths).size === paths.length, 'deploy map: no path listed twice');
const entryOf = (f) => map.filter((e) => e.path.endsWith('/') ? f.startsWith(e.path) : f === e.path).sort((a, b) => b.path.length - a.path.length)[0];
const orphans = execSync('git ls-files', { encoding: 'utf8' }).split('\n').filter(Boolean).filter((f) => !entryOf(f));
check(!orphans.length, 'deploy map: every tracked file has its entry' + (orphans.length ? ': ' + orphans.slice(0, 8).join(', ') : ''));
for (const e of map.filter((x) => x.workflow)) check(existsSync(`.github/workflows/${e.workflow}`), `deploy map: ${e.path} → ${e.workflow} exists`);
for (const wf of ['deploy.yml', 'hub.yml', 'plan.yml']) {
  const text = readFileSync(`.github/workflows/${wf}`, 'utf8');
  const sent = [...text.matchAll(/(?:mirror -R[^\n]*?|put) \$GITHUB_WORKSPACE\/([\w./-]+)/g)].map((m) => m[1]);
  const wrong = sent.filter((s) => !s.startsWith('build/')).filter((s) => entryOf(s.includes('.') ? s : s + '/')?.workflow !== wf);
  check(!wrong.length, `${wf} uploads only its own folders` + (wrong.length ? ': not its own: ' + wrong.join(', ') : ''));
  const filter = (text.match(/^ {4}paths: \[(.*)\]$/m) || [])[1];
  if (filter) {
    const mine = map.filter((e) => e.workflow === wf).map((e) => e.path);
    const missing = mine.filter((p) => !filter.includes(`'${p.endsWith('/') ? p + '**' : p}'`));
    check(!missing.length, `${wf} starts on a change to each of its folders` + (missing.length ? ': missing ' + missing.join(', ') : ''));
  }
}

console.log(failed ? `WORKFLOWS TEST: ${failed} FAILED` : 'WORKFLOWS TEST PASSED');
process.exit(failed ? 1 : 0);
