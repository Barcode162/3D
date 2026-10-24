// Dev (dev/, PLAN.md R4) at phone width on a throwaway webspace (PHP's server:
// ui/, dev/, the bar's account.php): a stranger gets the login, a player is
// told Dev is for admins, the owner gets the five tabs, every tab draws its
// screen file. Later pieces add their checks below. No game: seconds.
// Usage: NODE_PATH=$(npm root -g) node tools/dev_test.mjs
import { launchBrowser } from './lib/browser.mjs';
import { webspace } from './lib/webspace.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
import { mkdirSync, writeFileSync, copyFileSync } from 'node:fs';
import { join } from 'node:path';
const ws = await webspace({ dirs: { ui: 'ui', dev: 'dev', toolbelt: 'toolbelt', hub: 'hub' }, files: { 'bar/account.php': 'server/account.php' } });
// What CI and the deploy put there: the game's config, the plan, no deploy report yet.
writeFileSync(join(ws.root, 'home/bar/config.json'), JSON.stringify({ build: 'abc1234', ws: '' }));
mkdirSync(join(ws.root, 'private/dev'), { recursive: true });
copyFileSync('PLAN.md', join(ws.root, 'private/dev/plan.md'));
const browser = await launchBrowser();
process.on('uncaughtException', (e) => { console.log('FAIL ' + e.message); ws.close(); process.exit(1); });
const year = new Date().getFullYear();
const owner = await ws.post('bar/account.php', { a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${year - 30}-03-04` });
const player = await ws.post('bar/account.php', { a: 'signup', user: 'mia', pass: 'secret2', name: 'Mia', birth: `${year - 25}-03-04` });
check(owner.ok && player.ok, 'two accounts (the first is the owner)');

const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
page.on('response', (r) => { if (r.status() >= 400 && !(r.status() === 401 && /\.php/.test(r.url()))) errors.push(r.status() + ' ' + r.url()); });
const ed = (fn, arg) => page.evaluate(fn, arg);

// R4.1: the door and the tabs
await page.goto(ws.url + 'dev/');
await page.waitForSelector('#login:not([hidden])');
check(true, 'a stranger gets the login');
await page.fill('#user', 'mia'); await page.fill('#pass', 'secret2'); await page.click('#login button[type=submit]');
await page.waitForFunction(() => /for the owner and admins/.test(document.getElementById('loginWhy').textContent));
check(await ed(() => document.getElementById('tabs').hidden), 'a player is told Dev is for the owner and admins');
await ed(() => localStorage.removeItem('hubToken'));
await page.fill('#user', 'josh'); await page.fill('#pass', 'secret1'); await page.click('#login button[type=submit]');
await page.waitForSelector('#tabs:not([hidden]) button');
check(await ed(() => document.querySelectorAll('#tabs button').length === 5 && document.documentElement.dataset.look === 'ember'), 'the owner gets five tabs, in Ember');
for (const tab of ['overview', 'projects', 'plan', 'publish', 'people']) {
  await page.click(`#tabs [data-tab="${tab}"]`);
  await page.waitForFunction((t) => document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === t && document.querySelector('#view h2'), tab);
  check(await ed(() => document.querySelectorAll('#view [data-id]').length > 0 && document.documentElement.scrollWidth <= 390), `${tab}: its screen file draws, slots marked, nothing wider than the phone`);
}
// R4.3: Overview
await page.click('#tabs [data-tab="overview"]');
await page.waitForFunction(() => /build abc1234/.test(document.querySelector('#view [data-id="status"]')?.textContent || ''));
check(true, 'Overview shows the live build from the game\'s config');
await page.waitForFunction(() => /No report yet/.test(document.querySelector('#view [data-id="status"]').textContent));
check(true, 'no deploy report yet is said plainly');
await page.waitForFunction(() => /R4 · Dev/.test(document.querySelector('#view [data-id="now"]').textContent));
check(await ed(() => / of 7/.test(document.querySelector('#view [data-id="now"]').textContent)), 'Now shows R4 and how many of its pieces are done');
await page.waitForFunction(() => /Nothing waiting|to publish/.test(document.querySelector('#view [data-id="needs"]').textContent));
check(true, 'Needs you answers (news drafts)');
const ci = await ws.post('dev/api.php', { a: 'ci', token: owner.token });
check(ci.ok === 1 && ci.ci === null, 'the store answers the owner');
check((await ws.post('dev/api.php', { a: 'ci', token: player.token })).status === 403, 'the store keeps a player out');

// R4.4: Plan
await page.click('#tabs [data-tab="plan"]');
await page.waitForSelector('#view [data-chapter="The engine"]');
check(await ed(() => ['The world', 'The site', 'The engine', 'The tools', 'The backbone'].every((c) => document.querySelector(`#view [data-chapter="${c}"]`))), 'Plan shows the five chapters');
check(await ed(() => /R4\.1/.test(document.querySelector('#view [data-id="plan-now"]').textContent)), 'Now lists R4\'s pieces');
await page.click('#view [data-id="note-kind"] button:nth-child(2)');
await page.fill('#view [data-id="note"]', 'A rooftop terrace');
await page.click('#view [data-id="note-send"]');
await page.waitForFunction(() => /A rooftop terrace/.test(document.querySelector('#view [data-id="notes"]')?.textContent || ''));
const notes = await ws.post('dev/api.php', { a: 'notes', token: owner.token });
check(notes.ok && notes.notes[0].kind === 'park' && notes.notes[0].text === 'A rooftop terrace' && notes.notes[0].read === 0, 'Park an idea sends it to Claude, waiting');
check((await ws.post('dev/api.php', { a: 'read', ids: [notes.notes[0].id], token: owner.token })).status === 403, 'only Claude marks notes as read');

// R4.2: Projects
await page.click('#tabs [data-tab="projects"]');
await page.waitForSelector('#view [data-project="bar3d"] a[href="/studio/"]');
check(await ed(() => ['bar3d', 'looped', 'script', 'workshop'].every((p) => document.querySelector(`#view [data-project="${p}"]`))), 'Projects lists Bar3D, Looped, the script language and the workshop');
check(await ed(() => !!document.querySelector('#view [data-project="looped"] a[href="/toolbelt/ui-editor/"]')), 'the UI Editor sits under Looped');
check(await ed(() => !!document.querySelector('#view [data-project="bar3d"] .dev-ask input')), 'the perf report asks for its code');
await page.click('#tabs [data-tab="people"]');
await page.waitForFunction(() => document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === 'people');

await page.reload();
await page.waitForSelector('#tabs:not([hidden]) button');
check(await ed(() => location.hash === '#people' && document.querySelector('#tabs [aria-current="page"]').dataset.tab === 'people'), 'a reload stays logged in, on the same tab');

check(errors.length === 0, 'no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close(); ws.close();
console.log(failed ? `DEV TEST: ${failed} FAILED` : 'DEV TEST PASSED');
process.exit(failed ? 1 : 0);
