// The public site (homepage/ → joshuavelasco.com) on PHP's own server, with
// /ui/ next to it and a pretend accounts folder (SITE_ACCOUNTS): a staff
// login, a player login and nobody.
// Checks: every page loads with no errors and fits a phone, the tabs are
// there, the Portal tab and page exist for staff only (everyone else gets a
// 404 and no tab), the sign-in page switches to "New account".
// Screenshots: build/site-<page>.png (phone) and build/site-home-wide.png.
// Usage: NODE_PATH=$(npm root -g) node tools/site_test.mjs
import { mkdtempSync, mkdirSync, symlinkSync, writeFileSync, rmSync, readdirSync } from 'node:fs';
import { spawn } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { createHash } from 'node:crypto';
import { launchBrowser } from './lib/browser.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
// The pictures are for looking at, not checks: a slow one (a long, moving page
// on CI's machine with no graphics card) prints a note instead of failing the deploy.
const shot = (page, opts) => page.screenshot({ animations: 'disabled', timeout: 20000, ...opts })
  .catch((e) => console.log(`note ${opts.path}: no picture (${String(e.message).split('\n')[0]})`));
mkdirSync('build', { recursive: true });

// The site folder: homepage/'s files, /ui/ beside them.
const dir = mkdtempSync(join(tmpdir(), 'site-'));
for (const f of readdirSync('homepage')) symlinkSync(resolve('homepage', f), join(dir, f));
symlinkSync(resolve('ui'), join(dir, 'ui'));
symlinkSync(resolve('toolbelt'), join(dir, 'toolbelt'));  // the one list of projects (J5.5)

// Pretend accounts: "boss" (the owner), "mia" (a player).
const acc = mkdtempSync(join(tmpdir(), 'site-acc-'));
for (const d of ['users', 'tokens']) mkdirSync(join(acc, d));
const people = { boss: ['aaaaaaaaaaa1', 'a'.repeat(48), 'The Boss'], mia: ['bbbbbbbbbbb2', 'b'.repeat(48), 'Mia'] };
for (const [user, [id, token, name]] of Object.entries(people)) {
  writeFileSync(join(acc, 'users', id + '.json'), JSON.stringify({ id, user, name, avatar: 0 }));
  writeFileSync(join(acc, 'tokens', createHash('sha256').update(token).digest('hex') + '.json'), JSON.stringify({ id }));
}
writeFileSync(join(acc, 'admins.json'), JSON.stringify(['boss']));
writeFileSync(join(acc, 'names.json'), JSON.stringify({ boss: people.boss[0], mia: people.mia[0] }));

const port = 8797;
const php = spawn('php', ['-S', `127.0.0.1:${port}`, '-t', dir], { stdio: 'ignore', env: { ...process.env, SITE_ACCOUNTS: acc, SITE_TOOLS: resolve('toolbelt/tools.json') } });
await new Promise((r) => setTimeout(r, 900));
const base = `http://127.0.0.1:${port}`;
const browser = await launchBrowser();

