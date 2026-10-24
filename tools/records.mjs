#!/usr/bin/env node
// The Records (restructure J4.4; docs/records/README.md), in their fixed shapes:
//   node tools/records.mjs request "<the user's words>" --understood "<one line>"
//        → docs/records/requests/<YYYY-MM>.md, the next id of the day (prints it)
//   node tools/records.mjs incident <slug> "<what happened>"
//        → docs/records/incidents/<date>-<slug>.md with the five parts to fill
//   node tools/records.mjs decision <slug> "<the decision>"
//        → docs/records/decisions/<NNNN>-<slug>.md with its parts to fill
//   node tools/records.mjs session --asked "<request ids>" --done "<pieces>" --shipped "<builds>" [--errors "<incident files>"]
//        → one line in docs/records/sessions/<YYYY-MM>.md (the session's own link)
//   node tools/records.mjs index
//        → prints the incidents as JSON [{file, date, title, status, errors}], newest first
//          (the plan workflow puts it in Dev's store for Dev's "errors this week")
// Nothing here is ever reworded later; a correction is a new entry.
import { readFileSync, writeFileSync, existsSync, mkdirSync, readdirSync, appendFileSync } from 'node:fs';

const [cmd, ...rest] = process.argv.slice(2);
const opt = (n) => { const i = rest.indexOf('--' + n); return i >= 0 ? rest[i + 1] : ''; };
const pos = rest.filter((a, i) => !a.startsWith('--') && !(i > 0 && rest[i - 1].startsWith('--')));
const day = new Date().toISOString().slice(0, 10), month = day.slice(0, 7);
const session = (process.env.CLAUDE_CODE_REMOTE_SESSION_ID || '').replace(/^cse_/, '');
const sessionLink = session ? `[${session}](https://claude.ai/code/session_${session})` : '(no session id)';
const fail = (m) => { console.log(m); process.exit(2); };
const slug = (s) => { if (!/^[a-z0-9-]{3,60}$/.test(s || '')) fail('the slug: 3-60 lowercase letters, digits and dashes'); return s; };

if (cmd === 'request') {
  const words = pos[0], understood = opt('understood');
  if (!words || !understood) fail('request "<the user\'s words, exactly>" --understood "<what you took it to mean>"');
  const dir = 'docs/records/requests', file = `${dir}/${month}.md`;
  mkdirSync(dir, { recursive: true });
  if (!existsSync(file)) writeFileSync(file, `# Requests, ${month}\n\nThe user's words, exactly as sent, newest last. Each request gets an id\n(\`<date> #<n>\`); jobs link to the requests they come from. Nothing here is\never reworded; what Claude understood goes in the job's spec.\n`);
  const text = readFileSync(file, 'utf8');
  const n = Math.max(0, ...[...text.matchAll(new RegExp(`^## ${day} #(\\d+)(?:–#(\\d+))?`, 'gm'))].map((m) => +(m[2] || m[1]))) + 1;
  appendFileSync(file, `\n## ${day} #${n}\n\n${words.split('\n').map((l) => '> ' + l).join('\n')}\n\nUnderstood: ${understood}\n`);
  console.log(`${day} #${n}`);
} else if (cmd === 'incident') {
  const file = `docs/records/incidents/${day}-${slug(pos[0])}.md`;
  if (!pos[1]) fail('incident <slug> "<what happened>"');
  if (existsSync(file)) fail(`${file} exists already`);
  mkdirSync('docs/records/incidents', { recursive: true });
  writeFileSync(file, `# Incident ${day}: ${pos[1]}\n\n**Session:** ${sessionLink}\n**Errors counted:** 1\n**Status:** open\n\n## What happened\n${pos[1]}\n\n## Impact\n(what it affected: live, data, work, the user's time; or "nothing live")\n\n## Root cause\n(why the way we work let it through: which procedure, tool, check or doc)\n\n## Fix\n(what changed so it can't happen the same way again, with the commit)\n\n## Proof\n(the check that now fails on the old mistake, or the later step that will prove it)\n`);
  console.log(file);
} else if (cmd === 'decision') {
  if (!pos[1]) fail('decision <slug> "<the decision>"');
  const dir = 'docs/records/decisions';
  mkdirSync(dir, { recursive: true });
  const n = String(1 + Math.max(0, ...readdirSync(dir).map((f) => parseInt(f, 10)).filter((x) => x > 0))).padStart(4, '0');
  const file = `${dir}/${n}-${slug(pos[0])}.md`;
  writeFileSync(file, `# ${n} · ${pos[1]}\n\n**Date:** ${day}\n**Status:** current\n**Asked by:** (request id, or "Claude, within a job the user approved")\n\n## Context\n(what made a choice necessary)\n\n## Decision\n${pos[1]}\n\n## Options not taken\n(each, and why not)\n\n## Consequences\n(what changes because of it; what to watch)\n`);
  console.log(file);
} else if (cmd === 'session') {
  const dir = 'docs/records/sessions', file = `${dir}/${month}.md`;
  mkdirSync(dir, { recursive: true });
  if (!existsSync(file)) writeFileSync(file, `# Sessions, ${month}\n\nOne line per session that changed something (talk-only sessions leave none).\n\n| Date | Session | Asked | Done | Shipped | Errors |\n|---|---|---|---|---|---|\n`);
  const errors = opt('errors') ? opt('errors').split(',').map((f) => f.trim()).filter(Boolean) : [];
  for (const f of errors) if (!existsSync(`docs/records/incidents/${f}`)) fail(`no incident docs/records/incidents/${f}`);
  const errCell = errors.length ? `${errors.length}: ${errors.map((f) => `[${f.replace(/\.md$/, '')}](../incidents/${f})`).join(', ')}` : '0';
  appendFileSync(file, `| ${day} | ${sessionLink} | ${opt('asked') || '—'} | ${opt('done') || '—'} | ${opt('shipped') || '—'} | ${errCell} |\n`);
  console.log(`session line added to ${file}`);
} else if (cmd === 'index') {
  const dir = 'docs/records/incidents';
  const list = (existsSync(dir) ? readdirSync(dir) : []).filter((f) => /^\d{4}-\d\d-\d\d-.*\.md$/.test(f)).sort().reverse().map((f) => {
    const text = readFileSync(`${dir}/${f}`, 'utf8');
    const field = (name) => ((text.match(new RegExp(`^\\*\\*${name}:\\*\\* *(.*)$`, 'm')) || [])[1] || '').trim();
    return { file: f, date: f.slice(0, 10), title: (text.match(/^# Incident [\d-]+: (.*)$/m) || [, f])[1].trim(), status: field('Status') || 'open', errors: +field('Errors counted') || 1 };
  });
  console.log(JSON.stringify(list));
} else {
  console.log(readFileSync(new URL(import.meta.url), 'utf8').split('\n').slice(1, 15).join('\n')); process.exit(2);
}
