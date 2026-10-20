// The private bar's door (server/gate.php + account.php, docs/LOOPED.md
// A1-A3) on PHP's built-in server, with a router that does what the bar's
// .htaccess does. Checks: nothing without a login; the first account (the
// owner) gets in; a new account waits until invited; People lists and
// changes who may play; only the owner picks admins; files stream with the
// right type, Range (206), ETag (304) and gzip; no way around the gate to
// the private folders; the hub's Claude key opens it; sign out closes it.
// Then the sign-in page itself in Chromium: log in → the bar loads.
// Usage: NODE_PATH=$(npm root -g) node tools/gate_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, copyFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { gunzipSync } from 'node:zlib';
import { createRequire } from 'node:module';

const root = mkdtempSync(join(tmpdir(), 'gate-'));
const site = join(root, 'home', 'bar');
const hub = join(root, 'private', 'hub');
mkdirSync(site, { recursive: true });
mkdirSync(hub, { recursive: true });
for (const f of ['account.php', 'gate.php', 'gate.html']) copyFileSync(resolve('server', f), join(site, f));
writeFileSync(join(site, 'index.html'), '<!doctype html><title>The bar</title><p id=bar>THE BAR</p>');
writeFileSync(join(site, 'app.js'), 'console.log("bar");\n'.repeat(400));
mkdirSync(join(site, 'assets'));
const song = Buffer.from(Array.from({ length: 5000 }, (_, i) => i % 251));
writeFileSync(join(site, 'assets', 'song.mp3'), song);
writeFileSync(join(site, 'version.txt'), 'abc1234\n');
const KEY = 'claude-test-key-0123456789';
writeFileSync(join(hub, 'key.json'), JSON.stringify({ hash: createHash('sha256').update(KEY).digest('hex') }));
// What .htaccess does: the servers and version.txt as they are, the rest through the gate.
writeFileSync(join(root, 'router.php'), `<?php
$p = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);
if (preg_match('#^/bar/(account|report|relay|gate)\\.php$#', $p, $m)) { chdir(__DIR__ . '/home/bar'); require __DIR__ . '/home/bar/' . $m[1] . '.php'; return true; }
if ($p === '/bar/version.txt') { readfile(__DIR__ . '/home/bar/version.txt'); return true; }
$_SERVER['SCRIPT_NAME'] = '/bar/gate.php';
require __DIR__ . '/home/bar/gate.php';
`);

const port = 18000 + Math.floor(Math.random() * 1000);
const php = spawn('php', ['-S', `127.0.0.1:${port}`, join(root, 'router.php')], { cwd: root, env: { ...process.env, HUB_DATA: hub }, stdio: ['ignore', 'ignore', 'pipe'] });
let phpErr = '';
php.stderr.on('data', (d) => { phpErr += d; });
const base = `http://127.0.0.1:${port}/bar/`;

let failed = 0;
function check(ok, what) { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; }
const wait = (ms) => new Promise((r) => setTimeout(r, ms));

// A tiny cookie jar per person.
function person() {
  const jar = {};
  const p = {
    jar,
    async get(path, headers = {}) {
      const cookie = Object.entries(jar).map(([k, v]) => `${k}=${v}`).join('; ');
      const r = await fetch(base + path, { headers: { ...(cookie ? { Cookie: cookie } : {}), ...headers } });
      p.keep(r);
      return r;
    },
    async api(body) {
      const cookie = Object.entries(jar).map(([k, v]) => `${k}=${v}`).join('; ');
      const r = await fetch(base + 'account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain', ...(cookie ? { Cookie: cookie } : {}) }, body: JSON.stringify({ token: p.token, ...body }) });
      p.keep(r);
      const j = await r.json();
      if (j.token) p.token = j.token;
      return j;
    },
    keep(r) {
      for (const c of r.headers.getSetCookie()) {
        const [kv] = c.split(';');
        const [k, v] = kv.split('=');
        if (/expires=Thu, 01 Jan 1970|Max-Age=0/i.test(c) || v === '' || v === 'deleted') delete jar[k]; else jar[k] = v;
      }
    },
  };
  return p;
}

