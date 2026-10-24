// Every control the Handbook lists is on its page, and every control on the
// page is listed (restructure J6.4): the pages a test can open (Dev's demo,
// the UI Editor, the Dials, World notes, the site, the news page) crawled at
// phone width (tools/lib/labels.mjs) against the first column of their tables
// in docs/handbook/ui/<page>.md. A new button, a renamed tab or a removed
// field fails until its page says so.
// Usage: NODE_PATH=$(npm root -g) node tools/ui_labels_test.mjs
import { readFileSync, existsSync } from 'node:fs';
import { crawl, PAGES } from './lib/labels.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const listed = (name) => {
  const f = `docs/handbook/ui/${name}.md`;
  if (!existsSync(f)) return null;
  return [...readFileSync(f, 'utf8').matchAll(/^\| `([^`]*)` \|/gm)].map((m) => m[1]);
};

const live = await crawl();
for (const name of Object.keys(PAGES)) {
  const doc = listed(name), on = live[name] || [];
  if (!doc) { check(false, `${name}: docs/handbook/ui/${name}.md exists`); continue; }
  const missing = on.filter((l) => !doc.includes(l)), gone = doc.filter((l) => !on.includes(l));
  check(!missing.length && !gone.length, `${name}: the ${on.length} controls on the page are the ${doc.length} in its Handbook page` +
    (missing.length ? `; not in the Handbook: ${missing.map((x) => `"${x}"`).join(', ')}` : '') +
    (gone.length ? `; not on the page: ${gone.map((x) => `"${x}"`).join(', ')}` : ''));
}
console.log(failed ? `UI LABELS TEST: ${failed} FAILED` : 'UI LABELS TEST PASSED');
process.exit(failed ? 1 : 0);
