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

// R1.5: theme editing
await page.selectOption('#themePick', 'faceplate');
await page.click('[data-mode="theme"]');
check(await ed(() => document.querySelectorAll('#tree [data-group]').length === 7), 'Theme lists This theme + 6 token groups');
await page.click('#tree [data-group="shape"]');
const before = await ed(() => getComputedStyle(document.querySelector('#frame .key')).borderTopLeftRadius);
await page.$eval('#insp [data-token="key-r"] input[type=range]', (r) => { r.value = '20'; r.dispatchEvent(new Event('input', { bubbles: true })); });
const after = await ed(() => ({ r: getComputedStyle(document.querySelector('#frame .key')).borderTopLeftRadius, tok: UIEditor.state.themes.faceplate.tokens['key-r'], changed: document.querySelector('#insp [data-token="key-r"]').classList.contains('ed-changed') }));
check(before !== after.r && after.r === '20px' && after.tok === '20px', `the key-corner slider rounds every key in the preview (${before} → ${after.r})`);
check(after.changed, 'the changed token is marked');
await page.click('#tree [data-group="colour"]');
await page.$eval('#insp [data-token="accent"] input[type=color]', (c) => { c.value = '#00aaff'; c.dispatchEvent(new Event('input', { bubbles: true })); });
check(await ed(() => getComputedStyle(document.getElementById('frame')).getPropertyValue('--accent').trim() === '#00AAFF'), 'the accent picker recolours the preview');
await page.click('#tree [data-group="surface"]');
await page.$eval('#insp [data-token="over"] input[type=color]', (c) => { c.value = '#ff0000'; c.dispatchEvent(new Event('input', { bubbles: true })); });
check(await ed(() => /^rgba\(255,0,0,0\.86\)$/.test(UIEditor.state.themes.faceplate.tokens.over)), 'a see-through colour keeps its see-through when picked');
await page.click('#undo'); await page.click('#undo');
check(await ed(() => UIEditor.state.themes.faceplate.tokens.accent === '#F28C2C' && UIEditor.state.themes.faceplate.tokens.over === 'rgba(22,20,18,.86)'), 'Undo twice takes both colours back');
await page.click('#redo');
check(await ed(() => UIEditor.state.themes.faceplate.tokens.accent === '#00AAFF'), 'Redo puts the accent back');
await page.click('#tree [data-group="theme"]');
await page.click('#insp >> text=Copy as a new theme');
check(await ed(() => UIEditor.state.themeId === 'faceplate-2' && document.getElementById('themePick').value === 'faceplate-2'), 'Copy makes a new theme and picks it');
await page.click('[data-mode="screen"]');

// R1.6: screen editing
await page.click('#frame .key.go');
const goPath = await ed(() => UIEditor.state.sel);
await page.fill('#insp input[aria-label="text"]', 'Go live');
check(await ed(() => document.querySelector('#frame .key.go').textContent.trim() === 'Go live' && UIEditor.partAt(UIEditor.state.sel).text === 'Go live'), 'typing new words renames the key in the preview');
await page.selectOption('#insp .ed-add', 'chip');
const added = await ed(() => ({ sel: UIEditor.state.sel, type: UIEditor.partAt(UIEditor.state.sel).type }));
check(added.type === 'chip' && added.sel !== goPath, `Add after puts a chip next to the key (${added.sel})`);
await page.click('#insp [aria-label="Move up"]');
check(await ed((p) => UIEditor.state.sel === p && UIEditor.partAt(p).type === 'chip', goPath), 'Move up swaps it before the key');
await page.click('#insp [aria-label="Duplicate"]');
await page.click('#insp [aria-label="Delete"]');
const n = await ed(() => UIEditor.partAt('1.0.1').children.length);
check(n === 4, `Duplicate then Delete leaves one chip added (${n} in the row)`);
await page.click('#tree button[data-path="1.0"]');
await page.selectOption('#insp .ed-add', 'text');
check(await ed(() => UIEditor.state.sel === '1.0.' + (UIEditor.partAt('1.0').children.length - 1) && UIEditor.partAt(UIEditor.state.sel).type === 'text'), 'Add inside a panel puts it last in the panel');
await page.click('#tree button[data-path=""]');
check(await ed(() => document.querySelector('#insp [aria-label="Delete"]').disabled), 'the root can\'t be deleted');
await page.selectOption('#pick', '+new');
check(await ed(() => UIEditor.state.screenId === 'screen-1' && document.querySelectorAll('#tree button').length === 2), 'New screen starts with a title');

check(errors.length === 0, 'no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close(); close();
console.log(failed ? `UI EDITOR TEST: ${failed} FAILED` : 'UI EDITOR TEST PASSED');
process.exit(failed ? 1 : 0);