async function open(path, { token = null, width = 390, height = 844 } = {}) {
  const ctx = await browser.newContext({ viewport: { width, height }, deviceScaleFactor: 1 });
  if (token) await ctx.addCookies([{ name: 'bar_t', value: token, url: base }]);
  const page = await ctx.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  // The bar and the news live elsewhere on the real site: their misses here are expected.
  page.on('response', (r) => { if (r.status() >= 400 && !/\/(bar|news)\//.test(r.url()) && !r.url().endsWith(path)) errors.push(r.status() + ' ' + r.url()); });
  const res = await page.goto(base + path, { waitUntil: 'load' });
  await page.evaluate(() => document.fonts.ready);
  await page.waitForTimeout(400);
  return { ctx, page, errors, status: res.status() };
}
// Projects shows every project the one list puts on the site (toolbelt/tools.json, J5.5).
{
  const list = JSON.parse((await import('node:fs')).readFileSync('toolbelt/tools.json', 'utf8')).projects.filter((p) => p.site);
  const html = await (await fetch(base + '/projects/')).text();
  check(list.length > 0 && list.every((p) => html.includes(`<b>${p.site.name || p.name}</b>`) || html.includes(`<h3>${p.site.name || p.name}</h3>`)), `Projects lists the ${list.length} projects of toolbelt/tools.json`);
}
const tabsOf = (page) => page.$$eval('.tabs a', (a) => a.map((x) => x.textContent.trim()));

// Every page as a guest, on a phone.
for (const [name, path] of [['home', '/'], ['projects', '/projects/'], ['play', '/play/'], ['engine', '/engine/'], ['vision', '/vision/'], ['signin', '/signin/']]) {
  const { ctx, page, errors, status } = await open(path);
  check(status === 200, `${name}: 200 (${status})`);
  check(errors.length === 0, `${name}: no errors` + (errors.length ? ': ' + errors.join(' | ') : ''));
  check(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth + 1), `${name}: fits the phone`);
  check((await page.$$('h1')).length === 1, `${name}: one headline`);
  const tabs = await tabsOf(page);
  check(tabs.join(' ') === 'Home Projects News', `${name}: guest tabs (${tabs.join(' ')})`);
  // Every image on the page exists.
  const broken = await page.$$eval('img', (imgs) => imgs.filter((i) => i.getAttribute('src').startsWith('/site/')).map((i) => i.getAttribute('src')));
  for (const src of broken) {
    const r = await page.request.get(base + src);
    if (r.status() !== 200) check(false, `${name}: image ${src} (${r.status()})`);
  }
  // Scroll through so everything slides in, then a full-page shot.
  await page.evaluate(async () => { for (let y = 0; y < document.body.scrollHeight; y += 500) { window.scrollTo({ top: y, behavior: 'instant' }); await new Promise((r) => setTimeout(r, 60)); } window.scrollTo({ top: 0, behavior: 'instant' }); });
  await page.waitForTimeout(1100);
  await shot(page, { path: `build/site-${name}.png`, fullPage: true });
  await ctx.close();
}

// The phone menu opens and closes.
{
  const { ctx, page } = await open('/');
  await page.click('.burger');
  check(await page.isVisible('#drawer'), 'the menu opens');
  await shot(page, { path: 'build/site-menu.png' });
  await page.click('.burger');
  check(!(await page.isVisible('#drawer')), 'the menu closes');
  await ctx.close();
}

// Wide screen: the tabs are in the header.
{
  const { ctx, page, errors } = await open('/', { width: 1440, height: 900 });
  check(await page.isVisible('.tabs'), 'wide: tabs in the header');
  check(errors.length === 0, 'wide: no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
  await shot(page, { path: 'build/site-home-wide.png' });
  await ctx.close();
}

// The portal: a 404 for guests and players; staff are sent to Dev.
{
  const g = await open('/portal/');
  check(g.status === 404, `portal: a guest gets 404 (${g.status})`);
  check(!(await g.page.content()).includes('Open the Studio'), 'portal: a guest sees no tools');
  await g.ctx.close();
  const m = await open('/portal/', { token: people.mia[1] });
  check(m.status === 404, `portal: a player gets 404 (${m.status})`);
  await m.ctx.close();
  const p = await open('/', { token: people.mia[1] });
  check(!(await tabsOf(p.page)).includes('Dev'), 'a player has no Dev tab');
  check((await p.page.textContent('.who')).includes('Mia'), 'a player sees their name in the header');
  await p.ctx.close();

  // Staff: the portal is now Dev (PLAN.md R4.7): /portal/ sends them to /dev/, the tab says Dev.
  const r = await fetch(base + '/portal/', { redirect: 'manual', headers: { cookie: `bar_t=${people.boss[1]}` } });
  check(r.status === 302 && r.headers.get('location') === '/dev/', `portal: staff are sent to Dev (${r.status} ${r.headers.get('location')})`);
  const s = await open('/', { token: people.boss[1] });
  check((await tabsOf(s.page)).includes('Dev'), 'staff have the Dev tab');
  check((await s.page.$$('a[href="/dev/"]')).length > 0, 'the Dev tab opens /dev/');
  check(s.errors.length === 0, 'home as staff: no errors' + (s.errors.length ? ': ' + s.errors.join(' | ') : ''));
  await s.ctx.close();
}

// Sign in: the "New account" tab shows the extra fields.
{
  const { ctx, page } = await open('/signin/');
  check(!(await page.isVisible('#birth')), 'sign in: no birth date when signing in');
  await page.click('[data-mode="new"]');
  check(await page.isVisible('#birth'), 'sign in: New account asks for a birth date');
  await page.click('#go');
  check((await page.textContent('#msg')).length > 0, 'sign in: an empty form says what is missing');
  await ctx.close();
}

// A missing page: the site's own 404.
{
  const { ctx, page, status } = await open('/site/404.php');
  check(status === 404 && (await page.textContent('h1')).includes('Nothing'), 'the 404 page');
  await ctx.close();
}

await browser.close();
php.kill();
rmSync(dir, { recursive: true, force: true });
rmSync(acc, { recursive: true, force: true });
console.log(failed ? `SITE TEST FAILED (${failed})` : 'SITE TEST PASSED');
process.exit(failed ? 1 : 0);
