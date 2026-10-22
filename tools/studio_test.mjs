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

// ---- S2.2 cameras ----
const view = async () => {
  await page.evaluate(() => { window.__view = null; window.bar3dStudio.on('view', (e) => { window.__view = e.view; }); window.bar3dStudio.send('st.getview'); });
  await page.waitForFunction(() => window.__view, null, { timeout: 5000 }).catch(() => {});
  return page.evaluate(() => window.__view);
};
const v0 = await view();
check(Array.isArray(v0) && v0.length === 7, `S2.2 the camera says where it is (${v0 && v0.slice(0, 5).map((x) => x.toFixed(2)).join(', ')})`);
await page.mouse.move(200, 400); await page.mouse.down(); await page.mouse.move(280, 380, { steps: 8 }); await page.mouse.up();
await page.waitForTimeout(300);
const v1 = await view();
check(v1 && Math.abs(v1[3] - v0[3]) > 0.3, `S2.2 dragging one finger orbits (yaw ${v0 && v0[3].toFixed(2)} → ${v1 && v1[3].toFixed(2)})`);
await page.click('#camModes [data-cam="2"]');
await page.waitForTimeout(300);
const vTop = await view();
check(vTop && vTop[5] === 2 && vTop[4] < -1.5, `S2.2 Top looks straight down (pitch ${vTop && vTop[4].toFixed(2)})`);
await page.screenshot({ path: 'build/studio-top.png' });
await page.click('#camModes [data-cam="0"]');
await page.evaluate((v) => window.bar3dStudio.send('st.view', 0, 0, v.join(',')), v0);
await page.waitForTimeout(300);
const vBack = await view();
check(vBack && Math.abs(vBack[0] - v0[0]) < 0.05 && Math.abs(vBack[3] - v0[3]) < 0.01, 'S2.2 a saved view comes back exactly');
await page.click('#viewsKey');
await page.click('#views button >> nth=0');
await page.waitForTimeout(500);
check((await page.$$('#views button')).length === 2, 'S2.2 "Save this view" adds it to the list');

// ---- S2.3 picking ----
// Look at the bar counter from close by, then tap the middle of the screen.
const counter = (await S()).things.find((t) => t.id === 'bar_counter_l');
await page.evaluate((c) => window.bar3dStudio.send('st.view', 0, 0, [c.at[0] + 1.2, 1.7, c.at[2] + 2.6, -0.4, -0.35, 0, 2.5].join(',')), counter);
await page.waitForTimeout(400);
const box = await page.$('#world').then((f) => f.boundingBox());
await page.mouse.click(box.x + box.width / 2, box.y + box.height * 0.55);
await page.waitForTimeout(600);
const sel = await S();
const picked = sel.selected >= 0 ? sel.things[sel.selected] : null;
check(!!picked, `S2.3 a tap selects a thing (${picked && picked.id})`);
const crumb = await page.textContent('#crumbPath').catch(() => '');
check(/^Bar › /.test(crumb) && picked && crumb.endsWith(picked.id), `S2.3 the breadcrumb says where it sits (${crumb})`);
await page.screenshot({ path: 'build/studio-pick.png' });
const listed = await page.$$eval('#hits button', (b) => b.length).catch(() => 0);
check(listed === 0 || listed >= 2, `S2.3 several things under the finger → a list to choose from (${listed})`);
await page.click('#deselect');
await page.waitForTimeout(200);
check((await S()).selected === -1 && await page.$eval('#crumb', (c) => c.hidden), 'S2.3 ✕ deselects');

check(errors.length === 0, 'no page errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close();
close();
console.log(failed ? `STUDIO TEST FAILED (${failed})` : 'STUDIO TEST PASSED');
process.exit(failed ? 1 : 0);
