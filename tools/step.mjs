#!/usr/bin/env node
// The end of a step, done the same way every time (docs/AUTOMATION.md):
//   node tools/step.mjs status [STEP]          the step's pieces so far (from the gallery)
//   node tools/step.mjs finish <STEP> "<what>" [--skip-check] [--full-check] [--bench] [--no-main]
// finish: the quick check (--full-check: every test, an hour+ here with no
// graphics card; CI runs them all at deploy anyway), merges origin/main,
// pushes work-go and main once (CI deploys), writes the Build log line in
// NEXT.md (the previous "(this push)" gets its real sha), runs the benchmark
// (--bench), posts to the hub (if HUB_KEY is set) and prints the reply for
// the user: one line per piece with its phone check.
import { execSync, spawnSync } from 'node:child_process';
import { readFileSync, writeFileSync } from 'node:fs';
import { openGallery, readJsonl, appendJsonl, commitGallery, writeReadme } from './lib/gallery.mjs';

const sh = (cmd) => execSync(cmd, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }).trim();
const run = (cmd) => spawnSync('bash', ['-c', cmd], { stdio: 'inherit' }).status === 0;
const argv = process.argv.slice(2);
const has = (n) => argv.includes('--' + n);
const [cmd, stepName, what = ''] = argv;
const ATTRIBUTION = process.env.PIECE_ATTRIBUTION ??
  'Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\nClaude-Session: https://claude.ai/code/session_01Nrk1rpfmMgEQWZYFLjtXXi';

openGallery();
const steps = readJsonl('steps.jsonl');
const lastDone = steps.length ? steps[steps.length - 1] : null;
function piecesOf(step) {
  const all = readJsonl('pieces.jsonl');
  const since = lastDone ? all.findIndex((p) => p.sha === lastDone.lastPiece) + 1 : 0;
  return all.slice(Math.max(0, since)).filter((p) => !step || p.tag.startsWith(step.replace(/\.$/, '')));
}

if (cmd === 'status' || !cmd) {
  const ps = piecesOf(stepName);
  console.log(ps.length ? ps.map((p) => `${p.tag.padEnd(6)} ${p.sha} ${p.title}${p.check ? '\n        check: ' + p.check : ''}`).join('\n') : '(no pieces since the last finished step)');
  process.exit(0);
}
if (cmd !== 'finish' || !stepName) { console.log('usage: node tools/step.mjs finish <STEP> "<what it was>" [--skip-check] [--bench]'); process.exit(2); }

const pieces = piecesOf(null);
if (!pieces.length) { console.log('No pieces since the last finished step.'); process.exit(1); }
if (!has('skip-check')) {
  const full = has('full-check');
  console.log(full ? '▶ full check' : '▶ quick check (the browser tests run on CI)');
  if (!run(`node tools/check.mjs ${full ? 'all' : 'quick'}`)) { console.log('✗ the full check failed: fix, then finish again'); process.exit(1); }
}
console.log('▶ merge origin/main');
run('git fetch -q origin main');
if (!run('git merge -q --no-edit origin/main')) { console.log('✗ merge conflict: resolve, commit, finish again (--skip-check)'); process.exit(1); }

// The Build log line (NEXT.md): the previous "(this push)" gets its sha.
const range = `${pieces[0].sha}^..${pieces[pieces.length - 1].sha}`;
const tags = pieces.map((p) => p.tag);
let next = readFileSync('NEXT.md', 'utf8');
if (lastDone?.mainSha) next = next.replace('- (this push) ', `- \`${lastDone.mainSha}\` `);
const line = `- (this push) ${tags.join(', ')}: ${what} → ${range}`;
next = next.replace(/(## Build log[^\n]*\n)/, `$1${line}\n`);
writeFileSync('NEXT.md', next);
writeFileSync('build/step-msg.txt', `Notes: ${stepName} built (${tags.join(', ')})\n\n${ATTRIBUTION}`);
sh('git add NEXT.md');
try { sh('git commit -q -F build/step-msg.txt'); } catch {}
const mainSha = sh('git rev-parse --short HEAD');
for (let i = 0; i < 4 && !run('git push -q -u origin HEAD:work-go'); i++) execSync(`sleep ${2 ** (i + 1)}`);
if (!has('no-main')) {
  console.log('▶ push main (CI deploys in ~6 min)');
  for (let i = 0; i < 4 && !run('git push -q origin HEAD:main'); i++) execSync(`sleep ${2 ** (i + 1)}`);
}
if (has('bench')) run('node tools/bench.mjs');

appendJsonl('steps.jsonl', { step: stepName, what, mainSha, lastPiece: pieces[pieces.length - 1].sha, pieces: tags,
  date: new Date().toISOString().slice(0, 16).replace('T', ' ') });
writeReadme();
commitGallery(`${stepName} finished (${mainSha})`);

const reply = [`build ${mainSha}`, ...pieces.map((p) => `- ${p.tag} ${p.title}: ${p.check || '(nothing to see)'} If not, say: broke ${p.tag}`)];
if (process.env.HUB_KEY) {
  const text = reply.slice(1).join('\n') + '\nPictures: the gallery branch, folder per piece.';
  run(`node tools/hub.mjs post --build ${mainSha} --title ${JSON.stringify(`${stepName}: ${what}`)} --text ${JSON.stringify(text)}`);
}
console.log('\n=== reply for the user ===\n' + reply.join('\n'));
