// Dev (dev/, PLAN.md R4) at phone width on a throwaway webspace (PHP's server:
// ui/, dev/, the bar's account.php): a stranger gets the login, a player is
// told Dev is for admins, the owner gets the five tabs, every tab draws its
// screen file. Later pieces add their checks below (J90: the nested pages,
// the trail and Back, the Live panel and its reactions, Find, Calm). No
// game: seconds.
// Usage: NODE_PATH=$(npm root -g) node tools/dev_test.mjs
import { launchBrowser } from './lib/browser.mjs';
import { webspace } from './lib/webspace.mjs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
import { mkdirSync, writeFileSync, copyFileSync } from 'node:fs';
import { join } from 'node:path';
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
// The job in Now, whatever it is today (the test must not go stale when the plan moves on).
const PLAN = JSON.parse(readFileSync('docs/plan/plan.json', 'utf8'));
const NOW = [null, PLAN.now.id, PLAN.now.title];
const NOW_PIECES = PLAN.now.pieces.length;
const ws = await webspace({ dirs: { ui: 'ui', dev: 'dev', toolbelt: 'toolbelt', hub: 'hub', studio: 'studio', site: 'homepage/site' }, files: { 'bar/account.php': 'server/account.php' } });
// What CI and the deploy put there: the game's config, the plan, no deploy report yet.
writeFileSync(join(ws.root, 'home/bar/config.json'), JSON.stringify({ build: 'abc1234', ws: '' }));
mkdirSync(join(ws.root, 'private/dev'), { recursive: true });
copyFileSync('docs/plan/plan.json', join(ws.root, 'private/dev/plan.json'));
copyFileSync('docs/charter/rules.json', join(ws.root, 'private/dev/rules.json'));
const META_AT = Math.floor(Date.now() / 1000) - 120;  // the plan came two minutes ago
writeFileSync(join(ws.root, 'private/dev/plan-meta.json'), JSON.stringify({ sha: 'fed4321', at: META_AT }));  // plan.yml (J5.2)
// plan.yml's other files (J5.6): an incident today, a question only the user can settle, the job's spec.
const today = new Date().toISOString().slice(0, 10);
writeFileSync(join(ws.root, 'private/dev/incidents.json'), JSON.stringify([{ file: `${today}-x.md`, date: today, title: 'A test incident', status: 'closed', errors: 2 }]));
writeFileSync(join(ws.root, 'private/dev/questions.json'), JSON.stringify([{ id: 'KI-99', text: 'Pick a colour for the test.' }]));
writeFileSync(join(ws.root, 'private/dev/specs.json'), JSON.stringify({ [PLAN.now.id]: `### ${PLAN.now.id} · ${PLAN.now.title}\n\nThe test spec's words.` }));
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
mkdirSync('build', { recursive: true });  // the pictures: build/dev-overview.png, build/dev-plan.png
// The pictures are for looking at, not checks: a slow one (GitHub's machines draw
// in software) prints a note instead of failing the deploy (as in tools/site_test.mjs).
// The pictures hold the motion still and leave out the frosted glass (the
// picture is of the layout), and get 45 s: GitHub's software drawing prepares
// each new effect once, and the first picture pays for all of them (J90).
const STILL = '*, *::before, *::after { animation: none !important; transition: none !important; backdrop-filter: none !important; -webkit-backdrop-filter: none !important; }';
const shot = async (opts) => {
  const still = await page.addStyleTag({ content: STILL });
  await page.waitForTimeout(300);
  await page.screenshot({ animations: 'disabled', timeout: 45000, ...opts })
    .catch((e) => console.log(`note ${opts.path}: no picture (${String(e.message).split('\n')[0]})`));
  await still.evaluate((el) => el.remove());
};

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
await page.waitForFunction((t) => document.querySelector('#view [data-id="now"]').textContent.includes(t), `${NOW[1]} · ${NOW[2]}`);
check(await ed((n) => document.querySelector('#view [data-id="now"]').textContent.includes(` of ${n}`), NOW_PIECES), `Now shows ${NOW[1]} and how many of its ${NOW_PIECES} pieces are done`);
await page.waitForFunction(() => /Nothing waiting|to publish/.test(document.querySelector('#view [data-id="needs"]').textContent));
check(true, 'Needs you answers (news drafts)');
await page.waitForFunction(() => /Your call: KI-99/.test(document.querySelector('#view [data-id="needs"]').textContent));
check(await ed(() => /Pick a colour for the test/.test(document.querySelector('#view [data-id="needs"]').textContent)), 'Needs you lists the questions only you can settle (J5.6)');
await page.waitForFunction(() => /Errors this week/.test(document.querySelector('#view [data-id="status"]').textContent) && /A test incident/.test(document.querySelector('#view [data-id="status"]').textContent));
check(await ed(() => /Errors this week\s*Latest: A test incident \(closed\)\s*2/.test(document.querySelector('#view [data-id="status"]').textContent)), 'Overview counts this week\'s errors from the incidents (J5.6)');
await shot({ path: 'build/dev-overview.png', fullPage: true });
const ci = await ws.post('dev/api.php', { a: 'ci', token: owner.token });
check(ci.ok === 1 && ci.ci && ci.ci.game === null && ci.ci.site === null, 'the store answers the owner (no deploy reports yet)');
check((await ws.post('dev/api.php', { a: 'ci', token: player.token })).status === 403, 'the store keeps a player out');
// J2.3: each deploy's real state (tools/ci_status.mjs writes these at deploy)
const at = Math.floor(Date.now() / 1000);
writeFileSync(join(ws.root, 'private/dev/ci-game.json'), JSON.stringify({ workflow: 'game', build: 'abc1234', live: true, tests: 'failed', failed: ['smoke'], allowed: ['studio'], at: at - 120, url: 'https://github.com/' }));
writeFileSync(join(ws.root, 'private/dev/ci-site.json'), JSON.stringify({ workflow: 'site', build: 'abc1234', live: true, tests: 'passed', failed: [], allowed: [], at: at - 60, url: 'https://github.com/' }));
await page.click('#tabs [data-tab="projects"]');
await page.click('#tabs [data-tab="overview"]');
await page.waitForFunction(() => /1 failed: smoke/.test(document.querySelector('#view [data-id="status"]')?.textContent || ''));
check(await ed(() => { const t = document.querySelector('#view [data-id="status"]').textContent; return /Game deploy/.test(t) && /allowed to fail: studio/.test(t) && /Site deploy/.test(t) && /every test passed/.test(t); }),
  'Overview shows both deploys: the game\'s failed test and the one allowed to fail by name, the site green');
