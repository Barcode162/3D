// The Studio (docs/archive/STUDIO.md S2, S3...): the page at /studio/ with the bar's
// engine in studio mode in its frame, laid out like the IONOS webspace on
// PHP's built-in server (<root>/home/bar = the built site + account.php,
// <root>/home/studio, <root>/home/ui, <root>/private) with a real owner login.
// Screenshots: build/studio-*.png.   Usage: NODE_PATH=$(npm root -g) node tools/studio_test.mjs
import { mkdirSync, mkdtempSync, symlinkSync, copyFileSync, rmSync, readdirSync } from 'node:fs';
import { spawn } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { launchBrowser } from './lib/browser.mjs';

const PORT = Number(process.env.STUDIO_PORT) || 8798, url = `http://127.0.0.1:${PORT}/`;
const root = mkdtempSync(join(tmpdir(), 'studio-site-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
mkdirSync(join(root, 'private'), { recursive: true });
// (CI copies the server's PHP into build/site before the tests: this site gets its own copy.)
for (const f of readdirSync('build/site')) if (!/\.php$|^\.htaccess$|^accounts$/.test(f)) symlinkSync(resolve('build/site', f), join(root, 'home/bar', f));
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
symlinkSync(resolve('studio'), join(root, 'home/studio'));
symlinkSync(resolve('ui'), join(root, 'home/ui'));
symlinkSync(resolve('hub'), join(root, 'home/hub'));
symlinkSync(resolve('dev'), join(root, 'home/dev'));
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore',
  env: { ...process.env, STUDIO_DATA: join(root, 'private/studio'), HUB_DATA: join(root, 'private/hub'), DEV_DATA: join(root, 'private/dev'), HUB_ACCOUNTS: join(root, 'home/bar/accounts') } });
const close = () => { php.kill(); rmSync(root, { recursive: true, force: true }); };
process.on('exit', () => { try { php.kill(); } catch (e) {} });  // a crashed run leaves no server behind
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
page.on('console', (m) => { if (m.type() === 'error') console.log('CONSOLE', m.text().slice(0, 300)); });
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
await page.waitForFunction(() => document.querySelectorAll('#views button').length === 2, null, { timeout: 10000 }).catch(() => {});
check((await page.$$('#views button')).length === 2, 'S2.2 "Save this view" adds it to the list');

// ---- S2.3 picking ----
// Look at the bar counter from close by, then tap the middle of the screen.
const counter = (await S()).things.find((t) => t.id === 'bar_counter_l');
await page.evaluate((c) => window.bar3dStudio.send('st.view', 0, 0, [c.at[0] + 1.2, 1.7, c.at[2] + 2.6, -0.4, -0.35, 0, 2.5].join(',')), counter);
await page.waitForTimeout(400);
const box = await page.$('#world').then((f) => f.boundingBox());
// (A slow machine can miss a quick tap between frames: tap again if nothing came.)
for (let k = 0; k < 3; k++) {
  await page.mouse.click(box.x + box.width / 2, box.y + box.height * 0.55);
  if (await page.waitForFunction(() => window.bar3dStudio.S.selected >= 0, null, { timeout: 5000 }).then(() => true).catch(() => false)) break;
}
await page.waitForTimeout(300);
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

// ---- S5.2 part mode: the jukebox's tap area, 40 cm wider ----
{
  const jb5 = (await S()).things.find((t) => t.id === 'jukebox');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), jb5.i);
  await page.waitForTimeout(500);
  await page.click('#partKey');
  await page.click('#partList button:has-text("Tap area 1")');
  await page.click('#toolKeys [data-tool="2"]');
  await page.$eval('#axisKeys [data-axis="0"]', (b) => b.click());
  const w0 = parseFloat(await page.inputValue('#valField'));
  await page.fill('#valField', String(+(w0 + 0.4).toFixed(3)));
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(400);
  const box = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'jukebox').parts.uses[0].box);
  check(Math.abs((box[1][0] - box[0][0]) - (w0 + 0.4)) < 0.002 && Math.abs(box[0][0] + box[1][0]) < 0.002,
        `S5.2 Part → Tap area 1 → Size X: the jukebox's tap area is 40 cm wider, still centred (${w0} → ${box[1][0] - box[0][0]} m)`);
  check(/Tap area 1/.test(await page.textContent('#partText')), 'S5.2 the Part key names the part');
  await page.screenshot({ path: 'build/studio-part.png' });
  await page.click('#undo');
  await page.click('#partKey');
  await page.click('#partList button:has-text("the thing itself")');
  await page.click('#toolKeys [data-tool="0"]');
  const back = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'jukebox').parts.uses[0].box);
  check(Math.abs((back[1][0] - back[0][0]) - w0) < 0.002, 'S5.2 Undo puts the tap area back');
}

