// Smoke test: serves build/site, opens it in headless Chromium with software
// WebGPU, waits, then prints the on-screen log and saves a screenshot.
// Usage: node tools/smoke.mjs [siteDir] [screenshot.png]
// Fails (exit 1) unless: every model loaded within 120 s, the log shows no
// error, the profiler numbers exist, and the screenshot isn't blank.
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { readPng, brightnessSpread } from './lib/png.mjs';

const shotPath = process.argv[3] ?? 'build/smoke.png';
const MIN_SPREAD = 8;  // brightness spread (0-255) below this = a blank screen

const site = serveSite(process.argv[2] ?? 'build/site');
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
// ?skipintro closes the loading screen by itself once everything is in.
const loaded = await openBar(page, site.url, 'skipintro');
if (!loaded) console.log('(models not all loaded after 120 s, or startup failed)');
await page.waitForTimeout(1500);

const log = await logText(page);
const errors = await logErrors(page);
const status = await page.$eval('#status', (el) => el.textContent);
await page.screenshot({ path: shotPath });
const spread = brightnessSpread(readPng(shotPath));
// Frame profiler numbers (looped/profiler.cpp). Headless runs at 1-2 fps,
// so judge phase costs and counts here, never the frame rate.
const perf = await page.evaluate(() => window.bar3dPerf);
let perfLine = 'perf: (window.bar3dPerf missing)';
if (perf) {
  const top = Object.entries(perf.phases).sort((a, b) => b[1] - a[1]).slice(0, 4)
    .map(([k, v]) => `${k} ${v.toFixed(2)}`).join(', ');
  perfLine = `perf: avg ${perf.avg.toFixed(1)} p95 ${perf.p95.toFixed(1)} worst ${perf.worst.toFixed(0)} ms, ` +
    `cpu ${perf.cpu.toFixed(2)} ms, hitches ${perf.hitchesTotal}, profiler ${perf.phases.profiler.toFixed(3)} ms/frame\n` +
    `  phases: ${top}\n  max job ${perf.maxJobMs.toFixed(1)} ms, slowest loading frame ${perf.loadingWorstCpu.toFixed(0)} ms (CPU)\n  draws ${JSON.stringify(perf.draws)} tris ${JSON.stringify(perf.tris)} heap ${perf.heapMB} MB (grew ${perf.heapGrowths} times after start)\n` +
    `  stages ${JSON.stringify(Object.fromEntries(Object.entries(perf.stages).filter(([k]) => !k.startsWith('model:'))))}`;
}

// Walk forward, then turn right by dragging, to exercise input + collisions.
await page.keyboard.down('KeyW');
await page.waitForTimeout(2500);
await page.keyboard.up('KeyW');
await page.mouse.move(215, 400);
await page.mouse.down();
await page.mouse.move(415, 420, { steps: 10 });
await page.mouse.up();
await page.waitForTimeout(800);
await page.screenshot({ path: shotPath.replace(/\.png$/, '-walk.png') });

// Look down at your own body (first-person avatar).
await page.mouse.move(215, 300);
await page.mouse.down();
await page.mouse.move(215, 520, { steps: 10 });  // about 70 degrees down
await page.mouse.up();
await page.waitForTimeout(500);
await page.screenshot({ path: shotPath.replace(/\.png$/, '-down.png') });

