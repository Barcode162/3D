// Before/after pictures of the standard spots (tools/lib/spots.mjs), side by
// side, for each step's hub post and for judging a visual change by eye.
//   node tools/beforeafter.mjs "light=classic" ""      two page options
//   node tools/beforeafter.mjs "bounce=0" "" bartop piano   only these spots
// Saves OUT (default build/beforeafter/)<spot>.png: left = the first option
// ("before"), right = the second ("after"), labelled. WAIT=ms as shots.mjs.
import { mkdirSync } from 'node:fs';
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logErrors, openBar } from './lib/browser.mjs';
import { SPOTS } from './lib/spots.mjs';

const [before = 'light=classic', after = '', ...only] = process.argv.slice(2);
const out = process.env.OUT ?? 'build/beforeafter';
mkdirSync(out, { recursive: true });
const names = only.length ? only : Object.keys(SPOTS);
const [vw, vh] = (process.env.VIEW ?? '430x932').split('x').map(Number);

const site = serveSite();
const browser = await launchBrowser();
let failed = false;
async function shot(spot, option) {
  const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 1 });
  await openBar(page, site.url, `skipintro&server=off&still&cam=${SPOTS[spot]}${option ? '&' + option : ''}`);
  await page.waitForTimeout(+(process.env.WAIT ?? 4000));
  const png = await page.screenshot();
  const errors = await logErrors(page);
  if (errors.length) { failed = true; console.log(`ERRORS (${spot}, ${option || 'as is'}):\n` + errors.join('\n')); }
  await page.close();
  return png.toString('base64');
}
const label = (s) => (s ? s.replace(/&/g, ' ') : 'now');
for (const spot of names) {
  if (!SPOTS[spot]) { console.log(`No spot "${spot}" (${Object.keys(SPOTS).join(', ')})`); continue; }
  const a = await shot(spot, before), b = await shot(spot, after);
  const page = await browser.newPage({ viewport: { width: vw * 2 + 12, height: vh + 40 }, deviceScaleFactor: 1 });
  await page.setContent(`<body style="margin:0;background:#111;color:#eee;font:16px sans-serif">
    <div style="display:flex;gap:12px"><div><div style="height:40px;line-height:40px;padding-left:8px">before: ${label(before)}</div><img src="data:image/png;base64,${a}"></div>
    <div><div style="height:40px;line-height:40px;padding-left:8px">after: ${label(after)}</div><img src="data:image/png;base64,${b}"></div></div></body>`);
  await page.screenshot({ path: `${out}/${spot}.png` });
  await page.close();
  console.log(`${out}/${spot}.png`);
}
await browser.close();
site.close();
process.exit(failed ? 1 : 0);
