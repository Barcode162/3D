// The labs (J116, Looped rebuilt), on the built site (build/site):
//   the word lab reads a phrase and shows its light;
//   the light lab loads its baked pictures and words, relights, and its
//     bake is within 8% of the exact reference over everything seen;
//   the draw lab draws on WebGPU and on WebGL2, and the two pictures are the
//     same (lights held still) (J116.4: one interface, two backends);
//   the physics lab: a knocked stool settles, the light around it is worked
//     out again, and no frame's own work stutters (J116.5).
// Usage: node tools/labs_test.mjs (after the build).
import { launchBrowser } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const site = serveSite();
const browser = await launchBrowser();
const errorsOf = (page) => { const e = []; page.on('pageerror', (x) => e.push(x.message)); return e; };

try {
  // ---- The word lab ----
  {
    const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
    const errs = errorsOf(page);
    await page.goto(site.url + 'labs/words/index.html');
    await page.waitForFunction(() => document.querySelector('#words .word'));
    await page.fill('#phrase', 'a warm candle that flickers');
    await page.dispatchEvent('#phrase', 'input');
    await page.waitForTimeout(500);
    const lights = await page.textContent('#lights');
    check(/lumens/.test(lights), `word lab: "a warm candle that flickers" gives light (${lights.trim().replace(/\s+/g, ' ')})`);
    check(errs.length === 0, 'word lab: no page errors' + (errs.length ? ': ' + errs[0] : ''));
    await page.close();
  }
  // ---- The light lab ----
  {
    const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
    const errs = errorsOf(page);
    await page.goto(site.url + 'labs/light/index.html');
    await page.waitForFunction(() => document.querySelectorAll('.light').length > 0);
    await page.waitForTimeout(1500);
    const summary = await page.evaluate(() => fetch('data/summary.json').then((r) => r.json()));
    for (const s of summary.spots) check(s.error < 0.08, `light lab: ${s.name} is ${(s.error * 100).toFixed(1)}% off the exact reference (under 8%)`);
    const lit = await page.evaluate(() => {
      const c = document.getElementById('view'), d = c.getContext('2d').getImageData(0, 0, c.width, c.height).data;
      let s = 0;
      for (let i = 0; i < d.length; i += 4) s += d[i] + d[i + 1] + d[i + 2];
      return s / (d.length / 4) / 3;
    });
    check(lit > 10, `light lab: the room is drawn (average brightness ${lit.toFixed(0)} of 255)`);
    const words = await page.textContent('#words');
    check(/when the lights go down/.test(words), 'light lab: the room\'s words are shown, rule included');
    check(errs.length === 0, 'light lab: no page errors' + (errs.length ? ': ' + errs[0] : ''));
    await page.close();
  }
  // ---- The draw lab, on both backends ----
  const pictures = {};
  for (const [name, query] of [['WebGPU', ''], ['WebGL2', '?gl=1']]) {
    const page = await browser.newPage({ viewport: { width: 800, height: 700 } });
    const errs = errorsOf(page);
    await page.goto(site.url + 'labs/draw/index.html' + query);
    await page.waitForFunction(() => /frames/.test(document.getElementById('fps').textContent), null, { timeout: 60000 });
    await page.click('#alive');  // hold the lights still
    await page.waitForTimeout(2500);
    const backend = await page.textContent('#backend'), problem = await page.textContent('#problem');
    check(backend === name, `draw lab: draws with ${name} (says "${backend}")`);
    check(!problem, `draw lab (${name}): no problem shown` + (problem ? ': ' + problem : ''));
    pictures[name] = await page.locator('canvas').screenshot();
    check(errs.length === 0, `draw lab (${name}): no page errors` + (errs.length ? ': ' + errs[0] : ''));
    await page.close();
  }
  // ---- The physics lab: knock a stool over; it settles, the light around it is redone, no frame stutters ----
  {
    const page = await browser.newPage({ viewport: { width: 800, height: 900 } });
    const errs = errorsOf(page);
    await page.goto(site.url + 'labs/physics/index.html?gl=1');
    await page.waitForFunction(() => window.Module && Module._pl_ready && Module._pl_ready() === 1, null, { timeout: 60000 });
    await page.click('#k0');
    await page.waitForFunction(() => Module._pl_state(0) === 1, null, { timeout: 10000 });
    check(true, 'physics lab: the knocked stool moves (lit live)');
    let worst = 0;
    const t0 = Date.now();
    while (Date.now() - t0 < 240000) {
      const st = await page.evaluate(() => [Module._pl_state(0), document.getElementById('fps').textContent]);
      const m = /slowest frame (\d+) ms/.exec(st[1]);
      if (m) worst = Math.max(worst, Number(m[1]));
      if (st[0] === 3) break;
      await page.waitForTimeout(1000);
    }
    const state = await page.evaluate(() => Module._pl_state(0));
    check(state === 3, `physics lab: it comes to rest, the light around it is worked out again, and it joins the room (${((Date.now() - t0) / 1000).toFixed(0)} s here)`);
    check(worst > 0 && worst < 40, `physics lab: no frame's own work stutters (slowest ${worst} ms, under 40)`);
    check(errs.length === 0, 'physics lab: no page errors' + (errs.length ? ': ' + errs[0] : ''));
    await page.close();
  }
  // Compare the two pictures (decoded in the page, which has a PNG reader).
  const page = await browser.newPage();
  const diff = await page.evaluate(async ([a, b]) => {
    const load = async (b64) => {
      const img = new Image();
      img.src = 'data:image/png;base64,' + b64;
      await img.decode();
      const c = new OffscreenCanvas(img.width, img.height), x = c.getContext('2d');
      x.drawImage(img, 0, 0);
      return x.getImageData(0, 0, img.width, img.height);
    };
    const A = await load(a), B = await load(b);
    if (A.width !== B.width || A.height !== B.height) return { size: true };
    let sum = 0, big = 0, bright = 0;
    for (let i = 0; i < A.data.length; i += 4) {
      const d = (Math.abs(A.data[i] - B.data[i]) + Math.abs(A.data[i + 1] - B.data[i + 1]) + Math.abs(A.data[i + 2] - B.data[i + 2])) / 3;
      sum += d;
      if (d > 24) big++;
      bright += A.data[i] + A.data[i + 1] + A.data[i + 2];
    }
    const n = A.data.length / 4;
    return { mean: sum / n, big: big / n, bright: bright / n / 3 };
  }, [pictures.WebGPU.toString('base64'), pictures.WebGL2.toString('base64')]);
  check(!diff.size && diff.bright > 10, `draw lab: the picture isn't empty (average brightness ${diff.bright?.toFixed(0)})`);
  check(!diff.size && diff.mean < 3 && diff.big < 0.01,
        `draw lab: WebGPU and WebGL2 draw the same picture (average difference ${diff.mean?.toFixed(2)} of 255; ${(diff.big * 100).toFixed(2)}% of pixels differ by more than 24)`);
} finally {
  await browser.close();
  site.close();
}
console.log(failed ? `LABS TEST FAILED (${failed})` : 'LABS TEST PASSED');
process.exit(failed ? 1 : 0);
