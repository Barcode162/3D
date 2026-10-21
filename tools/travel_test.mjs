// Travel between places (docs/ENGINE.md E7.3): Menu → "Visit the test home"
// loads the home (its own place file and room), Menu → "Back to the bar"
// comes back. Offline (no room).
// Usage: NODE_PATH=$(npm root -g) node tools/travel_test.mjs
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = await serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 430, height: 932 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
const results = [];
const check = (name, ok, extra = '') => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}${extra}`); };
const things = async () => ((await logText(page)).match(/World: (\d+) things/) || [])[1];
const loaded = () => page.waitForFunction(() => /Everything loaded/.test(document.getElementById('log').textContent), null,
                                          { timeout: 120000, polling: 250 }).catch(() => {});

await openBar(page, url, 'skipintro&server=off');
await loaded();
check('the bar loads', +(await things()) > 150, ` (${await things()} things)`);
await page.click('#hudMenu');
await page.click('.mTile:has-text("Visit the test home")');
await page.waitForURL(/place=home/, { timeout: 30000 }).catch(() => {});
await loaded();
check('the menu takes you to the test home', /place=home/.test(page.url()) && +(await things()) < 40, ` (${await things()} things)`);
check('in its own room', /room=home/.test(page.url()));
await page.click('#hudMenu');
await page.click('.mTile:has-text("Back to the bar")');
await page.waitForURL((u) => !/place=home/.test(u.toString()), { timeout: 30000 }).catch(() => {});
await loaded();
check('and back to the bar', !/place=/.test(page.url()) && +(await things()) > 150, ` (${await things()} things)`);
check('no errors', errors.length === 0, errors.join(' '));
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'TRAVEL TEST PASSED' : 'TRAVEL TEST FAILED');
process.exit(ok ? 0 : 1);
