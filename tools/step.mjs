#!/usr/bin/env node
// The end of a step, done the same way every time (docs/AUTOMATION.md):
//   node tools/step.mjs status [STEP]          the step's pieces so far (from the gallery)
//   node tools/step.mjs finish <STEP> "<what>" [--news "<a few friendly lines for the news post>"] [--no-news] [--skip-check] [--full-check] [--bench] [--no-main]
// finish: the checks for everything the job changed since main (the user,
// 2026-10-24: test only what the change touches; --full-check: every test, an hour+ here with no
// graphics card; CI runs them all at deploy anyway), merges origin/main,
// pushes the job branch and main once (CI deploys), writes the Done line in
// PLAN.md (the previous "(this push)" gets its real sha), runs the benchmark
// (--bench), posts to the hub (if HUB_KEY is set) and prints the reply for
// the user: one line per piece with its phone check.
import { addDoneLine } from './plan.mjs';
import { attribution } from './lib/attribution.mjs';
import { execSync, spawnSync } from 'node:child_process';
import { readFileSync, writeFileSync } from 'node:fs';
import { openGallery, readJsonl, appendJsonl, commitGallery, writeReadme } from './lib/gallery.mjs';

const sh = (cmd) => execSync(cmd, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }).trim();
const run = (cmd) => spawnSync('bash', ['-c', cmd], { stdio: 'inherit' }).status === 0;
const argv = process.argv.slice(2);
const has = (n) => argv.includes('--' + n);
const [cmd, stepName, what = ''] = argv;
const ATTRIBUTION = attribution();

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

const target = process.env.PIECE_BRANCH ?? sh('git rev-parse --abbrev-ref HEAD');  // the job branch (docs/playbook/job.md)
if (!/^job\/J\d+$/.test(target)) { console.log(`✗ finish runs on a job branch (job/J<n>), not "${target}"`); process.exit(1); }
const pieces = piecesOf(null);
if (!pieces.length) { console.log('No pieces since the last finished step.'); process.exit(1); }
if (!has('skip-check')) {
  const full = has('full-check');
  console.log(full ? '▶ full check' : '▶ the checks for what this job changed since main');
  if (!run(`node tools/check.mjs ${full ? 'all' : ''}`)) { console.log('✗ a check failed: fix, then finish again'); process.exit(1); }
}
console.log('▶ merge origin/main');
run('git fetch -q origin main');
if (!run('git merge -q --no-edit origin/main')) { console.log('✗ merge conflict: resolve, commit, finish again (--skip-check)'); process.exit(1); }

// The Done line (PLAN.md → 7. Done, through the plan keeper): the previous "(this push)" gets its sha.
const range = `${pieces[0].sha}^..${pieces[pieces.length - 1].sha}`;
const tags = pieces.map((p) => p.tag);
addDoneLine({ sha: lastDone?.mainSha ?? '0000000', what: `${tags.join(', ')}: ${what}`, range });
writeFileSync('build/step-msg.txt', `Notes: ${stepName} built (${tags.join(', ')})\n\n${ATTRIBUTION}`);
sh('git add PLAN.md docs/plan docs/charter CLAUDE.md');
try { sh('git commit -q -F build/step-msg.txt'); } catch {}
const mainSha = sh('git rev-parse --short HEAD');
for (let i = 0; i < 4 && !run(`git push -q -u origin HEAD:${target}`); i++) execSync(`sleep ${2 ** (i + 1)}`);
if (!has('no-main')) {
  console.log('▶ push main (CI deploys in ~6 min)');
  for (let i = 0; i < 4 && !run('SHIP_OK=1 git push -q origin HEAD:main'); i++) execSync(`sleep ${2 ** (i + 1)}`);
}
if (has('bench')) run('node tools/bench.mjs');

appendJsonl('steps.jsonl', { step: stepName, what, mainSha, lastPiece: pieces[pieces.length - 1].sha, pieces: tags,
  date: new Date().toISOString().slice(0, 16).replace('T', ' ') });
writeReadme();
commitGallery(`${stepName} finished (${mainSha})`);

const reply = [`build ${mainSha}`, ...pieces.map((p) => `- ${p.tag} ${p.title}: ${p.check || '(nothing to see)'} If not, say: broke ${p.tag}`)];
if (process.env.HUB_KEY && !has('no-news')) {  // --no-news: nothing new for people to see (restructure D8)
  // The news post is for people (PLAN.md rule): --news gives it in plain, friendly words; without it, the phone checks.
  const news = argv.includes('--news') ? argv[argv.indexOf('--news') + 1] : null;
  const text = news ?? (reply.slice(1).join('\n') + '\nPictures: the gallery branch, folder per piece.');
  run(`node tools/hub.mjs post --build ${mainSha} --title ${JSON.stringify(`${stepName}: ${what}`)} --text ${JSON.stringify(text)}`);
}
console.log('\n=== reply for the user ===\n' + reply.join('\n'));