// ---- S5.3 this one | every one ----
{
  const stools = () => page.evaluate(() => window.bar3dStudio.S.draft['places/bar.json'].things.filter((x) => x.kind === 'bar_stool' && !x.row));
  const all0 = await stools();
  const st = (await S()).things.find((t) => t.id === all0[0].id);
  await page.evaluate((i) => window.bar3dStudio.select(i, true), st.i);
  await page.waitForTimeout(400);
  check(/Every bar_stool/.test(await page.textContent('#scopeSeg')), `S5.3 the tools offer "Every bar_stool" (${(await page.textContent('#scopeSeg')).trim()})`);
  await page.click('#scopeSeg [data-every="1"]');
  await page.click('#toolKeys [data-tool="2"]');
  await page.$eval('#axisKeys [data-axis="1"]', (b) => b.click());
  await page.fill('#valField', '1.05');
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(300);
  const all1 = await stools();
  const ys = all1.map((x) => (Array.isArray(x.scale) ? x.scale[1] : x.scale ?? 1));
  check(all1.length > 3 && ys.every((y) => Math.abs(y - 1.05) < 0.001), `S5.3 Every: all ${all1.length} bar stools 5 % taller in one change (${[...new Set(ys)].join(', ')})`);
  await page.click('#scopeSeg [data-every="0"]');
  await page.fill('#valField', '1.2');
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(300);
  const all2 = await stools();
  const ys2 = all2.map((x) => (Array.isArray(x.scale) ? x.scale[1] : x.scale ?? 1));
  check(ys2.filter((y) => Math.abs(y - 1.2) < 0.001).length === 1 && ys2.filter((y) => Math.abs(y - 1.05) < 0.001).length === all2.length - 1, 'S5.3 This one: only that stool changes');
  // Its own seat (Part, This one) → Inspect: New kind from this.
  await page.click('#toolKeys [data-tool="0"]');
  await page.click('#partKey');
  await page.click('#partList button:has-text("Seat 1")');
  await page.click('#scopeSeg [data-every="0"]');
  await page.$eval('#axisKeys [data-axis="1"]', (b) => b.click());
  const hy = parseFloat(await page.inputValue('#valField'));
  await page.fill('#valField', String(+(hy + 0.05).toFixed(3)));
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(300);
  const own = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((x) => x.id === id).parts, st.id);
  check(own && own.seats && Math.abs(own.seats[0].hip[1] - (hy + 0.05)) < 0.001, 'S5.3 This one on a part: the stool gets its own seat, 5 cm higher');
  await page.click('#partKey');
  await page.click('#partList button:has-text("the thing itself")');
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="inspect"]');
  await page.click('#monitor .key:has-text("New kind from this")');
  await page.waitForTimeout(300);
  await page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 60000 }).catch(() => {});
  const nk = await page.evaluate((id) => { const d = window.bar3dStudio.S.draft; const t = d['places/bar.json'].things.find((x) => x.id === id); const k = d['kinds/bar.json'].kinds.find((x) => x.id === t.kind); return { kind: t.kind, parts: t.parts, is: k && k.is, seats: k && k.parts && k.parts.seats }; }, st.id);
  check(nk.kind === 'bar_stool_2' && nk.is === 'bar_stool' && !nk.parts && nk.seats, `S5.3 New kind from this: ${st.id} is a ${nk.kind}, built on ${nk.is}, with its seat`);
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
  await page.$eval('#axisKeys [data-axis="0"]', (b) => b.click());
}

// ---- S5.4 Inspect edits every field: a seat's facing ----
{
  const s2 = (await S()).things.find((t) => t.id === 'bar_stool-2');
  await page.evaluate((i) => window.bar3dStudio.select(i, false), s2.i);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="inspect"]');
  const facing = page.locator('#monitor .ctl:has(.name:text-is("Facing")) input').first();
  check(await facing.count() === 1, 'S5.4 Inspect shows the seat\'s facing as a field you can change');
  await facing.fill('90');
  await facing.press('Enter');
  await page.waitForTimeout(300);
  const f = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'bar_stool').parts.seats[0].facing);
  check(f === 90, `S5.4 typing 90 sets every bar stool's seat facing (${f})`);
  const period = page.locator('#monitor .ctl:has(.name:text-is("Length")) input').first();
  check(await period.count() === 1, 'S5.4 its loops\' lengths are editable too');
  await page.screenshot({ path: 'build/studio-inspect.png' });
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
}

// ---- S6.1 the Library: add a candle to a table ----
{
  const table6 = (await S()).things.find((t) => t.id === 'round_table-2');
  await page.evaluate((i) => { window.bar3dStudio.select(i, true); }, table6.i);
  await page.waitForTimeout(600);
  await page.evaluate(() => window.bar3dStudio.select(-1));
  const n0 = (await S()).things.length;
  await page.click('#addKey');
  await page.waitForSelector('#library .libitem[data-kind="candle"]', { timeout: 10000 });
  await page.waitForTimeout(300);
  await page.waitForFunction(() => !window.bar3dBuild.busy, null, { timeout: 90000 }).catch(() => {});
  await page.waitForTimeout(300);
  check(await page.$eval('#library .libitem[data-kind="candle"]', (b) => /Table candle/.test(b.textContent) && /5 credits/.test(b.textContent)), 'S6.1 the Library lists the table candle with its price');
  check(await page.$eval('#library .libitem[data-kind="candle"] .pic', (p) => /url\(/.test(p.style.backgroundImage)), 'S6.1 it has a picture the engine took');
  await page.screenshot({ path: 'build/studio-library.png' });
  await page.click('#library .libitem[data-kind="candle"]');
  await page.waitForFunction((n) => window.bar3dStudio.S.things.length > n && !document.body.classList.contains('reloading'), n0, { timeout: 90000 }).catch(() => {});
  const s6 = await S();
  const added = s6.selected >= 0 ? s6.things[s6.selected] : null;
  const json = added && await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id), added.id);
  check(added && added.kind === 'candle' && json, `S6.1 a new candle is in the world and the draft, selected (${added && added.id})`);
  check(json && json.at[1] > 0.5, `S6.1 it landed on the table's top (y ${json && json.at[1]})`);
  await page.screenshot({ path: 'build/studio-added.png' });
}

