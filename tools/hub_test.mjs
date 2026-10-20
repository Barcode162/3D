// The project hub (hub/): its page and hub.php with the bar's account.php,
// on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/hub, <root>/private). The owner logs in on the
// page, adds a card, makes a Claude key; "Claude" reads the queue and reports;
// someone else is kept out; passwords change and a recovery code resets one.
// The old board is list.html; the hub v3 (index.html, docs/LOOPED.md C):
// Home answers a question in one tap, the news feed shows a post with its
// screenshot, Publish puts it on the public news board (news/), Explore's
// map opens a place with its things, a new idea and a message land there.
// Screenshots: build/hub-projects.png, build/hub.png (a section), build/hub-card.png (a card's Talk), build/hub-status.png,
// build/hub3-home.png, build/hub3-explore.png, build/hub3-spot.png, build/hub3-engine.png, build/news.png.
//   NODE_PATH=$(npm root -g) node tools/hub_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, mkdirSync, copyFileSync, rmSync, readFileSync } from 'node:fs';
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
for (const f of ['index.html', 'list.html', 'hub.php', 'seed.json']) copyFileSync(join('hub', f), join(root, 'home/hub', f));
mkdirSync(join(root, 'home/hub/projects'));
for (const f of ['bar3d.jpg', 'bar3d-spots.json', 'bar3d-map.json']) copyFileSync(join('hub/projects', f), join(root, 'home/hub/projects', f));
mkdirSync(join(root, 'home/news'));
copyFileSync('news/index.php', join(root, 'home/news/index.php'));
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
  await page.goto(base + '/hub/list.html');
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
  await page.waitForSelector('.tabs button', { timeout: 10000 });
  const tabs = await page.$$eval('.tabs button', (b) => b.map((x) => x.textContent));
  const map = await page.$$eval('.content .maptile .nm', (b) => b.map((x) => x.textContent));
  check(['Home', 'Queue', 'Sections', 'All'].every((t) => tabs.some((x) => x.startsWith(t))) && map.includes('Games') && map.includes('Performance'),
        `Home: the bottom tabs and a map of every section (${map.length})`);
  await wait(1200); await page.screenshot({ path: 'build/hub-home.png', fullPage: true });
  await page.click('.maptile:has(.nm:text-is("Games"))');
  // (Wait for the Games view itself: the previous view's items match too.)
  await page.waitForFunction(() => location.hash.endsWith('/s/games') && document.querySelector('.shead h2')?.textContent === 'Games' &&
                                   document.querySelectorAll('.content .item').length > 3,
                             null, { timeout: 5000 });
  const cards = await page.$$eval('.content .item', (c) => c.length);
  const strip = await page.$$eval('.nav button.on', (b) => b.map((x) => x.textContent));
  check(cards > 3 && page.url().endsWith('/s/games') && strip.some((t) => t.startsWith('Games')),
        `a section shows its cards, grouped by stage, with the sections strip (${cards} in Games)`);
  await wait(1200); await page.screenshot({ path: 'build/hub.png' });

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
  await wait(1100);  // "new from Claude" counts what comes after the page first loaded (seconds)
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
  await page.click('.tabs button:has-text("Home")');
  await page.waitForSelector('.content .item:has-text("Bowling lane") .tag.q', { timeout: 5000 });
  check(await page.$('.content .item:has-text("Bowling lane") .newdot') !== null, 'Home marks the card "new from Claude"');
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

  // Drag to reorder: the first card of a Games group, dragged below the next one.
  await page.goto(base + '/hub/list.html#/p/bar3d/s/games');
  await page.waitForFunction(() => [...document.querySelectorAll('.content .list')].some((l) => l.querySelectorAll(':scope > .item .grip').length > 1),
                             null, { timeout: 5000 });
  const ids = await page.evaluate(() => {
    const l = [...document.querySelectorAll('.content .list')].find((x) => x.querySelectorAll(':scope > .item .grip').length > 1);
    l.setAttribute('data-test', 'drag');
    return [...l.querySelectorAll(':scope > .item')].slice(0, 2).map((i) => i.getAttribute('data-id'));
  });
  const g0 = await page.$('[data-test=drag] > .item:first-child .grip');
  const box0 = await g0.boundingBox();
  const h1 = (await (await page.$('[data-test=drag] > .item:nth-child(2)')).boundingBox()).height;
  await page.mouse.move(box0.x + box0.width / 2, box0.y + box0.height / 2);
  await page.mouse.down();
  for (let k = 1; k <= 6; k++) await page.mouse.move(box0.x + box0.width / 2, box0.y + box0.height / 2 + (h1 * 1.3 * k) / 6);
  await page.mouse.up();
  await wait(900);
  const reordered = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards;
  const o = (id) => reordered.find((c) => c.id === id).order;
  check(o(ids[0]) > o(ids[1]) && page.url().endsWith('/s/games'), 'dragging a card by its grip reorders the section (and doesn\'t open it)');
  const placed = await post('/hub/hub.php', { a: 'place', id: ids[0], before: ids[1] }, { 'X-Hub-Key': key });
  const back = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards;
  check(placed.status === 403 && back.find((c) => c.id === ids[0]).order === o(ids[0]), 'only the owner reorders (Claude\'s key is refused)');

  // A second project, with its own board.
  await page.goto(base + '/hub/list.html#/');
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

  // ---- Hub v3: Home, the news, Explore (docs/LOOPED.md C) ----
  const spots = await post('/hub/hub.php', { a: 'spots' }, { 'X-Hub-Key': key });
  check(spots.ok === 1 && spots.spots.some((x) => x.id === 'jukebox') && spots.map.rooms.length >= 2 && spots.rules.length > 10,
        `the places, their rules and the bar's floor plan (${spots.spots.length} places)`);
  const pool = (await post('/hub/hub.php', { a: 'save', card: { section: 'games', title: 'Pool table', stage: 'idea' } }, { 'X-Hub-Key': key })).card;
  await post('/hub/hub.php', { a: 'report', id: pool.id, question: 'Pool table in the arcade or the lounge?', choices: ['Arcade', 'Lounge'] }, { 'X-Hub-Key': key });
  const jpg = 'data:image/jpeg;base64,' + readFileSync('hub/projects/bar3d.jpg').toString('base64');
  const shot = await post('/hub/hub.php', { a: 'shot', build: 'abc1234', name: 'door', image: jpg }, { 'X-Hub-Key': key });
  check(shot.ok === 1 && shot.file === 'abc1234-door.jpg', 'Claude uploads a screenshot');
  check((await post('/hub/hub.php', { a: 'shot', build: 'abc1234', name: 'bad', image: 'data:image/jpeg;base64,aGVsbG8=' }, { 'X-Hub-Key': key })).status === 400,
        'something that isn\'t a picture is refused');
  const news = await post('/hub/hub.php', { a: 'post', post: { build: 'abc1234', title: 'Neon lanes', text: 'Two bowling lanes in the arcade.', shots: [shot.file] } },
                          { 'X-Hub-Key': key });
  check(news.ok === 1 && news.post.shots[0].url.includes('abc1234-door.jpg') && !news.post.published, 'Claude writes a news post (private until published)');
  check((await post('/hub/hub.php', { a: 'publish', id: news.post.id, on: 1 }, { 'X-Hub-Key': key })).status === 403, 'only the owner publishes');
  check((await fetch(base + '/hub/hub.php?shot=bar3d/abc1234-door.jpg')).status === 403, 'a screenshot needs a login');

  // Home: the question as a bubble; one tap answers it.
  await page.goto(base + '/hub/');
  await page.waitForSelector('.bubble:has-text("Pool table in the arcade")', { timeout: 10000 });
  await wait(1200);
  await page.screenshot({ path: 'build/hub3-home.png' });
  await page.click('.bubble:has-text("Pool table in the arcade") .chip:has-text("Lounge")');
  await wait(900);
  const ans = (await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === pool.id);
  check(!!ans && ans.question === '' && ans.choices.length === 0 && ans.thread.at(-1).text === 'Lounge', 'one tap answers Claude\'s question (in Claude\'s inbox)');
  // The news feed: the post with its picture (the login cookie lets it load).
  await page.waitForSelector('.post h3:has-text("Neon lanes")', { timeout: 5000 });
  await page.waitForFunction(() => { const i = document.querySelector('.post img'); return i && i.complete && i.naturalWidth > 0; }, null, { timeout: 5000 })
    .catch(() => {});
  check(await page.$eval('.post img', (i) => i.naturalWidth > 0), 'the news feed shows the post with its screenshot');
  check((await fetch(base + '/news/?json').then((r) => r.json())).posts.length === 0, 'nothing on the public board before Publish');
  await page.click('.post .switch');
  await wait(800);
  const pub = (await fetch(base + '/news/?json').then((r) => r.json())).posts;
  check(pub.length === 1 && pub[0].title === 'Neon lanes', 'Publish puts the post on the public news board');
  check((await fetch(base + '/news/' + pub[0].shots[0])).status === 200 && (await fetch(base + '/news/?shot=bar3d/abc1234-other.jpg')).status === 404,
        'the board shows the published post\'s picture, nothing else');
  const newsPage = await browser.newPage({ viewport: { width: 430, height: 932 } });
  await newsPage.goto(base + '/news/');
  check((await newsPage.textContent('main')).includes('Neon lanes'), 'the public page lists it');
  await wait(900);
  await newsPage.screenshot({ path: 'build/news.png' });
  await newsPage.close();

  // Explore: the bar's map; a place opens with its things.
  await post('/hub/hub.php', { a: 'save', card: { section: 'bar', title: 'Jukebox glow', stage: 'ready' } }, { 'X-Hub-Key': key });
  await page.reload();
  await page.waitForSelector('.hello', { timeout: 5000 });
  await page.click('nav [data-tab=explore]');
  await page.waitForSelector('svg.map .spot', { timeout: 5000 });
  const places = await page.$$eval('svg.map .spot', (g) => g.map((x) => x.getAttribute('aria-label')));
  check(places.includes('Jukebox') && places.includes('Arcade') && places.length >= 12, `the map shows the places (${places.length})`);
  await wait(900);
  await page.screenshot({ path: 'build/hub3-explore.png', fullPage: true });
  await page.click('svg.map .spot[aria-label=Jukebox]');
  await page.waitForSelector('.sheet.on .sheetHead h3:has-text("Jukebox")', { timeout: 5000 });
  check((await page.textContent('.sheet')).includes('Jukebox glow'), 'the Jukebox opens with its things (from the rules)');
  await wait(600);
  await page.screenshot({ path: 'build/hub3-spot.png' });
  await page.fill('.sheet .row input', 'A neon sign over the jukebox');
  await page.click('.sheet .row button:has-text("Add")');
  await wait(900);
  const idea = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.find((c) => c.title === 'A neon sign over the jukebox');
  check(!!idea && idea.spot === 'jukebox' && idea.stage === 'idea', 'a new idea lands on the place');
  await page.fill('.sheet textarea', 'Make it louder at night');
  await page.click('.sheet button:has-text("Send")');
  await wait(900);
  const spotInbox = (await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key })).spots || [];
  check(spotInbox.some((x) => x.id === 'jukebox' && x.thread.at(-1).text === 'Make it louder at night'), 'a message about a place reaches Claude\'s inbox');
  const reply = await post('/hub/hub.php', { a: 'spotsay', id: 'jukebox', text: 'Noted: louder after 9 pm.' }, { 'X-Hub-Key': key });
  check(reply.ok === 1 && !reply.spot.unread, 'Claude answers on the place');
  await page.goto(base + '/hub/#/explore/engine');
  await page.waitForSelector('svg.circuit .spot', { timeout: 5000 });
  const engine = await page.$$eval('svg.circuit .spot', (g) => g.map((x) => x.getAttribute('aria-label')));
  check(engine.includes('Loops') && engine.includes('Light & drawing'), `under the floor: Looped's parts (${engine.join(', ')})`);
  await wait(900);
  await page.screenshot({ path: 'build/hub3-engine.png' });

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