try {
  for (let i = 0; i < 50; i++) { try { await fetch(base + 'version.txt'); break; } catch { await wait(100); } }

  const stranger = person();
  let r = await stranger.get('');
  let body = await r.text();
  check(r.status === 401 && body.includes('Come in') && body.includes('"mode":"signin"'), `no login → the sign-in page (${r.status})`);
  r = await stranger.get('app.js');
  check(r.status === 401 && !(await r.text()).includes('console.log'), 'no login → no game files either');
  r = await stranger.get('version.txt');
  check(r.status === 200 && (await r.text()).trim() === 'abc1234', 'version.txt stays open (build checks)');

  const josh = person();
  let j = await josh.api({ a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: '2000-01-02' });
  check(j.ok && j.me.play === 1 && j.me.admin === 1 && josh.jar.bar_t === j.token, 'the first account is the owner: may play, admin, gets the cookie');
  r = await josh.get('');
  check(r.status === 200 && (await r.text()).includes('THE BAR'), 'the owner gets the bar');

  const mia = person();
  j = await mia.api({ a: 'signup', user: 'mia', pass: 'secret2', name: 'Mia', birth: '2001-03-04' });
  check(j.ok && j.me.play === 0 && j.me.admin === 0, 'a new account may not play yet');
  r = await mia.get('');
  body = await r.text();
  check(r.status === 401 && body.includes('"mode":"waiting"'), 'a new account sees "waiting for an invite"');
  j = await mia.api({ a: 'people' });
  check(!j.ok, 'a non-admin can\'t list people');
  j = await mia.api({ a: 'invite', user: 'mia', on: 1 });
  check(!j.ok, 'nobody invites themselves');

  j = await josh.api({ a: 'people' });
  const st = Object.fromEntries((j.people || []).map((p) => [p.user, p.status]));
  check(j.ok && st.josh === 'owner' && st.mia === 'waiting', `People: josh owner, mia waiting (${JSON.stringify(st)})`);
  j = await josh.api({ a: 'invite', user: 'mia', on: 1 });
  check(j.ok, 'the owner invites mia');
  r = await mia.get('');
  check(r.status === 200, 'mia gets in once invited');
  j = await mia.api({ a: 'me' });
  check(j.me.play === 1, 'her profile says play: 1');

  // Files: types, Range, ETag, gzip.
  r = await mia.get('assets/song.mp3', { Range: 'bytes=10-19' });
  const part = Buffer.from(await r.arrayBuffer());
  check(r.status === 206 && r.headers.get('content-range') === 'bytes 10-19/5000' && part.equals(song.subarray(10, 20)), `Range → 206 with the right bytes (${r.status} ${r.headers.get('content-range')})`);
  r = await mia.get('assets/song.mp3', { Range: 'bytes=-100' });
  check(r.status === 206 && Buffer.from(await r.arrayBuffer()).equals(song.subarray(4900)), 'Range from the end');
  r = await mia.get('assets/song.mp3');
  const etag = r.headers.get('etag');
  check(r.status === 200 && r.headers.get('content-type') === 'audio/mpeg' && r.headers.get('accept-ranges') === 'bytes', 'mp3 type + Accept-Ranges');
  await r.arrayBuffer();
  r = await mia.get('assets/song.mp3', { 'If-None-Match': etag });
  check(r.status === 304, 'same ETag → 304');
  // fetch() unzips by itself; ask with a raw request to see the header.
  const raw = await new Promise((res) => {
    import('node:http').then(({ get }) => get(base + 'app.js', { headers: { Cookie: `bar_t=${mia.jar.bar_t}`, 'Accept-Encoding': 'gzip' } }, (x) => {
      const parts = []; x.on('data', (d) => parts.push(d)); x.on('end', () => res({ x, buf: Buffer.concat(parts) }));
    }));
  });
  check(raw.x.headers['content-encoding'] === 'gzip' && gunzipSync(raw.buf).toString().startsWith('console.log'), 'scripts are sent gzipped');

  // No way around the gate.
  for (const p of ['accounts/names.json', '../bar/accounts/names.json', '%2e%2e/bar/accounts/names.json', '.htaccess', 'gate.php.bak', 'account.php/../accounts/names.json']) {
    r = await mia.get(p);
    const t = await r.text();
    check(r.status === 404 && !t.includes('josh'), `blocked: ${p} (${r.status})`);
  }

  // Admins: only the owner chooses them.
  j = await mia.api({ a: 'admin', user: 'mia', on: 1 });
  check(!j.ok, 'an invited player can\'t make admins');
  j = await josh.api({ a: 'admin', user: 'mia', on: 1 });
  check(j.ok, 'the owner makes mia an admin');
  j = await mia.api({ a: 'people' });
  check(j.ok, 'an admin sees People');
  j = await mia.api({ a: 'admin', user: 'josh', on: 0 });
  check(!j.ok, 'an admin can\'t remove the owner');
  j = await josh.api({ a: 'admin', user: 'josh', on: 0 });
  check(!j.ok, 'the owner stays an admin');
  r = await mia.get('gate.php?people');
  check(r.status === 200 && (await r.text()).includes('"mode":"people"'), 'gate.php?people opens for an admin');
  j = await josh.api({ a: 'admin', user: 'mia', on: 0 });
  j = await josh.api({ a: 'invite', user: 'mia', on: 0 });
  r = await mia.get('');
  check(r.status === 401, 'removed → back to waiting');

  // Claude's checks.
  const claude = person();
  r = await claude.get('', { 'X-Hub-Key': KEY });
  check(r.status === 200 && claude.jar.bar_k === KEY, 'the hub\'s Claude key opens the gate (and keeps a cookie)');
  r = await claude.get('app.js');
  check(r.status === 200, '…for the rest of the files too');
  r = await person().get('', { 'X-Hub-Key': 'wrong' });
  check(r.status === 401, 'a wrong key does not');

  // Sign out.
  j = await josh.api({ a: 'logout' });
  check(j.ok && !josh.jar.bar_t, 'logout clears the cookie');
  r = await josh.get('');
  check(r.status === 401, 'signed out → the sign-in page');

  // The page in a browser: sign in → reload → the bar.
  const require = createRequire(import.meta.url);
  let chromium = null;
  try { ({ chromium } = require('playwright')); } catch { console.log('(playwright missing: browser part skipped)'); }
  if (chromium) {
    const { launchBrowser } = await import('./lib/browser.mjs');
    const browser = await launchBrowser();
    const page = await browser.newPage({ viewport: { width: 430, height: 932 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(String(e)));
    await page.goto(base);
    await page.fill('#lUser', 'josh');
    await page.fill('#lPass', 'nope00');
    await page.click('#fLogin .go');
    await page.waitForFunction(() => /Wrong/.test(document.getElementById('msg').textContent), null, { timeout: 5000 });
    check(true, 'browser: a wrong password says so');
    await page.fill('#lPass', 'secret1');
    await page.click('#fLogin .go');
    await page.waitForSelector('#bar', { timeout: 8000 }).catch(() => {});
    check(await page.$('#bar') !== null, 'browser: signing in loads the bar');
    check(await page.evaluate(() => /^[0-9a-f]{48}$/.test(localStorage.getItem('bar3dToken') || '')), 'browser: the game\'s login is set too');
    await page.screenshot({ path: 'build/gate-in.png' }).catch(() => {});
    const p2 = await browser.newPage({ viewport: { width: 430, height: 932 } });
    await p2.goto(base);
    await p2.waitForTimeout(600);
    await p2.screenshot({ path: 'build/gate.png' }).catch(() => {});
    await p2.click('[data-show=signup]');
    await p2.fill('#sUser', 'zed');
    await p2.fill('#sPass', 'secret3');
    await p2.fill('#sBirth', '1999-09-09');
    await p2.click('#fSignup .go');
    await p2.waitForSelector('#waiting.on', { timeout: 5000 }).catch(() => {});
    check(await p2.$('#waiting.on') !== null, 'browser: a new account lands on "waiting for an invite"');
    await p2.screenshot({ path: 'build/gate-waiting.png' }).catch(() => {});
    await page.goto(base + 'gate.php?people');
    await page.waitForSelector('.person', { timeout: 5000 }).catch(() => {});
    const names = await page.$$eval('.person b', (b) => b.map((x) => x.textContent));
    check(names.some((n) => n.startsWith('zed')), `browser: People lists the new account (${names.join(', ')})`);
    await page.locator('.person', { hasText: '@zed' }).locator('.act:not(.off)').click();
    await page.waitForTimeout(800);
    await page.screenshot({ path: 'build/gate-people.png' }).catch(() => {});
    await p2.click('#again');
    await p2.waitForSelector('#bar', { timeout: 8000 }).catch(() => {});
    check(await p2.$('#bar') !== null, 'browser: invited from People → "Check again" lets them in');
    check(errors.length === 0, `browser: no page errors (${errors.join(' | ')})`);
    await browser.close();
  }
} catch (e) {
  check(false, 'test crashed: ' + (e.stack || e));
} finally {
  php.kill();
}
if (/Fatal|Warning|Parse error/.test(phpErr)) { console.log(phpErr.split('\n').filter((l) => /Fatal|Warning|Parse/.test(l)).slice(0, 8).join('\n')); check(false, 'PHP printed no warnings'); }
if (failed) { console.log(`GATE TEST FAILED (${failed})`); process.exit(1); }
console.log('GATE TEST PASSED');
process.exit(0);
