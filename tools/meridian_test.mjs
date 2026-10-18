// Meridian inside the bar: sits in a racing rig, picks Meridian from the
// rig's menu, checks the game loads full screen (build/meridian.png), goes
// back to the bar and checks the bar draws again (build/meridian-back.png).
// Usage: NODE_PATH=$(npm root -g) node tools/meridian_test.mjs
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = await serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 932, height: 430 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
const loaded = await openBar(page, url, 'skipintro');
const results = [];
const check = (name, ok) => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}`); };
check('the bar loads', loaded);
await page.waitForTimeout(1500);
// Sit at the first racing rig the way a tap does (the menu opens).
const sat = await page.evaluate(() => {
  window.bar3dUi.open('sim');
  return !!document.querySelector('.mPanel') && /Meridian/.test(document.querySelector('.mPanel').textContent);
});
check('the rig menu offers Meridian', sat);
await page.evaluate(() => { [...document.querySelectorAll('.mPanel button')].find((b) => /Meridian/.test(b.textContent)).click(); });
const frame = await page.waitForSelector('iframe[title="Meridian"]', { timeout: 5000 }).catch(() => null);
check('Meridian opens full screen', !!frame);
const inner = frame && await frame.contentFrame();
const started = inner && await inner.waitForSelector('#startBtn', { timeout: 60000 }).then(() => true, () => false);
check('Meridian\'s title screen appears', !!started);
if (started) {
  await inner.click('#startBtn');
  await page.waitForTimeout(6000);
}
await page.screenshot({ path: 'build/meridian.png' });
const log1 = await logText(page);
check('the bar paused drawing', /Meridian: open/.test(log1));
await page.click('text=Back to the bar');
await page.waitForTimeout(2500);
check('back in the bar: Meridian is gone and the rig menu is up', !(await page.$('iframe[title="Meridian"]')) &&
  await page.evaluate(() => !!document.querySelector('.mPanel')));
await page.evaluate(() => window.bar3dUi.close());
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/meridian-back.png' });
check('no page errors', errors.length === 0);
if (errors.length) console.log(errors.join('\n'));
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'MERIDIAN TEST PASSED' : 'MERIDIAN TEST FAILED');
process.exit(ok ? 0 : 1);