// ---- S6.2 row, rename, delete ----
{
  const idle = () => page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  const stool = (await S()).things.find((t) => t.id === 'bar_stool-3');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), stool.i);
  await page.waitForTimeout(400);
  await page.click('#moreKey');
  await page.click('#thingMenu button[data-act="Row"]');
  await page.fill('#rowCount', '3');
  await page.fill('#rowStep', '0.7');
  await page.click('#rowGo');
  await page.waitForTimeout(300); await idle();
  const row = (await S()).things.filter((t) => /^bar_stool-row-1-\d$/.test(t.id));
  const xs = row.map((t) => +(t.at[0] - stool.at[0]).toFixed(2)).sort((a, b) => a - b);
  check(row.length === 3 && JSON.stringify(xs) === '[0.7,1.4,2.1]', `S6.2 Row: 3 more bar stools, 70 cm apart (${xs.join(', ')} m along X)`);
  if (row.length !== 3) { await page.screenshot({ path: 'build/studio-row-fail.png' }); console.log('ROWDBG', await page.evaluate(() => [document.body.className, JSON.stringify(window.bar3dStudio.S.reloading || null), (window.bar3dStudio.S.draft['places/bar.json'].things.filter((t) => /row/.test(t.id)).map((t) => t.id)).join(',')])); }
  await page.screenshot({ path: 'build/studio-row.png' });
  const c7 = (await S()).things.find((t) => t.id === 'candle-7');
  await page.evaluate((i) => window.bar3dStudio.select(i, false), c7.i);
  await page.click('#moreKey');
  await page.click('#thingMenu button[data-act="Rename"]');
  await page.fill('#renameField', 'my candle');
  await page.click('#renameGo');
  await page.waitForTimeout(300); await idle();
  check(!!(await S()).things.find((t) => t.id === 'my-candle'), 'S6.2 Rename: a new thing can be renamed (my-candle)');
  await page.click('#moreKey');
  await page.click('#thingMenu button[data-act="Delete"]');
  await page.waitForTimeout(300); await idle();
  check(!(await S()).things.find((t) => t.id === 'my-candle'), 'S6.2 Delete takes it away');
  await page.click('#undo');
  await page.waitForTimeout(300); await idle();
  check(!!(await S()).things.find((t) => t.id === 'my-candle'), 'S6.2 Undo brings it back');
  const live = (await S()).things.find((t) => t.id === 'bar_stool-3');
  await page.evaluate((i) => window.bar3dStudio.select(i, false), live.i);
  await page.click('#moreKey');
  await page.click('#thingMenu button[data-act="Rename"]');
  await page.fill('#renameField', 'stoolio');
  await page.click('#renameGo');
  await page.waitForTimeout(500);
  check(!!(await S()).things.find((t) => t.id === 'bar_stool-3'), 'S6.2 a thing from the live world keeps its name');
}

// ---- S6.3 boxes: a small stage ----
{
  const idle = () => page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  const table7 = (await S()).things.find((t) => t.id === 'round_table-3') || (await S()).things.find((t) => t.kind === 'round_table');
  await page.evaluate((i) => { window.bar3dStudio.select(i, true); }, table7.i);
  await page.waitForTimeout(500);
  await page.evaluate(() => window.bar3dStudio.select(-1));
  await page.click('#addKey');
  await page.click('#library .libitem[data-kind="box"]');
  await page.waitForFunction(() => window.bar3dStudio.S.things.some((t) => t.kind === 'built'), null, { timeout: 90000 }).catch(() => {});
  await idle();
  const built = (await S()).things.find((t) => t.kind === 'built');
  if (!built) console.log('ENGINE', (await page.frame({ url: /\/bar\/\?studio=1/ }).evaluate(() => document.getElementById('log').textContent).catch((e) => String(e))).slice(-1500));
  check(!!built && (await S()).selected === built.i, `S6.3 "A box" adds a box you build on, selected (${built && built.id})`);
  const boxes = () => page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).parts.boxes, built.id);
  await page.click('#partKey');
  await page.click('#partList button:has-text("Box 1")');
  await page.click('#toolKeys [data-tool="2"]');
  await page.$eval('#axisKeys [data-axis="1"]', (b) => b.click());
  await page.click('#faceSeg [data-face="1"]');
  await page.fill('#valField', '0.4');
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(300); await idle();
  const b1 = (await boxes())[0];
  check(Math.abs(b1.size[1] - 0.4) < 0.001 && Math.abs((b1.at[1] - b1.size[1] / 2) - 0) < 0.001, `S6.3 pull the top up 30 cm: the bottom stays on the floor (size ${b1.size[1]}, bottom ${+(b1.at[1] - b1.size[1] / 2).toFixed(3)})`);
  await page.$eval('#axisKeys [data-axis="2"]', (b) => b.click());
  await page.click('#faceSeg [data-face="1"]');
  await page.click('#extrudeKey');
  await page.waitForTimeout(300); await idle();
  const bs = await boxes();
  check(bs.length === 2 && Math.abs(bs[1].at[2] - (bs[0].at[2] + bs[0].size[2] / 2 + 0.15)) < 0.001 && bs[1].size[2] === 0.3, `S6.3 Extrude: a 30 cm box out of the front face (${bs.length} boxes)`);
  check(/Box 2/.test(await page.textContent('#partText')), 'S6.3 the new box is the one selected');
  await page.screenshot({ path: 'build/studio-boxes.png' });
  await page.click('#partKey');
  await page.click('#partList button:has-text("the thing itself")');
  await page.click('#toolKeys [data-tool="0"]');
  await page.$eval('#axisKeys [data-axis="0"]', (b) => b.click());
}

