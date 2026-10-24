#!/usr/bin/env node
// Ship a job (restructure J4.3; docs/playbook/job.md → Ship):
//   node tools/ship.mjs "<what the job did>" [--news "<friendly lines>"] [--skip-check "<why>"] [--no-wait]
// 1. Refuses unless: on a job branch job/J<n>; nothing unsaved; every commit
//    not on main belongs to the job (only the plan and the session line may be
//    unsaved: the finish commits them) (subject "J<x>.<k> …", "J<x> …", "Notes: …",
//    "World: …", "UI: …"); every piece of J<n> ticked; the plan's files agree.
// 2. Runs the checks for everything the job changed since main (--skip-check
//    only with the reason, which goes into the reply).
// 3. The step finisher: merge main, the Done line, the session's commit, push
//    the job branch and main once (tools/step.mjs finish).
// 4. Waits (up to 12 min, --no-wait to skip) until Dev's deploy reports show
//    the new build live, and says exactly what is and isn't confirmed.
import { execSync, spawnSync } from 'node:child_process';
import { readFileSync } from 'node:fs';

const argv = process.argv.slice(2);
const opt = (n) => { const i = argv.indexOf('--' + n); return i >= 0 ? argv[i + 1] : null; };
const has = (n) => argv.includes('--' + n);
const what = argv.find((a, i) => !a.startsWith('--') && !(i > 0 && argv[i - 1].startsWith('--')));
const sh = (c) => execSync(c, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }).trim();
const run = (c) => spawnSync('bash', ['-c', c], { stdio: 'inherit' }).status === 0;
const refuse = (why) => { console.log(`✗ not shipped: ${why}`); process.exit(1); };

if (!what) refuse('say what the job did: node tools/ship.mjs "<what>"');
const branch = sh('git rev-parse --abbrev-ref HEAD'), job = (/^job\/(J\d+)$/.exec(branch) || [])[1];
if (!job) refuse(`ship runs on a job branch (job/J<n>), not "${branch}"`);
// Only the finish's own files may be unsaved (the next job in Now, the session line): it commits them.
const unsaved = execSync('git status --porcelain', { encoding: 'utf8' }).split('\n').filter(Boolean).map((l) => l.slice(3))  // untrimmed: each line starts with its two status letters
  .filter((f) => !/^(PLAN\.md|CLAUDE\.md|docs\/plan\/|docs\/charter\/|docs\/records\/sessions\/)/.test(f));
if (unsaved.length) refuse(`there are unsaved changes (${unsaved.join(', ')}): commit them as a piece (or take them out) first`);
sh('git fetch -q origin main');
const foreign = sh('git log --format=%s origin/main..HEAD').split('\n').filter(Boolean)
  .filter((s) => !/^(J\d+(\.\d+)? |Notes: |World: |UI: |Merge )/.test(s));
if (foreign.length) refuse(`commits that belong to no job: ${foreign.map((s) => `"${s.slice(0, 60)}"`).join(', ')}`);
const plan = JSON.parse(readFileSync('docs/plan/plan.json', 'utf8'));
if (plan.now?.id === job && plan.now.pieces.some((p) => !p.done)) refuse(`${job} still has open pieces: ${plan.now.pieces.filter((p) => !p.done).map((p) => p.id).join(', ')}`);
if (Object.values(plan.road).flat().some((r) => r.id === job)) refuse(`${job} is still on the road, never started`);
if (!run('node tools/plan.mjs check >/dev/null')) refuse('the plan\'s files disagree (node tools/plan.mjs check)');

let checked = 'the checks for everything the job changed passed';
if (opt('skip-check')) checked = `the checks were skipped: ${opt('skip-check')}`;
else if (!run('node tools/check.mjs')) refuse('a check failed (its log is named above)');

const finish = ['node tools/step.mjs finish', job, JSON.stringify(what), '--skip-check', opt('news') ? `--news ${JSON.stringify(opt('news'))}` : '--no-news'].join(' ');
if (!run(`PIECE_BRANCH=${branch} ${finish}`)) refuse('the finish stopped (above): nothing was pushed to main unless it says so');
const sha = sh('git rev-parse --short HEAD');
if (sh('git rev-parse --short origin/main') !== sha) refuse(`main on GitHub is not ${sha}: the push didn't land`);

// Live: Dev's reports for this build (tools/ci_status.mjs writes them).
console.log(`\n▶ shipped ${sha}; ${checked}`);
if (has('no-wait') || !process.env.HUB_KEY) { console.log('Live: not confirmed yet (not waited for). Check later: node tools/project.mjs status'); process.exit(0); }
const report = async () => {
  try { const r = await fetch('https://joshuavelasco.com/dev/api.php', { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': process.env.HUB_KEY }, body: '{"a":"ci"}', signal: AbortSignal.timeout(5000) }); return (await r.json()).ci; }
  catch { return null; }
};
const t0 = Date.now();
let mine = [];
for (;;) {
  const ci = await report() || {};
  mine = ['game', 'site'].filter((w) => ci[w]?.build === sha).map((w) => ({ w, ...ci[w] }));
  const minutes = (Date.now() - t0) / 60000;
  if (mine.length && mine.every((m) => m.live || m.tests !== 'running')) break;
  if (!mine.length && minutes > 4) { console.log('Live: no deploy started for this build within 4 min (a change that deploys nothing?). Check later: node tools/project.mjs status'); process.exit(0); }
  if (minutes > 12) break;
  await new Promise((r) => setTimeout(r, 30000));
}
for (const m of mine) {
  console.log(`Live: ${m.w} deploy of ${sha}: ${m.live ? 'live' : 'NOT live'}, tests ${m.tests}${m.failed?.length ? ' (failed: ' + m.failed.join(', ') + ')' : ''}`);
}
if (mine.some((m) => !m.live)) console.log('Not everything is live yet, or something failed: docs/playbook/failures.md → A deploy is red.');
