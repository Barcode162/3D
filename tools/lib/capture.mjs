// Screenshots + performance numbers of the bar at the standard spots
// (tools/lib/spots.mjs), shared by piece.mjs, bench.mjs and beforeafter.mjs.
import { mkdirSync } from 'node:fs';
import { serveSite } from './site.mjs';
import { launchBrowser, logErrors, openBar } from './browser.mjs';
import { SPOTS } from './spots.mjs';

// One browser and site for a batch of captures: open() → shot()... → close().
export async function openCapture() {
  const site = serveSite();
  const browser = await launchBrowser();
  const [vw, vh] = (process.env.VIEW ?? '430x932').split('x').map(Number);
  return {
    vw, vh, browser, site,
    // A spot ("bartop") with page options ("bounce=0"). Saves `path` (PNG)
    // when given; returns { png (Buffer), perf (window.bar3dPerf), errors }.
    async shot(spot, option = '', path = null, waitMs = +(process.env.WAIT ?? 4000)) {
      const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 1 });
      const cam = SPOTS[spot] ?? spot;  // a spot name, or "x,y,z,yaw,pitch"
      // Bounced light (L1.5) is worked out after loading; the tests' software
      // GPU is slow, so pictures use the lighter build and wait for it.
      const classic = /(^|&)(light=classic|bounce=0)/.test(option);
      await openBar(page, site.url, `skipintro&server=off&still&cam=${cam}${classic ? '' : '&gilite=1'}${option ? '&' + option : ''}`);
      if (!classic) {
        await page.waitForFunction(() => /Bounced light: ready/.test(document.getElementById('log').textContent), null,
                                   { timeout: +(process.env.GI_WAIT ?? 90000) }).catch(() => {});
      }
      await page.waitForTimeout(waitMs);
      const png = await page.screenshot(path ? { path } : {});
      const perf = await page.evaluate(() => window.bar3dPerf).catch(() => null);
      const errors = await logErrors(page);
      await page.close();
      return { png, perf, errors };
    },
    // Two pictures side by side with labels, as a JPEG (small enough to keep
    // every piece's pictures in the gallery branch).
    async sideBySide(a, b, labelA, labelB, path, title = '') {
      const page = await browser.newPage({ viewport: { width: vw * 2 + 12, height: vh + 70 }, deviceScaleFactor: 1 });
      const img = (buf) => `data:image/png;base64,${buf.toString('base64')}`;
      await page.setContent(`<body style="margin:0;background:#111;color:#eee;font:15px sans-serif">
        <div style="height:30px;line-height:30px;padding-left:8px;font-weight:bold">${title}</div>
        <div style="display:flex;gap:12px">
        <div><div style="height:40px;line-height:40px;padding-left:8px;color:#bbb">before: ${labelA}</div><img src="${img(a)}"></div>
        <div><div style="height:40px;line-height:40px;padding-left:8px;color:#fc6">after: ${labelB}</div><img src="${img(b)}"></div>
        </div></body>`);
      await page.screenshot({ path, type: 'jpeg', quality: 72 });
      await page.close();
    },
    async close() { await browser.close(); site.close(); },
  };
}

export const SPOT_NAMES = Object.keys(SPOTS);
export function ensureDir(d) { mkdirSync(d, { recursive: true }); return d; }
