// The project hub (hub/): its page and hub.php with the bar's account.php,
// on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/hub, <root>/private). The owner logs in on the
// page, adds a card, makes a Claude key; "Claude" reads the queue and reports;
// someone else is kept out; passwords change and a recovery code resets one.
// Screenshots: build/hub.png (the board), build/hub-card.png (a card).
//   NODE_PATH=$(npm root -g) node tools/hub_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, copyFileSync, rmSync } from 'node:fs';
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
for (const f of ['index.html', 'hub.php', 'seed.json']) copyFileSync(join('hub', f), join(root, 'home/hub', f));
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

  browser = await chromium.launch({ executablePath: process.env.CHROMIUM || undefined });
  const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  await page.goto(base + '/hub/');
  await page.fill('input[autocomplete=username]', 'josh');
  await page.fill('input[type=password]', 'secret1');
  await page.click('button.main');
  await page.waitForSelector('section.sec', { timeout: 10000 });
  const sections = await page.$$eval('section.sec h2', (h) => h.map((x) => x.textContent));
  check(sections.includes('Games') && sections.includes('Your character') && !sections.includes('Performance'),
        `the World tab shows the player-facing sections (${sections.length})`);
  const cards = await page.$$eval('.card', (c) => c.length);
  check(cards > 30, `the board starts with the project's cards (${cards} in World)`);
  await page.screenshot({ path: 'build/hub.png' });

  // A new card in Games, ready for Claude.
  const games = page.locator('section.sec', { hasText: 'Darts, 4K Mania' });
  await games.locator('button', { hasText: '+ New card' }).click();
  await page.fill('.sheetBody input', 'Bowling lane in the arcade');
  await page.click('.stages button:has-text("Ready for Claude")');
  await page.fill('.sheetBody textarea', 'Two lanes, real pins with physics.');
  await page.click('.sheetBody button.main');
  await page.waitForSelector('.card:has-text("Bowling lane")', { timeout: 5000 });
  check(true, 'the owner adds a card from the page');

  const ownerToken = await page.evaluate(() => localStorage.getItem('hubToken'));
  const key = (await post('/hub/hub.php', { a: 'key', token: ownerToken })).key;
  check(/^[0-9a-f]{48}$/.test(key || ''), 'the owner makes a Claude key');
  const queue = await post('/hub/hub.php', { a: 'queue' }, { 'X-Hub-Key': key });
  const card = (queue.cards || []).find((c) => c.title === 'Bowling lane in the arcade');
  check(queue.ok === 1 && !!card && card.plan.includes('real pins'), 'Claude reads the queue with the key');
  const rep = await post('/hub/hub.php', { a: 'report', id: card.id, stage: 'check', report: 'Built two lanes.', build: 'abc1234',
                                           question: 'Wood or neon lanes?' }, { 'X-Hub-Key': key });
  check(rep.ok === 1 && rep.card.stage === 'check' && rep.card.history.length >= 2, 'Claude reports on the card (stage, report, question)');
  const bad = await post('/hub/hub.php', { a: 'queue' }, { 'X-Hub-Key': 'f'.repeat(48) });
  check(bad.status === 401, 'a wrong key is refused');

  // The owner sees it on the page and answers.
  await page.click('button:has-text("⟳")');
  await page.click('.tabs button:has-text("Check on phone")');
  await page.waitForSelector('.card:has-text("Bowling lane")', { timeout: 5000 });
  await page.click('.card:has-text("Bowling lane")');
  const shown = await page.textContent('.sheetBody');
  check(shown.includes('Built two lanes.') && shown.includes('Wood or neon lanes?') && shown.includes('abc1234'),
        'the card shows Claude\'s report, question and build');
  await page.screenshot({ path: 'build/hub-card.png' });
  await page.fill('.sheetBody textarea[placeholder="Your answer for Claude"]', 'Neon!');
  await page.click('.sheetBody button.main');
  await wait(500);
  const after = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === card.id);
  check(after.question === '' && after.plan.includes('Answer: Neon!'), 'the answer goes into the plan and the question clears');

  const before = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.filter((c) => c.section === 'games');
  const second = before[1];
  await post('/hub/hub.php', { a: 'move', token: ownerToken, id: second.id, dir: -1 });
  const moved = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.filter((c) => c.section === 'games');
  check(moved[0].id === second.id, 'moving a card up reorders its section');

  const stranger = await post('/hub/hub.php', { a: 'board', token: other.token });
  check(stranger.status === 403 && /private/.test(stranger.error), 'someone else is kept out ("This hub is private.")');

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
