// The menu after games (open bug: after some games the Menu button and a
// panel's ✕ stopped answering on the phone). Plays or opens each game in
// turn, leaves it, then taps the Menu button and the panel's ✕ where a
// finger would (not through code), checking each still answers and that
// nothing sits on top of the button. Last, a tap whose click the browser
// drops still opens the menu (bar/web/menu.js onTap).
// Usage: NODE_PATH=$(npm root -g) node tools/menu_games_test.mjs
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = await serveSite();
const browser = await launchBrowser();
let failed = false;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed = true; };

async function center(page, sel) {
  return page.evaluate((s) => {
    const n = document.querySelector(s);
    if (!n) return null;
    const b = n.getBoundingClientRect();
    const x = b.left + b.width / 2, y = b.top + b.height / 2, top = document.elementFromPoint(x, y);
    return { x, y, top: top ? (top.id || top.className || top.tagName) : '' };
  }, sel);
}
// Tap the Menu button, then the ✕: both must answer.
async function menuAnswers(page, label) {
  const m = await center(page, '#hudMenu');
  if (!m || m.top !== 'hudMenu') { check(false, `${label}: the Menu button is on top (found ${m && m.top})`); return; }
  await page.mouse.click(m.x, m.y);
  await page.waitForTimeout(400);
  const opened = await page.evaluate(() => bar3dUi.isOpen());
  let closed = false;
  if (opened) {
    const x = await center(page, '.mX');
    await page.mouse.click(x.x, x.y);
    await page.waitForTimeout(300);
    closed = !(await page.evaluate(() => bar3dUi.isOpen() || bar3dModal.isOpen()));
  }
  check(opened && closed, `${label}: the Menu button opens the menu and its ✕ closes it`);
}
async function closePanel(page) {
  if (!(await page.evaluate(() => bar3dUi.isOpen()))) return;
  const x = await center(page, '.mX');
  await page.mouse.click(x.x, x.y);
  await page.waitForTimeout(300);
}
async function escape(page) {
  for (let i = 0; i < 2; ++i) { await page.keyboard.press('Escape'); await page.waitForTimeout(700); }
}

// Canvas games, each from its test option, left with Escape.
for (const [q, name] of [['rhythm=menu', '4K Mania'], ['chess', 'chess'], ['sit=30', 'a racing rig']]) {
  const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  check(await openBar(page, url, `skipintro&server=off&${q}`), `the bar loads (${q})`);
  await page.waitForTimeout(1500);
  await closePanel(page);  // (a rig opens the sim panel)
  await escape(page);
  check(!(await page.evaluate(() => document.body.classList.contains('racing'))), `after ${name}: the top bar is back`);
  await menuAnswers(page, `after ${name}`);
  const errors = await logErrors(page);
  check(errors.length === 0, `after ${name}: no errors` + (errors.length ? ':\n' + errors.join('\n') : ''));
  await page.close();
}

// The page's own games and panels, one after another in one visit.
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
check(await openBar(page, url, 'skipintro&server=off&badge=bartender&shift'), 'the bar loads (bartender shift)');
await page.waitForTimeout(1500);
await closePanel(page);
for (const step of ['bartender', 'platformer', 'meridian', 'sim', 'carSetup', 'piano', 'voice', 'avatar']) {
  if (step === 'platformer' || step === 'meridian') {
    await page.evaluate((s) => (s === 'meridian' ? bar3dUi.openMeridian() : bar3dUi.open(s)), step);
    await page.waitForTimeout(2500);
    await page.click('text=Back to the bar');
    await page.waitForTimeout(800);
  } else {
    await page.evaluate((s) => bar3dUi.open(s), step);
    await page.waitForTimeout(400);
    if (step === 'bartender') {  // make a drink (closes the panel)
      await page.click('#menuRoot .mList button >> nth=0');
      await page.waitForTimeout(500);
    }
  }
  await closePanel(page);
  await menuAnswers(page, `after ${step}`);
}

// A tap whose click never comes (what the phone seemed to do): the menu opens anyway.
await page.evaluate(() => {
  const b = document.getElementById('hudMenu'), r = b.getBoundingClientRect();
  const o = { bubbles: true, clientX: r.left + 5, clientY: r.top + 5, pointerId: 1, isPrimary: true };
  b.dispatchEvent(new PointerEvent('pointerdown', o));
  b.dispatchEvent(new PointerEvent('pointerup', o));
});
await page.waitForTimeout(800);
check(await page.evaluate(() => bar3dUi.isOpen()), 'a tap without a click still opens the menu');
check(/got no click; handled it anyway/.test(await logText(page)), 'and the log says so (for reports)');
await closePanel(page);
await page.waitForTimeout(800);
check(!(await page.evaluate(() => bar3dUi.isOpen())), 'a normal tap on ✕ closes it once (no second, late action)');
const errors = await logErrors(page);
check(errors.length === 0, 'no errors' + (errors.length ? ':\n' + errors.join('\n') : ''));

await browser.close();
close();
console.log(failed ? 'MENU GAMES TEST FAILED' : 'MENU GAMES TEST PASSED');
process.exit(failed ? 1 : 0);
