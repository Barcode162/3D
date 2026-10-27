#!/usr/bin/env node
// The preflight checklist (request 2026-10-27 #2): a quick scan of the
// project before any real test, build or push. Each rule is one small file in
// tools/preflight/rules/<area>/ (copy tools/preflight/TEMPLATE.mjs): one plain
// sentence saying what passing means, which files it watches, and a check
// that names each broken spot and how to fix it. Not technical and meant to
// be easy to pass: it catches the obvious (a file changed without its
// partner, a light with nothing glowing, a model without credits) before the
// slow tests start.
//   node tools/preflight.mjs          the rules watching what changed (since origin/main, plus unsaved)
//   node tools/preflight.mjs all      every rule
//   node tools/preflight.mjs list     the rulebook: every rule, one line each
// tools/piece.mjs and tools/ship.mjs run it first; a broken rule stops them.
import { execSync } from 'node:child_process';
import { existsSync, readFileSync, readdirSync, statSync } from 'node:fs';
import { join, relative } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const root = join(fileURLToPath(import.meta.url), '..', '..');
process.chdir(root);
const arg = process.argv[2] || 'changed';
const shRaw = (c) => { try { return execSync(c, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }); } catch { return ''; } };
const sh = (c) => shRaw(c).trim();

// The rules, area by area.
const rulesDir = join(root, 'tools', 'preflight', 'rules');
const files = readdirSync(rulesDir).sort().flatMap((area) =>
  readdirSync(join(rulesDir, area)).filter((f) => f.endsWith('.mjs') && !f.startsWith('_')).sort().map((f) => join(rulesDir, area, f)));
const rules = [];
for (const f of files) {
  const r = (await import(pathToFileURL(f).href)).default;
  const name = relative(root, f);
  for (const k of ['id', 'area', 'rule', 'when', 'check'])
    if (!(k in r)) { console.log(`✗ ${name}: the rule has no "${k}" (copy tools/preflight/TEMPLATE.mjs)`); process.exit(1); }
  if (rules.some((x) => x.id === r.id)) { console.log(`✗ ${name}: the id ${r.id} is taken`); process.exit(1); }
  rules.push({ ...r, file: name });
}
if (arg === 'list') {
  for (const r of rules) console.log(`${r.id.padEnd(12)} ${r.rule}  [${r.when.join(', ')}]`);
  process.exit(0);
}

// What changed: committed since main, plus anything not committed yet.
const changed = [...new Set([
  ...sh('git diff --name-only origin/main...HEAD').split('\n'),
  // (untrimmed: each line starts with two status letters, the first may be a space)
  ...shRaw('git status --porcelain --untracked-files=all').split('\n').filter(Boolean).map((l) => l.slice(3).replace(/^.* -> /, '')),
].filter(Boolean))];
const globRe = (g) => new RegExp('^' + g.replace(/[.+^${}()|[\]\\]/g, '\\$&').replace(/\*\*/g, '\u0000').replace(/\*/g, '[^/]*').replace(/\u0000/g, '.*') + '$');
const watches = (r, f) => r.when.some((g) => globRe(g).test(f));

// What a rule may use.
const ctx = {
  changed,
  read: (p) => readFileSync(join(root, p), 'utf8'),
  json: (p) => JSON.parse(readFileSync(join(root, p), 'utf8')),
  exists: (p) => existsSync(join(root, p)),
  list: (dir) => existsSync(join(root, dir)) ? readdirSync(join(root, dir)) : [],
  isDir: (p) => existsSync(join(root, p)) && statSync(join(root, p)).isDirectory(),
  git: sh,
};

// A rule excused for this run, with why (tools/piece.mjs --docs-unchanged):
// PREFLIGHT_EXCUSE="R-DOCS-1=<why>,R-X-2=<why>".
const excused = new Map((process.env.PREFLIGHT_EXCUSE || '').split(',').filter((e) => e.includes('='))
  .map((e) => [e.slice(0, e.indexOf('=')).trim(), e.slice(e.indexOf('=') + 1).trim()]));
let broken = 0, ran = 0;
const t0 = Date.now();
for (const r of rules) {
  const touched = changed.filter((f) => watches(r, f));
  if (arg !== 'all' && !touched.length) continue;
  ran++;
  let spots;
  try { spots = (await r.check({ ...ctx, touched })) || []; }
  catch (e) { spots = [{ where: r.file, fix: `the rule itself failed: ${e.message}` }]; }
  if (!spots.length) { console.log(`✓ ${r.id}  ${r.rule}`); continue; }
  if (excused.has(r.id)) { console.log(`– ${r.id}  excused: ${excused.get(r.id)}`); continue; }
  broken++;
  console.log(`✗ ${r.id}  ${r.rule}`);
  for (const s of spots.slice(0, 12)) console.log(`    ${s.where}: ${s.fix}`);
  if (spots.length > 12) console.log(`    … and ${spots.length - 12} more`);
}
const secs = ((Date.now() - t0) / 1000).toFixed(1);
if (!ran) console.log(`preflight: no rule watches what changed (${changed.length} file(s)); ${rules.length} rules in the book`);
else console.log(broken ? `PREFLIGHT: ${broken} rule(s) broken (above): fix them before any test or push` :
  `PREFLIGHT PASSED: ${ran} of ${rules.length} rules (${secs} s)`);
process.exit(broken ? 1 : 0);
