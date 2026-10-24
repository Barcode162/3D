// The UI store (toolbelt/ui-editor/api.php, docs/UI_PLAN.md R1.7) with the
// bar's account.php on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/toolbelt/ui-editor, <root>/private). Checks:
// nothing live at first; the owner publishes; the version is public and cached
// forever; an old base is refused; Claude (the hub's key) publishes; history;
// strangers are kept out; files that aren't themes or screens are refused.
// Usage: node tools/ui_api_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, copyFileSync, rmSync, writeFileSync, readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createServer } from 'node:net';

const PORT = await new Promise((r) => { const s = createServer().listen(0, () => { const p = s.address().port; s.close(() => r(p)); }); }), base = `http://127.0.0.1:${PORT}`;
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };
const root = mkdtempSync(join(tmpdir(), 'uistore-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
mkdirSync(join(root, 'home/toolbelt/ui-editor'), { recursive: true });
mkdirSync(join(root, 'private/hub'), { recursive: true });
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
copyFileSync('toolbelt/ui-editor/api.php', join(root, 'home/toolbelt/ui-editor/api.php'));
const KEY = 'c'.repeat(48);
writeFileSync(join(root, 'private/hub/key.json'), JSON.stringify({ hash: createHash('sha256').update(KEY).digest('hex') }));
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore' });
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
const post = async (path, body, headers = {}) => {
  const r = await fetch(base + path, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', ...headers } });
  return { status: r.status, ...(await r.json()) };
};
const API = '/toolbelt/ui-editor/api.php';
const api = (body, headers) => post(API, body, headers);
const read = (f) => JSON.parse(readFileSync(f, 'utf8'));
const bundle = { format: 1, files: { 'themes/faceplate.json': read('ui/system/themes/faceplate.json'), 'screens/sampler.json': read('ui/system/screens/sampler.json') } };

try {
  await wait(700);
  const year = new Date().getFullYear();
  const owner = await post('/bar/account.php', { a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${year - 30}-03-04` });
  const other = await post('/bar/account.php', { a: 'signup', user: 'stranger', pass: 'secret2', birth: `${year - 20}-03-04` });
  check(owner.ok === 1 && other.ok === 1, 'two accounts (the first is the owner)');
  const live0 = await (await fetch(base + API + '?live')).json();
  check(live0.ok === 1 && live0.version === 0, 'nothing published at first');
  check((await api({ a: 'me', token: other.token })).status === 403, 'a stranger is kept out');
  check((await api({ a: 'me' })).status === 401, 'no login, no editor');
  const me = await api({ a: 'me', token: owner.token });
  check(me.ok === 1 && me.role === 'owner' && me.live.version === 0, 'the owner is let in');
  const pub = await api({ a: 'publish', token: owner.token, base: 0, note: 'Rounder keys', bundle });
  check(pub.ok === 1 && pub.live.version === 1 && pub.live.author === 'Josh', 'publishing makes version 1');
  const r = await fetch(base + API + '?bundle=1');
  const b1 = await r.json();
  check(b1.version === 1 && b1.files['themes/faceplate.json'].tokens.accent === '#F28C2C', 'anyone can read version 1');
  check(/immutable/.test(r.headers.get('cache-control') || ''), 'a version is cached forever');
  check((await (await fetch(base + API + '?bundle=live')).json()).version === 1, '?bundle=live is version 1');
  check((await api({ a: 'publish', token: owner.token, base: 0, bundle })).status === 409, 'a publish from an old base is refused');
  const claude = await api({ a: 'publish', base: 1, note: 'From the repo', bundle }, { 'X-Hub-Key': KEY });
  check(claude.ok === 1 && claude.live.version === 2 && claude.live.author === 'Claude', 'Claude publishes version 2 with the hub key');
  const hist = await api({ a: 'history', token: owner.token });
  check(hist.versions.length === 2 && hist.versions[0].version === 2 && hist.versions[1].note === 'Rounder keys', 'history lists both, newest first');
  const bad = await api({ a: 'publish', token: owner.token, base: 2, bundle: { format: 1, files: { '../../evil.json': {} } } });
  check(bad.status === 400, 'a file that is not a theme or screen is refused');
  const wrongId = await api({ a: 'publish', token: owner.token, base: 2, bundle: { format: 1, files: { 'themes/x.json': { ...bundle.files['themes/faceplate.json'] } } } });
  check(wrongId.status === 400, 'a theme whose id differs from its file name is refused');
} finally {
  php.kill();
  rmSync(root, { recursive: true, force: true });
}
console.log(failed ? `${failed} FAILED` : 'UI STORE TEST PASSED');
process.exit(failed ? 1 : 0);