// ---- S6.4 places: a new place, where people appear, back to the bar ----
{
  const idle = () => page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  await page.click('#placeKey');
  await page.click('#placeMenu button[data-act="New place"]');
  await page.fill('#placeName2', 'Rooftop');
  await page.click('#placeGo');
  await page.waitForFunction(() => window.bar3dStudio.S.place === 'rooftop', null, { timeout: 90000 }).catch(() => {});
  await idle();
  const sr = await S();
  check(sr.place === 'rooftop' && sr.things.some((t) => t.id === 'floor') && sr.things.some((t) => t.id === 'light-1'), `S6.4 New place "rooftop": the Studio is there, with a floor and a light (${sr.place}, ${sr.things.length} things)`);
  await page.screenshot({ path: 'build/studio-rooftop.png' });
  await page.click('#placeKey');
  await page.click('#placeMenu button[data-act="spawn"]');
  await page.waitForTimeout(800);
  const spawn = await page.evaluate(() => window.bar3dStudio.S.draft['places/rooftop.json'].spawn);
  check(spawn && Math.abs(spawn.at[1] - 1.65) < 0.2, `S6.4 People appear here: the spawn is set (${spawn && spawn.at.join(', ')})`);
  await page.click('#placeKey');
  await page.click('#placeMenu button[data-place="bar"]');
  await page.waitForFunction(() => window.bar3dStudio.S.place === 'bar', null, { timeout: 90000 }).catch(() => {});
  await idle();
  const sb = await S();
  check(sb.place === 'bar' && sb.things.length > 150, `S6.4 back to the bar (${sb.place}, ${sb.things.length} things)`);
}

// ---- S7.1 materials: make "blue velvet", give it to the stage box ----
{
  const idle = () => page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  const built = (await S()).things.find((t) => t.kind === 'built' && t.id.startsWith('built'));
  await page.evaluate((i) => window.bar3dStudio.select(i, true), built.i);
  await page.waitForTimeout(500);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="look"]');
  await page.click('#monitor .chip[data-mat="leather"]');
  await page.fill('#newMatName', 'Blue velvet');
  await page.click('#newMatGo');
  await page.waitForTimeout(300); await idle();
  await page.$eval('#monitor .ctl input[type=color]', (i) => { i.value = '#1a2a8c'; i.dispatchEvent(new Event('change')); });
  await page.waitForTimeout(300); await idle();
  const mat = await page.evaluate(() => window.bar3dStudio.S.draft['materials.json'].materials.find((m) => m.id === 'blue-velvet'));
  check(mat && Math.abs(mat.color[2] - 0.549) < 0.01 && mat.roughness === 0.5, `S7.1 New material "blue-velvet", made from leather, now blue (${mat && mat.color})`);
  await page.click('#monitor .tabs [data-tab="inspect"]');
  await page.click('#monitor .ctl.chips .chip:has-text("blue-velvet") >> nth=0');
  await page.waitForTimeout(300); await idle();
  const boxMat = await page.evaluate((id) => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).parts.boxes[0].material, built.id);
  check(boxMat === 'blue-velvet', `S7.1 the stage's box is blue velvet (${boxMat})`);
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
  await page.screenshot({ path: 'build/studio-velvet.png' });
}

// ---- S7.2 lights: the neon over the arcade door turns red, live ----
{
  const sign = (await S()).things.find((t) => t.id === 'light-sign');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), sign.i);
  await page.waitForTimeout(500);
  const caps0 = ((await engineLog()).match(/Reflections: the room captured/g) || []).length;
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="inspect"]');
  await page.$eval('#monitor .ctl:has(.name:text-is("Colour")) input[type=color]', (i) => { i.value = '#ff1a1a'; i.dispatchEvent(new Event('change')); });
  await page.waitForTimeout(300);
  const col = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'light-sign').parts.light.color);
  check(col && col[0] === 1 && col[1] < 0.15 && col[2] < 0.15, `S7.2 the arcade neon's colour is red in the draft (${col})`);
  await page.waitForFunction((n) => (document.querySelector('iframe').contentDocument.getElementById('log').textContent.match(/Reflections: the room captured/g) || []).length > n, caps0, { timeout: 15000 }).catch(() => {});
  const caps1 = ((await engineLog()).match(/Reflections: the room captured/g) || []).length;
  check(caps1 > caps0, `S7.2 the reflections are taken again after the light changed (${caps0} → ${caps1})`);
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
  await page.screenshot({ path: 'build/studio-red-neon.png' });
}

