#!/usr/bin/env node
// One command for every test. Prints one line per check (PASS / FAIL / SKIP),
// and for each failure the log file and its last lines, so the broken part is
// named at a glance.
//   node tools/check.mjs            checks for what changed since origin/main
//   node tools/check.mjs all        every check
//   node tools/check.mjs game room  only these systems
//   node tools/check.mjs changed    only the checks covering what isn't committed yet (each piece; --list: the systems)
//   node tools/check.mjs quick      build, C++ tests, shaders: no browser (each piece)
// Systems: build, native, game, room, hub, server, plus the game's own tests
// (mania, chess, arcade, meridian, race, menu, avatar, voice). The list itself:
// tools/lib/checks.mjs (one list; tools/workflows_test.mjs keeps GitHub's runs in step).
import { execSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync, readFileSync } from 'node:fs';

import { CHECKS, RULES } from './lib/checks.mjs';

const ORDER = Object.keys(CHECKS);

function changedSystems(uncommittedOnly = false) {
  const files = execSync((uncommittedOnly ? '' : 'git diff --name-only origin/main...HEAD; ') + 'git diff --name-only HEAD; git ls-files -o --exclude-standard',
    { encoding: 'utf8' }).split('\n').filter(Boolean);
  const want = new Set();
  for (const f of files) {
    for (const [re, sys] of RULES) {
      if (!re.test(f)) continue;
      if (sys === null) {
        const name = f.match(re)[1];
        for (const [s, list] of Object.entries(CHECKS))
          if (list.some(([, cmd]) => cmd.includes(`tools/${name}_test.mjs`))) want.add(s);
      } else sys.forEach(s => want.add(s));
      break;
    }
  }
  if ([...want].some(s => !['hub', 'server', 'room', 'build', 'native', 'site', 'plan', 'workflows', 'toolbelt', 'dev'].includes(s))) want.add('build');
  return { files, systems: ORDER.filter(s => want.has(s)) };
}

const args = process.argv.slice(2);
let systems;
if (args[0] === 'all') systems = ORDER.filter(s => s !== 'quick');
else if (args[0] === 'changed') {  // only what isn't committed yet (tools/piece.mjs)
  const c = changedSystems(true);
  // --list: just the systems (with "build" if the change needs one), for the piece runner.
  if (args.includes('--list')) { console.log(JSON.stringify(c.systems)); process.exit(0); }
  systems = c.systems.filter((s) => s !== 'build');  // piece.mjs has just built
  console.log(`${c.files.length} uncommitted file(s) → ${systems.join(', ') || 'nothing to test'}`);
}
else if (args.length) systems = args;
else {
  const c = changedSystems();
  systems = c.systems;
  console.log(`${c.files.length} changed file(s) since origin/main → ${systems.join(', ') || 'nothing to test'}`);
}
for (const s of systems) if (!CHECKS[s]) { console.error(`unknown system: ${s} (have: ${ORDER.join(', ')})`); process.exit(2); }

mkdirSync('build/check', { recursive: true });
const done = new Set(), rows = [];
let buildFailed = false;
for (const s of systems) {
  for (const [name, cmd] of CHECKS[s]) {
    if (done.has(name)) continue;
    done.add(name);
    const log = `build/check/${name}.log`;
    if (buildFailed && s !== 'native' && s !== 'room' && s !== 'hub' && s !== 'server') {
      rows.push({ s, name, r: 'SKIP', t: 0, log, why: 'build failed' }); continue;
    }
    process.stdout.write(`… ${s}/${name}\r`);
    const t0 = Date.now();
    const p = spawnSync('bash', ['-c', cmd], { encoding: 'utf8', maxBuffer: 1 << 28 });
    writeFileSync(log, (p.stdout || '') + (p.stderr || ''));
    const ok = p.status === 0;
    if (!ok && name === 'build') buildFailed = true;
    rows.push({ s, name, r: ok ? 'PASS' : 'FAIL', t: (Date.now() - t0) / 1000, log });
  }
}
console.log('\n=== check ===');
for (const r of rows) console.log(`${r.r.padEnd(4)}  ${(r.s + '/' + r.name).padEnd(22)} ${r.t.toFixed(0).padStart(4)} s  ${r.why || (r.r === 'FAIL' ? r.log : '')}`);
const fails = rows.filter(r => r.r === 'FAIL');
for (const r of fails) {
  const lines = readFileSync(r.log, 'utf8').trimEnd().split('\n');
  const hits = lines.filter(l => /FAIL|Error|error:|✗|assert|not ok/i.test(l)).slice(0, 8);
  console.log(`\n--- ${r.name}: first error lines ---\n${hits.join('\n') || '(none matched)'}\n--- ${r.name}: last 15 lines ---\n${lines.slice(-15).join('\n')}`);
}
console.log(fails.length ? `\n${fails.length} FAILED: ${fails.map(r => r.name).join(', ')}` : `\nALL ${rows.filter(r => r.r === 'PASS').length} PASSED`);
process.exit(fails.length ? 1 : 0);
