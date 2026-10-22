#!/usr/bin/env node
// Claude's side of the world store (docs/STUDIO.md 7.7). The live world the
// user publishes from the Studio and the repo's assets/world/ stay one world:
//   node tools/studio.mjs pull            the live world → assets/world/ (formatted); says what changed
//   node tools/studio.mjs push --note ".." the repo's world → a new live version by "Claude"
//                                          (refused if the live world moved since the last pull)
//   node tools/studio.mjs log             the versions: number, who, when, note
//   node tools/studio.mjs status          live version vs the one the repo has
// The version the repo matches is kept in studio/live-version.json (commit it).
// Needs HUB_KEY (the hub's Claude key) for push and log; STUDIO_URL overrides the store.
// "go" routine (CLAUDE.md): pull at the start; push after the step's deploy is live.
import { readFileSync, writeFileSync, readdirSync, existsSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

const URL_ = process.env.STUDIO_URL || 'https://joshuavelasco.com/studio/api.php';
const KEY = process.env.HUB_KEY || '';
const MARK = 'studio/live-version.json';
const WORLD = 'assets/world/';
const [cmd, ...rest] = process.argv.slice(2);
const flag = (name) => { const i = rest.indexOf('--' + name); return i >= 0 ? rest[i + 1] : undefined; };

const mark = () => (existsSync(MARK) ? JSON.parse(readFileSync(MARK, 'utf8')) : { version: 0 });
async function get(q) {
  const r = await fetch(URL_ + q, { cache: 'no-store' });
  if (!r.ok) throw new Error(`${q}: HTTP ${r.status}`);
  return r.json();
}
async function post(body) {
  if (!KEY) { console.log('No HUB_KEY in the environment (the hub → Account → Make a new Claude key).'); process.exit(2); }
  const r = await fetch(URL_, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY } });
  return { status: r.status, ...(await r.json()) };
}
// The world files a bundle carries (songs.json stays a site file).
function repoFiles() {
  const files = {};
  for (const dir of ['places', 'kinds'])
    for (const f of readdirSync(WORLD + dir)) if (f.endsWith('.json')) files[`${dir}/${f}`] = JSON.parse(readFileSync(`${WORLD}${dir}/${f}`, 'utf8'));
  files['catalog.json'] = JSON.parse(readFileSync(WORLD + 'catalog.json', 'utf8'));
  files['materials.json'] = JSON.parse(readFileSync(WORLD + 'materials.json', 'utf8'));
  return files;
}

if (cmd === 'status') {
  const live = await get('?live');
  console.log(`live: v${live.version} (${live.author || 'built in'}${live.note ? ': ' + live.note : ''}); repo matches v${mark().version}`);
} else if (cmd === 'pull') {
  const live = await get('?live');
  if (!live.version) { console.log('Nothing published yet: the repo\'s world is the world.'); process.exit(0); }
  if (live.version === mark().version) { console.log(`Up to date (v${live.version}).`); process.exit(0); }
  const b = await get('?bundle=' + live.version);
  const written = [];
  for (const [path, content] of Object.entries(b.files)) {
    if (!/^((places|kinds)\/[a-z0-9_-]+\.json|catalog\.json)$/.test(path)) continue;
    const file = WORLD + path;
    const before = existsSync(file) ? readFileSync(file, 'utf8') : '';
    writeFileSync(file, JSON.stringify(content, null, 2) + '\n');
    execFileSync('python3', ['tools/place_format.py', file]);
    if (readFileSync(file, 'utf8') !== before) written.push(path);
  }
  writeFileSync(MARK, JSON.stringify({ version: live.version, author: live.author, at: live.at, note: live.note }, null, 2) + '\n');
  console.log(`Pulled v${live.version} by ${live.author}${live.note ? ' ("' + live.note + '")' : ''}: ` + (written.length ? 'changed ' + written.join(', ') : 'no file changed'));
  if (written.length) console.log(`Commit it: git add assets/world ${MARK} && git commit -m "World: from the studio v${live.version} (${live.author})"`);
} else if (cmd === 'push') {
  const note = flag('note') || 'Claude';
  const base = mark().version;
  const r = await post({ a: 'publish', base, note, bundle: { format: 1, files: repoFiles() } });
  if (!r.ok) { console.log(`Refused: ${r.error}` + (r.status === 409 ? '\nPull first (node tools/studio.mjs pull), merge, test, push again.' : '')); process.exit(1); }
  writeFileSync(MARK, JSON.stringify({ version: r.live.version, author: r.live.author, at: r.live.at, note: r.live.note }, null, 2) + '\n');
  console.log(`Published v${r.live.version} ("${note}"). Commit ${MARK}.`);
} else if (cmd === 'log') {
  const r = await post({ a: 'history' });
  for (const v of r.versions || []) console.log(`v${v.version}  ${new Date(v.at * 1000).toISOString().slice(0, 16)}  ${v.author}  ${v.note || ''}`);
} else {
  console.log('Usage: node tools/studio.mjs pull | push --note ".." | log | status');
}
