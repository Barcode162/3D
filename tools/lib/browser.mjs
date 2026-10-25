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
// lighting (bar/test_hooks.h). TEST_FPS=N (deploy.yml too) adds ?fps=N: the
// game draws at most N frames a second, so the software GPU is free for the
// test's pictures and for a second page (KI-5's cause, J54.1). Only the bar's
// own page gets them (the site root or /bar/, with or without index.html),
// and only what its address doesn't name itself; they're added before the
// page's scripts run. Unset (here, and for the pictures): the new light.
export function testPageScript(params) {
  return `(() => { try {
    if (!/^https?:$/.test(location.protocol)) return;
    if (!/^\\/(bar\\/)?(index\\.html)?$/.test(location.pathname)) return;
    const add = Object.entries(${JSON.stringify(params)}).filter(([k, v]) => v && !new RegExp('[?&]' + k + '=').test(location.search));
    if (!add.length) return;
    const extra = add.map(([k, v]) => k + '=' + v).join('&');
    history.replaceState(history.state, '', location.pathname + (location.search ? location.search + '&' : '?') + extra + location.hash);
  } catch (e) {} })();`;
}
export function testLightScript(light) { return testPageScript({ light }); }
// What GitHub's tests add to the bar's address (empty here).
export function testParams() {
  return { light: process.env.TEST_LIGHT || '', fps: process.env.TEST_FPS || '' };
}

// A test's pictures are for looking at, not checks (KI-5, J54.3): one that
// isn't taken within PICTURE_MS prints a note and the test goes on. A picture
// that *is* a check (the smoke test's "not a blank screen") passes
// `strict: true` and fails as before.
const PICTURE_MS = process.env.CI ? 90000 : 45000;
function notePictures(page) {
  const take = page.screenshot.bind(page);
  page.screenshot = async (opts = {}) => {
    const { strict, ...rest } = opts;
    try {
      return await take({ timeout: PICTURE_MS, ...rest });
    } catch (e) {
      if (strict) throw e;
      console.log(`note: no picture ${rest.path || ''} (${String(e.message).split('\n')[0]}); the test goes on`);
      return null;
    }
  };
  return page;
}

// Every test that opens a browser ends within TEST_LIMIT_MIN minutes (30 by
// default): past it, it says so, closes the browser and fails, instead of
// sitting idle (the Studio test sat 19 min on 2026-10-25, J54.3).
function timeLimit(browser) {
  const min = Number(process.env.TEST_LIMIT_MIN) || 30;
  const name = (process.argv[1] || 'the test').split('/').pop();
  setTimeout(async () => {
    console.log(`TEST FAILED: ${name} ran over its ${min}-minute limit (TEST_LIMIT_MIN)`);
    await Promise.race([browser.close().catch(() => {}), new Promise((r) => setTimeout(r, 5000))]);
    process.exit(1);
  }, min * 60000).unref();
}

// GitHub's runners are sometimes slow (software WebGPU at 1-2 frames a
// second): every page waits up to 90 s there for a click or a screenshot
// (30 s locally), so a busy runner doesn't fail a test that's fine.
export async function launchBrowser(extraArgs = []) {
  const browser = await loadPlaywright().chromium.launch({ args: [...GPU_ARGS, ...extraArgs] });
  timeLimit(browser);
  const wait = process.env.CI ? 90000 : 30000;
  const params = testParams();
  const any = Object.values(params).some(Boolean);
  const newPage = browser.newPage.bind(browser), newContext = browser.newContext.bind(browser);
  browser.newPage = async (...args) => {
    const page = await newPage(...args);
    page.setDefaultTimeout(wait);
    if (any) await page.addInitScript(testPageScript(params));
    return notePictures(page);
  };
  browser.newContext = async (...args) => {
    const context = await newContext(...args);
    if (any) await context.addInitScript(testPageScript(params));
    const contextPage = context.newPage.bind(context);
    context.newPage = async (...a) => {
      const page = await contextPage(...a);
      page.setDefaultTimeout(wait);
      return notePictures(page);
    };
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

// Two game pages on GitHub's machine (2 cores, drawing in software) starve
// each other: a page that only waits draws one frame every 5 s (its logic and
// its connection go on), and draws as before when it's its turn again (KI-5).
export function drawLess(page) {
  return page.evaluate(() => window.bar3dBridge && window.bar3dBridge.command('fps', 0.2));
}
export function drawAgain(page) {
  return page.evaluate((n) => window.bar3dBridge && window.bar3dBridge.command('fps', n), Number(testParams().fps) || 0);
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
