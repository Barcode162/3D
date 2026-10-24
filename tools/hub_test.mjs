// The project hub (hub/): its page and hub.php with the bar's account.php,
// on PHP's built-in server, laid out like the IONOS webspace
// (<root>/home/bar, <root>/home/hub, <root>/private). The owner logs in on the
// page, adds a card, makes a Claude key; "Claude" reads the queue and reports;
// someone else is kept out; passwords change and a recovery code resets one.
// The old board pages are gone (Dev is the control room, PLAN.md R6.6): the
// owner's side runs on hub.php directly. The news post goes public with
// Publish (news/), a message about a place lands there, /hub/ opens Dev,
// the World page lists the bar's kinds.
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
for (const f of ['index.html', 'world.html', 'hub.php', 'seed.json']) copyFileSync(join('hub', f), join(root, 'home/hub', f));
mkdirSync(join(root, 'home/hub/projects'));
for (const f of ['bar3d.jpg', 'bar3d-spots.json', 'bar3d-map.json']) copyFileSync(join('hub/projects', f), join(root, 'home/hub/projects', f));
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

  // The hub's old board pages are gone (Dev is the control room, PLAN.md R6.6);
  // hub.php stays: Dev, the tools and Claude talk to it.
  const ownerToken = owner.token;
  const as = (body) => post('/hub/hub.php', { ...body, token: ownerToken });
  const projects = await as({ a: 'projects' });
  check(projects.ok === 1 && projects.projects.some((p) => p.id === 'bar3d'), 'the projects list has Bar3D');
  const board0 = await as({ a: 'board' });
  check(board0.ok === 1 && board0.sections.some((s) => s.id === 'games') && board0.sections.some((s) => s.id === 'perf'),
        `the board has every section (${board0.sections.length})`);

  // A new card in Games, straight to Claude's queue.
  const made = await as({ a: 'save', card: { section: 'games', title: 'Bowling lane in the arcade', notes: 'Two lanes, real pins with physics.', stage: 'ready' } });
  check(made.ok === 1 && made.card.title === 'Bowling lane in the arcade', 'the owner adds a card');

  const key = (await as({ a: 'key' })).key;
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

  // The owner answers in plain words.
  await as({ a: 'say', id: card.id, text: 'Neon!' });
  const inbox = await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key });
  const after = inbox.cards.find((c) => c.id === card.id);
  check(!!after && after.question === '' && after.thread.at(-1).kind === 'answer' && after.thread.at(-1).text === 'Neon!' && !after.plan.includes('Neon'),
        'the answer goes on the conversation, the question clears, and the card is in Claude\'s inbox');
  await as({ a: 'save', card: { id: card.id, stage: 'done', notes: 'Two lanes, neon, a scoreboard.' } });
  const notes = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === card.id);
  const claudeNotes = await post('/hub/hub.php', { a: 'save', card: { id: card.id, notes: 'overwritten' } }, { 'X-Hub-Key': key });
  check(notes.stage === 'done' && notes.notes === 'Two lanes, neon, a scoreboard.' && claudeNotes.card.notes === notes.notes,
        'the owner\'s words save, and Claude can\'t overwrite them');

  // Reordering: only the owner.
  const games = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.filter((c) => c.section === 'games');
  const placed = await post('/hub/hub.php', { a: 'place', id: games[1].id, before: games[0].id }, { 'X-Hub-Key': key });
  check(placed.status === 403, 'only the owner reorders (Claude\'s key is refused)');
  const second = games[1];
  await as({ a: 'move', id: second.id, dir: -1 });
  const moved = (await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key })).cards.filter((c) => c.section === 'games');
  check(moved[0].id === second.id, 'moving a card up reorders its section');

  // A second project, with its own board.
  const kidsMade = await as({ a: 'project', project: { name: 'Kids Game' } });
  const kids = await post('/hub/hub.php', { a: 'board', project: 'kids-game' }, { 'X-Hub-Key': key });
  const kidsAdd = await post('/hub/hub.php', { a: 'save', project: 'kids-game', card: { section: 'features', title: 'Jump' } }, { 'X-Hub-Key': key });
  const barStill = await post('/hub/hub.php', { a: 'board' }, { 'X-Hub-Key': key });
  check(kidsMade.ok === 1 && kids.ok === 1 && kids.sections.some((s) => s.id === 'features') && kids.cards.length === 0 && kidsAdd.ok === 1 &&
        !barStill.cards.some((c) => c.title === 'Jump'), 'a new project gets its own empty board');
  check((await post('/hub/hub.php', { a: 'board', project: 'nope' }, { 'X-Hub-Key': key })).status === 404, 'an unknown project is refused');

  const stranger = await post('/hub/hub.php', { a: 'board', token: other.token });
  check(stranger.status === 403 && /private/.test(stranger.error), 'someone else is kept out ("This hub is private.")');

  // Places, questions, the news (docs/LOOPED.md C).
  const spots = await post('/hub/hub.php', { a: 'spots' }, { 'X-Hub-Key': key });
  check(spots.ok === 1 && spots.spots.some((x) => x.id === 'jukebox') && spots.map.rooms.length >= 2 && spots.rules.length > 10,
        `the places, their rules and the bar's floor plan (${spots.spots.length} places)`);
  const pool = (await post('/hub/hub.php', { a: 'save', card: { section: 'games', title: 'Pool table', stage: 'idea' } }, { 'X-Hub-Key': key })).card;
  await post('/hub/hub.php', { a: 'report', id: pool.id, question: 'Pool table in the arcade or the lounge?', choices: ['Arcade', 'Lounge'] }, { 'X-Hub-Key': key });
  await as({ a: 'say', id: pool.id, text: 'Lounge' });
  const ans = (await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key })).cards.find((c) => c.id === pool.id);
  check(!!ans && ans.question === '' && ans.choices.length === 0 && ans.thread.at(-1).text === 'Lounge', 'the owner answers Claude\'s question (in Claude\'s inbox)');
  const jpg = 'data:image/jpeg;base64,' + readFileSync('hub/projects/bar3d.jpg').toString('base64');
  const shot = await post('/hub/hub.php', { a: 'shot', build: 'abc1234', name: 'door', image: jpg }, { 'X-Hub-Key': key });
  check(shot.ok === 1 && shot.file === 'abc1234-door.jpg', 'Claude uploads a screenshot');
  check((await post('/hub/hub.php', { a: 'shot', build: 'abc1234', name: 'bad', image: 'data:image/jpeg;base64,aGVsbG8=' }, { 'X-Hub-Key': key })).status === 400,
        'something that isn\'t a picture is refused');
  const news = await post('/hub/hub.php', { a: 'post', post: { build: 'abc1234', title: 'Neon lanes', text: 'Two bowling lanes in the arcade.', shots: [shot.file] } },
                          { 'X-Hub-Key': key });
  check(news.ok === 1 && news.post.shots[0].url.includes('abc1234-door.jpg') && !news.post.published, 'Claude writes a news post (private until published)');
  check((await post('/hub/hub.php', { a: 'publish', id: news.post.id, on: 1 }, { 'X-Hub-Key': key })).status === 403, 'only the owner publishes');
  const status = async (url) => { const r = await fetch(url); await r.arrayBuffer(); return r.status; };  // always read the body (undici)
  check((await status(base + '/hub/hub.php?shot=bar3d/abc1234-door.jpg')) === 403, 'a screenshot needs a login');
  check((await fetch(base + '/news/?json').then((r) => r.json())).posts.length === 0, 'nothing on the public board before Publish');
  check((await as({ a: 'publish', id: news.post.id, on: 1 })).ok === 1, 'the owner publishes');
  const pub = (await fetch(base + '/news/?json').then((r) => r.json())).posts;
  check(pub.length === 1 && pub[0].title === 'Neon lanes', 'Publish puts the post on the public news board');
  check((await status(base + '/news/' + pub[0].shots[0])) === 200 && (await status(base + '/news/?shot=bar3d/abc1234-other.jpg')) === 404,
        'the board shows the published post\'s picture, nothing else');

  // A message about a place reaches Claude, and Claude answers there.
  await as({ a: 'spotsay', id: 'jukebox', text: 'Make it louder at night' });
  const spotInbox = (await post('/hub/hub.php', { a: 'inbox' }, { 'X-Hub-Key': key })).spots || [];
  check(spotInbox.some((x) => x.id === 'jukebox' && x.thread.at(-1).text === 'Make it louder at night'), 'a message about a place reaches Claude\'s inbox');
  const reply = await post('/hub/hub.php', { a: 'spotsay', id: 'jukebox', text: 'Noted: louder after 9 pm.' }, { 'X-Hub-Key': key });
  check(reply.ok === 1 && !reply.spot.unread, 'Claude answers on the place');

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
