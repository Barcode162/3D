// The Studio (docs/STUDIO.md S2, S3...): the page at /studio/ with the bar's
// engine in studio mode in its frame, laid out like the IONOS webspace on
// PHP's built-in server (<root>/home/bar = the built site + account.php,
// <root>/home/studio, <root>/home/ui, <root>/private) with a real owner login.
// Screenshots: build/studio-*.png.   Usage: NODE_PATH=$(npm root -g) node tools/studio_test.mjs
import { mkdirSync, mkdtempSync, symlinkSync, copyFileSync, rmSync, readdirSync } from 'node:fs';
import { spawn } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { launchBrowser } from './lib/browser.mjs';

const PORT = 8798, url = `http://127.0.0.1:${PORT}/`;
const root = mkdtempSync(join(tmpdir(), 'studio-site-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
mkdirSync(join(root, 'private'), { recursive: true });
// (CI copies the server's PHP into build/site before the tests: this site gets its own copy.)
for (const f of readdirSync('build/site')) if (!/\.php$|^\.htaccess$|^accounts$/.test(f)) symlinkSync(resolve('build/site', f), join(root, 'home/bar', f));
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
symlinkSync(resolve('studio'), join(root, 'home/studio'));
symlinkSync(resolve('ui'), join(root, 'home/ui'));
symlinkSync(resolve('hub'), join(root, 'home/hub'));
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore',
  env: { ...process.env, STUDIO_DATA: join(root, 'private/studio'), HUB_DATA: join(root, 'private/hub'), HUB_ACCOUNTS: join(root, 'home/bar/accounts') } });
