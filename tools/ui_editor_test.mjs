// The UI Editor (toolbelt/ui-editor/, docs/archive/UI_PLAN.md R1.4-R1.7) at phone
// width, on PHP's built-in server laid out like the webspace (ui/, toolbelt/,
// bar/account.php; the store's data in a temp folder): the preview and
// selection (R1.4), theme editing and undo (R1.5), screen editing (R1.6), the
// draft kept on the phone and Publish with history (R1.7). No game: seconds.
// Usage: NODE_PATH=$(npm root -g) node tools/ui_editor_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, cpSync, copyFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createServer } from 'node:net';
import { launchBrowser } from './lib/browser.mjs';

// A free port, so a server left over from another run can't answer instead.
const PORT = await new Promise((r) => { const s = createServer().listen(0, () => { const p = s.address().port; s.close(() => r(p)); }); });
const url = `http://127.0.0.1:${PORT}/`;
const root = mkdtempSync(join(tmpdir(), 'uied-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
cpSync('ui', join(root, 'home/ui'), { recursive: true });
cpSync('toolbelt', join(root, 'home/toolbelt'), { recursive: true });
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore',
  env: { ...process.env, UI_DATA: join(root, 'private/ui'), HUB_DATA: join(root, 'private/hub'), HUB_ACCOUNTS: join(root, 'home/bar/accounts') } });
const close = () => { php.kill(); rmSync(root, { recursive: true, force: true }); };
process.on('exit', () => { try { php.kill(); } catch {} });
process.on('uncaughtException', (e) => { console.log('FAIL ' + (e.message || e)); close(); process.exit(1); });
process.on('unhandledRejection', (e) => { console.log('FAIL ' + (e?.message || e)); close(); process.exit(1); });
await new Promise((r) => setTimeout(r, 700));
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
page.on('response', (r) => { if (r.status() >= 400 && !(r.status() === 401 && /api\.php/.test(r.url()))) errors.push(r.status() + ' ' + r.url()); });  // 401: not logged in yet
await page.goto(url + 'toolbelt/ui-editor/index.html?screen=sampler');
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
check(await ed(() => document.querySelectorAll('#tree [data-group]').length === 8), 'Theme lists This theme + 7 token groups');
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

// R2.2: light and dark in one theme
await page.selectOption('#themePick', 'obsidian');
await page.click('#schemeSeg [data-scheme="light"]');
const lightBg = await ed(() => ({ bg: getComputedStyle(document.getElementById('frame')).getPropertyValue('--bg').trim(), other: UIEditor.state.themes.obsidian.other?.scheme }));
check(lightBg.other === 'light' && lightBg.bg === '#F4F4F2', `Light on a dark-only theme starts its light half (${lightBg.bg})`);
await page.click('[data-mode="theme"]'); await page.click('#tree [data-group="colour"]');
await page.$eval('#insp [data-token="accent"] input[type=color]', (c) => { c.value = '#112233'; c.dispatchEvent(new Event('input', { bubbles: true })); });
check(await ed(() => UIEditor.state.themes.obsidian.other.tokens.accent === '#112233' && UIEditor.state.themes.obsidian.tokens.accent === '#8BE9FF'), 'an edit in the light half leaves the dark half alone');
await page.click('#schemeSeg [data-scheme="dark"]');
check(await ed(() => getComputedStyle(document.getElementById('frame')).getPropertyValue('--accent').trim() === '#8BE9FF'), 'Dark shows the dark half again');
// R2.4: mixing: take Shape from Bold into Soft (both have both halves)
await page.selectOption('#themePick', 'soft');
await page.click('#tree [data-group="shape"]');
await page.selectOption('#insp select[aria-label="Take Shape from another theme"]', 'bold');
check(await ed(() => { const t = UIEditor.state.themes.soft; return t.tokens['key-r'] === '4px' && t.tokens['panel-bw'] === '2px' && getComputedStyle(document.getElementById('frame')).getPropertyValue('--key-r').trim() === '4px'; }), 'Take Shape from Bold squares Soft\'s corners');
await page.click('#undo');
check(await ed(() => UIEditor.state.themes.soft.tokens['key-r'] === '999px'), 'one undo takes the whole group back');
// R2.5: motion: a title can move
await page.selectOption('#themePick', 'faceplate');
await page.click('#tree [data-group="motion"]');
await page.selectOption('#insp [data-token="title-motion"] select', 'lp-float');
check(await ed(() => getComputedStyle(document.querySelector('#frame h1, #frame h2')).animationName === 'lp-float'), 'Motion → title-motion float makes the preview\'s title move');
await page.click('#undo');
await page.click('[data-mode="screen"]');

// R1.7: the draft on the phone, login, publish, history
check(await ed(() => /unpublished/.test(document.getElementById('status').textContent) && /Publish \d/.test(document.getElementById('pubOpen').textContent)), 'the top bar counts what is unpublished');
await page.goto(url + 'toolbelt/ui-editor/index.html?screen=sampler');
await page.waitForFunction(() => document.querySelectorAll('#tree button').length > 0);
check(await ed(() => !!UIEditor.state.screens['screen-1'] && UIEditor.state.themes.faceplate.tokens.accent === '#00AAFF'), 'after a reload the draft is back');
const year = new Date().getFullYear();
const signup = await page.evaluate(async (y) => (await fetch('/bar/account.php', { method: 'POST', body: JSON.stringify({ a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${y - 30}-03-04` }) })).json(), year);
check(signup.ok === 1, 'an owner account');
await page.click('#pubOpen');
await page.waitForSelector('#login:not([hidden])');
check(true, 'Publish asks for a login first');
await page.fill('#user', 'josh'); await page.fill('#pass', 'secret1');
await page.click('#login button[type=submit]');
await page.waitForSelector('#pubForm:not([hidden])');
check(await ed(() => document.querySelectorAll('#pubChanges li').length >= 2), 'the sheet lists what changed (a theme, screens)');
await page.fill('#pubNote', 'Blue accent, a new screen');
await page.click('#pubGo');
await page.waitForFunction(() => document.getElementById('pub').hidden);
const live = await page.evaluate(async () => (await fetch('/toolbelt/ui-editor/api.php?bundle=live')).json());
check(live.version === 1 && live.files['themes/faceplate.json'].tokens.accent === '#00AAFF' && !!live.files['screens/screen-1.json'], 'Publish makes version 1 with the edits');
check(await ed(() => /v1 live/.test(document.getElementById('status').textContent) && !localStorage.getItem('uiEditorDraft')), 'the draft is cleared, v1 is live');
await page.click('#pubOpen');
await page.waitForFunction(() => document.querySelectorAll('#history li b').length === 1);
check(await ed(() => /Blue accent/.test(document.getElementById('history').textContent)), 'History shows v1 and its note');
await page.click('#pubClose');
await page.goto(url + 'toolbelt/ui-editor/index.html?screen=sampler');
await page.waitForFunction(() => document.querySelectorAll('#tree button').length > 0);
check(await ed(() => UIEditor.state.themes.faceplate.tokens.accent === '#00AAFF' && UIEditor.state.live.version === 1), 'a fresh load starts from the published version (newer than the repo)');

check(errors.length === 0, 'no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close(); close();
console.log(failed ? `UI EDITOR TEST: ${failed} FAILED` : 'UI EDITOR TEST PASSED');
process.exit(failed ? 1 : 0);
