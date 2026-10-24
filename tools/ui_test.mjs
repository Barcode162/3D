// The shared look (ui/, docs/archive/STUDIO.md S1.1): the style page in every theme,
// at two phone widths. Checks: no errors, fonts and icons load, nothing wider
// than the screen, the theme switch changes the colours, the step lights
// step, the knob and fader answer a finger, the save lamps light.
// Screenshots: build/ui-<theme>.png.  Usage: NODE_PATH=$(npm root -g) node tools/ui_test.mjs
import { mkdirSync } from 'node:fs';
import { launchBrowser } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = serveSite('.');
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
mkdirSync('build', { recursive: true });

const looks = ['faceplate', 'backbar', 'obsidian', 'jukebox'];
const accents = new Set();
for (const width of [375, 430]) {
  const page = await browser.newPage({ viewport: { width, height: 900 } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('requestfailed', (r) => errors.push('failed: ' + r.url()));
  page.on('response', (r) => { if (r.status() >= 400) errors.push(r.status() + ' ' + r.url()); });
  await page.goto(url + 'ui/index.html');
  await page.evaluate(() => document.fonts.ready);
  await page.waitForTimeout(600);
  for (const look of looks) {
    await page.click(`#looks [data-look="${look}"]`);
    await page.waitForTimeout(250);
    const r = await page.evaluate(() => ({
      look: document.documentElement.dataset.look,
      wide: document.documentElement.scrollWidth,
      accent: getComputedStyle(document.documentElement).getPropertyValue('--accent').trim(),
      name: document.getElementById('lookName').textContent,
      fonts: document.fonts.check('16px Archivo') && document.fonts.check('16px "Martian Mono"'),
      icons: document.querySelectorAll('#icons svg').length,
    }));
    accents.add(r.accent);
    check(r.look === look && r.name.length > 0, `${width}px ${look}: the switch puts it on (${r.name})`);
    check(r.wide <= width, `${width}px ${look}: nothing wider than the screen (${r.wide})`);
    if (look === 'faceplate') {
      check(r.fonts, `${width}px: Archivo and Martian Mono load`);
      check(r.icons >= 40, `${width}px: the icon set shows (${r.icons})`);
    }
    if (width === 430) await page.screenshot({ path: `build/ui-${look}.png` });
  }
  if (width === 375) {
    await page.click('#looks [data-look="faceplate"]');
    const steps1 = await page.evaluate(() => document.querySelector('.hero .steps .on') && [...document.querySelectorAll('.hero .steps i')].findIndex((i) => i.classList.contains('on')));
    await page.waitForTimeout(600);
    const steps2 = await page.evaluate(() => [...document.querySelectorAll('.hero .steps i')].findIndex((i) => i.classList.contains('on')));
    check(steps1 !== steps2 && steps2 >= 0, `the step light steps (${steps1} → ${steps2})`);
    const knob = await page.$('[data-knob]');
    await knob.scrollIntoViewIfNeeded(); await page.waitForTimeout(200);
    const kb = await knob.boundingBox();
    const before = await page.textContent('#kout');
    await page.mouse.move(kb.x + kb.width / 2, kb.y + kb.height / 2);
    await page.mouse.down(); await page.mouse.move(kb.x + kb.width / 2, kb.y + kb.height / 2 - 60, { steps: 5 }); await page.mouse.up();
    const after = await page.textContent('#kout');
    check(parseFloat(after) > parseFloat(before), `dragging the knob up turns it up (${before} → ${after})`);
    const lit = await page.evaluate(() => document.querySelectorAll('[data-knob] .dot.on').length);
    check(lit > 4, `the knob's ring lights follow (${lit} lit)`);
    const fader = await page.$('[data-fader]');
    await fader.scrollIntoViewIfNeeded(); await page.waitForTimeout(200);
    const fb = await fader.boundingBox();
    await page.mouse.click(fb.x + fb.width * 0.2, fb.y + fb.height / 2);
    const fv = await fader.evaluate((el) => el.getAttribute('aria-valuetext'));
    check(/^(1[0-9]|2[0-9]|[0-9])%$/.test(fv), `a tap on the fader moves it there (${fv})`);
    await page.click('#change');
    const edit = await page.evaluate(() => [...document.querySelectorAll('#lamps .kled')].findIndex((l) => l.classList.contains('on')));
    await page.waitForTimeout(1900);
    const saved = await page.evaluate(() => [...document.querySelectorAll('#lamps .kled')].findIndex((l) => l.classList.contains('on')));
    check(edit === 0 && saved === 2, `the save lamps go edit → saved (${edit} → ${saved})`);
    await page.click('#earn');
    await page.waitForTimeout(500);
    check((await page.textContent('#credits')) === '1,265', 'numbers roll to their new value');
    await page.click('.switch');
    check((await page.getAttribute('.switch', 'aria-checked')) === 'true', 'the flip switch flips');
  }
  check(errors.length === 0, `${width}px: no errors` + (errors.length ? ': ' + errors.join(' | ') : ''));
  await page.close();
}
// The front door (homepage/) has its own test now: tools/site_test.mjs.
check(accents.size === 4, `each theme has its own accent (${[...accents].join(' ')})`);
await browser.close();
close();
console.log(failed ? `UI TEST FAILED (${failed})` : 'UI TEST PASSED');
process.exit(failed ? 1 : 0);