const close = () => { php.kill(); rmSync(root, { recursive: true, force: true }); };
await new Promise((r) => setTimeout(r, 800));
const year = new Date().getFullYear();
const owner = await (await fetch(url + 'bar/account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' },
  body: JSON.stringify({ a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${year - 30}-03-04` }) })).json();
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
mkdirSync('build', { recursive: true });
check(owner.ok === 1, 'the owner has an account');

const page = await browser.newPage({ viewport: { width: 393, height: 852 }, hasTouch: true });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
await page.addInitScript((t) => { try { localStorage.setItem('hubToken', t); } catch (e) {} }, owner.token);
await page.goto(url + 'studio/');
const S = () => page.evaluate(() => { const s = window.bar3dStudio && window.bar3dStudio.S; return s && { things: s.things, selected: s.selected, place: s.place }; });
await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.things.length > 100, null, { timeout: 120000, polling: 250 }).catch(() => {});
const s1 = await S();
check(s1 && s1.things.length > 100, `S2.1 the engine opens in studio mode and lists the world (${s1 && s1.things.length} things)`);
const frame = page.frame({ url: /\/bar\/\?studio=1/ });
check(!!frame, 'S2.1 the bar runs in the Studio\'s frame');
const engineLog = async () => frame.evaluate(() => document.getElementById('log').textContent);
if (frame) {
  check(/Studio: ready/.test(await engineLog()), 'S2.1 the engine says it is in studio mode');
  const hud = await frame.evaluate(() => { const m = document.getElementById('hudMenu'); return m ? getComputedStyle(m).display : 'none'; });
  check(hud === 'none', 'S2.1 no bar HUD inside the Studio');
  check(!/Multiplayer: connected/.test(await engineLog()), 'S2.1 no room: nobody else is in the Studio');
}
check((await page.evaluate(() => document.querySelector('#top').getBoundingClientRect().height)) > 30, 'S2.1 the top bar shows');
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/studio-open.png' });

// ---- S2.2 cameras ----
const view = async () => {
  await page.evaluate(() => { window.__view = null; window.bar3dStudio.on('view', (e) => { window.__view = e.view; }); window.bar3dStudio.send('st.getview'); });
  await page.waitForFunction(() => window.__view, null, { timeout: 5000 }).catch(() => {});
  return page.evaluate(() => window.__view);
};
const v0 = await view();
check(Array.isArray(v0) && v0.length === 7, `S2.2 the camera says where it is (${v0 && v0.slice(0, 5).map((x) => x.toFixed(2)).join(', ')})`);
await page.mouse.move(200, 400); await page.mouse.down(); await page.mouse.move(280, 380, { steps: 8 }); await page.mouse.up();
await page.waitForTimeout(300);
const v1 = await view();
check(v1 && Math.abs(v1[3] - v0[3]) > 0.3, `S2.2 dragging one finger orbits (yaw ${v0 && v0[3].toFixed(2)} → ${v1 && v1[3].toFixed(2)})`);
await page.click('#camModes [data-cam="2"]');
await page.waitForTimeout(300);
const vTop = await view();
check(vTop && vTop[5] === 2 && vTop[4] < -1.5, `S2.2 Top looks straight down (pitch ${vTop && vTop[4].toFixed(2)})`);
await page.screenshot({ path: 'build/studio-top.png' });
await page.click('#camModes [data-cam="0"]');
await page.evaluate((v) => window.bar3dStudio.send('st.view', 0, 0, v.join(',')), v0);
await page.waitForTimeout(300);
const vBack = await view();
check(vBack && Math.abs(vBack[0] - v0[0]) < 0.05 && Math.abs(vBack[3] - v0[3]) < 0.01, 'S2.2 a saved view comes back exactly');
await page.click('#viewsKey');
await page.click('#views button >> nth=0');
await page.waitForTimeout(500);
check((await page.$$('#views button')).length === 2, 'S2.2 "Save this view" adds it to the list');

// ---- S2.3 picking ----
// Look at the bar counter from close by, then tap the middle of the screen.
const counter = (await S()).things.find((t) => t.id === 'bar_counter_l');
await page.evaluate((c) => window.bar3dStudio.send('st.view', 0, 0, [c.at[0] + 1.2, 1.7, c.at[2] + 2.6, -0.4, -0.35, 0, 2.5].join(',')), counter);
await page.waitForTimeout(400);
const box = await page.$('#world').then((f) => f.boundingBox());
await page.mouse.click(box.x + box.width / 2, box.y + box.height * 0.55);
await page.waitForTimeout(600);
const sel = await S();
const picked = sel.selected >= 0 ? sel.things[sel.selected] : null;
check(!!picked, `S2.3 a tap selects a thing (${picked && picked.id})`);
const crumb = await page.textContent('#crumbPath').catch(() => '');
check(/^Bar › /.test(crumb) && picked && crumb.endsWith(picked.id), `S2.3 the breadcrumb says where it sits (${crumb})`);
await page.screenshot({ path: 'build/studio-pick.png' });
const listed = await page.$$eval('#hits button', (b) => b.length).catch(() => 0);
check(listed === 0 || listed >= 2, `S2.3 several things under the finger → a list to choose from (${listed})`);
await page.click('#deselect');
await page.waitForTimeout(200);
check((await S()).selected === -1 && await page.$eval('#crumb', (c) => c.hidden), 'S2.3 ✕ deselects');

// ---- S2.4 the Monitor ----
await page.click('#monitorKey');
await page.waitForSelector('#monitor .tabs', { timeout: 10000 });
await page.click('#monitor .tabs [data-tab="world"]');
await page.fill('#monitor input.field', 'pendant_lamp');
const rows = await page.$$eval('#tree button', (b) => b.map((x) => x.querySelector('.id').textContent));
check(rows.length >= 4 && rows.every((r) => r.startsWith('pendant_lamp')), `S2.4 World: search finds the lamps (${rows.length})`);
const before = await view();
await page.click('#tree button >> nth=0');
await page.waitForTimeout(400);
const after = await view();
check((await S()).things[(await S()).selected].id === rows[0] && Math.hypot(after[0] - before[0], after[2] - before[2]) > 0.5, 'S2.4 tapping one selects it and flies to it');
await page.click('#monitor .tabs [data-tab="inspect"]');
const inspect = await page.textContent('#monitor .body');
check(/pendant_lamp/.test(inspect) && /States/.test(inspect) && /swinging/.test(inspect) && /Light|Model/.test(inspect), 'S2.4 Inspect shows it in words (parts, states, loops)');
await page.screenshot({ path: 'build/studio-inspect.png' });
await page.click('#monitor .tabs [data-tab="problems"]');
await page.waitForFunction(() => /All clear|problem/.test(document.querySelector('#monitor .body').textContent) && !/Asking/.test(document.querySelector('#monitor .body').textContent), null, { timeout: 10000 }).catch(() => {});
check(/All clear/.test(await page.textContent('#monitor .body')), 'S2.4 Problems: all clear for the bar as it is');

// ---- S2.5 X-ray ----
await page.click('#monitor .tabs [data-tab="world"]');
await page.click('.xray [data-layer="1"]');
await page.waitForTimeout(300);
check(await page.$eval('.xray [data-layer="1"]', (k) => k.classList.contains('lit')), 'S2.5 the tap-areas key lights up');
await page.click('#monitor .key[aria-label="Close the Monitor"]');
const jb = (await S()).things.find((t) => t.id === 'jukebox');
await page.evaluate((j) => window.bar3dStudio.send('st.view', 0, 0, [j.at[0] + 1.5, 1.8, j.at[2] + 3, 0.3, -0.25, 0, 3].join(',')), jb);
await page.waitForTimeout(800);
await page.screenshot({ path: 'build/studio-xray.png' });
const xrayLog = await engineLog();
check(!/error/i.test(xrayLog.split('Studio: ready')[1] || ''), 'S2.5 X-ray draws without errors');

// ---- S5.1 the magnifier: hold still, the lens names what's above the finger; let go selects it ----
{
  await page.evaluate(() => window.bar3dStudio.select(-1));
  const fb5 = await page.$('#world').then((f) => f.boundingBox());
  await page.mouse.move(fb5.x + fb5.width / 2, fb5.y + fb5.height * 0.62);
  await page.mouse.down();
  await page.waitForSelector('#lens:not([hidden])', { timeout: 6000 }).catch(() => {});
  const lensName = await page.textContent('#lens .name').catch(() => '');
  check(await page.$eval('#lens', (l) => !l.hidden).catch(() => false) && lensName.length > 0, `S5.1 holding a finger still opens the lens (${lensName})`);
  await page.mouse.move(fb5.x + fb5.width / 2 + 6, fb5.y + fb5.height * 0.62, { steps: 3 });
  await page.waitForTimeout(300);
  const named = await page.textContent('#lens .name');
  await page.screenshot({ path: 'build/studio-lens.png' });
  await page.mouse.up();
  await page.waitForFunction(() => document.getElementById('lens').hidden, null, { timeout: 5000 }).catch(() => {});
  const sel5 = await S();
  const got = sel5.selected >= 0 ? sel5.things[sel5.selected].id : '(none)';
  check(named === 'nothing here' ? sel5.selected < 0 : got === named, `S5.1 letting go selects what the lens showed (${named} → ${got})`);
}

// ---- S3.2 Move / Turn / Size ----
// Where a thing is now: asked twice, so the frame that applied the last
// change has finished (the second answer comes a frame after the first).
const where = async (i) => {
  let at = null;
  for (let k = 0; k < 2; k++) {
    await page.evaluate((i) => { window.__where = null; window.__whereFor = i; if (!window.__whereOn) { window.__whereOn = 1; window.bar3dStudio.on('where', (e) => { if (e.thing === window.__whereFor) window.__where = e.at; }); } window.bar3dStudio.send('st.where', i); }, i);
    await page.waitForFunction(() => window.__where, null, { timeout: 5000 }).catch(() => {});
    at = await page.evaluate(() => window.__where);
  }
  return at;
};
await page.waitForFunction(() => window.bar3dStudio.S.draft, null, { timeout: 20000 }).catch(() => {});
const tbl = (await S()).things.find((t) => t.id === 'round_table-1');
await page.evaluate((i) => window.bar3dStudio.select(i, true), tbl.i);
await page.waitForTimeout(500);
check(await page.$eval('#tools', (t) => !t.hidden), 'S3.2 the tools show under a selected thing');
const w0 = await where(tbl.i);
const x0 = parseFloat(await page.inputValue('#valField'));
await page.fill('#valField', String(+(x0 + 0.25).toFixed(3)));
await page.press('#valField', 'Enter');
await page.waitForTimeout(500);
const w1 = await where(tbl.i);
check(w0 && w1 && Math.abs(w1[0] - w0[0] - 0.25) < 0.01 && Math.abs(w1[2] - w0[2]) < 0.01, `S3.2 typing X + 0.25 moves it exactly 0.25 m (${w0 && w0[0]} → ${w1 && w1[0]})`);
const draftAt = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at, tbl.id);
check(Math.abs(draftAt[0] - (x0 + 0.25)) < 0.001, 'S3.2 the draft has the new place');
// The fader: drag right, snapped to 5 cm steps.
const jb2 = await page.$('#jog').then((j) => j.boundingBox());
await page.mouse.move(jb2.x + jb2.width / 2, jb2.y + jb2.height / 2);
await page.mouse.down();
await page.mouse.move(jb2.x + jb2.width / 2 + 60, jb2.y + jb2.height / 2, { steps: 6 });
await page.mouse.up();
await page.waitForTimeout(400);
const x2 = parseFloat(await page.inputValue('#valField'));
check(x2 > x0 + 0.25 && Math.abs((x2 - x0) / 0.05 - Math.round((x2 - x0) / 0.05)) < 0.02, `S3.2 the fader moves it in 5 cm steps (${x0} → ${x2})`);
await page.click('#toolKeys [data-tool="1"]');
await page.fill('#valField', '30');
await page.press('#valField', 'Enter');
await page.waitForTimeout(300);
const turned = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).turn, tbl.id);
check(turned === 30, `S3.2 Turn sets its turn (${turned}°)`);
await page.click('#toolKeys [data-tool="0"]');
await page.screenshot({ path: 'build/studio-tools.png' });

// ---- S3.3 slide, drop, put on ----
await page.evaluate((i) => window.bar3dStudio.select(i, true), tbl.i);
await page.waitForTimeout(600);
const fb = await page.$('#world').then((f) => f.boundingBox());
const before3 = await where(tbl.i);
// The table is in the middle of the view after flying to it: drag it.
await page.mouse.move(fb.x + fb.width / 2, fb.y + fb.height * 0.5);
await page.mouse.down();
await page.mouse.move(fb.x + fb.width / 2 + 50, fb.y + fb.height * 0.5 + 30, { steps: 10 });
await page.mouse.up();
await page.waitForTimeout(500);
const after3 = await where(tbl.i);
check(before3 && after3 && Math.hypot(after3[0] - before3[0], after3[2] - before3[2]) > 0.05 && Math.abs(after3[1] - before3[1]) < 0.3,
      `S3.3 dragging the thing slides it over the floor (${before3 && before3.map((v) => v.toFixed(2))} → ${after3 && after3.map((v) => v.toFixed(2))})`);
await page.click('#dropKey');
await page.waitForFunction((id) => Math.abs(window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[1]) < 0.05, tbl.id, { timeout: 8000 }).catch(() => {});
const dropped = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[1], tbl.id);
check(Math.abs(dropped) < 0.05, `S3.3 Drop puts it on the floor (y ${dropped})`);
const glass = (await S()).things.find((t) => t.kind === 'pint_glass' && t.parent < 0);
await page.evaluate((i) => window.bar3dStudio.select(i, true), glass.i);
await page.waitForTimeout(400);
await page.click('#putOnKey');
await page.evaluate((t) => window.bar3dStudio.S.engine.bar3dStudioSink({ ev: 'pick', hits: [[t, 1]] }), tbl.i);
await page.waitForFunction(([g, t]) => (window.bar3dStudio.S.draft['places/bar.json'].things.find((x) => x.id === g) || {}).parent === t, [glass.id, tbl.id], { timeout: 8000 }).catch(() => {});
const gj = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id), glass.id);
check(gj.parent === tbl.id, `S3.3 Put on… makes it sit on the table (${gj.parent})`);
const g0 = await where(glass.i);
await page.evaluate((i) => window.bar3dStudio.select(i, false), tbl.i);
await page.$eval('#axisKeys [data-axis="0"]', (b) => b.click());
await page.fill('#valField', String(+(parseFloat(await page.inputValue('#valField')) + 0.5).toFixed(3)));
await page.press('#valField', 'Enter');
await page.waitForTimeout(500);
const g1 = await where(glass.i);
check(g0 && g1 && Math.abs(g1[0] - g0[0] - 0.5) < 0.02, `S3.3 move the table: the glass goes with it (${g0 && g0[0].toFixed(2)} → ${g1 && g1[0].toFixed(2)})`);

// ---- S3.4 undo, saving, changes ----
const xBeforeUndo = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[0], tbl.id);
await page.click('#undo');
await page.waitForTimeout(300);
const xAfterUndo = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[0], tbl.id);
check(Math.abs(xBeforeUndo - xAfterUndo - 0.5) < 0.001, `S3.4 Undo takes the last move back (${xBeforeUndo} → ${xAfterUndo})`);
await page.click('#redo');
await page.waitForTimeout(2000);
check(/saved/.test(await page.textContent('#savedText')), `S3.4 the draft saves itself (${await page.textContent('#savedText')})`);
await page.click('#monitorKey');
await page.click('#monitor .tabs [data-tab="changes"]');
const changeText = await page.textContent('#monitor .body');
check(new RegExp(tbl.id + ' moved').test(changeText) && new RegExp(glass.id + '.*now sits on ' + tbl.id).test(changeText), 'S3.4 Changes says it in words');
await page.click('#monitor .key[aria-label="Close the Monitor"]');
// Open the Studio again: the draft is still there.
const page2 = await browser.newPage({ viewport: { width: 393, height: 852 } });
await page2.addInitScript((t) => { try { localStorage.setItem('hubToken', t); } catch (e) {} }, owner.token);
await page2.goto(url + 'studio/');
await page2.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft, null, { timeout: 120000, polling: 250 }).catch(() => {});
const again = await page2.evaluate((id) => window.bar3dStudio.S.draft && window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[0], tbl.id);
check(Math.abs(again - xBeforeUndo) < 0.001, 'S3.4 reopening the Studio: your draft is still there');
await page2.close();

// ---- S3.5 publish, the bar has it, history, restore ----
await page.click('#publish');
await page.fill('#pubNote', 'Moved a table');
await page.click('#pubGo');
await page.waitForFunction(() => window.bar3dStudio.S.live && window.bar3dStudio.S.live.version === 1, null, { timeout: 15000 }).catch(() => {});
check((await page.evaluate(() => window.bar3dStudio.S.live.version)) === 1, 'S3.5 Publish makes version 1');
const bar = await browser.newPage({ viewport: { width: 393, height: 852 } });
await bar.goto(url + 'bar/?skipintro&net=off');
await bar.waitForFunction(() => /World: \d+ things/.test(document.getElementById('log').textContent), null, { timeout: 120000, polling: 250 }).catch(() => {});
check(/World: v1 \(live, by Josh\)/.test(await bar.evaluate(() => document.getElementById('log').textContent)), 'S3.5 the bar opens with version 1');
await bar.close();
await page.click('#monitorKey');
await page.click('#monitor .tabs [data-tab="history"]');
await page.waitForSelector('.hist', { timeout: 5000 }).catch(() => {});
check(/v1 · Moved a table/.test(await page.textContent('#monitor .body')), 'S3.5 History lists it');
await page.screenshot({ path: 'build/studio-history.png' });

// ---- S4.1 the hub's Dials: the same draft, the same Publish ----
await page.goto(url + 'hub/dials.html#kind=pendant_lamp');
await page.waitForSelector('[data-kind="pendant_lamp"] .dial input', { timeout: 15000 }).catch(() => {});
const lenInput = page.locator('[data-kind="pendant_lamp"] .dial input').first();
check(await lenInput.count() === 1 && (await page.textContent('[data-kind="pendant_lamp"]')).includes('Loop: sway'), 'S4.1 Dials opens the pendant lamp with its loops');
await lenInput.fill('7');
await lenInput.press('Enter');
await page.waitForFunction(() => /saved/.test(document.getElementById('savedText').textContent), null, { timeout: 8000 }).catch(() => {});
check(/saved/.test(await page.textContent('#savedText')), 'S4.1 a dial change saves the draft');
await page.click('#publish');
check(/sway|pendant/i.test(await page.textContent('#sheetList')), `S4.1 Publish says what changed (${(await page.textContent('#sheetList')).slice(0, 80)})`);
await page.click('#go');
await page.waitForFunction(() => /Live · v2/.test(document.getElementById('savedText').textContent), null, { timeout: 8000 }).catch(() => {});
const v2 = await (await fetch(url + 'studio/api.php?bundle=2')).json().catch(() => ({}));
const lamp = v2.files && v2.files['kinds/bar.json'].kinds.find((k) => k.id === 'pendant_lamp');
check(lamp && lamp.loops.sway.period === 7, `S4.1 version 2 has the lamps' new swing length (${lamp && lamp.loops.sway.period} s)`);
check((await page.getAttribute('[data-kind="pendant_lamp"] a.key', 'href') || '').startsWith('/studio/#thing='), 'S4.1 "Open in the Studio" links to the thing');
await page.screenshot({ path: 'build/hub-dials.png' });
check(errors.length === 0, 'no page errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close();
close();
console.log(failed ? `STUDIO TEST FAILED (${failed})` : 'STUDIO TEST PASSED');
process.exit(failed ? 1 : 0);
