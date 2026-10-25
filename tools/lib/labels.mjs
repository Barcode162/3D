// The controls every page shows, by their exact labels (restructure J6.4):
// the pages a test can open without the game (Dev in its ?demo mode, the UI
// Editor, the Dials, World notes, the site's pages, the news page) on a
// throwaway copy of the webspace, at phone width. A control is a button, a
// tab, a field, a switch or a link in a page's navigation; its label is what
// it says (its aria-label, else its words, else its placeholder). Numbers in
// a label read as N ("Show all N"), so data doesn't change the list.
//   const pages = await crawl();   → { 'dev-overview': ['Menu', ...], ... }
// Used by tools/ui_labels_test.mjs (against docs/handbook/ui/*.md) and by
// node tools/handbook.mjs labels (prints them, to write a page's table).
import { readdirSync, mkdirSync, copyFileSync } from 'node:fs';
import { join } from 'node:path';
import { webspace } from './webspace.mjs';
import { launchBrowser } from './browser.mjs';

// Each Handbook page (docs/handbook/ui/<name>.md): the address it covers and
// the part of the page it lists (a frame shared by several pages is listed once),
// and what to tap first to get there (a page's other sections).
export const PAGES = {
  'dev-frame': ['dev/?demo#plan/engine', '#tabs, #bar'],  // a page one step down, so Back shows (J90)
  'dev-overview': ['dev/?demo#overview', '#view'], 'dev-projects': ['dev/?demo#projects', '#view'], 'dev-plan': ['dev/?demo#plan', '#view'],
  'dev-live-tile': ['dev/?demo#overview', '.dev-sheet', '#view [data-tile="bar"]'], 'dev-find': ['dev/?demo#overview', '.dev-find', '#find'],
  'dev-project': ['dev/?demo#projects/bar3d', '#view'], 'dev-chapter': ['dev/?demo#plan/engine', '#view'], 'dev-plan-done': ['dev/?demo#plan/done', '#view'],
  'dev-publish': ['dev/?demo#publish', '#view'],
  'dev-publish-highlights': ['dev/?demo#publish', '#view', '#view [data-id="pub-tab"] button:nth-child(2)'],
  'dev-publish-site': ['dev/?demo#publish', '#view', '#view [data-id="pub-tab"] button:nth-child(3)'],
  'dev-people': ['dev/?demo#people', '#view'],
  'ui-editor': ['toolbelt/ui-editor/', 'body'], 'dials': ['hub/dials.html', 'body'], 'world-notes': ['hub/world.html', 'body'],
  'site-frame': ['', 'header.top, footer.foot'],
  'site-home': ['', '#main'], 'site-projects': ['projects/', '#main'], 'site-play': ['play/', '#main'], 'site-engine': ['engine/', '#main'],
  'site-vision': ['vision/', '#main'], 'site-signin': ['signin/', '#main'], 'news': ['news/', 'main'],
};

// The labels in `scope` now (visible controls only), sorted, each once. What's
// drawn from data (Dev's list rows, its chips and tool tiles, a key named after
// a project or a chapter: .dev-data) and the UI Editor's preview aren't controls of the page.
const READ = (scope) => {
  const seen = new Set();
  const visible = (el) => { const r = el.getBoundingClientRect(), s = getComputedStyle(el); return r.width > 0 && r.height > 0 && s.visibility !== 'hidden' && s.display !== 'none'; };
  const sel = 'button, [role=button], [role=tab], [role=switch], input:not([type=hidden]), textarea, select, a[href]';
  for (const root of document.querySelectorAll(scope)) for (const el of root.querySelectorAll(sel)) {
    if (!visible(el) || el.closest('.dev-row-btn, .dev-data, .dev-chips, #frame, .ed-tree')) continue;  // Dev's data; the UI Editor's preview and parts list (the screen being edited)
    let t = el.getAttribute('aria-label') || (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA' ? el.placeholder : el.innerText) || el.placeholder || el.title || '';
    t = t.split('\n').map((x) => x.trim()).filter(Boolean)[0] || '';
    t = t.replace(/(?<![A-Za-z])\d+([.,:]\d+)*(?![A-Za-z])/g, 'N').replace(/\s+/g, ' ').trim();
    if (t) seen.add(t);
  }
  return [...seen].sort((a, b) => a.localeCompare(b));
};

export async function crawl(only = null) {
  const dirs = { ui: 'ui', dev: 'dev', toolbelt: 'toolbelt', hub: 'hub', studio: 'studio', news: 'news', 'bar/assets/world': 'assets/world' };
  for (const f of readdirSync('homepage')) dirs[f] = join('homepage', f);
  const ws = await webspace({ dirs, files: { 'bar/account.php': 'server/account.php' }, env: { SITE_TOOLS: join(process.cwd(), 'toolbelt/tools.json') } });
  // Dev's demo mode reads the plan straight from these files.
  mkdirSync(join(ws.root, 'home/docs/plan'), { recursive: true }); mkdirSync(join(ws.root, 'home/docs/charter'), { recursive: true });
  copyFileSync('docs/plan/plan.json', join(ws.root, 'home/docs/plan/plan.json'));
  copyFileSync('docs/charter/rules.json', join(ws.root, 'home/docs/charter/rules.json'));
  const browser = await launchBrowser(), out = {};
  try {
    for (const [name, [path, scope, tap]] of Object.entries(PAGES)) {
      if (only && !only.includes(name)) continue;
      const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
      await page.goto(ws.url + path, { waitUntil: 'load' }).catch(() => {});
      await page.waitForTimeout(1500);  // the page fills its slots
      if (tap) { await page.click(tap).catch(() => {}); await page.waitForTimeout(800); }
      out[name] = await page.evaluate(READ, scope).catch(() => []);
      await page.close();
    }
  } finally { await browser.close(); ws.close(); }
  return out;
}
