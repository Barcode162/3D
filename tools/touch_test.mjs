// Touching things (docs/ENGINE.md E3-E4): from a spot in front of each living
// thing, a tap in the middle of the screen must touch it, follow its kind's
// "tap" rule and say so in the log ("World: you touched <id> (<state>)").
// Offline (no room), so this is the phone's own world.
// Usage: NODE_PATH=$(npm root -g) node tools/touch_test.mjs   (SPOT=name for one)
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

// name → camera (x, y, z, yaw, pitch in degrees) and what the log must say.
const SPOTS = {
  lamp:    ['-4.3,1.66,-2.6,0,24', /you touched pendant_lamp-2 \(swinging\)/],
  counter: ['-3.2,1.66,-3.3,0,-35', /you touched bar_counter_l \(knocked\)/],
  backbar: ['-5.5,1.66,-3.5,0,3', /you touched back_bar_tall \(knocked\)/],
  candle:  ['-3.0,1.66,0.3,0,-42.7', /you touched candle-1 \(out\)/],
  fire:    ['-6.6,1.2,0.8,-90,-12', /you touched fire_flames \(out\)/],
  mirror:  ['6.6,1.66,3.6,90,0', /you touched big_mirror \(avatars\)/],
};

const { url, close } = await serveSite();
const browser = await launchBrowser();
const results = [];
const check = (name, ok, extra = '') => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}${extra}`); };
for (const [name, [cam, want]] of Object.entries(SPOTS)) {
  if (process.env.SPOT && process.env.SPOT !== name) continue;
  const page = await browser.newPage({ viewport: { width: 430, height: 932 } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  // The hub's live tuning (E8.2), as the hub would answer: lamps sway twice as fast.
  if (name === 'lamp') await page.route('**/hub/hub.php?tuning=1', (r) => r.fulfill({ contentType: 'application/json',
    body: JSON.stringify({ kinds: { pendant_lamp: { speed: 2, off: false }, nope: { speed: 3 } } }) }));
  await openBar(page, url, `skipintro&server=off&cam=${encodeURIComponent(cam)}`);
  // Every model in (a touch needs the model's box), then a moment more.
  await page.waitForFunction(() => /Everything loaded/.test(document.getElementById('log').textContent), null,
                             { timeout: 120000, polling: 250 }).catch(() => {});
  await page.waitForTimeout(1500);
  await page.mouse.click(215, 466);
  await page.waitForTimeout(800);
  const log = await logText(page);
  const touched = (log.match(/World: you touched [^\n]*/) || ['(nothing touched)'])[0];
  check(`${name}: a tap touches it`, want.test(log), ` — ${touched}`);
  if (name === 'lamp') check('lamp: the hub\'s tuning reaches the bar', /World: pendant_lamp tuned from the hub \(2\.00x, \d+ things\)/.test(log));
  if (!want.test(log)) await page.screenshot({ path: `build/touch-${name}.png` });
  check(`${name}: no errors`, errors.length === 0 && !/error/i.test(log.split('\n').filter((l) => /World|touch/.test(l)).join('\n')), errors.join(' '));
  await page.close();
}
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'TOUCH TEST PASSED' : 'TOUCH TEST FAILED');
process.exit(ok ? 0 : 1);
