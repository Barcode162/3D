// Rhythm game test: opens the 4K MANIA game on a song with ?autoplay, taps
// once (sound unlocks only after a tap), takes screenshots while it plays,
// and waits for the chart and the result in the log.
// The test Chromium can't decode AAC, so this replaces the songs in
// build/site with Ogg copies first (needs ffmpeg).
// Usage: NODE_PATH=$(npm root -g) node tools/rhythm_test.mjs [song] [difficulty]
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { execSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';
const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const song = process.argv[2] ?? '0', difficulty = process.argv[3] ?? '1';
const siteDir = resolve('build/site');
for (const f of ['smooth-lovin', 'bossa-antigua', 'night-on-the-docks-sax', 'lobby-time']) {
  const src = `assets/music/${f}.m4a`, dst = join(siteDir, `assets/music/${f}.m4a`);
  if (existsSync(src)) execSync(`ffmpeg -loglevel error -y -i ${src} -t 50 -c:a libvorbis -f ogg ${dst}`);
}
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
const server = createServer(async (req, res) => {
  let path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  if (path === '/') path = '/index.html';
  try {
    const body = await readFile(join(siteDir, path));
    res.writeHead(200, { 'content-type': types[extname(path)] ?? 'application/octet-stream' });
    res.end(body);
  } catch { res.writeHead(404).end(); }
}).listen(0);
const browser = await chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist',
         '--autoplay-policy=no-user-gesture-required'],
});
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
await page.goto(`http://localhost:${server.address().port}/?skipintro&rhythm=${song},${difficulty}&autoplay`);
const logText = () => page.$eval('#log', (el) => [...el.children].map((c) => c.textContent).join('\n'));
const waitLog = (re, ms) => page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent),
                                                 re.source, { timeout: ms, polling: 250 }).catch(() => console.log('timed out: ' + re));
await waitLog(/Rhythm chart:/, 180000);
await page.mouse.click(215, 500);  // unlocks sound, starts the song
await page.waitForTimeout(6000);
await page.screenshot({ path: 'build/rhythm-1.png' });
await page.waitForTimeout(5000);
await page.screenshot({ path: 'build/rhythm-2.png' });
await waitLog(/Rhythm game result/, 120000);
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/rhythm-result.png' });
await page.mouse.click(215, 761);  // SONGS: back to the song list
await page.waitForTimeout(2500);
await page.screenshot({ path: 'build/rhythm-menu.png' });
const log = await logText();
console.log(log.split('\n').filter((l) => /Rhythm|Chart|error/i.test(l)).join('\n'));
await browser.close();
server.close();
const ok = /Rhythm game result/.test(log) && !/\[error\]/.test(log);
console.log(ok ? 'RHYTHM TEST PASSED' : 'RHYTHM TEST FAILED');
process.exit(ok ? 0 : 1);
