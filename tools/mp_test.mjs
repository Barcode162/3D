// Multiplayer test: runs server/relay.php with PHP's built-in web server,
// opens the game twice (players "Alice" and "Bob") in headless Chromium,
// lets Alice walk while Bob looks at her, and saves Bob's view to
// build/mp-bob.png. Then the shared world (M2): Carol joins, moves a glass
// along the bar and turns the full mirror off; Bob must see both, and Carol
// (rejoining) must find the glass where she left it. Needs `php` installed.
// Usage: NODE_PATH=$(npm root -g) node tools/mp_test.mjs
// RELAY=https://joshuavelasco.com/bar/relay.php tests through the live relay;
// CLOUD=ws://127.0.0.1:8787 through the Cloudflare room server (cloud/).
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
  const via = process.env.CLOUD ? `cloud=${encodeURIComponent(process.env.CLOUD)}` : `server=${encodeURIComponent(relay)}`;
  await page.goto(`http://localhost:${port}/?skipintro&room=test&name=${name}&${via}` +
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

const logOf = (page) => page.$eval('#log', (el) => [...el.children].map((c) => c.textContent).join('\n'));
const show = (who, log) => console.log(`--- ${who}\n` + log.split('\n').filter((l) => /Multiplayer|Put down|Jukebox|error/i.test(l)).join('\n'));
const status = await bob.$eval('#status', (el) => el.textContent);
await bob.screenshot({ path: 'build/mp-bob.png' });
console.log('Bob status:', status);

// Shared world. Carol stands at the bar looking down at glass 1 (x = -3.7),
// picks it up (tap in the middle), puts it down to the right. Alice leaves
// first so three software-rendered pages don't crawl.
await alice.close();
const waitLog = (page, re, ms = 60000) =>
  page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent), re.source,
                       { timeout: ms, polling: 250 }).catch(() => console.log('timed out waiting for ' + re));
const carol = await open('Carol', '-3.7,1.66,-3.41,0,-34');
await waitLog(carol, /connected to room/);
await waitLog(carol, /Everything loaded/, 180000);  // the loading screen covers the canvas until then
await carol.waitForTimeout(3000);
const vw = 430, vh = 932;
await carol.mouse.click(vw / 2, vh / 2);
await carol.waitForTimeout(2500);
await carol.mouse.click(vw * 0.75, vh * 0.45);  // onto the bar top, past the arm rest
await waitLog(carol, /Put down at/);
await carol.waitForTimeout(1000);
const carolLog = await logOf(carol);
// Wait (up to 60 s, the headless world runs in slow motion) until the relay
// says the glass has come to rest.
for (let i = 0; i < 60; i++) {
  const r = await fetch(relay, { method: 'POST', body: JSON.stringify({ room: 'test', id: 'probe', s: [0], since: 0 }) });
  const g1 = (await r.json()).world.find((e) => e[0] === 'g1');
  if (g1 && g1[2][10] === 2) break;
  await carol.waitForTimeout(1000);
}
await fetch(relay, { method: 'POST', body: JSON.stringify({ room: 'test', id: 'probe', leave: 1 }) });
// Then rejoins at the big mirror and taps it (full -> avatars only).
await carol.goto(carol.url().replace(/cam=[^&]*/, 'cam=' + encodeURIComponent('6.6,1.66,3.6,90,0')));
await waitLog(carol, /glass 1 is at/);
await waitLog(carol, /Everything loaded/, 180000);
await carol.waitForTimeout(2000);
const carolLog2 = await logOf(carol);
await carol.screenshot({ path: 'build/mp-carol.png' });
await carol.mouse.click(vw / 2, vh / 2);
await carol.waitForTimeout(2500);
await bob.bringToFront();
await waitLog(bob, /mirror 0 set to avatars only/, 20000);
await waitLog(bob, /Everything loaded/, 180000);
// Jukebox: a stand-in player picks song 2 ("Bossa Antigua"); Bob, after a
// tap (phones only play sound after one), must start the same song.
await bob.mouse.click(215, 300);
if (process.env.CLOUD) {
  const probe = new WebSocket(`${process.env.CLOUD}/room/test?id=probe`);
  await new Promise((r) => (probe.onopen = r));
  probe.send(JSON.stringify({ hello: 1, name: 'Probe', t: 1, s: [0], w: { jb: [1, Date.now() / 1000] } }));
  await new Promise((r) => setTimeout(r, 500));
  probe.close();
} else {
  await fetch(relay, { method: 'POST', body: JSON.stringify({ room: 'test', id: 'probe', s: [0], w: { jb: [1, Date.now() / 1000] } }) });
}
await waitLog(bob, /Bossa Antigua" \(chosen by another player\)/, 20000);
await fetch(relay, { method: 'POST', body: JSON.stringify({ room: 'test', id: 'probe', leave: 1 }) });
const log = await logOf(bob);
show('Bob', log);
show('Carol', carolLog);
show('Carol again', carolLog2);
await browser.close();
php.kill();
site.close();
const putDown = /Put down at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog);
const rested = /glass 1 came to rest at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(log);
const rejoined = /glass 1 is at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog2);
const near = (m) => m && putDown && Math.abs(+m[1] - +putDown[1]) < 0.05 && Math.abs(+m[3] - +putDown[3]) < 0.05;
const checks = {
  'Bob sees Alice': /online: 2/.test(status) && /Alice joined/.test(log),
  'Bob sees the glass land where Carol put it': near(rested),
  'Carol finds the glass there after rejoining': near(rejoined),
  'Bob sees the mirror switch to avatars only': /mirror 0 set to avatars only/.test(log),
  'Bob plays the song another player picked': /Bossa Antigua" \(chosen by another player\)/.test(log),
};
for (const [k, v] of Object.entries(checks)) console.log((v ? 'ok   ' : 'FAIL ') + k);
const ok = Object.values(checks).every(Boolean);
console.log(ok ? 'MULTIPLAYER TEST PASSED' : 'MULTIPLAYER TEST FAILED');
process.exit(ok ? 0 : 1);
