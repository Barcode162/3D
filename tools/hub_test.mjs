// The hub's server (hub/hub.php) with the bar's account.php, on PHP's
// built-in server, laid out like the IONOS webspace (<root>/home/bar,
// <root>/home/hub, <root>/private). The owner makes a Claude key; Claude
// uploads a screenshot and writes a news draft; only the owner publishes it
// (news/) and tunes the loops; someone else is kept out; the old board's
// actions are gone (J5.4); /hub/ opens Dev; the World page lists the bar's
// kinds; passwords change and a recovery code resets one.
// Screenshots: build/news.png, build/hub-world.png.
//   NODE_PATH=$(npm root -g) node tools/hub_test.mjs
import { spawn } from 'node:child_process';
import { cpSync, mkdtempSync, mkdirSync, copyFileSync, rmSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');
const PORT = 8793;
const base = `http://127.0.0.1:${PORT}`;
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };
const wait = (ms) => new Promise((r) => setTimeout(r, ms));

const root = mkdtempSync(join(tmpdir(), 'bar3d-hub-'));
mkdirSync(join(root, 'home/bar'), { recursive: true });
mkdirSync(join(root, 'home/hub'), { recursive: true });
copyFileSync('server/account.php', join(root, 'home/bar/account.php'));
for (const f of ['index.html', 'world.html', 'hub.php']) copyFileSync(join('hub', f), join(root, 'home/hub', f));
for (const d of ['kinds', 'places']) {  // the World page reads the bar's files
  mkdirSync(join(root, 'home/bar/assets/world', d), { recursive: true });
  copyFileSync(`assets/world/${d}/bar.json`, join(root, 'home/bar/assets/world', d, 'bar.json'));
}
mkdirSync(join(root, 'home/news'));
copyFileSync('news/index.php', join(root, 'home/news/index.php'));
cpSync('ui', join(root, 'home/ui'), { recursive: true });  // the look kit (news, the hub)
const php = spawn('php', ['-S', `127.0.0.1:${PORT}`, '-t', join(root, 'home')], { stdio: 'ignore' });

