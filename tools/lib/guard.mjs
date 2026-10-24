#!/usr/bin/env node
// Claude Code's locks (restructure J4.5; .claude/settings.json → PreToolUse).
// Reads the tool call (JSON on stdin); exit 2 with the reason refuses it.
//   Bash:       git push to main (only the ship moves main: node tools/project.mjs
//               ship), any force push, --no-verify, turning the git locks off.
//   Edit|Write: files only a tool writes (the plan's data and PLAN.md: tools/plan.mjs;
//               the theme CSS: tools/uikit.mjs css).
import { readFileSync } from 'node:fs';
import { relative, resolve } from 'node:path';

let call = {};
try { call = JSON.parse(readFileSync(0, 'utf8') || '{}'); } catch { process.exit(0); }
const input = call.tool_input || {};
const refuse = (why) => { process.stderr.write(`Refused by the project's lock (tools/lib/guard.mjs): ${why}\n`); process.exit(2); };

if (call.tool_name === 'Bash') {
  const cmd = String(input.command || '');
  for (const part of cmd.split(/&&|\|\||;|\n|\|/)) {
    // Only a git command itself (after env settings and shell words), not text that mentions git.
    const words = part.trim().split(/\s+/);
    while (words.length && /^(\w+=\S*|[({]|then|do|else|sudo|command|exec|env|time)$/.test(words[0])) words.shift();
    if (words[0] !== 'git') continue;
    const args = words.slice(1);
    while (/^-[Cc]$/.test(args[0])) args.splice(0, 2);
    if (args.includes('--no-verify') || (args[0] === 'commit' && args.includes('-n'))) refuse('--no-verify skips the locks; fix what they refuse instead.');
    if (args[0] === 'config' && args.some((a) => /hooksPath/i.test(a)) && !args.includes('tools/githooks')) refuse('the git locks stay on (core.hooksPath = tools/githooks).');
    if (!args.includes('push')) continue;
    if (args.some((a) => /^(--force|--force-with-lease|-f|--mirror|--delete|-d)(=|$)/.test(a) || /^\+/.test(a) || /^:/.test(a))) refuse('no force pushes and no deleting branches (docs/charter/charter.md).');
    if (args.some((a) => /(^|:)(refs\/heads\/)?main$/.test(a))) refuse('main moves only through the ship: node tools/project.mjs ship "<what>" (docs/playbook/job.md).');
  }
} else if (call.tool_name === 'Edit' || call.tool_name === 'Write' || call.tool_name === 'MultiEdit' || call.tool_name === 'NotebookEdit') {
  const root = process.env.CLAUDE_PROJECT_DIR || process.cwd();
  const file = relative(root, resolve(root, String(input.file_path || input.notebook_path || '')));
  const owner = { 'PLAN.md': 'node tools/plan.mjs', 'docs/plan/plan.json': 'node tools/plan.mjs', 'docs/charter/rules.json': 'node tools/plan.mjs rule', 'ui/themes.css': 'node tools/uikit.mjs css' }[file];
  if (owner) refuse(`${file} is written only by ${owner}; never by hand.`);
}
process.exit(0);
