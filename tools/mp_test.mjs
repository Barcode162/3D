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
import { launchBrowser, drawLess, drawAgain } from './lib/browser.mjs';
import { startRoom, probe } from './lib/room.mjs';

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

const browser = await launchBrowser();  // the tests' light and frame cap on GitHub (KI-5)
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
await drawLess(bob);  // the check is how fast the word comes, not Bob's drawing (KI-5)
const hiddenAt = Date.now();
await setHidden(alice, true);
const awayOk = await bob.waitForFunction(() => /Alice is away/.test(document.getElementById('log').textContent), null,
                                         { timeout: 10000, polling: 100 }).then(() => true).catch(() => false);
const awayMs = Date.now() - hiddenAt;
await setHidden(alice, false);
const backOk = await bob.waitForFunction(() => /Alice is back/.test(document.getElementById('log').textContent), null,
                                         { timeout: 20000, polling: 250 }).then(() => true).catch(() => false);
await drawAgain(bob);
console.log(`Bob saw Alice away after ${awayMs} ms (headless Bob runs at a few fps)`);

// Shared world. Carol stands at the bar looking down at glass 1 (x = -3.7),
// picks it up (tap in the middle), puts it down to the right. Alice leaves
// first so three software-rendered pages don't crawl.
await alice.close();
await drawLess(bob);  // Bob only watches while Carol loads and rejoins (KI-5)
const waitLog = (page, re, ms = 60000) =>
  page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent), re.source,
                       { timeout: ms, polling: 250 }).catch(() => console.log('timed out waiting for ' + re));
const glassWatch = await probe(CLOUD, ROOM, 'Watcher');
const carol = await open('Carol', '-3.7,1.66,-3.41,0,-34');
const carolConsole = [];
carol.on('console', (m) => carolConsole.push(m.text()));
await waitLog(carol, /connected to room/);
await waitLog(carol, /Everything loaded/, 180000);  // the loading screen covers the canvas until then
await waitLog(carol, /Stage entered/, 180000);  // and the warm-up (slow here with compressed textures)
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
  if (i % 10 === 0 || (g1 && g1[3][10] === 2)) console.log('room has g1:', JSON.stringify(g1));
  if (g1 && g1[3][10] === 2) break;
  await carol.waitForTimeout(1000);
}
// Then rejoins at the big mirror and taps it (full -> avatars only).
await carol.goto(carol.url().replace(/cam=[^&]*/, 'cam=' + encodeURIComponent('6.6,1.66,3.6,90,0')));
await waitLog(carol, /glass 1 is at/);
await waitLog(carol, /Everything loaded/, 180000);
await carol.waitForTimeout(2000);
const carolLog2 = await logOf(carol);
console.log('carol console:', carolConsole.filter((l) => /welcome brought/.test(l)).join('\n'));
console.log('--- Carol again (all)\n' + carolLog2.split('\n').filter((l) => !/Model assets|Texture|Stage /.test(l)).slice(-25).join('\n'));
await carol.screenshot({ path: 'build/mp-carol.png' });
await carol.mouse.click(vw / 2, vh / 2);
await carol.waitForTimeout(2500);
await drawAgain(bob);
await bob.bringToFront();
await waitLog(bob, /big_mirror is avatars \(the room\)/, 20000);
await waitLog(bob, /Everything loaded/, 180000);
// Jukebox: a stand-in player picks song 2 ("Bossa Antigua"); Bob, after a
// tap (phones only play sound after one), must start the same song.
await bob.mouse.click(215, 300);
const glassClaims = glassWatch.got.filter((m) => m.letter === 'D').flatMap((m) => m.body.d).filter((x) => x[1] === 'claim.glass.1').map((x) => x[2]);
glassWatch.close();
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
sitter.send('I', { i: 2, k: 'claim', key: 'machine.darts' });
await sitter.until(() => sitter.find('R', (r) => r.r === 2));
await bob.goto(bob.url().replace(/&sit=\d+/, '') + '&darts');
await waitLog(bob, /Machine machine.darts: Sitter is (throwing|using it)/, 120000);
const bobDartsLog = await logOf(bob);
const carol2 = await open('Carol', '-3.7,1.66,-3.41,0,-34', '&sit=5');
await waitLog(carol2, /connected to room/, 120000);
const seatOwner = await sitter.until(() => sitter.find('D', (d) => d.d.some((x) => x[1] === 'claim.seat.5' && x[2] >= 0)), 120000);
console.log('--- Carol (seat)\n' + (await logOf(carol2)).split('\n').filter((l) => /Seat|sit|Multiplayer|Stage entered/i.test(l)).join('\n'));
const carolSlot = (await sitter.until(() => sitter.find('J', (j) => j.name === 'Carol') || sitter.find('F', (f) => f.name === 'Carol'), 20000))?.body.slot;
// Darts scored by the room: the board is free again, Bob throws three darts
// at the middle of his view; the room scores them and tells everyone.
await carol2.close();
sitter.send('I', { i: 3, k: 'release', key: 'machine.darts' });
await sitter.until(() => sitter.find('R', (r) => r.r === 3));
await bob.goto(bob.url().replace(/&sit=\d+|&darts/g, '') + '&darts');
await waitLog(bob, /Stage entered/, 120000);
await bob.waitForTimeout(4000);
for (let i = 0; i < 3; i++) { await bob.mouse.click(215, 466); await bob.waitForTimeout(2500); }
await waitLog(bob, /(scored by the room)[\s\S]*(scored by the room)[\s\S]*(scored by the room)/, 60000);
const throwsSeen = sitter.got.filter((m) => m.letter === 'D').flatMap((m) => m.body.d).filter((x) => x[1] === 'darts').map((x) => x[2]);
const bobThrows = [...(await logOf(bob)).matchAll(/Darts: (\S+) = (\d+), (\d+) left \(scored by the room\)/g)].map((m) => [m[1], +m[2], +m[3]]);
console.log('Bob\'s darts:', JSON.stringify(bobThrows), 'room told the others:', JSON.stringify(throwsSeen.map((t) => [t.label, t.pts, t.left])));
sitter.close();

