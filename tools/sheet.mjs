// A contact sheet: one spot with several page options, in one labelled
// picture, for tuning by eye (docs/AUTOMATION.md):
//   node tools/sheet.mjs bartop "light=classic" "" "eyekeep=0.3" "eyekeep=0.6"
//   SPOTS=bartop,piano node tools/sheet.mjs - "a" "b"    (rows = spots)
// Saves OUT (default build/sheet.jpg). SCALE (default 0.5) shrinks each shot.
import { openCapture } from './lib/capture.mjs';

const [spotArg, ...options] = process.argv.slice(2);
const spots = spotArg === '-' ? (process.env.SPOTS ?? 'bartop').split(',') : [spotArg ?? 'bartop'];
const opts = options.length ? options : [''];
const out = process.env.OUT ?? 'build/sheet.jpg';
const scale = +(process.env.SCALE ?? 0.5);
const cap = await openCapture();
const cells = [];
for (const spot of spots) {
  for (const o of opts) {
    const r = await cap.shot(spot, o);
    if (r.errors.length) console.log(`errors (${spot} ${o}):\n${r.errors.join('\n')}`);
    cells.push({ spot, o, src: `data:image/png;base64,${r.png.toString('base64')}` });
  }
}
const w = Math.round(cap.vw * scale), h = Math.round(cap.vh * scale);
const page = await cap.browser.newPage({ viewport: { width: opts.length * (w + 6), height: spots.length * (h + 26) }, deviceScaleFactor: 1 });
await page.setContent(`<body style="margin:0;background:#111;color:#eee;font:12px sans-serif">
  <div style="display:grid;grid-template-columns:repeat(${opts.length},${w + 6}px)">
  ${cells.map((c) => `<div><div style="height:24px;line-height:24px;padding-left:4px;white-space:nowrap;overflow:hidden">${c.spot} ${c.o || '(as is)'}</div><img width="${w}" height="${h}" src="${c.src}"></div>`).join('')}
  </div></body>`);
await page.screenshot({ path: out, type: 'jpeg', quality: 80 });
await cap.close();
console.log(out);
