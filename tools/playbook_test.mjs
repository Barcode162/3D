// The Playbook names only what exists (restructure J4.6). In docs/playbook/, the Handbook's recipes,
// CLAUDE.md and docs/plan/jobs/TEMPLATE.md: every `node tools/<x>.mjs` is a
// file; every `project.mjs <command>` is in tools/lib/commands.mjs; every
// `<tool>.mjs <word>` names a command that tool has; every link leads to a
// file; no "(J<n>)" marker is left for a job that has already started.
// Usage: node tools/playbook_test.mjs
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { COMMANDS } from './lib/commands.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const docs = [...readdirSync('docs/playbook').filter((f) => f.endsWith('.md')).map((f) => `docs/playbook/${f}`), 'CLAUDE.md', 'docs/plan/jobs/TEMPLATE.md', 'docs/handbook/recipes.md'];
const now = +((JSON.parse(readFileSync('docs/plan/plan.json', 'utf8')).now?.id || '').match(/^J(\d+)$/) || [0, 0])[1];
const sources = {};
const source = (f) => (sources[f] ??= readFileSync(f, 'utf8'));
const bad = { tools: [], commands: [], words: [], links: [], marks: [] };

for (const doc of docs) {
  const text = readFileSync(doc, 'utf8').replace(/\n\s*/g, ' ');  // a command may wrap across lines
  for (const [, tool, word] of text.matchAll(/node tools\/([\w/]+\.mjs)(?: ([a-z][\w-]*))?/g)) {
    const file = `tools/${tool}`;
    if (!existsSync(file)) { bad.tools.push(`${doc}: ${file}`); continue; }
    if (tool === 'project.mjs') { if (word && !COMMANDS.some((c) => c.name === word)) bad.commands.push(`${doc}: project.mjs ${word}`); continue; }
    if (word && !new RegExp(`['"\`]${word}['"\`]`).test(source(file))) bad.words.push(`${doc}: ${tool} ${word}`);
  }
  for (const [, link] of text.matchAll(/\]\(([^)#\s]+)(?:#[^)]*)?\)/g)) {
    if (/^(https?:|mailto:)/.test(link)) continue;
    if (!existsSync(join(dirname(doc), link))) bad.links.push(`${doc}: ${link}`);
  }
  for (const [, n] of text.matchAll(/\(J(\d+)\)\*\*/g)) if (+n <= now) bad.marks.push(`${doc}: (J${n})`);
}
check(!bad.tools.length, 'every tool the Playbook names exists' + (bad.tools.length ? ': ' + bad.tools.join(', ') : ''));
check(!bad.commands.length, 'every front-door command it names is in tools/lib/commands.mjs' + (bad.commands.length ? ': ' + bad.commands.join(', ') : ''));
check(!bad.words.length, 'every tool command it names is one the tool has' + (bad.words.length ? ': ' + bad.words.join(', ') : ''));
check(!bad.links.length, 'every link leads to a file' + (bad.links.length ? ': ' + bad.links.join(', ') : ''));
check(!bad.marks.length, 'no "arrives with J<n>" marker for a job that has started' + (bad.marks.length ? ': ' + bad.marks.join(', ') : ''));
for (const c of COMMANDS) check(c.own || existsSync(c.run[0]), `front-door command "${c.name}" runs a tool that exists`);
console.log(failed ? `PLAYBOOK TEST: ${failed} FAILED` : 'PLAYBOOK TEST PASSED');
process.exit(failed ? 1 : 0);
