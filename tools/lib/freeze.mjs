// The freeze's lock (J7.1, after incident 2026-10-24-freeze-unlocked): while
// the user's "Freeze" rule stands (docs/charter/rules.json), a commit may only
// change the files docs/archive/restructure.md §7 allows. The user's own changes
// pulled from the stores ("World: …", "UI: …") and merges pass; an urgent fix
// for something live passes with FREEZE_OK="<why>" (docs/playbook/ways.md).
// Run by tools/githooks/commit-msg: node tools/lib/freeze.mjs <message file>
import { readFileSync } from 'node:fs';
import { execSync } from 'node:child_process';

export const ALLOWED = [/^docs\//, /^tools\//, /^\.github\/workflows\//, /^\.claude\//, /^CLAUDE\.md$/, /^README\.md$/, /^NEXT\.md$/, /^PLAN\.md$/,
  /^dev\//, /^hub\//, /^homepage\/projects\//, /^toolbelt\/tools\.json$/, /^bar\/test_hooks\./];
export const frozenOn = (rules) => rules.rules.some((r) => /^\*\*Freeze:/.test(r.text));
// The files a commit may not change while the freeze holds ([] = fine).
export function outside(names, message) {
  if (/^(World|UI): |^Merge /.test(message)) return [];
  return names.filter((n) => n && !ALLOWED.some((re) => re.test(n)));
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const rules = JSON.parse(readFileSync('docs/charter/rules.json', 'utf8'));
  if (!frozenOn(rules)) process.exit(0);
  const message = readFileSync(process.argv[2], 'utf8');
  const bad = outside(execSync('git diff --cached --name-only', { encoding: 'utf8' }).split('\n'), message);
  if (bad.length && process.env.FREEZE_OK) { console.log(`(outside the freeze, allowed as: ${process.env.FREEZE_OK}) ${bad.join(', ')}`); process.exit(0); }
  if (bad.length) {
    console.log(`✗ commit refused: the freeze holds (docs/archive/restructure.md §7) and this changes ${bad.slice(0, 8).join(', ')}${bad.length > 8 ? ` and ${bad.length - 8} more` : ''}. Take them out (git restore --staged <file>), or, for an urgent fix of something live, FREEZE_OK="<why>".`);
    process.exit(1);
  }
}