const post = async (path, body, headers = {}) => {
  const r = await fetch(base + path, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', ...headers } });
  return { status: r.status, ...(await r.json()) };
};
const year = new Date().getFullYear();
let browser;
try {
  await wait(800);
  const owner = await post('/bar/account.php', { a: 'signup', user: 'josh', pass: 'secret1', name: 'Josh', birth: `${year - 30}-03-04` });
  const other = await post('/bar/account.php', { a: 'signup', user: 'stranger', pass: 'secret2', birth: `${year - 20}-03-04` });
  check(owner.ok === 1 && other.ok === 1, 'two accounts (the first one is the owner)');

  const ownerToken = owner.token;
  const as = (body) => post('/hub/hub.php', { ...body, token: ownerToken });
  const key = (await as({ a: 'key' })).key;
  check(/^[0-9a-f]{48}$/.test(key || ''), 'the owner makes a Claude key');
  const claude = (body) => post('/hub/hub.php', body, { 'X-Hub-Key': key });
  check((await post('/hub/hub.php', { a: 'posts' }, { 'X-Hub-Key': 'f'.repeat(48) })).status === 401, 'a wrong key is refused');
  const stranger = await post('/hub/hub.php', { a: 'posts', token: other.token });
  check(stranger.status === 403 && /private/.test(stranger.error), 'someone else is kept out ("This hub is private.")');
  const gone = await Promise.all(['board', 'queue', 'inbox', 'save', 'say', 'report', 'projects', 'spots', 'section', 'move'].map((a) => claude({ a })));
  check(gone.every((r) => r.status === 400 && /Unknown action/.test(r.error)), 'the old board\'s actions are gone (J5.4)');

  // The news: a screenshot, a draft, the owner's Publish.
  const jpg = 'data:image/jpeg;base64,' + readFileSync('homepage/site/img/jukebox.jpg').toString('base64');
  const shot = await claude({ a: 'shot', build: 'abc1234', name: 'door', image: jpg });
  check(shot.ok === 1 && shot.file === 'abc1234-door.jpg', 'Claude uploads a screenshot');
  check((await claude({ a: 'shot', build: 'abc1234', name: 'bad', image: 'data:image/jpeg;base64,aGVsbG8=' })).status === 400,
        'something that isn\'t a picture is refused');
  const news = await claude({ a: 'post', post: { build: 'abc1234', title: 'Neon lanes', text: 'Two bowling lanes in the arcade.', shots: [shot.file] } });
  check(news.ok === 1 && news.post.shots[0].url.includes('abc1234-door.jpg') && !news.post.published, 'Claude writes a news post (private until published)');
  check((await claude({ a: 'publish', id: news.post.id, on: 1 })).status === 403, 'only the owner publishes');
  const listed = await claude({ a: 'posts' });
  check(listed.ok === 1 && listed.posts.length === 1 && listed.posts[0].title === 'Neon lanes', 'the drafts are listed');

  // The loops' tuning: anyone reads, only the owner tunes.
  check((await claude({ a: 'tune', kind: 'fire_flames', speed: 2 })).status === 403, 'only the owner tunes');
  check((await as({ a: 'tune', kind: 'fire_flames', speed: 2 })).ok === 1, 'the owner tunes a kind');
  check((await fetch(base + '/hub/hub.php?tuning').then((r) => r.json())).kinds.fire_flames.speed === 2, 'anyone reads the tuning');
  const status = async (url) => { const r = await fetch(url); await r.arrayBuffer(); return r.status; };  // always read the body (undici)
  check((await status(base + '/hub/hub.php?shot=bar3d/abc1234-door.jpg')) === 403, 'a screenshot needs a login');
  check((await fetch(base + '/news/?json').then((r) => r.json())).posts.length === 0, 'nothing on the public board before Publish');
  check((await as({ a: 'publish', id: news.post.id, on: 1 })).ok === 1, 'the owner publishes');
  const pub = (await fetch(base + '/news/?json').then((r) => r.json())).posts;
  check(pub.length === 1 && pub[0].title === 'Neon lanes', 'Publish puts the post on the public news board');
  check((await status(base + '/news/' + pub[0].shots[0])) === 200 && (await status(base + '/news/?shot=bar3d/abc1234-other.jpg')) === 404,
        'the board shows the published post\'s picture, nothing else');

  browser = await chromium.launch({ executablePath: process.env.CHROMIUM || undefined });
  const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  // The hub's address now opens Dev.
  await page.goto(base + '/hub/');
  await page.waitForURL(/\/dev\//, { timeout: 5000 }).catch(() => {});
  check(/\/dev\//.test(page.url()), '/hub/ sends you to Dev');
  // The public news page lists the post.
  await page.goto(base + '/news/');
  check((await page.textContent('main')).includes('Neon lanes'), 'the public page lists it');
  await wait(900);
  await page.screenshot({ path: 'build/news.png' });

  // The World page (E8.1): every kind in plain words, each with a link to its Dials.
  await page.goto(base + '/hub/world.html');
  await page.waitForSelector('.kind h2', { timeout: 5000 });
  const kindNames = await page.$$eval('.kind h2', (h) => h.map((x) => x.firstChild.textContent));
  check(kindNames.includes('fire_flames') && kindNames.includes('bar_stool') && kindNames.length > 10, `the World page lists the alive kinds (${kindNames.length})`);
  await page.click('#filters button:has-text("Everything")');
  check((await page.$$('.kind')).length > kindNames.length, 'Everything shows the still ones too');
  await page.click('#filters button:has-text("Alive")');
  check((await page.getAttribute('.kind:has(h2:text-is("fire_flames")) a.dials', 'href')) === 'dials.html#kind=fire_flames', 'each kind links to its Dials (S4.2: no more speed sliders)');
  await page.screenshot({ path: 'build/hub-world.png', fullPage: false });

  // Passwords: change, recovery code, reset.
  check((await post('/bar/account.php', { a: 'password', token: owner.token, old: 'wrong', new: 'newpass1' })).status === 401,
        'changing the password needs the current one');
  check((await post('/bar/account.php', { a: 'password', token: owner.token, old: 'secret1', new: 'newpass1' })).ok === 1,
        'the password changes');
  check((await post('/bar/account.php', { a: 'login', user: 'josh', pass: 'newpass1' })).ok === 1, 'the new password logs in');
  const rec = await post('/bar/account.php', { a: 'recovery', token: owner.token, pass: 'newpass1' });
  check(/^[A-Z2-9]{4}-[A-Z2-9]{4}-[A-Z2-9]{4}$/.test(rec.code || ''), `a recovery code (${rec.code})`);
  check((await post('/bar/account.php', { a: 'reset', user: 'josh', code: 'AAAA-BBBB-CCCC', new: 'x123456' })).status === 401,
        'a wrong recovery code does nothing');
  const reset = await post('/bar/account.php', { a: 'reset', user: 'josh', code: rec.code.toLowerCase(), new: 'fresh123' });
  check(reset.ok === 1 && reset.token, 'the recovery code sets a new password (and logs in)');
  check((await post('/bar/account.php', { a: 'me', token: owner.token })).status === 401, '...and signs out the old logins');
  check((await post('/bar/account.php', { a: 'reset', user: 'josh', code: rec.code, new: 'again123' })).status === 401,
        'a recovery code works once');
  check(errors.length === 0, 'no page errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
} catch (e) {
  console.log('ERROR ' + (e && e.stack || e));
  failed++;
} finally {
  if (browser) await browser.close();
  php.kill();
  rmSync(root, { recursive: true, force: true });
}
console.log(failed ? `HUB TEST FAILED (${failed})` : 'HUB TEST PASSED');
process.exit(failed ? 1 : 0);
