// A throwaway copy of the IONOS webspace on PHP's built-in server, for the
// tests of pages that talk to PHP (Dev, the UI Editor): <root>/home is the
// site root (folders copied in), <root>/private holds the stores' data.
//   const ws = await webspace({ dirs: { ui: 'ui', dev: 'dev' }, files: { 'bar/account.php': 'server/account.php' } });
//   ws.url, ws.root, ws.post(path, body), ws.close()
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, cpSync, copyFileSync, rmSync } from 'node:fs';
import { createServer } from 'node:net';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';

const freePort = () => new Promise((r) => { const s = createServer().listen(0, () => { const p = s.address().port; s.close(() => r(p)); }); });

export async function webspace({ dirs = {}, files = {}, env = {} } = {}) {
  const root = mkdtempSync(join(tmpdir(), 'webspace-'));
  for (const [to, from] of Object.entries(dirs)) cpSync(from, join(root, 'home', to), { recursive: true });
  for (const [to, from] of Object.entries(files)) { mkdirSync(dirname(join(root, 'home', to)), { recursive: true }); copyFileSync(from, join(root, 'home', to)); }
  mkdirSync(join(root, 'private'), { recursive: true });
  const port = await freePort(), url = `http://127.0.0.1:${port}/`;
  const php = spawn('php', ['-S', `127.0.0.1:${port}`, '-t', join(root, 'home')], { stdio: 'ignore',
    env: { ...process.env, HUB_ACCOUNTS: join(root, 'home/bar/accounts'), HUB_DATA: join(root, 'private/hub'), UI_DATA: join(root, 'private/ui'), DEV_DATA: join(root, 'private/dev'), ...env } });
  let closed = false;
  const close = () => { if (closed) return; closed = true; try { php.kill(); } catch {} rmSync(root, { recursive: true, force: true }); };
  process.on('exit', close);
  await new Promise((r) => setTimeout(r, 700));
  const post = async (path, body, headers = {}) => {
    const r = await fetch(url + path, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', ...headers } });
    return { status: r.status, ...(await r.json()) };
  };
  return { url, root, post, close };
}
