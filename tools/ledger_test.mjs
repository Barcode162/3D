// The Ledger (cloud/ledger.js): money and owned things, on a local
// `wrangler dev` (needs `npm ci --prefix cloud`). Checks: only servers change
// money, nothing is counted twice, balances never go below zero, the daily
// earning cap, numbered items.
//   node tools/ledger_test.mjs
import { spawn } from 'node:child_process';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const PORT = 8791;
const CAP = 1000;  // cents a day, small for the test
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };
let server = null, stateDir = null;
const base = `http://127.0.0.1:${PORT}`;

async function startServer() {
  stateDir = mkdtempSync(join(tmpdir(), 'bar3d-ledger-'));
  server = spawn('npx', ['wrangler', 'dev', '--port', String(PORT), '--ip', '127.0.0.1', '--persist-to', stateDir,
    '--var', 'DEV:1', '--var', `DAILY_CAP_CENTS:${CAP}`],
  { cwd: new URL('../cloud/', import.meta.url).pathname, stdio: ['ignore', 'pipe', 'pipe'], detached: true });
  let log = '';
  server.stdout.on('data', (d) => (log += d));
  server.stderr.on('data', (d) => (log += d));
  for (let i = 0; i < 120; i++) {
    await wait(500);
    if (/Ready on/.test(log)) return;
    if (server.exitCode !== null) break;
  }
  console.log(log);
  throw new Error('wrangler dev did not start');
}

async function stopServer() {
  if (server) { try { process.kill(-server.pid, 'SIGTERM'); } catch (e) {} }
  await wait(500);
  if (stateDir) { try { rmSync(stateDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 }); } catch (e) {} }
}

const post = async (path, body) => {
  const r = await fetch(base + path, { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify(body) });
  return { status: r.status, ...(await r.json()) };
};
const pub = (body) => post('/bank', body);
const internal = (body) => post('/bank-internal', body);
const TOKEN = 'dev' + 'a1b2c3d4'.repeat(5) + '12345';  // 48 hex-ish characters, a test login
const USER = 'dev-a1b2c3d4';

try {
  await startServer();
  let me = await pub({ a: 'me', token: TOKEN });
  check(me.ok === 1 && me.user === USER && me.balance === 0 && me.cap === CAP, 'a new player has 0 credits');
  check((await pub({ a: 'me', token: 'nope' })).status === 401, 'no login: no wallet');
  const sneaky = await pub({ a: 'credit', user: USER, cents: 500, reason: 'x', ref: 'hack-1' });
  check(sneaky.status === 403 && sneaky.ok === 0, 'a player can\'t give themselves credits');
  let r = await internal({ a: 'credit', user: USER, cents: 500, reason: 'darts win', ref: 'darts-1' });
  check(r.ok === 1 && r.balance === 500, 'the game credits a win: 500');
  r = await internal({ a: 'credit', user: USER, cents: 500, reason: 'darts win', ref: 'darts-1' });
  check(r.ok === 1 && r.duplicate === 1 && r.balance === 500, 'the same win sent twice counts once');
  r = await internal({ a: 'debit', user: USER, cents: 200, reason: 'mall: stool', ref: 'buy-1' });
  check(r.ok === 1 && r.balance === 300, 'spending 200 leaves 300');
  r = await internal({ a: 'debit', user: USER, cents: 1000, reason: 'mall: sofa', ref: 'buy-2' });
  check(r.ok === 0 && r.error === 'not enough credits', 'can\'t spend more than you have');
  me = await pub({ a: 'me', token: TOKEN });
  check(me.balance === 300 && me.earnedToday === 500, '...and the balance stays 300 (earned today: 500)');
  r = await internal({ a: 'credit', user: USER, cents: 900, reason: 'race win', ref: 'race-1' });
  check(r.ok === 1 && r.credited === 500 && r.capped === 1 && r.balance === 800, 'the daily cap: only 500 of 900 more');
  r = await internal({ a: 'credit', user: USER, cents: 100, reason: 'chess win', ref: 'chess-1' });
  check(r.ok === 1 && r.capped === 1 && r.credited === 0 && r.balance === 800, 'at the cap, more wins add nothing today');
  const g1 = await internal({ a: 'grant', user: USER, item: 'jacket.gold', numbered: 1, tradeable: 1, reason: 'vip', ref: 'g-1' });
  const g2 = await internal({ a: 'grant', user: 'dev-other', item: 'jacket.gold', numbered: 1, reason: 'vip', ref: 'g-2' });
  const g1again = await internal({ a: 'grant', user: USER, item: 'jacket.gold', numbered: 1, reason: 'vip', ref: 'g-1' });
  check(g1.serial === 1 && g2.serial === 2 && g1again.duplicate === 1 && g1again.serial === 1,
        'numbered items: #1, then #2, and a repeat gives nothing new');
  me = await pub({ a: 'me', token: TOKEN });
  check(me.items.length === 1 && me.items[0].item === 'jacket.gold' && me.items[0].serial === 1 && me.items[0].tradeable === 1,
        'your wallet lists your gold jacket #1 (tradeable)');
  const h = await internal({ a: 'history', user: USER, limit: 10 });
  check(h.ok === 1 && h.rows.length === 4 && h.rows[0].ref === 'race-1', 'the history has every change, newest first');
  check((await internal({ a: 'credit', user: USER, cents: -5, reason: 'x', ref: 'neg' })).ok === 0, 'negative amounts are refused');
} catch (e) {
  console.log('ERROR ' + (e && e.stack || e));
  failed++;
} finally {
  await stopServer();
}
console.log(failed ? `LEDGER TEST FAILED (${failed})` : 'LEDGER TEST PASSED');
process.exit(failed ? 1 : 0);
