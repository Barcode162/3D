// The arcade (upgrade plan items 3-5): stands in front of the platformer
// machine, taps it, checks the user's platformer ("Test 2",
// web/arcade/platformer/) opens full screen and draws (build/arcade.png),
// goes back to the bar, and checks the arcade's music came on in there.
// Usage: NODE_PATH=$(npm root -g) node tools/arcade_test.mjs
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = await serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 932, height: 430 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
const results = [];
const check = (name, ok) => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}`); };
check('the bar loads', await openBar(page, url, 'skipintro&cam=4.4,1.65,-13.9,0,-12'));
await page.waitForTimeout(2000);
await page.screenshot({ path: 'build/arcade-machine.png' });
// A tap on the machine (the middle of the screen, where it stands).
await page.mouse.click(466, 215);
await page.waitForTimeout(400);
await page.mouse.click(466, 215);
const frame = await page.waitForSelector('iframe[title="Platformer"]', { timeout: 8000 }).catch(() => null);
check('tapping the machine opens the platformer full screen', !!frame);
const inner = frame && await frame.contentFrame();
const drew = inner && await inner.waitForSelector('canvas', { timeout: 60000 }).then(() => true, () => false);
check('the platformer draws (its canvas is there)', !!drew);
await page.waitForTimeout(4000);
await page.screenshot({ path: 'build/arcade.png' });
const log = await logText(page);
check('the bar paused while it plays', /Platformer: opened/.test(log));
check('the arcade music came on inside the arcade', /Arcade music: on/.test(log));
await page.click('text=Back to the bar');
await page.waitForTimeout(1500);
check('back in the bar: the platformer is gone', !(await page.$('iframe[title="Platformer"]')));
check('no page errors', errors.length === 0);
if (errors.length) console.log(errors.join('\n'));
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'ARCADE TEST PASSED' : 'ARCADE TEST FAILED');
process.exit(ok ? 0 : 1);
