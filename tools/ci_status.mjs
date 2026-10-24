#!/usr/bin/env node
// A deploy's real state, for Dev (restructure J2.3). The two workflows call it
// at three moments; it writes build/ci-<workflow>.json and puts it in Dev's
// store (private/dev/ci-<workflow>.json) on IONOS:
//   node tools/ci_status.mjs game start    the run began: tests running, this build not live yet
//   node tools/ci_status.mjs game live     the upload worked: this build is live, tests still running
//   node tools/ci_status.mjs game end      the run ended (always): from STEPS_JSON = ${{ toJSON(steps) }}
//                                          and JOB_STATUS = ${{ job.status }}; in the tests job (J66)
//                                          LIVE says whether the deploy job's upload worked
// workflow: game (deploy.yml) or site (hub.yml). The file:
//   {workflow, build, live, tests: running|passed|failed|cancelled, failed: [step ids],
//    allowed: [step ids that failed but are allowed to fail], at, url}
// "live" at the end is whether the step with id "upload" succeeded. A report
// that can't be uploaded prints a warning and never turns the run red.
import { writeFileSync, mkdirSync } from 'node:fs';
import { spawnSync } from 'node:child_process';

const [workflow, phase] = process.argv.slice(2);
if (!['game', 'site'].includes(workflow) || !['start', 'live', 'end'].includes(phase)) {
  console.log('usage: node tools/ci_status.mjs game|site start|live|end'); process.exit(2);
}
const env = process.env;
const report = {
  workflow, build: (env.GITHUB_SHA || '').slice(0, 7), live: phase === 'live', tests: 'running', failed: [], allowed: [],
  at: Math.floor(Date.now() / 1000),
  url: env.GITHUB_RUN_ID ? `${env.GITHUB_SERVER_URL}/${env.GITHUB_REPOSITORY}/actions/runs/${env.GITHUB_RUN_ID}` : '',
};
if (phase === 'end') {
  let steps = {};
  try { steps = JSON.parse(env.STEPS_JSON || '{}'); } catch { console.log('WARNING: STEPS_JSON is not JSON'); }
  for (const [id, s] of Object.entries(steps)) {
    if (s.conclusion === 'failure') report.failed.push(id);
    else if (s.outcome === 'failure') report.allowed.push(id);  // continue-on-error: failed, allowed to
  }
  // The deploy job has the upload step; the tests job (J66) gets it from the
  // deploy job (LIVE = needs.build-and-publish.outputs.live).
  report.live = steps.upload ? steps.upload.outcome === 'success' : env.LIVE === 'true';
  report.tests = env.JOB_STATUS === 'success' ? 'passed' : env.JOB_STATUS === 'cancelled' ? 'cancelled' : 'failed';
}

mkdirSync('build', { recursive: true });
const file = `build/ci-${workflow}.json`;
writeFileSync(file, JSON.stringify(report));
console.log(`${file}: ${JSON.stringify(report)}`);

if (!env.IONOS_SFTP_HOST) { console.log('(no IONOS host: not uploaded)'); process.exit(0); }
// The tests job's end report for a build that's no longer the live one (a
// newer deploy went out while these tests ran, J66) would overwrite the newer
// build's row in Dev: it isn't sent.
if (phase === 'end' && env.LIVE !== undefined) {
  const r = spawnSync('curl', ['-s', '--max-time', '10', 'https://joshuavelasco.com/bar/version.txt'], { encoding: 'utf8' });
  const liveNow = (r.stdout || '').trim();
  if (/^[0-9a-f]{7}$/.test(liveNow) && liveNow !== report.build) { console.log(`(build ${liveNow} is live now, not ${report.build}: this report isn't sent to Dev)`); process.exit(0); }
}
const sh = (cmd) => spawnSync('bash', ['-c', cmd], { stdio: 'inherit' }).status === 0;
if (!sh('command -v lftp >/dev/null || sudo apt-get install -y lftp >/dev/null')) {
  console.log('WARNING: lftp could not be installed; Dev did not get this report'); process.exit(0);
}
const put = `set sftp:auto-confirm yes; set net:max-retries 3; set net:timeout 30; mkdir -p -f private/dev; put ${file} -o private/dev/ci-${workflow}.json; bye`;
if (!sh(`lftp --env-password -u "$IONOS_SFTP_USER" "sftp://$IONOS_SFTP_HOST" -e ${JSON.stringify(put)}`)) {
  console.log('WARNING: the report did not reach Dev (the upload failed)');
}
process.exit(0);