// J90: the Live panel: a tile per answer, coloured by it; tap one for its details and the reactions that go to Claude.
check(await ed(() => ['bar', 'room', 'ci-game', 'ci-site', 'errors', 'latest'].every((k) => document.querySelector(`#view [data-id="status"] [data-tile="${k}"]`))), 'the Live panel has its six tiles (J90)');
check(await ed(() => document.querySelector('#view [data-tile="ci-game"]').dataset.tone === 'bad' && document.querySelector('#view [data-tile="ci-site"]').dataset.tone === 'ok'), 'the red deploy\'s tile is red, the green one green (J90)');
await page.click('#view [data-tile="ci-game"]');
await page.waitForSelector('.dev-sheet.open [data-react="Broke it"]');
check(await ed(() => /Game deploy/.test(document.querySelector('.dev-sheet').textContent) && /1 failed: smoke/.test(document.querySelector('.dev-sheet').textContent)), 'tapping a tile opens its details (J90)');
await page.fill('.dev-sheet input', 'the smoke test again');
await page.click('.dev-sheet [data-react="Broke it"]');
await page.waitForFunction(() => !document.querySelector('.dev-sheet'));
const reacted = await ws.post('dev/api.php', { a: 'notes', token: owner.token });
check(reacted.ok && reacted.notes.length === 1 && reacted.notes[0].kind === 'note' && /^Broke it: game deploy, build abc1234/.test(reacted.notes[0].text) && /the smoke test again/.test(reacted.notes[0].text) && reacted.notes[0].read === 0,
  'Broke it sends Claude a note naming the tile, with your words, waiting (J90)');
