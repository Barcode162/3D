// Multiplayer test: runs server/relay.php with PHP's built-in web server,
// opens the game twice (players "Alice" and "Bob") in headless Chromium,
// lets Alice walk while Bob looks at her, and saves Bob's view to
// build/mp-bob.png. Needs `php` installed.
// Usage: NODE_PATH=$(npm root -g) node tools/mp_test.mjs
// RELAY=https://joshuavelasco.org/relay.php tests through the live relay.
import { createServer } from 'node:http';
import { spawn } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { mkdtempSync, copyFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const siteDir = resolve('build/site');
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };

const site = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  try {
    const body = await readFile(join(siteDir, path === '/' ? 'index.html' : path));
    res.writeHead(200, { 'content-type': types[extname(path)] ?? 'text/html' });
    res.end(body);
  } catch {
    res.writeHead(404).end();
  }
}).listen(0);
const port = site.address().port;

const relayDir = mkdtempSync(join(tmpdir(), 'relay-'));
copyFileSync('server/relay.php', join(relayDir, 'relay.php'));
const relayPort = port + 1;
const php = spawn('php', ['-S', `127.0.0.1:${relayPort}`, '-t', relayDir], { stdio: 'ignore' });
const relay = process.env.RELAY ?? `http://127.0.0.1:${relayPort}/relay.php`;

const browser = await playwright.chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'],
});
async function open(name, cam) {
  const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  await page.goto(`http://localhost:${port}/?room=test&name=${name}&server=${encodeURIComponent(relay)}` +
                  `&cam=${encodeURIComponent(cam)}`);
  return page;
}
const alice = await open('Alice', process.env.ALICE ?? '0,1.66,-0.5,0,0');
const bob = await open('Bob', process.env.BOB ?? '0,1.66,2.5,0,0');
await bob.waitForTimeout(7000);
await alice.bringToFront();
await alice.keyboard.down('KeyD');  // Alice side-steps (walk animation)
await alice.waitForTimeout(1500);
await alice.keyboard.up('KeyD');
await bob.bringToFront();
await bob.waitForTimeout(2500);

const log = await bob.$eval('#log', (el) => el.textContent.replace(/\[(info|error)\]/g, '\n[$1]'));
const status = await bob.$eval('#status', (el) => el.textContent);
await bob.screenshot({ path: 'build/mp-bob.png' });
console.log(log.split('\n').filter((l) => /Multiplayer|error/i.test(l)).join('\n'));
console.log('Bob status:', status);
await browser.close();
php.kill();
site.close();
const ok = /online: 2/.test(status) && /Alice joined/.test(log);
console.log(ok ? 'MULTIPLAYER TEST PASSED' : 'MULTIPLAYER TEST FAILED');
process.exit(ok ? 0 : 1);
