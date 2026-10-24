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

// TEST_LIGHT=test (deploy.yml sets it; or classic): GitHub's machines draw
// in software, too slowly for the real light (docs/records/known-issues.md
// KI-5), so there the bar's page opens with ?light=test, the tests' own
// lighting (bar/test_hooks.h). Only the bar's
// own page gets it (the site root or /bar/, with or without index.html), and
// only when its address names no light itself; it's added before the page's
// scripts run. Unset (here, and for the pictures): the new light.
export function testLightScript(light) {
  return `(() => { try {
    if (!/^https?:$/.test(location.protocol) || /[?&]light=/.test(location.search)) return;
    if (!/^\\/(bar\\/)?(index\\.html)?$/.test(location.pathname)) return;
    history.replaceState(history.state, '', location.pathname + (location.search ? location.search + '&' : '?') + 'light=' + ${JSON.stringify(light)} + location.hash);
  } catch (e) {} })();`;
}

// GitHub's runners are sometimes slow (software WebGPU at 1-2 frames a
// second): every page waits up to 90 s there for a click or a screenshot
// (30 s locally), so a busy runner doesn't fail a test that's fine.
export async function launchBrowser(extraArgs = []) {
  const browser = await loadPlaywright().chromium.launch({ args: [...GPU_ARGS, ...extraArgs] });
  const wait = process.env.CI ? 90000 : 30000;
  const light = process.env.TEST_LIGHT || '';
  const newPage = browser.newPage.bind(browser), newContext = browser.newContext.bind(browser);
  browser.newPage = async (...args) => {
    const page = await newPage(...args);
    page.setDefaultTimeout(wait);
    if (light) await page.addInitScript(testLightScript(light));
    return page;
  };
  browser.newContext = async (...args) => {
    const context = await newContext(...args);
    if (light) await context.addInitScript(testLightScript(light));
    return context;
  };
  return browser;
}

// The log panel as text, one line per entry.
export function logText(page) {
  return page.$eval('#log', (el) => [...el.children].map((c) => c.textContent).join('\n'));
}
export function logErrors(page) {
  return page.$$eval('#log .err', (els) => els.map((e) => e.textContent));
}

// Opens the bar with page options (query without "?") and waits until every
// model is in (or startup failed). Returns true if everything loaded. With
// `skipintro` it also waits until you're in the bar (past the warm-up, which
// is slow here: the software GPU unpacks compressed textures itself).
export async function openBar(page, url, query, timeoutMs = 120000) {
  await page.goto(`${url}?${query}`);
  const loaded = await page.waitForFunction(() => /Everything loaded|Startup failed/.test(document.getElementById('log').textContent),
                                            null, { timeout: timeoutMs })
    .then(() => page.evaluate(() => /Everything loaded/.test(document.getElementById('log').textContent)))
    .catch(() => false);
  if (loaded && /(^|&)skipintro(&|$)/.test(query)) {
    await page.waitForFunction(() => /Stage entered/.test(document.getElementById('log').textContent), null,
                               { timeout: timeoutMs }).catch(() => {});
  }
  return loaded;
}
