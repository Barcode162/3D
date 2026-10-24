#!/usr/bin/env node
// The front door (restructure J4.2): every tool through one command.
//   node tools/project.mjs help              every command, one line each
//   node tools/project.mjs status [--brief]  the whole truth on one screen
//   node tools/project.mjs <command> …       the command (tools/lib/commands.mjs)
// Status: the work here (branch, saved or not), the job in Now, the live build,
// both deploys, the stores, the user's notes and news drafts, open problems,
// whether the plan's files agree. Every part says how old it is, and each
// network question gives up after 5 seconds (it says so, never guesses).
import { spawnSync, execSync } from 'node:child_process';
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { COMMANDS } from './lib/commands.mjs';

const [cmd, ...rest] = process.argv.slice(2);
const sh = (c) => { try { return execSync(c, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'], timeout: 8000 }).trim(); } catch { return ''; } };
const ago = (t) => { const s = Math.max(0, Date.now() / 1000 - t); return s < 90 ? 'just now' : s < 5400 ? `${Math.round(s / 60)} min ago` : s < 129600 ? `${Math.round(s / 3600)} h ago` : `${Math.round(s / 86400)} days ago`; };
async function get(url, opt = {}) {
  try { const r = await fetch(url, { ...opt, signal: AbortSignal.timeout(5000) }); return { ok: r.ok, text: await r.text() }; }
  catch { return { ok: false, text: '' }; }
}
const dev = async (a) => {
  if (!process.env.HUB_KEY) return null;
  const r = await get('https://joshuavelasco.com/dev/api.php', { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': process.env.HUB_KEY }, body: JSON.stringify({ a }) });
  try { return JSON.parse(r.text); } catch { return null; }
};

async function status(brief) {
  const out = [], plan = JSON.parse(readFileSync('docs/plan/plan.json', 'utf8'));
  const rules = JSON.parse(readFileSync('docs/charter/rules.json', 'utf8'));
  // Here.
  sh('git fetch -q origin main');
  const branch = sh('git rev-parse --abbrev-ref HEAD'), dirty = sh('git status --porcelain').split('\n').filter(Boolean).length;
  const unpushed = sh(`git rev-list --count origin/${branch}..HEAD 2>/dev/null`) || (sh(`git ls-remote --heads origin ${branch}`) ? '0' : 'not on GitHub');
  out.push(`Here: branch ${branch}, ${dirty ? dirty + ' changed file(s) not saved' : 'nothing unsaved'}, ${unpushed === '0' ? 'everything pushed' : unpushed + ' commit(s) not pushed'}; main is ${sh('git log -1 --format="%h (%cr)" origin/main')}`);
  // The plan.
  const n = plan.now;
  if (n) {
    const done = n.pieces.filter((p) => p.done).length, next = n.pieces.find((p) => !p.done);
    out.push(`Now: ${n.id} · ${n.title}: ${done} of ${n.pieces.length} pieces done${next ? `; next ${next.id}` : '; ready to ship'}`);
  } else out.push('Now: nothing (start the next job)');
  if (!brief) out.push(`Up next: ${plan.upNext}`);
  const freeze = rules.rules.find((r) => /^\*\*Freeze:/.test(r.text));
  if (freeze) out.push('Freeze: on (no features, world, engine or language changes; docs/archive/restructure.md §7)');
  // Live.
  const [version, ci, notes, posts] = await Promise.all([get('https://joshuavelasco.com/bar/version.txt'), dev('ci'), dev('notes'),
    process.env.HUB_KEY ? get('https://joshuavelasco.com/hub/hub.php', { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': process.env.HUB_KEY }, body: '{"a":"posts"}' }).then((r) => { try { return JSON.parse(r.text); } catch { return null; } }) : null]);
  out.push(`Live bar: ${version.ok ? 'build ' + version.text.trim() : 'did not answer within 5 s'}`);
  const deploy = (name, c) => !c ? `${name}: no report` : `${name}: build ${c.build}, ${c.live ? 'live' : 'not live'}, tests ${c.tests}${c.failed?.length ? ' (failed: ' + c.failed.join(', ') + ')' : ''}${c.allowed?.length ? ' (allowed to fail: ' + c.allowed.join(', ') + ')' : ''}, ${ago(c.at)}`;
  if (ci?.ok) out.push(`Deploys: ${deploy('game', ci.ci.game)}; ${deploy('site', ci.ci.site)}`);
  else out.push(`Deploys: ${process.env.HUB_KEY ? "Dev's store did not answer" : 'no HUB_KEY: not checked'}`);
  // The user's channels.
  const unread = notes?.ok ? notes.notes.filter((x) => !x.read).length : null;
  const drafts = posts?.ok ? posts.posts.filter((p) => !p.published).length : null;
  out.push(`From the user: ${unread === null ? 'notes not checked' : unread ? unread + ' unread note(s) in Dev → Plan (node tools/project.mjs notes notes)' : 'no unread notes'}; ${drafts === null ? 'news not checked' : drafts + ' news draft(s) waiting for the user'}`);
  if (!brief) {
    const st = (c) => sh(`node tools/${c} status`).split('\n').pop();
    out.push(`World store: ${st('studio.mjs') || 'did not answer'}`);
    out.push(`UI store: ${st('uikit.mjs') || 'did not answer'}`);
  }
  // Problems and records.
  const known = readFileSync('docs/records/known-issues.md', 'utf8').split('## Closed')[0];
  const open = [...known.matchAll(/^\| (KI-\d+) \|/gm)].map((m) => m[1]);
  const today = new Date().toISOString().slice(0, 10);
  const incidents = existsSync('docs/records/incidents') ? readdirSync('docs/records/incidents').filter((f) => f.startsWith(today)).length : 0;
  out.push(`Problems: ${open.length} open known issue(s)${brief ? '' : ' (' + open.join(', ') + ')'}; ${incidents} incident(s) recorded today`);
  const check = spawnSync('node', ['tools/plan.mjs', 'check'], { encoding: 'utf8' });
  out.push(`Plan files: ${check.status === 0 ? 'agree (PLAN.md, the Charter, CLAUDE.md match the data)' : check.stdout.trim()}`);
  console.log(out.join('\n'));
}

if (!cmd || cmd === 'help') {
  console.log('node tools/project.mjs <command> … (docs/playbook/ says when to use each)\n');
  for (const c of COMMANDS) console.log(`  ${c.name.padEnd(9)} ${c.summary}\n  ${''.padEnd(9)} usage: ${c.usage}`);
} else if (cmd === 'status') {
  await status(rest.includes('--brief'));
} else {
  const c = COMMANDS.find((x) => x.name === cmd);
  if (!c) { console.log(`no command "${cmd}" (node tools/project.mjs help)`); process.exit(2); }
  if (!existsSync(c.run[0])) { console.log(`"${cmd}" isn't built yet (${c.run[0]})`); process.exit(2); }
  const r = spawnSync('node', [...c.run, ...rest], { stdio: 'inherit' });
  process.exit(r.status ?? 1);
}