// ---- S7.3 moods and the meter: Evening → Party, the meter stays green ----
{
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="look"]');
  await page.click('#monitor .chip[data-mood="Evening"]');
  await page.waitForTimeout(300);
  await page.click('#monitor .chip[data-mood="Party"]');
  await page.waitForTimeout(300);
  const place = await page.evaluate(() => window.bar3dStudio.S.draft['places/bar.json']);
  const ev = place.moods && place.moods.Evening && place.moods.Evening.lights['light-sign'];
  const pa = place.moods && place.moods.Party && place.moods.Party.lights['light-sign'];
  check(place.mood === 'Party' && ev && pa && JSON.stringify(ev.color) !== JSON.stringify(pa.color), `S7.3 Evening → Party: the place's mood is Party, its lights differ (${ev && ev.color} → ${pa && pa.color})`);
  await page.waitForSelector('#monitor .meter[data-state]', { timeout: 10000 }).catch(() => {});
  const state = await page.getAttribute('#monitor .meter', 'data-state').catch(() => null);
  const rows = await page.$$eval('#monitor .meterrow small', (s) => s.map((x) => x.textContent)).catch(() => []);
  if (state !== 'green') console.log('BUDGET', JSON.stringify(await page.evaluate(() => window.bar3dLook && window.bar3dLook.budget)), await page.textContent('#monitor .body').then((t) => t.slice(-300)));
  check(state === 'green', `S7.3 the meter stays green (${state}: ${rows.join(' · ')})`);
  await page.screenshot({ path: 'build/studio-moods.png' });
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
}

// ---- S8.1 states and rules: the dartboard wobbles when touched ----
{
  const board = (await S()).things.find((t) => t.kind === 'dartboard');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), board.i);
  await page.waitForTimeout(500);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="life"]');
  check(!!(await page.$('#monitor svg.lifemap')), 'S8.1 Life shows the dartboard\'s states as a diagram');
  await page.fill('#newState', 'wobble');
  await page.click('#newStateGo');
  await page.waitForTimeout(300);
  await page.click('#monitor .chip[data-state="idle"]');
  await page.fill('#ruleEvent', 'tap');
  await page.click('#monitor .chip[data-to="wobble"]');
  await page.waitForTimeout(400);
  const k = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((x) => x.id === 'dartboard'));
  check(k.states.wobble && k.states.wobble.loop === 'wobble' && k.loops.wobble && k.states.idle.on.tap === 'wobble', `S8.1 a state "wobble" (its own loop, back after one loop), and touching it in idle leads there (${k.states.idle.on.tap})`);
  await page.click('#monitor [data-test="tap"]');
  await page.waitForFunction(() => /now wobble/.test(window.bar3dLife.lastTest), null, { timeout: 30000 }).catch(() => {});  // (slow software GPUs)
  check(/now wobble/.test(await page.evaluate(() => window.bar3dLife.lastTest)), `S8.1 Test: a touch puts it in wobble, live (${await page.evaluate(() => window.bar3dLife.lastTest)})`);
  await page.screenshot({ path: 'build/studio-life.png' });
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
}

// ---- S8.2 the Loop Rack: the pendant lamps sway twice as wide; pause ----
{
  const lamp = (await S()).things.find((t) => t.kind === 'pendant_lamp');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), lamp.i);
  await page.waitForTimeout(500);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="life"]');
  await page.click('#monitor .chip[data-state="lit"]');
  const amps0 = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'pendant_lamp').loops.sway.pieces.map((p) => p.amp));
  for (let i = 0; i < 2; i++) {
    const inp = page.locator('#monitor .lane2').nth(i).locator('.ctl.num:has(.name:text-is("Amount")) input');
    await inp.fill(String(amps0[i] * 2));
    await inp.press('Enter');
    await page.waitForTimeout(300);
  }
  const amps1 = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'pendant_lamp').loops.sway.pieces.map((p) => p.amp));
  check(Math.abs(amps1[0] - amps0[0] * 2) < 0.01 && Math.abs(amps1[1] - amps0[1] * 2) < 0.01, `S8.2 the sway lanes twice as wide (${amps0.slice(0, 2)} → ${amps1.slice(0, 2)})`);
  await page.click('#rackPause');
  check(await page.evaluate(() => window.bar3dRack.paused), 'S8.2 Pause holds the loops');
  await page.screenshot({ path: 'build/studio-rack.png' });
  await page.click('#rackPause');
  // S8.3: draw the swing with a finger; a preset on the dartboard's wobble.
  await page.click('#monitor .chip[data-state="swinging"]');
  const pad = await page.$('#monitor .keyspad');
  await pad.scrollIntoViewIfNeeded();
  const pb = await pad.boundingBox();
  await page.mouse.move(pb.x + 2, pb.y + pb.height / 2);
  await page.mouse.down();
  for (let k = 1; k <= 20; k++) await page.mouse.move(pb.x + 2 + (pb.width - 4) * k / 20, pb.y + pb.height / 2 - Math.sin(k / 20 * Math.PI * 2) * pb.height * 0.4);
  await page.mouse.up();
  await page.waitForTimeout(400);
  const keys = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'pendant_lamp').loops.swing.pieces[0].keys);
  check(keys.length === 9 && keys[2][1] > 0 && keys[6][1] < 0, `S8.3 drawing the curve makes 9 keys (up, then down: ${keys.map((k) => k[1]).join(' ')})`);
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
  const board = (await S()).things.find((t) => t.kind === 'dartboard');
  await page.evaluate((i) => window.bar3dStudio.select(i, false), board.i);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="life"]');
  await page.click('#monitor .chip[data-state="wobble"]');
  await page.click('#monitor .chip[data-preset="thump"]');
  await page.waitForTimeout(300);
  const wob = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((k) => k.id === 'dartboard').loops.wobble);
  check(wob.beat === true && wob.pieces[0].shape === 'beat', 'S8.3 the "thump" preset puts a beat on the dartboard\'s wobble');
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
}