// Calls from the game into JavaScript per frame (?crossings counts them).
{
  const cp = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  await openBar(cp, site.url, 'skipintro&crossings&server=off');
  await cp.waitForTimeout(1000);
  const a = await cp.evaluate(() => ({ ...JSON.parse(JSON.stringify(window.bar3dCrossings)), pulls: bar3dBridge.pulls() }));
  await cp.waitForTimeout(3000);
  const b = await cp.evaluate(() => ({ ...JSON.parse(JSON.stringify(window.bar3dCrossings)), pulls: bar3dBridge.pulls() }));
  const frames = Math.max(1, b.pulls - a.pulls);  // game frames (one mailbox pull each)
  const per = (b.total - a.total) / frames;
  const rows = Object.keys(b.byName).map((k) => [k, ((b.byName[k] || 0) - (a.byName[k] || 0)) / frames])
    .filter(([, v]) => v > 0.05).sort((x, y) => y[1] - x[1]);
  // With real names (a Debug build) split them: graphics API, clock, page.
  if (rows.some(([k]) => k.length > 3)) {
    const sum = (f) => rows.filter(([k]) => f(k)).reduce((t, [, v]) => t + v, 0);
    const gpu = (k) => /^e?m?wgpu|^emwgpu/.test(k), clock = (k) => k === 'emscripten_get_now';
    const detail = rows.filter(([k]) => k.includes('#')).map(([k, v]) => `${k} ${v.toFixed(1)}`).join(', ');
    const page = rows.filter(([k]) => !gpu(k) && !clock(k) && !k.includes('#')).map(([k, v]) => `${k} ${v.toFixed(1)}`).join(', ');
    console.log(`JS crossings per frame: ${per.toFixed(1)} = graphics ${sum(gpu).toFixed(1)} + clock ` +
                `${sum(clock).toFixed(1)} + page ${sum((k) => !gpu(k) && !clock(k) && !k.includes('#')).toFixed(1)} (${page})` +
                (detail ? `\n  EM_ASM blocks: ${detail}` : ''));
  } else {
    console.log(`JS crossings per frame: ${per.toFixed(1)} (names are minified; use a Debug build to split them)`);
  }
  // Sound commands per frame (they ride the frame's mailbox flush).
  const au = await cp.evaluate(() => window.bar3dAudio && bar3dAudio.stats ? bar3dAudio.stats() : null);
  if (au) console.log(`audio: ${((au.commands - (a.audio || 0)) / Math.max(1, b.pulls)).toFixed(2)} commands per frame, ${au.voices} voices playing`);
  // Rotating the phone: each new size must reconfigure the canvas exactly once
  // (the size settles for 120 ms first).
  const resizes = async () => (await logText(cp)).split('\n').filter((l) => /Canvas resized/.test(l)).length;
  const r0 = await resizes();
  // Headless runs at a few frames per second: wait for the resize, then a
  // bit more to catch a second one.
  const settle = async (from) => {
    for (let t = 0; t < 15000 && (await resizes()) === from; t += 250) await cp.waitForTimeout(250);
    await cp.waitForTimeout(1500);
    return resizes();
  };
  await cp.setViewportSize({ width: 932, height: 430 });
  const r1 = await settle(r0);
  await cp.setViewportSize({ width: 430, height: 932 });
  const r2 = await settle(r1);
  console.log(`rotation: ${r1 - r0} resize(s) to landscape, ${r2 - r1} back`);
  if (r1 - r0 !== 1 || r2 - r1 !== 1) { console.log('SMOKE TEST FAILED: rotation should resize exactly once each way'); process.exit(1); }
  await cp.close();
}

// A piano song: never more than 32 sounds at once (looped/audio.h kMaxVoices).
{
  const pp = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  await openBar(pp, site.url, 'skipintro&server=off&pianosong=0&cam=-3.6,1.65,3.2,-115,-10');
  await pp.mouse.click(5, 900);  // sound starts after a tap
  let most = 0, stolen = 0;
  for (let t = 0; t < 12000; t += 250) {
    const st = await pp.evaluate(() => bar3dAudio.stats());
    most = Math.max(most, st.voices);
    stolen = st.stolen;
    await pp.waitForTimeout(250);
  }
  console.log(`piano song: at most ${most} voices at once (${stolen} replaced)`);
  if (most > 32) { console.log('SMOKE TEST FAILED: more than 32 voices'); process.exit(1); }
  await pp.close();
}

// FAIL_MODEL=name: that model's download is blocked (every retry too); the
// loading screen must say "1 part failed - Retry" and still let you in.
if (process.env.FAIL_MODEL) {
  const failPage = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  await failPage.route(`**/assets/models/${process.env.FAIL_MODEL}.glb.gz*`, (r) => r.abort());
  await failPage.goto(`${site.url}?skipintro`);
  await failPage.waitForFunction(() => /part failed|parts failed/.test(document.getElementById('loadText').textContent),
                                 null, { timeout: 120000 }).catch(() => {});
  const text = await failPage.$eval('#loadText', (e) => e.textContent);
  await failPage.waitForTimeout(1500);
  const entered = await failPage.evaluate(() => /Stage entered/.test(document.getElementById('log').textContent));
  console.log(`failed-download check: "${text}", entered: ${entered}`);
  if (!/1 part failed/.test(text) || !entered) process.exitCode = 1;
  await failPage.close();
}

// Optional close-ups for checking spots: CAM="x,y,z,yaw,pitch" (degrees)
// saves build/smoke-cam.png from there. Several spots separated by ";"
// save build/smoke-cam-1.png, -2.png, ... (one page at a time).
if (process.env.CAM) {
  const cams = process.env.CAM.split(';').map((c) => c.trim()).filter(Boolean);
  for (let i = 0; i < cams.length; ++i) {
    const camPage = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
    await openBar(camPage, site.url, `skipintro&cam=${encodeURIComponent(cams[i])}`);
    await camPage.waitForTimeout(1500);
    const suffix = cams.length === 1 ? '-cam.png' : `-cam-${i + 1}.png`;
    await camPage.screenshot({ path: shotPath.replace(/\.png$/, suffix) });
    await camPage.close();
  }
}
await browser.close();
site.close();

console.log('--- status ---\n' + status + '\n--- log ---\n' + log);
console.log(perfLine);
console.log(`screenshot brightness spread ${spread.toFixed(1)} (blank below ${MIN_SPREAD})`);
const ok = loaded && errors.length === 0 && log.includes('Running.') && !!perf && spread >= MIN_SPREAD;
console.log(ok ? 'SMOKE TEST PASSED' : 'SMOKE TEST FAILED');
process.exit(ok ? 0 : 1);
