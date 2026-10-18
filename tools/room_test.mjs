// Tests the Cloudflare room (cloud/worker.js + cloud/room.js) through real
// WebSockets: slots, resume tokens, grace, close codes, login → PROFILE,
// Origin allowlist, connect limit, storage left behind. Speaks the protocol
// with cloud/proto.js (docs/PROTOCOL.md).
// Usage: node tools/room_test.mjs
//   Starts `wrangler dev` itself (needs `npm ci --prefix cloud`) with test
//   settings: DEV=1, PROTO=2 (to check that PROTO-1 is still accepted),
//   GRACE_MS=3000, CONNECT_LIMIT_PER_MIN=20, and fresh local storage.
//   ROOM_URL=http://127.0.0.1:8787 uses a server you started yourself (with the same vars).
import { spawn } from 'node:child_process';
import { request } from 'node:http';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { decodeText, encodeText } from '../cloud/proto.js';

const PORT = 8790;
const GRACE_MS = 3000;
const SERVER_PROTO = 2;
let base = process.env.ROOM_URL;
let server = null, stateDir = null;
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };

async function startServer() {
  stateDir = mkdtempSync(join(tmpdir(), 'bar3d-room-'));
  server = spawn('npx', ['wrangler', 'dev', '--port', String(PORT), '--ip', '127.0.0.1', '--persist-to', stateDir,
    '--var', 'DEV:1', '--var', `PROTO:${SERVER_PROTO}`, '--var', `GRACE_MS:${GRACE_MS}`, '--var', 'CONNECT_LIMIT_PER_MIN:20'],
  { cwd: new URL('../cloud/', import.meta.url).pathname, stdio: ['ignore', 'pipe', 'pipe'], detached: true });
  let log = '';
  server.stdout.on('data', (d) => (log += d));
  server.stderr.on('data', (d) => (log += d));
  base = `http://127.0.0.1:${PORT}`;
  for (let i = 0; i < 120; i++) {
    await wait(500);
    if (/Ready on/.test(log)) return;
    if (server.exitCode !== null) break;
  }
  console.log(log);
  throw new Error('wrangler dev did not start');
}

function stopServer() {
  if (server) { try { process.kill(-server.pid, 'SIGTERM'); } catch (e) {} }
  if (stateDir) rmSync(stateDir, { recursive: true, force: true });
}

// A fake player: a socket plus everything it received, decoded.
function client(room) {
  const ws = new WebSocket(`${base.replace(/^http/, 'ws')}/room/${room}`);
  ws.binaryType = 'arraybuffer';
  const c = { ws, got: [], closed: null };
  ws.onmessage = (e) => {
    if (typeof e.data !== 'string') return;
    const d = decodeText(e.data, 'toClient');
    c.got.push(d.ok ? d : { letter: '!', why: d.why, raw: e.data });
  };
  ws.onclose = (e) => { c.closed = { code: e.code, reason: e.reason }; };
  c.open = new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = () => reject(new Error('socket error')); });
  c.send = (letter, body) => ws.send(encodeText(letter, body, 'toServer') ?? (() => { throw new Error('bad message ' + letter); })());
  c.find = (letter, pred = () => true) => c.got.find((m) => m.letter === letter && pred(m.body));
  c.until = async (fn, ms = 3000) => { for (let t = 0; t < ms; t += 20) { const v = fn(); if (v) return v; await wait(20); } return fn(); };
  c.welcome = () => c.until(() => c.find('W'));
  c.hello = (extra = {}) => c.send('H', { proto: SERVER_PROTO, build: 'test', name: 'P', look: 0, ...extra });
  return c;
}

async function joined(room, extra) {
  const c = client(room);
  await c.open;
  c.hello(extra);
  await c.welcome();
  return c;
}

// A raw HTTP upgrade request (to set Origin / X-Test-IP, which WebSocket can't).
function upgrade(path, headers) {
  return new Promise((resolve) => {
    const req = request(`${base}${path}`, {
      headers: { Connection: 'Upgrade', Upgrade: 'websocket', 'Sec-WebSocket-Version': '13', 'Sec-WebSocket-Key': 'dGhlIHNhbXBsZSBub25jZQ==', ...headers },
    });
    req.on('upgrade', (res, socket) => { socket.destroy(); resolve(101); });
    req.on('response', (res) => { res.resume(); resolve(res.statusCode); });
    req.on('error', () => resolve(0));
    req.end();
  });
}

