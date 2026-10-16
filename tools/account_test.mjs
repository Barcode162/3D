// Accounts test: runs server/relay.php + server/account.php with PHP's
// built-in web server. Josh signs up through the real forms, uploads a
// profile picture and fills in his card; Mia (a guest) stands facing him,
// must see his picture and name over his head, and taps him to open his
// card. Screenshots: build/acct-card.png (Josh's own card),
// build/acct-mia.png (Mia's view), build/acct-mia-card.png (his card).
// Usage: NODE_PATH=$(npm root -g) node tools/account_test.mjs
import { createServer } from 'node:http';
import { spawn, execSync } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { mkdtempSync, copyFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');
const siteDir = resolve('build/site');
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
const site = createServer(async (req, res) => {
  let path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  if (path === '/') path = '/index.html';
  try {
    const body = await readFile(join(siteDir, path));
    res.writeHead(200, { 'content-type': types[extname(path)] ?? 'application/octet-stream' });
    res.end(body);
  } catch { res.writeHead(404).end(); }
}).listen(0);
const port = site.address().port;
const dir = mkdtempSync(join(tmpdir(), 'acct-'));
copyFileSync('server/relay.php', join(dir, 'relay.php'));
copyFileSync('server/account.php', join(dir, 'account.php'));
const php = spawn('php', ['-S', `127.0.0.1:${port + 1}`, '-t', dir], { stdio: 'ignore' });
const relay = `http://127.0.0.1:${port + 1}/relay.php`;
// A test picture: a gradient with a circle (ffmpeg makes the JPEG).
const pic = join(dir, 'me.jpg');
execSync(`ffmpeg -loglevel error -y -f lavfi -i "gradients=s=400x300:c0=0xff5ab4:c1=0x5ad1ff,format=yuv420p" -frames:v 1 ${pic}`);

const browser = await chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'],
});
const url = (cam) => `http://localhost:${port}/?skipintro&room=acct&server=${encodeURIComponent(relay)}&cam=${encodeURIComponent(cam)}`;
const waitLog = (page, re, ms = 120000) =>
  page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent), re.source,
                       { timeout: ms, polling: 250 }).then(() => true).catch(() => { console.log('timed out: ' + re); return false; });

// Josh signs up and makes his card.
const josh = await browser.newPage({ viewport: { width: 430, height: 932 } });
await josh.goto(url('0,1.66,1.0,180,0'));  // facing the front of the bar
await waitLog(josh, /Everything loaded/);
await josh.click('#acctMe');
await josh.click('text=New here? Sign up');
const inputs = josh.locator('#acctRoot input');
await inputs.nth(0).fill('josh');
await inputs.nth(1).fill('Josh');
await inputs.nth(2).fill('secret123');
await josh.click('text=Create account');
await waitLog(josh, /Account: logged in as Josh/);
await josh.setInputFiles('#acctFile', pic);
await josh.waitForFunction(() => !/Uploading/.test(document.querySelector('#acctRoot .acctErr').textContent),
                           null, { timeout: 15000 }).catch(() => {});
await josh.fill('#acctRoot textarea', 'Piano on Thursdays. Ask me about jazz.');
await josh.locator('#acctRoot input').nth(2).fill('Old Fashioned');
await josh.locator('#acctRoot input').nth(3).fill('Bossa Antigua');
await josh.click('text=Save');
await josh.waitForSelector('.acctCard', { timeout: 15000 });
await josh.screenshot({ path: 'build/acct-card.png' });
await josh.click('#acctRoot >> text=Close');

// Mia (guest) faces Josh, sees his picture, taps him.
const mia = await browser.newPage({ viewport: { width: 430, height: 932 } });
await mia.goto(url('0,1.66,3.2,0,0'));
await waitLog(mia, /Everything loaded/);
const joined = await waitLog(mia, /Josh joined/, 30000);
await mia.waitForTimeout(8000);
await mia.screenshot({ path: 'build/acct-mia.png' });
await mia.mouse.click(215, 470);
const card = await mia.waitForSelector('.acctCard', { timeout: 20000 }).then(() => true).catch(() => false);
await mia.waitForTimeout(1500);
await mia.screenshot({ path: 'build/acct-mia-card.png' });
const cardText = card ? await mia.$eval('.acctCard', (e) => e.textContent) : '';
const errors = (await mia.$$eval('#log .err', (els) => els.map((e) => e.textContent)))
  .concat(await josh.$$eval('#log .err', (els) => els.map((e) => e.textContent)));
await browser.close();
php.kill();
site.close();

const checks = {
  'Mia sees Josh by his account name': joined,
  'Tapping Josh opens his card': card && /Josh/.test(cardText) && /@josh/.test(cardText),
  'His card shows his bio and favorites': /Piano on Thursdays/.test(cardText) && /Old Fashioned/.test(cardText),
  'No errors': errors.length === 0,
};
if (errors.length) console.log(errors.join('\n'));
for (const [k, v] of Object.entries(checks)) console.log((v ? 'ok   ' : 'FAIL ') + k);
const ok = Object.values(checks).every(Boolean);
console.log(ok ? 'ACCOUNT TEST PASSED' : 'ACCOUNT TEST FAILED');
process.exit(ok ? 0 : 1);
