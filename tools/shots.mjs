// Screenshots of the page with different options, for checking features
// headless: node tools/shots.mjs "sit=0" "darts" "pianosong=0&cam=..." ...
// Each argument is added to the page URL (with ?skipintro); saves
// build/shot-1.png, ... and prints any errors from the on-screen log.
// WAIT=ms waits longer after loading (default 2500). VIEW=932x430 sets the
// viewport (default 430x932, an iPhone held upright). HOLD=ArrowUp:9000
// holds a key that long before the shot (e.g. to drive in the race).
// An argument with its own server= replaces the default server=off.
// REQUESTS=1 prints every request to a host other than this test server.
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const siteDir = resolve('build/site');
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
const server = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  try {
    const file = path === '/' ? 'index.html' : path;
    const body = await readFile(join(siteDir, file));
    res.writeHead(200, { 'content-type': types[extname(file)] ?? 'application/octet-stream' });
    res.end(body);
  } catch {
    res.writeHead(404).end();
  }
}).listen(0);
const port = server.address().port;
const browser = await playwright.chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist',
         '--autoplay-policy=no-user-gesture-required'],
});
let failed = false;
const args = process.argv.slice(2);
for (let i = 0; i < args.length; ++i) {
  const [vw, vh] = (process.env.VIEW ?? '430x932').split('x').map(Number);
  const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 1 });
  const outside = [];
  page.on('request', (r) => { const u = new URL(r.url()); if (u.port !== String(port)) outside.push(r.url()); });
  page.on('websocket', (w) => outside.push(w.url()));
  const serverOpt = /(^|&)server=/.test(args[i]) ? '' : 'server=off&';
  await page.goto(`http://localhost:${port}/?skipintro&${serverOpt}${args[i]}`);
  await page.waitForFunction(() => /Everything loaded|Startup failed/.test(document.getElementById('log').textContent),
                             null, { timeout: 120000 }).catch(() => {});
  // A click unlocks sound (piano notes need it).
  await page.mouse.click(5, 900);
  await page.waitForTimeout(+(process.env.WAIT ?? 2500));
  if (process.env.HOLD) {
    const [key, ms] = process.env.HOLD.split(':');
    await page.keyboard.down(key);
    await page.waitForTimeout(+ms);
    await page.screenshot({ path: `build/shot-${i + 1}-held.png` });
    await page.keyboard.up(key);
  }
  await page.screenshot({ path: `build/shot-${i + 1}.png` });
  const errors = await page.$$eval('#log .err', (els) => els.map((e) => e.textContent));
  const log = await page.$eval('#log', (el) => el.textContent);
  const lines = log.split('[info]').filter((l) => /Piano|Bartender|Darts|Voice|seat|Seats|Race|Multiplayer|DBG/i.test(l));
  console.log(`--- shot-${i + 1}: ${args[i]}\n${lines.slice(-(+(process.env.LINES ?? 4))).join('\n')}`);
  if (process.env.REQUESTS) console.log('requests outside: ' + (outside.length ? [...new Set(outside)].join(' ') : 'none'));
  if (errors.length) { failed = true; console.log('ERRORS:\n' + errors.join('\n')); }
  await page.close();
}
await browser.close();
server.close();
process.exit(failed ? 1 : 0);
