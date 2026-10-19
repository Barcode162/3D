// Two players race each other on the two rigs (upgrade plan item 1): Alice
// sits in the left rig, Bob in the right one, both open the garage, pick a
// car and press READY. The room (local wrangler dev) must start one race for
// both, each must see the other's car, and Alice driving off must show up on
// Bob's screen. Saves build/race-mp-bob.png (Bob's view after Alice drove).
// Usage: NODE_PATH=$(npm root -g) node tools/race_mp_test.mjs
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';
import { startRoom } from './lib/room.mjs';

const site = serveSite();
const room = await startRoom(8794, { CONNECT_LIMIT_PER_MIN: 1000 });
const ROOM = 'race' + Math.floor(Math.random() * 1e6);
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };

const W = 932, H = 430;
async function player(name, seat) {
  const page = await browser.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 1 });
  const ok = await openBar(page, site.url, `skipintro&room=${ROOM}&name=${name}&cloud=${encodeURIComponent(room.ws)}&sit=${seat}&garage`, 180000);
  check(ok, `${name} loaded`);
  return page;
}
const waitLog = (page, re, ms = 30000) => page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent),
  re.source, { timeout: ms, polling: 200 }).then(() => true).catch(() => false);

try {
  const alice = await player('Alice', 30);
  const bob = await player('Bob', 31);
  check(await waitLog(bob, /Alice joined/), 'Bob sees Alice in the room');
  await bob.waitForTimeout(2500);  // both claim their rig and join the garage
  // READY (bottom right of the garage, app/race/game.cpp garageButtons).
  const u = Math.min(W, H) / 400;
  const ready = { x: W - 0.05 * W - 75 * u, y: H - 42 * u };
  for (const p of [alice, bob]) {
    await p.bringToFront();
    await p.mouse.click(ready.x, ready.y);
    await p.waitForTimeout(400);
  }
  check(await waitLog(alice, /Race: you vs Bob/), 'Alice races Bob (not the bots)');
  check(await waitLog(bob, /Race: you vs Alice/), 'Bob races Alice');
  // The lights go out about 7 s after the start; Alice drives off.
  await alice.bringToFront();
  await alice.waitForTimeout(7500);
  await alice.keyboard.down('ArrowUp');
  await alice.waitForTimeout(5000);
  await alice.screenshot({ path: 'build/race-mp-alice.png' });
  await alice.keyboard.up('ArrowUp');
  await bob.bringToFront();
  await bob.waitForTimeout(1500);
  await bob.screenshot({ path: 'build/race-mp-bob.png' });
  const where = await bob.evaluate(() => window.bar3dNet && window.bar3dNet.count());
  check(where >= 1, 'still connected after the start');
  for (const [n, p] of [['Alice', alice], ['Bob', bob]]) {
    const errs = await logErrors(p);
    check(errs.length === 0, `${n}: no errors${errs.length ? ' (' + errs.join(' | ') + ')' : ''}`);
  }
  console.log((await logText(bob)).split('\n').filter((l) => /Race|Multiplayer/.test(l)).join('\n'));
} finally {
  await browser.close();
  await room.stop();
  site.close();
}
console.log(failed ? `RACE MP TEST FAILED (${failed})` : 'RACE MP TEST PASSED');
process.exit(failed ? 1 : 0);
