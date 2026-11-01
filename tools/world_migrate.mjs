#!/usr/bin/env node
// Changes the world's files (assets/world/) by a named migration, the same
// way every time (docs/archive/AUTOMATION.md):
//   node tools/world_migrate.mjs <name> [--dry]
// A migration is tools/migrations/<name>.mjs exporting `migrate(file, json)`
// that changes `json` in place and returns a list of what it changed (plain
// words). Every world file is passed (kinds/*.json, places/*.json, the rest),
// then reformatted with tools/place_format.py. Migrations are kept, so how
// the world got the way it is stays readable.
import { readdirSync, readFileSync, writeFileSync, existsSync } from 'node:fs';
import { execSync } from 'node:child_process';
import { join } from 'node:path';

const [name, flag] = process.argv.slice(2);
if (!name) { console.log('usage: node tools/world_migrate.mjs <name> [--dry]\nmigrations: ' + readdirSync('tools/migrations').join(', ')); process.exit(2); }
const { migrate } = await import(`./migrations/${name}.mjs`);
const files = [];
for (const dir of ['assets/world', 'assets/world/kinds', 'assets/world/places']) {
  for (const f of readdirSync(dir)) if (f.endsWith('.json') && f !== 'schema.json') files.push(join(dir, f));
}
let total = 0;
for (const f of files) {
  const json = JSON.parse(readFileSync(f, 'utf8'));
  const changes = migrate(f, json) ?? [];
  if (!changes.length) continue;
  total += changes.length;
  console.log(`${f}: ${changes.length} change(s)\n  ` + changes.join('\n  '));
  if (flag !== '--dry') {
    writeFileSync(f, JSON.stringify(json));
    execSync(`python3 tools/place_format.py ${f}`);
    // A place written as a list in words (J118.1: the bar) keeps its words the source: they're written again from the JSON.
    { const m = /places\/([a-z0-9_-]+)\.json$/.exec(f); if (m && existsSync(`assets/world/places/${m[1]}.looped`) && /^\s*kinds from /m.test(readFileSync(`assets/world/places/${m[1]}.looped`, 'utf8'))) execSync(`node tools/place_words.mjs ${m[1]} --force`, { stdio: 'inherit' }); }
  }
}
console.log(`${total} change(s)${flag === '--dry' ? ' (dry run, nothing written)' : ''}`);
