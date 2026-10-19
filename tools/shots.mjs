// Screenshots of the page with different options, for checking features
// headless: node tools/shots.mjs "sit=0" "darts" "pianosong=0&cam=..." ...
// Each argument is added to the page URL (with ?skipintro); saves
// build/shot-1.png, ... and prints any errors from the on-screen log.
// WAIT=ms waits longer after loading (default 2500). VIEW=932x430 sets the
// viewport (default 430x932, an iPhone held upright). HOLD=ArrowUp:9000
// holds a key that long before the shot (e.g. to drive in the race).
// An argument with its own server= replaces the default server=off.
// REQUESTS=1 prints every request to a host other than this test server.
//
// node tools/shots.mjs spots   captures the standard spots (tools/lib/spots.mjs)
// PERF=1 also prints each shot's draw calls and triangles (main / mirror).
// with ?still into OUT (default build/shots/), named <spot>.png, for the
// "looks the same" check: node tools/imgdiff.mjs build/before build/after.
import { mkdirSync } from 'node:fs';
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { SPOTS } from './lib/spots.mjs';

let args = process.argv.slice(2);
let outFor = (i) => `build/shot-${i + 1}.png`;
if (args[0] === 'spots') {
  const out = process.env.OUT ?? 'build/shots';
  mkdirSync(out, { recursive: true });
  const names = Object.keys(SPOTS);
  args = names.map((n) => `still&cam=${SPOTS[n]}`);
  outFor = (i) => `${out}/${names[i]}.png`;
}

const site = serveSite();
const browser = await launchBrowser(['--autoplay-policy=no-user-gesture-required']);
let failed = false;
for (let i = 0; i < args.length; ++i) {
  const [vw, vh] = (process.env.VIEW ?? '430x932').split('x').map(Number);
  const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 1 });
  const outside = [];
  page.on('request', (r) => { const u = new URL(r.url()); if (u.port !== String(site.port)) outside.push(r.url()); });
  page.on('websocket', (w) => outside.push(w.url()));
  const serverOpt = /(^|&)server=/.test(args[i]) ? '' : 'server=off&';
  await openBar(page, site.url, `skipintro&${serverOpt}${args[i]}`);
  // Past the warm-up (slow here: the software GPU unpacks compressed textures).
  await page.waitForFunction(() => /Stage entered|Startup failed/.test(document.getElementById('log').textContent),
                             null, { timeout: 120000 }).catch(() => {});
  // A click unlocks sound (piano notes need it). Not for still shots: the
  // tap would count as input.
  if (!/(^|&)still/.test(args[i])) await page.mouse.click(5, vh - 32);
  await page.waitForTimeout(+(process.env.WAIT ?? 2500));
  if (process.env.HOLD) {
    const [key, ms] = process.env.HOLD.split(':');
    await page.keyboard.down(key);
    await page.waitForTimeout(+ms);
    await page.screenshot({ path: outFor(i).replace(/\.png$/, '-held.png') });
    await page.keyboard.up(key);
  }
  await page.screenshot({ path: outFor(i) });
  const errors = await logErrors(page);
  const log = await logText(page);
  const lines = log.split('\n').filter((l) => /Piano|Bartender|Darts|Voice|seat|Seats|Race|Multiplayer|DBG/i.test(l));
  console.log(`--- ${outFor(i)}: ${args[i]}\n${lines.slice(-(+(process.env.LINES ?? 4))).join('\n')}`);
  if (process.env.PERF) {
    const p = await page.evaluate(() => window.bar3dPerf);
    if (p) console.log(`draws ${JSON.stringify(p.draws)} tris ${JSON.stringify(p.tris)}`);
  }
  if (process.env.REQUESTS) console.log('requests outside: ' + (outside.length ? [...new Set(outside)].join(' ') : 'none'));
  if (errors.length) { failed = true; console.log('ERRORS:\n' + errors.join('\n')); }
  await page.close();
}
await browser.close();
site.close();
process.exit(failed ? 1 : 0);
