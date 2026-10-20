// The project hub (hub/): its page and hub.php with the bar's account.php,
// on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/hub, <root>/private). The owner logs in on the
// page, adds a card, makes a Claude key; "Claude" reads the queue and reports;
// someone else is kept out; passwords change and a recovery code resets one.
// Screenshots: build/hub-projects.png, build/hub.png (a section), build/hub-card.png (a card's Talk), build/hub-status.png.
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
mkdirSync(join(root, 'home/hub/projects'));
copyFileSync('hub/projects/bar3d.jpg', join(root, 'home/hub/projects/bar3d.jpg'));
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
  await page.click('button.primary');
  // The projects screen: Bar3D with its picture.
  await page.waitForSelector('.tile h2', { timeout: 10000 });
  const tiles = await page.$$eval('.tile h2', (h) => h.map((x) => x.textContent));
  const pic = await page.$eval('.tile .shot', (s) => getComputedStyle(s).backgroundImage);
  check(tiles.includes('Bar3D') && /bar3d\.jpg/.test(pic), `the projects screen shows Bar3D with its picture (${tiles.join(', ')})`);
  await page.screenshot({ path: 'build/hub-projects.png' });
  await page.click('.tile:has-text("Bar3D")');
  await page.waitForSelector('.nav button', { timeout: 10000 });
  const nav = await page.$$eval('.nav button', (b) => b.map((x) => x.textContent));
  check(nav.some((t) => t.startsWith('Needs you')) && nav.some((t) => t.startsWith('Games')) && nav.some((t) => t.startsWith('Performance')),
        `the board's nav has the views and every section (${nav.length})`);
  await page.click('.nav button:has-text("Games")');
  // (Wait for the Games view itself: the previous view's items match too.)
  await page.waitForFunction(() => location.pathname.endsWith('/s/games') && document.querySelectorAll('.content .item').length > 3,
                             null, { timeout: 5000 }).catch(() => {});
  const cards = await page.$$eval('.content .item', (c) => c.length);
  check(cards > 3 && page.url().endsWith('/s/games'), `a section shows its cards, grouped by stage (${cards} in Games)`);
  await page.screenshot({ path: 'build/hub.png' });

  // A new card in Games, straight to Claude's queue.
  await page.click('.shead button:has-text("+ Card")');
  await page.fill('.sheetBody input', 'Bowling lane in the arcade');
  await page.fill('.sheetBody textarea', 'Two lanes, real pins with physics.');
  await page.click('.sheetBody button:has-text("Ready for Claude")');
  await page.waitForSelector('.screen .crumb:has-text("Bowling lane")', { timeout: 5000 });
  check(true, 'the owner adds a card from the page (it opens)');
  await page.click('.screen .bar button');
  await page.waitForSelector('.content .item:has-text("Bowling lane")', { timeout: 5000 });

  const ownerToken = await page.evaluate(() => localStorage.getItem('hubToken'));
  const key = (await post('/hub/hub.php', { a: 'key', token: ownerToken })).key;
  check(/^[0-9a-f]{48}$/.test(key || ''), 'the owner makes a Claude key');
  const queue = await post('/hub/hub.php', { a: 'queue' }, { 'X-Hub-Key': key });
  const card = (queue.cards || []).find((c) => c.title === 'Bowling lane in the arcade');
  check(queue.ok === 1 && !!card && card.notes.includes('real pins') && card.stage === 'ready', 'Claude reads the queue with the key (the owner\'s words in notes)');
  const rep = await post('/hub/hub.php', { a: 'report', id: card.id, stage: 'check', report: 'Built two lanes.', build: 'abc1234',
                                           question: 'Wood or neon lanes?' }, { 'X-Hub-Key': key });
  check(rep.ok === 1 && rep.card.stage === 'check' && rep.card.thread.length === 2 && !rep.card.unread,
        'Claude reports on the card (stage, report, question; both on the conversation)');
  const sec = await post('/hub/hub.php', { a: 'section', id: 'games', about: 'What you play.' }, { 'X-Hub-Key': key });
  const secBoard = await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key });
  check(sec.ok === 1 && secBoard.sections.find((s) => s.id === 'games').about === 'What you play.', 'Claude sets a section\'s "what goes here" line');
  const bad = await post('/hub/hub.php', { a: 'queue' }, { 'X-Hub-Key': 'f'.repeat(48) });
  check(bad.status === 401, 'a wrong key is refused');

  // The owner sees it in "Needs you" and answers in plain words.
  await page.click('.top button[title=Refresh]');
  await page.click('.nav button:has-text("Needs you")');
  await page.waitForSelector('.content .item:has-text("Bowling lane")', { timeout: 5000 });
  await page.click('.content .item:has-text("Bowling lane")');
  await page.waitForSelector('.screen .composer textarea', { timeout: 5000 });
  const shown = await page.textContent('.screen');
  check(shown.includes('Built two lanes.') && shown.includes('Wood or neon lanes?') && shown.includes('abc1234'),
        'the card opens on Talk: Claude\'s report, question and build');
  await page.screenshot({ path: 'build/hub-card.png' });
  await page.fill('.screen .composer textarea', 'Neon!');
  await page.click('.screen .composer button:has-text("Answer")');
  await page.waitForSelector('.msg.me:has-text("Neon!")', { timeout: 5000 });
  const inbox = await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key });
  const after = inbox.cards.find((c) => c.id === card.id);
  check(!!after && after.question === '' && after.thread.at(-1).kind === 'answer' && after.thread.at(-1).text === 'Neon!' && !after.plan.includes('Neon'),
        'the answer goes on the conversation, the question clears, and the card is in Claude\'s inbox');
  await page.click('.seg button:has-text("Status")');
  await page.screenshot({ path: 'build/hub-status.png' });
  await page.click('.screen button:has-text("It works")');
  await wait(600);
  const done = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === card.id);
  check(done.stage === 'done', 'one tap on Status: "It works → Done"');
  await page.click('.seg button:has-text("Your plan")');
  await page.fill('.screen textarea', 'Two lanes, neon, a scoreboard.');
  await page.click('.screen .block button:has-text("Save")');
  await page.waitForSelector('.saved:has-text("Saved")', { timeout: 5000 });
  const notes = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === card.id);
  const claudeNotes = await post('/hub/hub.php', { a: 'save', card: { id: card.id, notes: 'overwritten' } }, { 'X-Hub-Key': key });
  check(notes.notes === 'Two lanes, neon, a scoreboard.' && claudeNotes.card.notes === notes.notes,
        '"What you want" saves, and Claude can\'t overwrite it');

  // A second project, with its own board.
  await page.goto(base + '/hub/#/');
  await page.waitForSelector('.tile.add', { timeout: 5000 });
  await page.click('.tile.add');
  await page.fill('.sheetBody input', 'Kids Game');
  await page.click('.sheetBody button.primary');
  await page.waitForSelector('.tile h2:has-text("Kids Game")', { timeout: 5000 });
  const kids = await post('/hub/hub.php', { a: 'board', project: 'kids-game' }, { 'X-Hub-Key': key });
  const kidsAdd = await post('/hub/hub.php', { a: 'save', project: 'kids-game', card: { section: 'features', title: 'Jump' } }, { 'X-Hub-Key': key });
  const barStill = await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key });
  check(kids.ok === 1 && kids.sections.some((s) => s.id === 'features') && kids.cards.length === 0 && kidsAdd.ok === 1 &&
        !barStill.cards.some((c) => c.title === 'Jump'), 'a new project gets its own empty board');
  check((await post('/hub/hub.php', { a: 'board', project: 'nope' }, { 'X-Hub-Key': key })).status === 404, 'an unknown project is refused');

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
