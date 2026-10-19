// Avatar customizer test (upgrade plan item 14): opens Menu → Avatar → Make
// my own look, tries several styles (screenshots build/avatar-*.png, the
// camera in front of you), checks the game wears it and tells the menu, and
// that the look comes back after a reload (remembered on the phone), and
// that walking feet stay planted (?walkcheck).
// Usage: NODE_PATH=$(npm root -g) node tools/avatar_test.mjs
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { SPOTS } from './lib/spots.mjs';

const site = serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
let failed = false;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed = true; };
const until = async (fn, ms = 20000, arg = null) => {
  for (let t = 0; t < ms; t += 250) { if (await page.evaluate(fn, arg)) return true; await page.waitForTimeout(250); }
  return false;
};
const tab = (name) => page.click(`.avTab:has-text("${name}")`);
const chip = (text) => page.click(`.avChip:text-is("${text}")`);
const swatch = (row, i) => page.click(`h3:text-is("${row}") + .avSwatches .avSwatch >> nth=${i}`);

await openBar(page, site.url, `skipintro&server=off&cam=${SPOTS.mirror}`);
await page.waitForTimeout(1500);
check(await until(() => !!(bar3dUi.info().avatarPalette || {}).cloth), 'the game sent the palette');
await page.click('#hudMenu');
await page.click('.mTile:has-text("Avatar")');
await page.click('.mBtn:has-text("own look")');
check(await until(() => bar3dUi.info().look === -1), 'the game wears a custom look');
const LOOKS = [
  ['fem', async () => { await chip('Feminine'); await tab('Hair'); await chip('Long'); await swatch('Colour', 6);
                        await chip('Tips'); await swatch('Highlight colour', 10); await tab('Top'); await chip('Tank top');
                        await swatch('Colour', 5); await tab('Bottom'); await chip('Skirt'); await swatch('Colour', 9); }],
  ['hoodie', async () => { await tab('Body'); await chip('Masculine'); await chip('Beard'); await swatch('Skin', 6);
                           await tab('Hair'); await chip('Afro'); await chip('None'); await tab('Top'); await chip('Hoodie');
                           await chip('Gradient'); await swatch('Second colour', 12); await tab('Bottom'); await chip('Joggers'); }],
  ['jacket', async () => { await tab('Hair'); await chip('Mohawk'); await chip('Streaks'); await swatch('Highlight colour', 12);
                           await tab('Top'); await chip('Jacket'); await chip('Solid'); await tab('Bottom'); await chip('Shorts');
                           await chip('Stripes'); }],
];
for (const [name, steps] of LOOKS) {
  await steps();
  await page.waitForTimeout(1200);
  await page.screenshot({ path: `build/avatar-${name}.png` });
  console.log(`     build/avatar-${name}.png: ${JSON.stringify(await page.evaluate(() => bar3dUi.info().avatarStyle))}`);
}
const style = await page.evaluate(() => JSON.stringify(bar3dUi.info().avatarStyle));
await page.click('.mX');
check(await until(() => !bar3dUi.isOpen()), 'the customizer closes');

await page.reload();
await page.waitForTimeout(1500);
check(await until((want) => bar3dUi.info().look === -1 && JSON.stringify(bar3dUi.info().avatarStyle) === want, 20000, style),
      'after a reload you still wear your look');

// The walk (upgrade item 15): planted feet barely slide (app/avatar.cpp walkReport).
await openBar(page, site.url, 'skipintro&server=off&walkcheck');
const report = (await logText(page)).split('\n').find((l) => l.includes('Walk check')) || '';
console.log('     ' + report.replace(/;/g, ';\n     '));
const LIMITS = { forward: 8, backward: 8, slow: 20, sideways: 40 };
for (const [name, limit] of Object.entries(LIMITS)) {
  const m = new RegExp(`${name}: planted feet slide (\\d+)%`).exec(report);
  check(!!m && +m[1] <= limit, `walking ${name}: feet slide ${m ? m[1] : '?'}% (at most ${limit}%)`);
}
await browser.close();
site.close();
process.exit(failed ? 1 : 0);