async function debug(room) {
  const r = await fetch(`${base}/room/${room}/debug`);
  return r.json();
}

const room = (n) => `t${n}-${Math.floor(Math.random() * 1e6)}`;

try {
  if (!base) await startServer();
  const health = await (await fetch(`${base}/health`)).json();
  check(health.ok === 1, '/health answers');

  // Slots, welcome, join, ping.
  const r1 = room('slots');
  const a = await joined(r1, { name: 'Alice', look: 3 });
  const b = await joined(r1, { name: 'Bob' });
  const wa = a.find('W'), wb = b.find('W');
  check(wa && wb && wa.body.slot !== wb.body.slot, `distinct slots (${wa?.body.slot}, ${wb?.body.slot})`);
  check(wb && wb.body.roster.some((p) => p.name === 'Alice' && p.look === 3) && wb.body.room === r1 && wb.body.tickHz === 20,
    'Bob\'s welcome lists Alice, the room and the tick rate');
  check(wa && /^[0-9a-f]{32}$/.test(wa.body.resume) && Math.abs(wa.body.serverMs - Date.now()) < 5000, 'welcome has a resume token and the server clock');
  check(await a.until(() => a.find('J', (p) => p.name === 'Bob')), 'Alice is told Bob joined');
  check(a.got.every((m) => m.letter !== '!') && b.got.every((m) => m.letter !== '!'), 'every server message decodes with proto.js');
  a.send('P', { c: 42.5 });
  const pong = await a.until(() => a.find('P'));
  check(pong && pong.body.c === 42.5 && Math.abs(pong.body.s - Date.now()) < 5000, 'PING → PONG with server time');
  a.ws.send('ping');
  await wait(200);
  check(a.closed === null, 'the plain "ping" keepalive is answered without trouble');
  a.ws.send('garbage'); a.ws.send('H{"proto":'); a.ws.send(new Uint8Array([0x81, 1, 2]));
  a.send('P', { c: 7 });
  check(await a.until(() => a.find('P', (p) => p.c === 7)), 'bad frames are dropped and the socket keeps working');

  // Resume while the old socket is open → same slot, old one gets 4003.
  const a2 = await joined(r1, { name: 'Alice', resume: wa.body.resume });
  check(a2.find('W')?.body.slot === wa.body.slot, 'resume token → same slot');
  check(await a.until(() => a.closed?.code === 4003), `old socket replaced (4003, got ${a.closed?.code})`);
  await wait(300);
  check(!b.find('L') && b.got.filter((m) => m.letter === 'J').length === 0, 'Bob saw no leave/join for a resume');
  check(a2.find('W').body.resume !== wa.body.resume, 'a fresh resume token each welcome');

  // Dropped socket → grace → resume keeps the slot; no resume → LEAVE after grace.
  const resume2 = a2.find('W').body.resume;
  a2.ws.close(3001, 'network lost');
  await wait(500);
  const roster = await joined(r1, { name: 'Carl' });
  check(roster.find('W').body.roster.some((p) => p.slot === wa.body.slot && p.away === 1), 'a dropped player shows as away during the grace period');
  const a3 = await joined(r1, { name: 'Alice', resume: resume2 });
  check(a3.find('W')?.body.slot === wa.body.slot, 'reconnect within the grace period → same slot');
  check(!b.find('L'), 'no LEAVE for a reconnect within grace');
  a3.ws.close(3001, 'network lost');
  await b.until(() => b.find('L'), GRACE_MS + 4000);
  check(b.find('L', (l) => l.slot === wa.body.slot && /lost/.test(l.why)), 'LEAVE after the grace period');
  roster.ws.close(1000);
  check(await b.until(() => b.find('L', (l) => l.why === 'left')), 'a normal close frees the slot at once');

  // Login: guest first, then PROFILE.
  const r2 = room('login');
  const l = await joined(r2, { name: 'Guest', ticket: 'test:alice' });
  check(l.find('W').body.roster.find((p) => p.slot === l.find('W').body.slot).name === 'Guest', 'welcome comes first, as a guest');
  check(await l.until(() => l.find('F', (p) => p.acct === 'alice' && p.name === 'alice')), 'then PROFILE with the account');
  l.ws.close(1000);

  // Protocol versions: N and N-1 accepted, others 4002.
  const r3 = room('proto');
  const old = await joined(r3, { proto: SERVER_PROTO - 1 });
  check(!!old.find('W'), `proto ${SERVER_PROTO - 1} (N-1) accepted`);
  const nw = client(r3); await nw.open; nw.hello({ proto: SERVER_PROTO + 1 });
  check(await nw.until(() => nw.closed?.code === 4002), `proto ${SERVER_PROTO + 1} → 4002 (got ${nw.closed?.code}: ${nw.closed?.reason})`);
  old.ws.close(1000);

  // Room full: the 17th player gets 4001 with a reason.
  const r4 = room('full');
  const many = [];
  for (let i = 0; i < 16; i++) many.push(await joined(r4, { name: 'P' + i }));
  const slots = new Set(many.map((c) => c.find('W')?.body.slot));
  check(slots.size === 16, `16 players get 16 different slots (${slots.size})`);
  const extra = client(r4); await extra.open; extra.hello();
  check(await extra.until(() => extra.closed?.code === 4001), `17th → 4001 (got ${extra.closed?.code})`);
  check(/full/.test(extra.closed?.reason || ''), `with a reason ("${extra.closed?.reason}")`);
  many.forEach((c) => c.ws.close(1000));

  // No HELLO → closed after 5 s.
  const r5 = room('hello');
  const silent = client(r5); await silent.open;
  // (wrangler dev only delivers a close to a socket that has sent something,
  // so this one sends a non-HELLO frame first; see DECISIONS → Gotchas.)
  silent.ws.send('junk');
  await silent.until(() => silent.closed, 8000);
  check(silent.closed?.code === 4004, `a socket without HELLO is closed (got ${silent.closed?.code} after ≤ 8 s)`);

  // Origin allowlist (exact match only).
  check(await upgrade(`/room/${room('o')}`, { Origin: 'https://evil.example' }) === 403, 'another website is refused');
  check(await upgrade(`/room/${room('o')}`, { Origin: 'https://barcode162.github.io.evil.example' }) === 403, 'no suffix matching');
  check(await upgrade(`/room/${room('o')}`, { Origin: 'https://barcode162.github.io' }) === 101, 'our site is allowed');
  check(await upgrade(`/room/${room('o')}`, { Origin: 'http://localhost:5173' }) === 101, 'localhost is allowed');

  // Connect limit per IP (20 per minute in the test).
  const r6 = room('limit');
  const codes = [];
  for (let i = 0; i < 21; i++) codes.push(await upgrade(`/room/${r6}`, { 'X-Test-IP': '10.0.0.1' }));
  check(codes.slice(0, 20).every((c) => c === 101) && codes[20] === 429, `21st connect in a minute → 429 (${codes.slice(18).join(',')})`);
  check(await upgrade(`/room/${r6}`, { 'X-Test-IP': '10.0.0.2' }) === 101, 'another IP still gets in');

  // Storage: a non-main room leaves nothing; main keeps its schemaVersion.
  const r7 = room('trace');
  const t = await joined(r7);
  t.ws.close(3001);  // grace writes to storage
  await wait(500);
  const during = await debug(r7);
  check(during.storageKeys.includes('grace'), `grace is kept in storage while it runs (${during.storageKeys})`);
  await wait(GRACE_MS + 1500);
  const after = await debug(r7);
  check(after.storageKeys.length === 0 && after.slots.length === 0, `a non-main room leaves no storage (${after.storageKeys})`);
  const m = await joined('main');
  m.ws.close(3001);
  await wait(500);
  check((await debug('main')).storageKeys.includes('schemaVersion'), 'room "main" records its schemaVersion');
  b.ws.close(1000);
} catch (e) {
  check(false, 'test crashed: ' + (e.stack || e));
} finally {
  stopServer();
}

if (failed) { console.log(`ROOM TEST FAILED (${failed})`); process.exit(1); }
console.log('ROOM TEST PASSED');
process.exit(0);
