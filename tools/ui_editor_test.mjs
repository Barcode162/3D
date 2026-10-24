// The UI Editor (toolbelt/ui-editor/, docs/UI_PLAN.md R1.4-R1.7) at phone
// width: it loads the theme and screen files, draws the preview, and a tap
// selects a part in both the preview and the list. Later pieces add their
// checks below. A static page: seconds, no game.
// Usage: NODE_PATH=$(npm root -g) node tools/ui_editor_test.mjs
import { launchBrowser } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = serveSite('.');
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
page.on('response', (r) => { if (r.status() >= 400) errors.push(r.status() + ' ' + r.url()); });
await page.goto(url + 'toolbelt/ui-editor/index.html');
await page.waitForFunction(() => document.querySelectorAll('#tree button').length > 0);
const ed = (fn, arg) => page.evaluate(fn, arg);

// R1.4: the shell
check(await ed(() => document.querySelectorAll('#frame [data-path]').length > 20), 'the preview draws the sampler, every part marked');
check(await ed(() => document.documentElement.scrollWidth <= 390), 'nothing wider than the phone');
await page.click('#frame .key.go');
const sel = await ed(() => ({ sel: UIEditor.state.sel, tree: document.querySelector('#tree [aria-current="true"]')?.dataset.path, title: document.getElementById('inspTitle').textContent, outlined: !!document.querySelector('#frame .ed-sel.go') }));
check(sel.sel !== null && sel.tree === sel.sel, `a tap on Publish selects it in the list too (${sel.sel})`);
check(sel.title === 'Key' && sel.outlined, 'the inspector says Key; the preview outlines it');
await page.click('#tree button[data-path=""]');
check(await ed(() => UIEditor.state.sel === ''), 'a tap in the list selects the root');
await page.click('[data-device="computer"]');
check(await ed(() => document.getElementById('frame').classList.contains('ed-computer')), 'Computer widens the preview');
await page.selectOption('#themePick', 'jukebox');
check(await ed(() => document.documentElement.dataset.look === 'jukebox' && getComputedStyle(document.getElementById('frame')).getPropertyValue('--accent').trim() === '#FF3B5C'), 'another theme dresses the preview');

check(errors.length === 0, 'no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close(); close();
console.log(failed ? `UI EDITOR TEST: ${failed} FAILED` : 'UI EDITOR TEST PASSED');
process.exit(failed ? 1 : 0);
