// Multiplayer test: starts the Cloudflare room (cloud/) locally with
// wrangler dev, opens the game twice (players "Alice" and "Bob") in headless
// Chromium, lets Alice walk while Bob looks at her, and saves Bob's view to
// build/mp-bob.png. Then the shared world: Carol joins, moves a glass along
// the bar and turns the full mirror off; Bob must see both, and Carol
// (rejoining) must find the glass where she left it. Then seats: the room
// decides who sits where. Needs `npm ci --prefix cloud`.
// Usage: NODE_PATH=$(npm root -g) node tools/mp_test.mjs
// ROOM_URL=ws://127.0.0.1:8787 uses a room server you started yourself;
// ROOM_URL=wss://bar3d.joshuavelasco60.workers.dev tests the live one.
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';
import { startRoom, probe } from './lib/room.mjs';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const siteDir = resolve('build/site');
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.json': 'application/json' };

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
const roomServer = await startRoom(8792, { CONNECT_LIMIT_PER_MIN: 1000 });
const CLOUD = roomServer.ws;
const ROOM = 'mp' + Math.floor(Math.random() * 1e6);

const browser = await playwright.chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'],
});
async function open(name, cam, extra = '') {
  const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  await page.goto(`http://localhost:${port}/?skipintro&room=${ROOM}&name=${name}&cloud=${encodeURIComponent(CLOUD)}` +
                  `&cam=${encodeURIComponent(cam)}${extra}`);
  return page;
}
const alice = await open('Alice', process.env.ALICE ?? '0,1.66,-0.5,0,0');
const bob = await open('Bob', process.env.BOB ?? '0,1.66,2.5,0,0');
// Players join the room when they enter (after loading), so wait for both.
for (const p of [alice, bob]) {
  await p.waitForFunction(() => /Everything loaded/.test(document.getElementById('log').textContent), null,
                          { timeout: 180000, polling: 250 }).catch(() => console.log('timed out loading'));
}
await bob.waitForFunction(() => /Alice joined/.test(document.getElementById('log').textContent), null,
                          { timeout: 30000, polling: 250 }).catch(() => console.log('timed out waiting for Alice'));
await bob.waitForTimeout(1000);
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

// Alice switches to another app (the page is hidden): Bob must see her as
// away quickly, and back when she returns.
const setHidden = (page, hidden) => page.evaluate((h) => {
  Object.defineProperty(document, 'hidden', { configurable: true, get: () => h });
  Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => (h ? 'hidden' : 'visible') });
  document.dispatchEvent(new Event('visibilitychange'));
}, hidden);
const hiddenAt = Date.now();
await setHidden(alice, true);
const awayOk = await bob.waitForFunction(() => /Alice is away/.test(document.getElementById('log').textContent), null,
                                         { timeout: 10000, polling: 100 }).then(() => true).catch(() => false);
const awayMs = Date.now() - hiddenAt;
await setHidden(alice, false);
const backOk = await bob.waitForFunction(() => /Alice is back/.test(document.getElementById('log').textContent), null,
                                         { timeout: 20000, polling: 250 }).then(() => true).catch(() => false);
console.log(`Bob saw Alice away after ${awayMs} ms (headless Bob runs at a few fps)`);

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
// Wait (up to 60 s, the headless world runs in slow motion) until the room
// says the glass has come to rest.
for (let i = 0; i < 60; i++) {
  const pr = await probe(CLOUD, ROOM);
  const g1 = (pr.welcome?.kv || []).find((e) => e[1] === 'g1');
  pr.close();
  if (g1 && g1[3][10] === 2) break;
  await carol.waitForTimeout(1000);
}
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
const dj = await probe(CLOUD, ROOM, 'DJ');
dj.send('K', { w: { jb: [1, Date.now() / 1000] } });
await waitLog(bob, /Bossa Antigua" \(chosen by another player\)/, 20000);
dj.close();
const log = await logOf(bob);

// Seats: the room decides. Someone (a probe) already has seat 6; Bob tries
// it (the ?sit test option) and must be told and stood up. Carol sits on
// seat 5: the room must know it's hers.
await carol.close();
const sitter = await probe(CLOUD, ROOM, 'Sitter');
sitter.send('I', { i: 1, k: 'claim', key: 'seat.6' });
const sitterOk = (await sitter.until(() => sitter.find('R', (r) => r.r === 1)))?.body.ok === 1;
await bob.goto(bob.url().replace(/&sit=\d+/, '') + '&sit=6');
await waitLog(bob, /sat there first|is sitting there/, 120000);
const bobSeatLog = await logOf(bob);
const carol2 = await open('Carol', '-3.7,1.66,-3.41,0,-34', '&sit=5');
await waitLog(carol2, /connected to room/, 120000);
const seatOwner = await sitter.until(() => sitter.find('D', (d) => d.d.some((x) => x[1] === 'claim.seat.5' && x[2] >= 0)), 120000);
const carolSlot = (await sitter.until(() => sitter.find('J', (j) => j.name === 'Carol') || sitter.find('F', (f) => f.name === 'Carol'), 20000))?.body.slot;
sitter.close();
show('Bob', log);
show('Carol', carolLog);
show('Carol again', carolLog2);
await browser.close();
await roomServer.stop();
site.close();
const putDown = /Put down at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog);
const rested = /glass 1 came to rest at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(log);
const rejoined = /glass 1 is at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog2);
const near = (m) => m && putDown && Math.abs(+m[1] - +putDown[1]) < 0.05 && Math.abs(+m[3] - +putDown[3]) < 0.05;
const checks = {
  'Bob sees Alice': /online: 2/.test(status) && /Alice joined/.test(log),
  'Bob sees Alice away when her page is hidden, and back after': awayOk && backOk && awayMs < 2000,
  'Bob sees the glass land where Carol put it': near(rested),
  'Carol finds the glass there after rejoining': near(rejoined),
  'Bob sees the mirror switch to avatars only': /mirror 0 set to avatars only/.test(log),
  'Bob plays the song another player picked': /Bossa Antigua" \(chosen by another player\)/.test(log),
  'the room refuses Bob a seat someone else has': sitterOk && /Sitter (sat there first|is sitting there)/.test(bobSeatLog),
  'the room records Carol on seat 5': !!seatOwner && seatOwner.body.d.some((x) => x[1] === 'claim.seat.5' && x[2] === carolSlot),
};
for (const [k, v] of Object.entries(checks)) console.log((v ? 'ok   ' : 'FAIL ') + k);
const ok = Object.values(checks).every(Boolean);
console.log(ok ? 'MULTIPLAYER TEST PASSED' : 'MULTIPLAYER TEST FAILED');
process.exit(ok ? 0 : 1);
