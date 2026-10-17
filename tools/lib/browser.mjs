// Headless Chromium with software WebGPU (SwiftShader), shared by the tests.
import { createRequire } from 'node:module';
import { join } from 'node:path';

const require = createRequire(import.meta.url);

export function loadPlaywright() {
  try { return require('playwright'); } catch {}
  try { return require(join(process.cwd(), 'node_modules/playwright')); } catch {}
  return require(join(process.execPath, '../../lib/node_modules/playwright'));
}

export const GPU_ARGS = ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
                         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'];

export function launchBrowser(extraArgs = []) {
  return loadPlaywright().chromium.launch({ args: [...GPU_ARGS, ...extraArgs] });
}

// The log panel as text, one line per entry.
export function logText(page) {
  return page.$eval('#log', (el) => [...el.children].map((c) => c.textContent).join('\n'));
}
export function logErrors(page) {
  return page.$$eval('#log .err', (els) => els.map((e) => e.textContent));
}

// Opens the bar with page options (query without "?") and waits until every
// model is in (or startup failed). Returns true if everything loaded.
export async function openBar(page, url, query, timeoutMs = 120000) {
  await page.goto(`${url}?${query}`);
  return page.waitForFunction(() => /Everything loaded|Startup failed/.test(document.getElementById('log').textContent),
                              null, { timeout: timeoutMs })
    .then(() => page.evaluate(() => /Everything loaded/.test(document.getElementById('log').textContent)))
    .catch(() => false);
}
