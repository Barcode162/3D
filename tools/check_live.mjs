// Opens the live site in headless Chromium (software WebGPU) and prints the
// build label and the on-screen log, e.g. to confirm a deploy loaded its models.
// Usage: NODE_PATH=$(npm root -g) node tools/check_live.mjs [url] [screenshot.png]
// The url can carry ?cam=x,y,z,yaw,pitch to look at a specific spot.
import { createRequire } from 'node:module';
const require = createRequire(import.meta.url);
const { chromium } = require('playwright');
const url = process.argv[2] ?? 'https://barcode162.github.io/3D/';
const shot = process.argv[3] ?? 'build/live.png';
const browser = await chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'],
});
const page = await browser.newPage({ viewport: { width: 430, height: 932 } });
// ?skipintro closes the loading screen by itself once everything is in.
await page.goto(url + (url.includes('?') ? '&' : '?') + 'skipintro');
await page.waitForFunction(() => /Everything loaded/.test(document.getElementById('log').textContent), null,
                           { timeout: 120000 }).catch(() => console.log('(still loading after 120 s)'));
await page.waitForTimeout(2000);
console.log('build', await page.$eval('#build', (e) => e.textContent));
console.log(await page.$eval('#log', (e) => e.textContent.replace(/\[(info|error)\]/g, '\n[$1]')));
await page.screenshot({ path: shot });
await browser.close();
