// The Studio (docs/STUDIO.md S2, S3...): the page at /studio/ with the bar's
// engine in studio mode in its frame, served like the real site (/bar/,
// /studio/, /ui/; no PHP: ?nologin on localhost). Each step adds its checks.
// Screenshots: build/studio-*.png.   Usage: NODE_PATH=$(npm root -g) node tools/studio_test.mjs
import { mkdirSync } from 'node:fs';
import { launchBrowser } from './lib/browser.mjs';
import { serveSites } from './lib/sites.mjs';

const { url, close } = serveSites({ '/bar/': 'build/site', '/studio/': 'studio', '/ui/': 'ui' });
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
mkdirSync('build', { recursive: true });

const page = await browser.newPage({ viewport: { width: 393, height: 852 }, hasTouch: true });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
await page.goto(url + 'studio/?nologin');
const S = () => page.evaluate(() => { const s = window.bar3dStudio && window.bar3dStudio.S; return s && { things: s.things, selected: s.selected, place: s.place }; });
await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.things.length > 100, null, { timeout: 120000, polling: 250 }).catch(() => {});
const s1 = await S();
check(s1 && s1.things.length > 100, `S2.1 the engine opens in studio mode and lists the world (${s1 && s1.things.length} things)`);
const frame = page.frame({ url: /\/bar\/\?studio=1/ });
check(!!frame, 'S2.1 the bar runs in the Studio\'s frame');
const engineLog = async () => frame.evaluate(() => document.getElementById('log').textContent);
if (frame) {
  check(/Studio: ready/.test(await engineLog()), 'S2.1 the engine says it is in studio mode');
  const hud = await frame.evaluate(() => { const m = document.getElementById('hudMenu'); return m ? getComputedStyle(m).display : 'none'; });
  check(hud === 'none', 'S2.1 no bar HUD inside the Studio');
  check(!/Multiplayer: connected/.test(await engineLog()), 'S2.1 no room: nobody else is in the Studio');
}
check((await page.evaluate(() => document.querySelector('#top').getBoundingClientRect().height)) > 30, 'S2.1 the top bar shows');
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/studio-open.png' });

check(errors.length === 0, 'no page errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close();
close();
console.log(failed ? `STUDIO TEST FAILED (${failed})` : 'STUDIO TEST PASSED');
process.exit(failed ? 1 : 0);
