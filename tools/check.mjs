#!/usr/bin/env node
// One command for every test. Prints one line per check (PASS / FAIL / SKIP),
// and for each failure the log file and its last lines, so the broken part is
// named at a glance.
//   node tools/check.mjs            checks for what changed since origin/main
//   node tools/check.mjs all        every check
//   node tools/check.mjs game room  only these systems
//   node tools/check.mjs changed    only for what isn't committed yet
//   node tools/check.mjs quick      build, C++ tests, shaders: no browser (each piece)
// Systems: build, native, game, room, hub, server, plus the game's own tests
// (mania, chess, arcade, meridian, race, menu, avatar, voice).
import { execSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync, readFileSync } from 'node:fs';

const EMSDK = 'source /opt/emsdk/emsdk_env.sh >/dev/null 2>&1;';
const NP = 'NODE_PATH=$(npm root -g):$PWD/node_modules';
const CHECKS = {
  // The quick set: no game running in a browser, so a few minutes even on the
  // cloud machine (no graphics card). Each piece runs this; the slow browser
  // tests run on CI at deploy (user, 2026-10-23).
  quick:    [['build', `${EMSDK} [ -f build/build.ninja ] || emcmake cmake -B build -G Ninja && cmake --build build`],
             ['native', `${EMSDK} FAST=1 bash tools/native/run.sh`], ['schema', 'node tools/schema_test.mjs'],
             ['albedo', 'node tools/albedo_test.mjs'], ['wgsl', `${NP} node tools/wgsl_check.mjs`],
             ['toolbelt', 'node tools/toolbelt_test.mjs'], ['uikit', 'node tools/uikit.mjs check']],
  build:    [['build', `${EMSDK} [ -f build/build.ninja ] || emcmake cmake -B build -G Ninja && cmake --build build`],
             ['lint_emasm', 'node tools/lint_emasm.mjs']],
  native:   [['native', `${EMSDK} bash tools/native/run.sh`], ['schema', 'node tools/schema_test.mjs'], ['albedo', 'node tools/albedo_test.mjs']],
  game:     [['wgsl', `${NP} node tools/wgsl_check.mjs`], ['smoke', `${NP} node tools/smoke.mjs`], ['live_world', `${NP} node tools/live_world_test.mjs`], ['touch', `${NP} node tools/touch_test.mjs`], ['travel', `${NP} node tools/travel_test.mjs`]],
  room:     [['proto', 'node tools/proto_test.mjs'], ['room', 'node tools/room_test.mjs'],
             ['ledger', 'node tools/ledger_test.mjs'], ['mp', `${NP} node tools/mp_test.mjs`]],
  hub:      [['hub', 'node tools/hub_test.mjs']],
  ui:       [['uikit', 'node tools/uikit.mjs check'], ['ui', `${NP} node tools/ui_test.mjs`]],
  uikit:    [['uikit', 'node tools/uikit.mjs check']],
  toolbelt: [['toolbelt', 'node tools/toolbelt_test.mjs'], ['ui_php', 'php -l toolbelt/ui-editor/api.php >/dev/null'],
             ['ui_api', 'node tools/ui_api_test.mjs'], ['ui_editor', `${NP} node tools/ui_editor_test.mjs`]],
  site:     [['site_php', 'for f in $(find homepage -name "*.php"); do php -l "$f" >/dev/null || exit 1; done'], ['site', `${NP} node tools/site_test.mjs`]],
  studio:   [['studio_api', 'node tools/studio_api_test.mjs'], ['ops', 'node tools/ops_test.mjs'], ['studio', `${NP} node tools/studio_test.mjs`]],
  server:   [['php', 'for f in server/*.php hub/*.php; do php -l "$f" >/dev/null || exit 1; done'],
             ['hub', 'node tools/hub_test.mjs'], ['gate', `${NP} node tools/gate_test.mjs`]],
  mania:    [['rhythm', `${NP} node tools/rhythm_test.mjs`]],
  chess:    [['chess', `${NP} node tools/chess_test.mjs`]],
  arcade:   [['arcade', `${NP} node tools/arcade_test.mjs`]],
  meridian: [['meridian', `${NP} node tools/meridian_test.mjs`]],
  race:     [['race_mp', `${NP} node tools/race_mp_test.mjs`]],
  menu:     [['menu', `${NP} node tools/menu_test.mjs`], ['menu_games', `${NP} node tools/menu_games_test.mjs`]],
  avatar:   [['avatar', `${NP} node tools/avatar_test.mjs`]],
  voice:    [['voice', `${NP} node tools/voice_test.mjs`]],
};
// Which systems a changed file needs (first match wins; unknown files → game).
const RULES = [
  [/\.md$|^docs\//, []],
  [/^homepage\/|^tools\/site_test/, ['site']],
  [/^hub\/.*\.php$|^server\//, ['server']],
  [/^hub\/|^tools\/hub/, ['hub']],
  [/^ui\/system\/|^tools\/uikit/, ['uikit', 'ui']],
  [/^ui\//, ['ui', 'build', 'game']],  // the bar's site carries /ui too (bar/ui/, S4.3)
  [/^tools\/ui_test/, ['ui']],
  [/^toolbelt\/|^tools\/(toolbelt|ui_editor|ui_api)_test/, ['toolbelt']],
  [/^studio\/|^tools\/studio/, ['build', 'studio']],
  [/^assets\/world\/|^bar\/studio_mode/, ['build', 'native', 'game', 'studio']],
  [/^cloud\/|^proto\//, ['room', 'build', 'native']],
  [/^bar\/projects\/mania\//, ['build', 'game', 'mania']],
  [/^bar\/projects\/chess\//, ['build', 'native', 'game', 'chess']],
  [/^bar\/projects\/platformer\//, ['build', 'arcade']],
  [/^bar\/projects\/meridian\//, ['build', 'meridian']],
  [/^bar\/projects\/racing\/|^looped\/(drive|vehicle)/, ['build', 'native', 'game', 'race']],
  [/^bar\/web\/menu\.js$/, ['build', 'game', 'menu']],
  [/^bar\/web\/voice\.js$/, ['build', 'voice']],
  [/^bar\/web\/net\.js$|^looped\/netproto|^bar\/(players|shared_world)/, ['build', 'native', 'game', 'room']],
  [/avatar/, ['build', 'native', 'game', 'avatar']],
  [/^tools\/(\w+)_test\.mjs$/, null], // a changed test runs itself (below)
  [/./, ['build', 'native', 'game']],
];
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
  if ([...want].some(s => !['hub', 'server', 'room', 'build', 'native', 'site'].includes(s))) want.add('build');
  return { files, systems: ORDER.filter(s => want.has(s)) };
}

const args = process.argv.slice(2);
let systems;
if (args[0] === 'all') systems = ORDER.filter(s => s !== 'quick');
else if (args[0] === 'changed') {  // only what isn't committed yet (tools/piece.mjs)
  const c = changedSystems(true);
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
