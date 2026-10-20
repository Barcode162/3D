// Opens the live site in headless Chromium (software WebGPU) and prints the
// build label and the on-screen log, e.g. to confirm a deploy loaded its models.
// Usage: NODE_PATH=$(npm root -g) node tools/check_live.mjs [url] [screenshot.png]
// The url can carry ?cam=x,y,z,yaw,pitch to look at a specific spot.
// EXPECT_SHA=abc1234 exits 1 unless that build is the one live. The test
// player stays offline (doesn't show up in the real room) unless NET=1.
// The bar is private (docs/LOOPED.md A3): HUB_KEY (the hub's Claude key) is
// given to joshuavelasco.com only, as the gate's cookie bar_k.
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';

const url = process.argv[2] ?? 'https://joshuavelasco.com/bar/';
const shot = process.argv[3] ?? 'build/live.png';
const [base, query = ''] = url.split('?');
const options = ['skipintro', process.env.NET ? '' : 'server=off', query].filter(Boolean).join('&');

const browser = await launchBrowser();
const context = await browser.newContext({ viewport: { width: 430, height: 932 } });
const host = new URL(base).hostname;
if (process.env.HUB_KEY && /(^|\.)joshuavelasco\.com$/.test(host)) {
  await context.addCookies([{ name: 'bar_k', value: process.env.HUB_KEY, domain: host, path: '/', secure: true, httpOnly: true, sameSite: 'Lax' }]);
}
const page = await context.newPage();
const loaded = await openBar(page, base, options);
if (!loaded) console.log('(still loading after 120 s, or startup failed)');
await page.waitForTimeout(2000);
const build = await page.$eval('#build', (e) => e.textContent);
console.log('build', build);
console.log(await logText(page));
const errors = await logErrors(page);
await page.screenshot({ path: shot });
await browser.close();

let ok = loaded && errors.length === 0;
if (process.env.EXPECT_SHA && build !== process.env.EXPECT_SHA) {
  console.log(`WRONG BUILD: live is ${build}, expected ${process.env.EXPECT_SHA}`);
  ok = false;
}
console.log(ok ? 'LIVE CHECK PASSED' : 'LIVE CHECK FAILED');
process.exit(ok ? 0 : 1);