// ---- S8.4 sounds: the arcade machine hums when you're near it ----
{
  const cab = (await S()).things.find((t) => t.kind === 'arcade_cabinet');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), cab.i);
  await page.waitForTimeout(500);
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="life"]');
  const had = await page.evaluate(() => { const k = window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((x) => x.id === 'arcade_cabinet'); return !!(k.parts && k.parts.sound); });
  if (!had) await page.click('#monitor [data-add-sound="sound"]');
  await page.waitForTimeout(300);
  await page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  if (!(await page.$('#monitor [data-hear="sound"]'))) { await page.click('#monitorKey').catch(() => {}); await page.click('#monitor .tabs [data-tab="life"]').catch(() => {}); }
  const snd = await page.evaluate(() => window.bar3dStudio.S.draft['kinds/bar.json'].kinds.find((x) => x.id === 'arcade_cabinet').parts.sound);
  check(snd && snd.effect && snd.near > 0, `S8.4 the arcade machine has a sound of its own (${snd && snd.effect}, heard from ${snd && snd.near} m)`);
  await page.click('#monitor [data-hear="sound"]');
  await page.waitForFunction(() => window.bar3dSound.heard, null, { timeout: 8000 }).catch(() => {});
  check(!!(await page.evaluate(() => window.bar3dSound.heard)), `S8.4 ▶ plays it at the machine (${await page.evaluate(() => window.bar3dSound.heard)})`);
  await page.screenshot({ path: 'build/studio-sound.png' });
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
}

// ---- S10.1 a note for Claude on the fireplace: it lands in Dev's notes ----
{
  const fire = (await S()).things.find((t) => t.id === 'fire_flames');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), fire.i);
  await page.waitForTimeout(500);
  await page.click('#moreKey');
  await page.click('#thingMenu button[data-act="Note for Claude"]');
  await page.fill('#noteText', 'Make the fire a bit warmer, please');
  await page.click('#noteGo');
  await page.waitForFunction(() => window.bar3dNotes.last, null, { timeout: 15000 }).catch(() => {});
  const dev = await (await fetch(url + 'dev/api.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify({ a: 'notes', token: owner.token }) })).json();
  const note = (dev.notes || []).find((n) => /^Studio: /.test(n.text) && /warmer/.test(n.text));
  check(!!note && note.text.includes(`Thing: ${fire.id}`) && /Camera: -?\d/.test(note.text) && /Picture: \/hub\/hub\.php\?shot=/.test(note.text) && !note.read, `S10.1 the note reaches Dev's notes with the thing, the camera and a picture (J5.4)`);
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
await page2.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft && !document.body.classList.contains('reloading'), null, { timeout: 240000, polling: 250 }).catch(() => {});
const again = await page2.evaluate((id) => window.bar3dStudio.S.draft && window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === id).at[0], tbl.id);
check(Math.abs(again - xBeforeUndo) < 0.001, `S3.4 reopening the Studio: your draft is still there (${again} vs ${xBeforeUndo})`);
await page2.close();

// ---- S3.5 publish, the bar has it, history, restore ----
await page.click('#publish');
await page.fill('#pubNote', 'Moved a table');
if (await page.$eval('#pubGo', (b) => b.disabled)) console.log('PUBLISH BLOCKED:', await page.textContent('#publishSheet'));
await page.click('#pubGo');
await page.waitForFunction(() => window.bar3dStudio.S.live && window.bar3dStudio.S.live.version === 1, null, { timeout: 15000 }).catch(() => {});
check((await page.evaluate(() => window.bar3dStudio.S.live.version)) === 1, 'S3.5 Publish makes version 1');
const bar = await browser.newPage({ viewport: { width: 393, height: 852 } });
await bar.goto(url + 'bar/?skipintro&net=off');
await bar.waitForFunction(() => /World: \d+ things/.test(document.getElementById('log').textContent), null, { timeout: 120000, polling: 250 }).catch(() => {});
check(/World: v1 \(live, by Josh\)/.test(await bar.evaluate(() => document.getElementById('log').textContent)), 'S3.5 the bar opens with version 1');
// S6.4: the bar's menu lists the new place; it opens.
await bar.waitForFunction(() => /Stage entered/.test(document.getElementById('log').textContent), null, { timeout: 120000, polling: 250 }).catch(() => {});
await bar.click('#hudMenu').catch(() => {});
await bar.click('.mTile:has-text("Places")').catch(() => {});
await bar.waitForSelector('.mList button[data-place="rooftop"]', { timeout: 15000 }).catch(() => {});
check(!!(await bar.$('.mList button[data-place="rooftop"]')), 'S6.4 the bar\'s menu → Places lists rooftop');
await bar.goto(url + 'bar/?skipintro&net=off&place=rooftop');
await bar.waitForFunction(() => /World: \d+ things/.test(document.getElementById('log').textContent), null, { timeout: 120000, polling: 250 }).catch(() => {});
const roofLog = await bar.evaluate(() => document.getElementById('log').textContent);
check(/World: \d+ things/.test(roofLog) && !/no place/.test(roofLog), `S6.4 the bar opens the rooftop (${(roofLog.match(/World: \d+ things[^\n]*/) || [''])[0]})`);
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