// Things (docs/archive/ENGINE.md E3): Dave touches a lamp over the counter; the room
// runs its rule and everyone sees it swing.
const dave = await open('Dave', '-4.3,1.66,-2.6,0,24');
await dave.waitForFunction(() => /Stage entered/.test(document.getElementById('log').textContent), null,
                           { timeout: 180000, polling: 250 }).catch(() => console.log('timed out loading Dave'));
await dave.waitForFunction(() => /connected to room/.test(document.getElementById('log').textContent), null,
                           { timeout: 30000, polling: 250 }).catch(() => console.log('Dave never connected'));
await dave.waitForTimeout(1500);
await dave.mouse.click(215, 466);
await bob.waitForFunction(() => /pendant_lamp-2 is swinging \(the room\)/.test(document.getElementById('log').textContent), null,
                          { timeout: 15000, polling: 250 }).catch(() => {});
const daveLog = await logOf(dave), bobThingLog = await logOf(bob);
console.log('Dave:', (daveLog.match(/World: you touched.*/) || ['(no touch)'])[0]);
await dave.close();

// E3.6b: Fay puts the fire out; Erin, who arrives after, finds it out.
const loaded = async (page, who) => {
  await page.waitForFunction(() => /Stage entered/.test(document.getElementById('log').textContent), null,
                             { timeout: 180000, polling: 250 }).catch(() => console.log('timed out loading ' + who));
  await page.waitForFunction(() => /connected to room/.test(document.getElementById('log').textContent), null,
                             { timeout: 30000, polling: 250 }).catch(() => console.log(who + ' never connected'));
};
const fay = await open('Fay', '-6.6,1.2,0.8,-90,-12');
await loaded(fay, 'Fay');
await fay.waitForTimeout(1500);
// A busy page can miss a tap right after its warm-up: tap again, as a person would.
for (let i = 0; i < 4; i++) {
  await fay.mouse.click(215, 466);
  await fay.waitForTimeout(2000);
  if (/World: you touched/.test(await logOf(fay))) break;
}
const fayLog = await logOf(fay);
console.log('Fay:', (fayLog.match(/World: you touched.*/) || ['(no touch)'])[0]);
if (!/World: you touched/.test(fayLog)) console.log('--- Fay (no touch)\n' + fayLog.split('\n').slice(-25).join('\n'));
await fay.close();
const erin = await open('Erin', '0,1.66,2.5,0,0');
await loaded(erin, 'Erin');
await erin.waitForTimeout(1500);
const erinLog = await logOf(erin);
await erin.close();
show('Bob', log);
show('Carol', carolLog);
show('Carol again', carolLog2);
await browser.close();
await roomServer.stop();
site.close();
const putDown = /Put down at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog);
const rested = /glass 1 (?:came to rest at|is at) (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(log);
const rejoined = /glass 1 is at (-?\d+\.\d+), (-?\d+\.\d+), (-?\d+\.\d+)/.exec(carolLog2);
const near = (m) => m && putDown && Math.abs(+m[1] - +putDown[1]) < 0.05 && Math.abs(+m[3] - +putDown[3]) < 0.05;
const checks = {
  'Bob sees Alice': /online: 2/.test(status) && /Alice joined/.test(log),
  'Bob sees Alice away when her page is hidden, and back after': awayOk && backOk && awayMs < 2000,
  'Bob sees the glass land where Carol put it': near(rested),
  'Carol finds the glass there after rejoining': near(rejoined),
  'Bob sees the mirror switch to avatars only': /big_mirror is avatars \(the room\)/.test(log),
  'Bob plays the song another player picked': /Bossa Antigua" \(chosen by another player\)/.test(log),
  'the room refuses Bob a seat someone else has': sitterOk && /Sitter (sat there first|is sitting there)/.test(bobSeatLog),
  'the room records Carol holding the glass, then frees it': glassClaims.length >= 2 && glassClaims[0] >= 0 && glassClaims[glassClaims.length - 1] === -1,
  'the room refuses Bob the darts someone else is playing': /Machine machine.darts: Sitter is (throwing|using it)/.test(bobDartsLog),
  'the room scores Bob\'s three darts and everyone gets the same numbers': bobThrows.length === 3 && throwsSeen.length === 3 &&
    bobThrows.every((t, i) => t[0] === throwsSeen[i].label && t[1] === throwsSeen[i].pts && t[2] === throwsSeen[i].left),
  'Dave touches a lamp and Bob sees it swing (the room ran its rule)':
    /you touched pendant_lamp-2 \(swinging\)/.test(daveLog) && /pendant_lamp-2 is swinging \(the room\)/.test(bobThingLog),
  'Erin, arriving after Fay put the fire out, finds it out (E3.6b)':
    /you touched fire_flames \(out\)/.test(fayLog) && /fire_flames is out \(the room\)/.test(erinLog),
  'the room records Carol on seat 5': !!seatOwner && seatOwner.body.d.some((x) => x[1] === 'claim.seat.5' && x[2] === carolSlot),
};
for (const [k, v] of Object.entries(checks)) console.log((v ? 'ok   ' : 'FAIL ') + k);
const ok = Object.values(checks).every(Boolean);
console.log(ok ? 'MULTIPLAYER TEST PASSED' : 'MULTIPLAYER TEST FAILED');
process.exit(ok ? 0 : 1);
