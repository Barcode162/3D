// Smoke test: serves build/site, opens it in headless Chromium with software
// WebGPU, waits, then prints the on-screen log and saves a screenshot.
// Usage: node tools/smoke.mjs [siteDir] [screenshot.png]
// Fails (exit 1) if the log shows any error or the app never starts running.
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
let playwright;
try {
  playwright = require('playwright');
} catch {
  playwright = require(join(process.execPath, '../../lib/node_modules/playwright'));
}

const siteDir = resolve(process.argv[2] ?? 'build/site');
const shotPath = process.argv[3] ?? 'build/smoke.png';
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };

const server = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  try {
    const body = await readFile(join(siteDir, path === '/' ? 'index.html' : path));
    res.writeHead(200, { 'content-type': types[extname(path)] ?? 'text/html' });
    res.end(body);
  } catch {
    res.writeHead(404).end();
  }
}).listen(0);
const port = server.address().port;

const browser = await playwright.chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'],
});
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
await page.goto(`http://localhost:${port}/`);
await page.waitForTimeout(4000);

const log = await page.$eval('#log', (el) => el.innerText);
const errors = await page.$$eval('#log .err', (els) => els.map((e) => e.textContent));
const status = await page.$eval('#status', (el) => el.textContent);
await page.screenshot({ path: shotPath });
await browser.close();
server.close();

console.log('--- status ---\n' + status + '\n--- log ---\n' + log);
const ok = errors.length === 0 && log.includes('Running.');
console.log(ok ? 'SMOKE TEST PASSED' : 'SMOKE TEST FAILED');
process.exit(ok ? 0 : 1);