// ---- S9.1 live for everyone: a bar that's open gets the new version within seconds ----
{
  const live = await browser.newPage({ viewport: { width: 393, height: 852 } });
  await live.goto(url + 'bar/?skipintro&server=off');
  await live.waitForFunction(() => /Stage entered/.test(document.getElementById('log').textContent), null, { timeout: 180000, polling: 250 }).catch(() => {});
  const v0 = await live.evaluate(() => window.bar3dLive && window.bar3dLive.version());
  // Publish a change from the Dials (the pendant lamps' sway: 7 s → 8 s).
  const len = page.locator('[data-kind="pendant_lamp"] .dial input').first();
  await len.fill('8');
  await len.press('Enter');
  await page.waitForFunction(() => /saved/.test(document.getElementById('savedText').textContent), null, { timeout: 8000 }).catch(() => {});
  await page.click('#publish');
  await page.click('#go');
  await page.waitForFunction(() => /Live · v3/.test(document.getElementById('savedText').textContent), null, { timeout: 8000 }).catch(() => {});
  await live.waitForFunction(() => /World: v3 \(live/.test(document.getElementById('log') ? document.getElementById('log').textContent : ''), null, { timeout: 90000, polling: 500 }).catch(() => {});
  const logText = await live.evaluate(() => (document.getElementById('log') || {}).textContent || '');
  check(v0 === 2 && /World: v3 \(live/.test(logText), `S9.1 the open bar picked up version 3 by itself (was v${v0})`);
  await live.close();
  // S9.3: Undo publish puts version 2 back (as version 4).
  await page.waitForSelector('#undoPub:not([hidden])', { timeout: 5000 }).catch(() => {});
  await page.click('#undoPub');
  await page.click('#undoPub');
  await page.waitForTimeout(1500);
  const v4 = await (await fetch(url + 'studio/api.php?live')).json();
  const b4 = await (await fetch(url + 'studio/api.php?bundle=' + v4.version)).json();
  const lamp4 = b4.files['kinds/bar.json'].kinds.find((k) => k.id === 'pendant_lamp').loops.sway.period;
  check(v4.version === 4 && lamp4 === 7, `S9.3 Undo publish: version 2 is live again as v${v4.version} (sway ${lamp4} s)`);
}

// ---- S9.2 try a draft with a friend: the link opens the bar with it ----
{
  await page.goto(url + 'studio/');
  await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft && !document.body.classList.contains('reloading'), null, { timeout: 180000, polling: 250 }).catch(() => {});
  const tb = (await S()).things.find((t) => t.id === 'round_table-2');
  await page.evaluate((i) => window.bar3dStudio.select(i, true), tb.i);
  await page.waitForTimeout(400);
  const x0 = parseFloat(await page.inputValue('#valField'));
  await page.fill('#valField', String(+(x0 + 0.5).toFixed(3)));
  await page.press('#valField', 'Enter');
  await page.waitForTimeout(400);
  await page.click('#publish');
  await page.click('#shareKey');
  await page.waitForSelector('#shareBox:not([hidden])', { timeout: 10000 }).catch(() => {});
  const link = await page.inputValue('#shareLink').catch(() => '');
  check(/\/bar\/\?draft=[a-z0-9]{8}&room=draft-/.test(link), `S9.2 "Try it with a friend first" gives a link (${link})`);
  const friend = await browser.newPage({ viewport: { width: 393, height: 852 } });
  await friend.goto(link.replace(/^https?:\/\/[^/]+\//, url) + '&skipintro&server=off');
  await friend.waitForFunction(() => /World: \d+ things/.test(document.getElementById('log').textContent), null, { timeout: 180000, polling: 250 }).catch(() => {});
  check(/World: v0 \(live, by Josh's draft\)/.test(await friend.evaluate(() => document.getElementById('log').textContent)), 'S9.2 the friend\'s bar opens with Josh\'s draft');
  await friend.close();
  await page.click('#pubCancel');
}

// ---- S10.2-S10.4 Claude in the loop: as an author, deep links, proposals ----
{
  const post = async (path, body, headers = {}) => (await fetch(url + path, { method: 'POST', headers: { 'Content-Type': 'text/plain', ...headers }, body: JSON.stringify(body) })).json();
  const key = (await post('hub/hub.php', { a: 'key', token: owner.token })).key;
  const live = await (await fetch(url + 'studio/api.php?live')).json();
  const files = (await (await fetch(url + 'studio/api.php?bundle=' + live.version)).json()).files;
  const moved = JSON.parse(JSON.stringify(files));
  const tbl3 = moved['places/bar.json'].things.find((t) => t.id === 'round_table-3');
  tbl3.at = [tbl3.at[0] + 0.4, tbl3.at[1], tbl3.at[2]];
  const pub = await post('studio/api.php', { a: 'publish', base: live.version, note: 'nudged a table', bundle: { format: 1, files: moved } }, { 'X-Hub-Key': key });
  await page.goto(url + 'studio/');
  await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft && !document.body.classList.contains('reloading'), null, { timeout: 180000, polling: 250 }).catch(() => {});
  await page.click('#monitorKey');
  await page.click('#monitor .tabs [data-tab="history"]');
  await page.waitForSelector(`#monitor [data-what="${pub.live && pub.live.version}"]`, { timeout: 10000 }).catch(() => {});
  // (The Monitor can redraw History right after it opens: click again if the list didn't come.)
  for (let k = 0; k < 3; k++) {
    await page.$eval(`#monitor [data-what="${pub.live && pub.live.version}"]`, (b) => b.click()).catch((e) => console.log('HISTCLICK', e.message));
    if (await page.waitForFunction(() => /moved/.test((document.querySelector('#monitor .histwhat') || {}).textContent || ''), null, { timeout: 6000 }).then(() => true).catch(() => false)) break;
  }
  const hist = await page.textContent('#monitor .body');
  if (!/round_table-3 moved/.test(hist)) console.log('HISTDBG', JSON.stringify(pub).slice(0, 200), hist.slice(0, 400), await page.evaluate(() => JSON.stringify({ open: [...((window.bar3dMonitor || {}).whatOpen || [])], list: (window.bar3dMonitor || {}).whatList, dis: [...document.querySelectorAll('#monitor [data-what]')].map((b) => b.disabled) }).slice(0, 600)));
  check(/Claude: nudged a table/.test(hist) && /round_table-3 moved/.test(hist), 'S10.2 Claude\'s version reads in History as "Claude: …", with what it changed');
  // S10.3: Inspect links to the kind's dials.
  const tb3 = (await S()).things.find((t) => t.id === 'round_table-3');
  await page.evaluate((i) => window.bar3dStudio.select(i, false), tb3.i);
  await page.click('#monitor .tabs [data-tab="inspect"]');
  check((await page.getAttribute('#toDials', 'href')) === '/hub/dials.html#kind=round_table', 'S10.3 Inspect → "Dials for every round_table"');
  await page.click('#monitor .key[aria-label="Close the Monitor"]');
  // S10.4: a proposal from Claude: preview it, then apply it.
  const prop = JSON.parse(JSON.stringify(moved));
  const tbl3b = prop['places/bar.json'].things.find((t) => t.id === 'round_table-3');
  tbl3b.at = [tbl3b.at[0], tbl3b.at[1], tbl3b.at[2] + 1];
  const pr = await post('studio/api.php', { a: 'propose', note: 'move that table back a metre', bundle: { format: 1, files: prop } }, { 'X-Hub-Key': key });
  await page.reload();
  await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft && !document.body.classList.contains('reloading'), null, { timeout: 180000, polling: 250 }).catch(() => {});
  await page.waitForSelector('#propKey:not([hidden])', { timeout: 10000 }).catch(() => {});
  check(pr.ok === 1 && /Proposals 1/.test(await page.textContent('#propKey').catch(() => '')), 'S10.4 a proposal from Claude waits in the Studio');
  await page.click('#propKey');
  await page.click(`#propSheet [data-act="preview"]`);
  await page.waitForSelector('#previewBar:not([hidden])', { timeout: 10000 }).catch(() => {});
  await page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  const z = await (async () => { const t = (await S()).things.find((x) => x.id === 'round_table-3'); return t && t.at[2]; })();
  check(Math.abs(z - tbl3b.at[2]) < 0.01, `S10.4 Preview shows it in the world (the table at z ${z})`);
  await page.screenshot({ path: 'build/studio-proposal.png' });
  await page.click('#previewBar .key.go');
  await page.waitForTimeout(500);
  await page.waitForFunction(() => !document.body.classList.contains('reloading'), null, { timeout: 90000 }).catch(() => {});
  const dz = await page.evaluate(() => window.bar3dStudio.S.draft['places/bar.json'].things.find((t) => t.id === 'round_table-3').at[2]);
  const left = await post('studio/api.php', { a: 'proposals', token: owner.token });
  check(Math.abs(dz - tbl3b.at[2]) < 0.001 && left.proposals.length === 0, `S10.4 Apply puts it in your draft (one undo) and it leaves the list (z ${dz})`);
  // P2.1 Start over: the draft goes away, the live world comes back.
  await page.waitForTimeout(1500);
  await page.click('#monitorKey').catch(() => {});
  await page.click('#monitor .tabs [data-tab="changes"]');
  await page.waitForSelector('#monitor [data-start-over]', { timeout: 10000 }).catch(() => {});
  await page.$eval('#monitor [data-start-over]', (b) => b.click()).catch(() => {});
  await Promise.all([page.waitForNavigation({ timeout: 30000 }).catch(() => {}), page.$eval('#monitor [data-start-over]', (b) => b.click()).catch(() => {})]);
  await page.waitForFunction(() => window.bar3dStudio && window.bar3dStudio.S.draft && !document.body.classList.contains('reloading'), null, { timeout: 180000, polling: 250 }).catch(() => {});
  const fresh = await page.evaluate(() => { const s = window.bar3dStudio.S; return JSON.stringify(s.draft['places/bar.json']) === JSON.stringify(s.base['places/bar.json']); });
  const srv = await post('studio/api.php', { a: 'draft', token: owner.token });
  check(fresh && !srv.draft, 'P2.1 Start over throws the draft away and opens the live world');
}
check(errors.length === 0, 'no page errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close();
close();
console.log(failed ? `STUDIO TEST FAILED (${failed})` : 'STUDIO TEST PASSED');
process.exit(failed ? 1 : 0);
