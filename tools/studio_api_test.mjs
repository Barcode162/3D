// The world store (studio/api.php, docs/archive/STUDIO.md S1.4) with the bar's
// account.php on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/studio, <root>/private). Checks: nothing live
// at first; the owner saves a draft and publishes; the bundle is public and
// cached forever; a publish from an old base is refused; Claude (the hub's
// key) publishes too; history and restore; strangers are kept out; bad and
// oversized bundles are refused.   Usage: node tools/studio_api_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, copyFileSync, rmSync, writeFileSync, readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const PORT = 8797, base = `http://127.0.0.1:${PORT}`;
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };
const root = mkdtempSync(join(tmpdir(), 'studio-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
mkdirSync(join(root, 'home/studio'), { recursive: true });
mkdirSync(join(root, 'private/hub'), { recursive: true });
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
copyFileSync('studio/api.php', join(root, 'home/studio/api.php'));
const KEY = 'c'.repeat(48);
writeFileSync(join(root, 'private/hub/key.json'), JSON.stringify({ hash: createHash('sha256').update(KEY).digest('hex') }));
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore' });
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
const post = async (path, body, headers = {}) => {
  const r = await fetch(base + path, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', ...headers } });
  return { status: r.status, ...(await r.json()) };
};
const api = (body, headers) => post('/studio/api.php', body, headers);
const read = (f) => JSON.parse(readFileSync(f, 'utf8'));
const world = { format: 1, files: { 'places/bar.json': read('assets/world/places/bar.json'), 'kinds/bar.json': read('assets/world/kinds/bar.json'), 'catalog.json': read('assets/world/catalog.json') } };

try {
  await wait(700);
  const year = new Date().getFullYear();
  const owner = await post('/bar/account.php', { a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${year - 30}-03-04` });
  const other = await post('/bar/account.php', { a: 'signup', user: 'stranger', pass: 'secret2', birth: `${year - 20}-03-04` });
  check(owner.ok === 1 && other.ok === 1, 'two accounts (the first is the owner)');

  const live0 = await (await fetch(base + '/studio/api.php?live')).json();
  check(live0.ok === 1 && live0.version === 0, 'nothing live at first (version 0: the built-in world)');
  check((await api({ a: 'draft', token: other.token })).status === 403, 'a stranger is kept out');
  check((await api({ a: 'draft' })).status === 401, 'no login, no studio');

  const saved = await api({ a: 'save', token: owner.token, base: 0, bundle: world });
  check(saved.ok === 1, 'the owner saves a draft');
  const draft = await api({ a: 'draft', token: owner.token });
  check(draft.draft && draft.draft.base === 0 && draft.draft.files['places/bar.json'].things.length === world.files['places/bar.json'].things.length, 'the draft comes back whole');
  const pub = await api({ a: 'publish', token: owner.token, base: 0, note: 'The bar as it is' });
  check(pub.ok === 1 && pub.live.version === 1 && pub.live.author === 'Josh', 'publishing makes version 1');
  check((await api({ a: 'draft', token: owner.token })).draft === null, 'the draft is gone once published');

  const r = await fetch(base + '/studio/api.php?bundle=1');
  const b1 = await r.json();
  check(b1.version === 1 && b1.files['kinds/bar.json'].kinds.length > 10, 'anyone can read version 1');
  check(/immutable/.test(r.headers.get('cache-control') || ''), 'a version is cached forever (it never changes)');
  check((await (await fetch(base + '/studio/api.php?live')).json()).version === 1, 'live says 1');

  const moved = JSON.parse(JSON.stringify(world));
  moved.files['places/bar.json'].things[0].at = [9, 9, 9];
  const late = await api({ a: 'publish', token: owner.token, base: 0, bundle: moved });
  check(late.status === 409 && late.live.version === 1, 'a publish from an old version is refused (update your draft first)');
  const claude = await api({ a: 'publish', base: 1, bundle: moved, note: 'Claude: moved a thing' }, { 'X-Hub-Key': KEY });
  check(claude.ok === 1 && claude.live.version === 2 && claude.live.author === 'Claude', 'Claude publishes with the key (version 2)');
  check((await api({ a: 'draft' }, { 'X-Hub-Key': 'd'.repeat(48) })).status === 401, 'a wrong key is refused');

  const hist = await api({ a: 'history', token: owner.token });
  check(hist.versions.length === 2 && hist.versions[0].version === 2 && hist.versions[1].note === 'The bar as it is', 'history lists both, newest first');
  const back = await api({ a: 'restore', token: owner.token, v: 1 });
  check(back.ok === 1 && back.live.version === 3 && back.live.note === 'Back to version 1', 'restore puts version 1 back as version 3');
  const b3 = await (await fetch(base + '/studio/api.php?bundle=live')).json();
  check(JSON.stringify(b3.files['places/bar.json'].things[0].at) !== '[9,9,9]', 'version 3 is version 1 again');

  check((await api({ a: 'save', token: owner.token, base: 3, bundle: { files: { '../evil.php': {} } } })).status === 400, 'unknown file names are refused');
  check((await api({ a: 'save', token: owner.token, base: 3, bundle: { files: { 'kinds/bar.json': {} } } })).status === 400, 'a world without the bar is refused');
  check((await api({ a: 'save', token: owner.token, base: 3, bundle: { format: 9, files: world.files } })).status === 400, 'a newer format is refused');
  const huge = { format: 1, files: { 'places/bar.json': { things: [], pad: 'x'.repeat(2100000) } } };
  check([400, 413].includes((await api({ a: 'save', token: owner.token, base: 3, bundle: huge })).status), 'an oversized world is refused');
} catch (e) {
  console.log('ERROR ' + (e && e.stack || e)); failed++;
} finally {
  php.kill();
  rmSync(root, { recursive: true, force: true });
}
console.log(failed ? `STUDIO API TEST FAILED (${failed})` : 'STUDIO API TEST PASSED');
process.exit(failed ? 1 : 0);