await page.click('#view [data-tile="bar"]');
await page.waitForSelector('.dev-sheet.open [data-react="Note"]');
await page.click('.dev-sheet [data-react="Note"]');
await page.waitForTimeout(300);
check((await ws.post('dev/api.php', { a: 'notes', token: owner.token })).notes.length === 1 && await ed(() => !!document.querySelector('.dev-sheet')), 'Note with nothing written sends nothing, and waits for the words (J90)');
await page.click('.dev-sheet >> text=Close');
await page.waitForFunction(() => !document.querySelector('.dev-sheet'));

// R4.4: Plan
await page.click('#tabs [data-tab="plan"]');
await page.waitForSelector('#view [data-chapter="The engine"]');
check(await ed(() => ['The world', 'The site', 'The engine', 'The tools', 'The backbone'].every((c) => document.querySelector(`#view [data-chapter="${c}"]`))), 'Plan shows the five chapters');
check(await ed((t) => document.querySelector('#view [data-id="plan-now"]').textContent.includes(t + '.1'), NOW[1]), `Now lists ${NOW[1]}'s pieces`);
// J7.1: every road item on screen shows its verdict (the short word, the reason), and its old tag.
check(await ed(() => { const rows = [...document.querySelectorAll('#view [data-chapter] [data-tag]')]; return rows.length > 0 && rows.every((r) => r.dataset.verdict && r.querySelector('.ls-lr-value')?.textContent && / — |: /.test(r.querySelector('.ls-lr-detail')?.textContent || '')); }), 'each road item shows its verdict and why (J7.1)');
check(await ed(() => [...document.querySelectorAll('#view [data-chapter] [data-formerly]')].some((r) => /Formerly /.test(r.textContent))), 'a road item shows its old tag (Formerly …)');
check(await ed(() => [...document.querySelectorAll('#view [data-chapter] [data-parked]')].every((r) => r.dataset.verdict)), 'each parked idea on screen shows its verdict');
// How long ago, in minutes, as it was when the tab was drawn (a few seconds back) or now: a slow machine reaches here later than two minutes.
const agoNow = [0, 15].map((back) => Math.round((Date.now() / 1000 - back - META_AT) / 60) + ' min ago');
check(await ed((ago) => ago.some((a) => document.querySelector('#view [data-id="plan-now"]').textContent.includes('As of build fed4321, ' + a)), agoNow), 'Plan says which build it came from and how long ago (J5.2)');
await shot({ path: 'build/dev-plan.png' });
// J90: the plan nests. The job in Now opens its own page (its spec, its pieces); the trail says where you are; Back comes back.
await page.click('#view [data-id="plan-now"] .dev-row-btn');
await page.waitForSelector('#view [data-spec]');
check(await ed(() => /The test spec's words/.test(document.querySelector('#view [data-spec]').textContent)), 'tapping the job in Now opens its page, with its spec (J5.6, J90)');
check(await ed((id) => { const t = document.getElementById('trail').textContent; return location.hash === '#plan/' + id && /Dev/.test(t) && /Plan/.test(t) && t.includes(id) && !document.getElementById('back').hidden; }, NOW[1]),
  'the job has its own address; the trail says Dev, Plan, the job; Back shows (J90)');
check(await ed((id) => document.getElementById('view').textContent.includes(id + '.1') && !!document.querySelector('#view .dev-ring'), NOW[1]), 'the job\'s page lists its pieces, with a ring of how many are done (J90)');
await page.click('#back');
await page.waitForSelector('#view [data-id="note-kind"]');
check(await ed(() => location.hash === '#plan' && document.getElementById('back').hidden && document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === 'plan'), 'Back comes back to Plan (J90)');
// A chapter's card shows a few of its jobs; its page shows all of them.
const ENGINE_N = PLAN.road.engine.length;
await page.click('#view [data-chapter="The engine"] [data-go="plan/engine"]');
await page.waitForFunction((n) => location.hash === '#plan/engine' && document.getElementById('view').textContent.includes(n + ' on the road'), ENGINE_N);
check(await ed((n) => document.querySelectorAll('#view [data-chapter="The engine"] [data-tag]').length === n && document.getElementById('view').textContent.includes(n + ' on the road'), ENGINE_N),
  `the engine's page lists all ${ENGINE_N} of its jobs (J90)`);
check(await ed(() => { const rows = [...document.querySelectorAll('#view [data-chapter] [data-tag]')]; return rows.length > 0 && rows.every((r) => r.dataset.verdict && r.querySelector('.ls-lr-value')?.textContent && / — |: /.test(r.querySelector('.ls-lr-detail')?.textContent || '')); }), 'there too, each job shows its verdict and why (J7.1)');
// A job that waits for another says so, and the other is a tap away.
const WAITS = Object.values(PLAN.road).flat().find((x) => x.verdict && x.verdict.kind === 'after' && /J\d+/.test(x.verdict.after || ''));
if (WAITS) {
  const first = WAITS.verdict.after.match(/J\d+/)[0];
  await page.evaluate((id) => { location.hash = 'plan/' + id; }, WAITS.id);
  await page.waitForFunction((id) => location.hash === '#plan/' + id && /Waits for/.test(document.getElementById('view').textContent), WAITS.id);
  check(await ed((f) => !!document.querySelector(`#view .dev-chips [data-go="plan/${f}"]`), first), `${WAITS.id}'s page says it waits for ${first}, a tap away (J90)`);
}
const nested = await ed(() => location.hash);
await page.reload();
await page.waitForSelector('#tabs:not([hidden]) button');
await page.waitForFunction(() => document.querySelector('#view .dev-card'));
check(await ed((h) => location.hash === h && /^#plan\//.test(h) && document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === 'plan' && document.getElementById('trail').textContent.includes(h.split('/').pop() === 'engine' ? 'The engine' : h.split('/').pop()), nested),
  'a reload stays on the nested page, its tab lit, its trail drawn (J90)');
await page.click('#tabs [data-tab="plan"]');
await page.waitForSelector('#view [data-id="note-kind"]');
await page.click('#view [data-id="note-kind"] button:nth-child(2)');
await page.fill('#view [data-id="note"]', 'A rooftop terrace');
await page.click('#view [data-id="note-send"]');
await page.waitForFunction(() => /A rooftop terrace/.test(document.querySelector('#view [data-id="notes"]')?.textContent || ''));
const notes = await ws.post('dev/api.php', { a: 'notes', token: owner.token });
check(notes.ok && notes.notes[0].kind === 'park' && notes.notes[0].text === 'A rooftop terrace' && notes.notes[0].read === 0, 'Park an idea sends it to Claude, waiting');
check((await ws.post('dev/api.php', { a: 'read', ids: [notes.notes[0].id], token: owner.token })).status === 403, 'only Claude marks notes as read');

// R4.5: Publish
const hubKey = 'c'.repeat(48);
mkdirSync(join(ws.root, 'private/hub'), { recursive: true });
writeFileSync(join(ws.root, 'private/hub/key.json'), JSON.stringify({ hash: createHash('sha256').update(hubKey).digest('hex') }));
const made = await ws.post('hub/hub.php', { a: 'post', post: { build: 'abc1234', title: 'R4: Dev', text: 'The control room is open.' } }, { 'X-Hub-Key': hubKey });
check(made.ok === 1, 'Claude writes a news post (the hub key)');
await page.click('#tabs [data-tab="publish"]');
await page.waitForSelector(`#view [data-post="${made.post.id}"] [data-act="publish"]`);
await page.click(`#view [data-post="${made.post.id}"] [data-act="publish"]`);
await page.waitForFunction((id) => /Unpublish/.test(document.querySelector(`#view [data-post="${id}"] [data-act="publish"]`)?.textContent || ''), made.post.id);
check((await ws.post('hub/hub.php', { a: 'posts', token: owner.token })).posts[0].published === true, 'Publish puts it on the public news');
// R5.2: highlights (made, previewed, pushed: one on the front page at a time)
await page.click('#view [data-id="pub-tab"] button:nth-child(2)');
await page.click('#view [data-act="new-hl"]');
await page.fill('#view [data-field="title"]', 'Live music on Fridays');
await page.fill('#view [data-field="text"]', 'Pull up a stool: the band starts at nine.');
await page.click('#view [data-pic="/site/img/stage.jpg"]');
check(await ed(() => /Live music on Fridays/.test(document.querySelector('#view .dev-hl').textContent) && /stage\.jpg/.test(document.querySelector('#view .dev-hl').style.backgroundImage)), 'the preview shows the title and the picture as the front page will');
await page.click('#view [data-act="save-hl"]');
await page.waitForSelector('#view [data-highlight] [data-act="push"]');
check((await (await fetch(ws.url + 'dev/api.php?highlight')).json()).highlight === null, 'saved, but not on the front page until pushed');
await page.click('#view [data-highlight] [data-act="push"]');
await page.waitForFunction(() => /On the front page/.test(document.querySelector('#view [data-highlight]')?.textContent || ''));
const hl = (await (await fetch(ws.url + 'dev/api.php?highlight')).json()).highlight;
check(hl && hl.title === 'Live music on Fridays' && hl.img === '/site/img/stage.jpg' && hl.link === '/bar/', 'Push puts it on the front page (public)');
check((await ws.post('dev/api.php', { a: 'push', id: hl.id, on: 0 }, { 'X-Hub-Key': hubKey })).status === 403, 'Claude can\'t push highlights');

// The saved words answer slowly (as on GitHub's machines, or a slow phone): what you type first must survive (J5.7).
await page.route(/\/dev\/api\.php\?site$/, async (r) => { await new Promise((s) => setTimeout(s, 1500)); await r.continue(); });
await page.click('#view [data-id="pub-tab"] button:nth-child(3)');
await page.waitForSelector('#view [data-field="headline"]');
await page.fill('#view [data-field="headline"]', 'The lights are on.');
await page.waitForTimeout(2000);  // the slow answer has come
await page.click('#view >> text=Save the words');
await page.waitForFunction(() => /Saved just now/.test(document.querySelector('#view [data-id="posts"]').textContent));
const site = await (await fetch(ws.url + 'dev/api.php?site')).json();
check(site.site.headline === 'The lights are on.' && site.site.by === 'Josh', 'the site\'s words are saved, and public for the front page');

// R4.6: People
await page.click('#tabs [data-tab="people"]');
await page.waitForSelector('#view [data-user="mia"]');
check(await ed(() => /Owner/.test(document.querySelector('#view [data-user="josh"]').textContent) && /Waiting/.test(document.querySelector('#view [data-user="mia"]').textContent)), 'People lists the owner and a waiting player');
await page.click('#view [data-user="mia"] >> text=Let in');
await page.waitForFunction(() => /Player/.test(document.querySelector('#view [data-user="mia"]')?.textContent || ''));
await page.click('#view [data-user="mia"] >> text=Make admin');
await page.waitForFunction(() => /Admin/.test(document.querySelector('#view [data-user="mia"]')?.textContent || ''));
const people = await ws.post('bar/account.php', { a: 'people', token: owner.token });
check(people.people.find((p) => p.user === 'mia').status === 'admin', 'Let in, then Make admin: Mia is an admin');

// J83: Claude's key, made from People (owner only): it asks first, shows the
// key once, and the hub takes that key.
await page.waitForSelector('#view .dev-claude-key');
page.once('dialog', (d) => d.accept());
await page.click('#view .dev-claude-key >> text=Make a new Claude key');
await page.waitForSelector('#view .dev-key-shown input');
const madeKey = await ed(() => document.querySelector('#view .dev-key-shown input').value);
check(/^[0-9a-f]{48}$/.test(madeKey), 'Make a new Claude key: the key shows once, after a yes');
const withKey = await (await fetch(ws.url + 'hub/hub.php', { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': madeKey }, body: JSON.stringify({ a: 'posts' }) })).json();
check(withKey.ok === 1, 'the hub takes the new key');

// R4.2: Projects
await page.click('#tabs [data-tab="projects"]');
await page.waitForSelector('#view [data-project="bar3d"] a[href="/studio/"]');
check(await ed(() => ['bar3d', 'looped', 'script', 'workshop'].every((p) => document.querySelector(`#view [data-project="${p}"]`))), 'Projects lists Bar3D, Looped, the script language and the workshop');
check(await ed(() => !!document.querySelector('#view [data-project="looped"] a[href="/toolbelt/ui-editor/"]')), 'the UI Editor sits under Looped');
check(await ed(() => !!document.querySelector('#view [data-project="bar3d"] .dev-ask input')), 'the perf report asks for its code');
// J90: a project's own page: its tools by group; the trail and Back.
await page.click('#view [data-project="looped"] .key[data-go="projects/looped"]');
await page.waitForSelector('#view [data-group] a[href="/toolbelt/ui-editor/"]');
check(await ed(() => location.hash === '#projects/looped' && /Looped/.test(document.getElementById('trail').textContent) && !document.getElementById('back').hidden), 'Open Looped: its own page, its tools by group, the trail says Projects, Looped (J90)');
await page.click('#back');
await page.waitForSelector('#view [data-project="looped"]');
check(await ed(() => location.hash === '#projects'), 'Back comes back to Projects (J90)');
// J90: Find: one field over every page, tool and job; Enter opens the first answer.
await page.click('#find');
await page.waitForSelector('.dev-find.open input');
await page.fill('.dev-find input', 'ui editor');
await page.waitForFunction(() => document.querySelector('.dev-find-list a')?.dataset.find === '/toolbelt/ui-editor/', null, { timeout: 10000 }).catch(() => {});
check(await ed(() => document.querySelector('.dev-find-list a')?.dataset.find === '/toolbelt/ui-editor/'), 'Find: "ui editor" puts the UI Editor first (J90)');
const JOB = PLAN.road.engine[0];
await page.fill('.dev-find input', JOB.id.toLowerCase());
await page.waitForFunction((id) => document.querySelector('.dev-find-list a')?.dataset.find === 'plan/' + id, JOB.id, { timeout: 10000 }).catch(() => {});
check(await ed((id) => document.querySelector('.dev-find-list a')?.dataset.find === 'plan/' + id, JOB.id), `Find: "${JOB.id.toLowerCase()}" puts the job ${JOB.id} first (J90)`);
await page.press('.dev-find input', 'Enter');
await page.waitForFunction((id) => location.hash === '#plan/' + id && !document.querySelector('.dev-find'), JOB.id);
check(await ed(() => document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === 'plan' && !!document.querySelector('#view .dev-card')), `Enter opens ${JOB.id}'s page under Plan (J90)`);
// J90: Calm stops the motion (and the drifting glow), and stays on after a reload.
check(await ed(() => !document.documentElement.classList.contains('calm') && getComputedStyle(document.querySelector('.dev-sky i')).animationName !== 'none'), 'the motion is on at first (J90)');
await page.click('#calm');
check(await ed(() => document.documentElement.classList.contains('calm') && document.getElementById('calm').getAttribute('aria-pressed') === 'true' && getComputedStyle(document.querySelector('.dev-sky')).display === 'none' && getComputedStyle(document.querySelector('#view .dev-card')).animationName === 'none'),
  'Calm turns the motion off (J90)');
await page.click('#tabs [data-tab="people"]');
await page.waitForFunction(() => document.querySelector('#tabs [aria-current="page"]')?.dataset.tab === 'people');

await page.reload();
await page.waitForSelector('#tabs:not([hidden]) button');
check(await ed(() => location.hash === '#people' && document.querySelector('#tabs [aria-current="page"]').dataset.tab === 'people'), 'a reload stays logged in, on the same tab');
check(await ed(() => document.documentElement.classList.contains('calm')), 'Calm stays on after a reload (J90)');
await page.click('#calm');
check(await ed(() => !document.documentElement.classList.contains('calm') && getComputedStyle(document.querySelector('.dev-sky')).display !== 'none'), 'tapped again, the motion is back (J90)');

// R4.7: the old doors lead to Dev
await page.goto(ws.url + 'hub/');
await page.waitForURL(/\/dev\/#?/);
check(true, '/hub/ opens Dev');
await page.goto(ws.url + 'toolbelt/');
await page.waitForURL(/\/dev\/#projects/);
await page.waitForSelector('#view [data-project="looped"]');
check(true, '/toolbelt/ opens Dev → Projects');

check(errors.length === 0, 'no errors' + (errors.length ? ': ' + errors.join(' | ') : ''));
await browser.close(); ws.close();
console.log(failed ? `DEV TEST: ${failed} FAILED` : 'DEV TEST PASSED');
process.exit(failed ? 1 : 0);
