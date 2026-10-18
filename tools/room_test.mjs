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
import { decodeText, encodeText, decodeBinary, encodeState } from '../cloud/proto.js';

const PORT = 8790;
const GRACE_MS = 3000;
const GHOST_MS = 4000;
const AFK_MS = 9000;
const SERVER_PROTO = 2;
let base = process.env.ROOM_URL;
let server = null, stateDir = null;
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };

async function startServer() {
  stateDir = mkdtempSync(join(tmpdir(), 'bar3d-room-'));
  server = spawn('npx', ['wrangler', 'dev', '--port', String(PORT), '--ip', '127.0.0.1', '--persist-to', stateDir,
    '--var', 'DEV:1', '--var', `PROTO:${SERVER_PROTO}`, '--var', `GRACE_MS:${GRACE_MS}`, '--var', 'CONNECT_LIMIT_PER_MIN:20',
    '--var', `GHOST_MS:${GHOST_MS}`, '--var', `AFK_MS:${AFK_MS}`, '--var', 'GHOST_CHECK_MS:2000'],
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
    if (typeof e.data !== 'string') {
      const d = decodeBinary(e.data);
      c.got.push(d.ok ? { letter: 'T', body: d.msg, at: Date.now() } : { letter: '!', why: d.why });
      return;
    }
    const d = decodeText(e.data, 'toClient');
    c.got.push(d.ok ? d : { letter: '!', why: d.why, raw: e.data });
  };
  ws.onclose = (e) => { c.closed = { code: e.code, reason: e.reason }; };
  c.open = new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = () => reject(new Error('socket error')); });
  c.send = (letter, body) => ws.send(encodeText(letter, body, 'toServer') ?? (() => { throw new Error('bad message ' + letter); })());
  c.find = (letter, pred = () => true) => c.got.find((m) => m.letter === letter && pred(m.body));
  c.until = async (fn, ms = 3000) => { for (let t = 0; t < ms; t += 20) { const v = fn(); if (v) return v; await wait(20); } return fn(); };
  c.welcome = () => c.until(() => c.find('W'));
  c.state = (seq, x, extra = {}) => ws.send(encodeState({ seq, avatar: { x, y: 1.65, z: 0, yaw: 0, pitch: 0, anim: 0, animTick: 0, held: 0, look: 0, flags: 0, ...extra }, act: '' }));
  c.all = (letter) => c.got.filter((m) => m.letter === letter);
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

  // ---- 3.2b: movement, world, events, signals, claims ----
  const r8 = room('tick');
  const ta = await joined(r8, { name: 'Ann' }), tb = await joined(r8, { name: 'Ben' });
  const sa = ta.find('W').body.slot, sb = tb.find('W').body.slot;
  const sentAt = Date.now();
  ta.state(1, 1.25);
  const tick1 = await tb.until(() => tb.find('T', (t) => t.players.some((p) => p.slot === sa)));
  check(!!tick1, 'Ann\'s STATE reaches Ben in a TICK');
  check(tick1 && tick1.at - sentAt <= 150, `within 150 ms here (${tick1 && tick1.at - sentAt} ms; target ≤ 100 on the real server)`);
  check(tick1 && Math.abs(tick1.body.players.find((p) => p.slot === sa).avatar.x - 1.25) < 0.01 && Math.abs(tick1.body.serverMs - Date.now()) < 5000,
    'the TICK carries her position and the server time');
  await wait(200);
  check(!ta.find('T'), 'Ann doesn\'t get her own STATE back');
  ta.state(1, 9);  // same seq again: dropped
  ta.state(2, 2.5);
  await wait(300);
  const lastX = tb.all('T').flatMap((t) => t.body.players).filter((p) => p.slot === sa).map((p) => Math.round(p.avatar.x * 100) / 100);
  check(!lastX.includes(9) && lastX[lastX.length - 1] === 2.5, `an old or repeated STATE is dropped (${lastX})`);
  await wait(2600);
  const ticks = tb.all('T').length;
  await wait(600);
  check(tb.all('T').length === ticks && !(await debug(r8)).ticking, 'the TICK stops after 2 s without changes');
  const late = await joined(r8, { name: 'Cat' });
  check(!!(await late.until(() => late.find('T', (t) => t.players.some((p) => p.slot === sa)))), 'a newcomer gets everyone\'s pose at once');

  // Both write the same world key 50 times at once → the same final value everywhere.
  for (let i = 0; i < 50; i++) { ta.send('K', { w: { g3: [1, i] } }); tb.send('K', { w: { g3: [2, i] } }); }
  await wait(1200);
  const final = (c) => { const k = c.all('K').flatMap((m) => m.body.w).filter((e) => e[1] === 'g3'); return k.length ? JSON.stringify(k[k.length - 1]) : ''; };
  check(final(ta) && final(ta) === final(tb) && final(ta) === final(late), `same final world value on every phone (${final(ta)})`);
  check(ta.all('K').flatMap((m) => m.body.w).some((e) => e[2] === sa), 'writes are echoed to the writer too');
  const seqs = tb.all('K').flatMap((m) => m.body.w).map((e) => e[0]);
  check(seqs.every((v, i) => i === 0 || v > seqs[i - 1]), 'world writes arrive in rising sequence order');
  const late2 = await joined(r8, { name: 'Dan' });
  const wkv = late2.find('W').body.kv || [];
  check(wkv.some((e) => e[1] === 'g3' && JSON.stringify(e) === final(ta)), 'a newcomer\'s welcome has the world');

  // Events: stamped, in order, not back to the sender.
  ta.send('E', { e: [['note', [60]], ['note', [62]]] });
  ta.send('E', { e: [['note', [64]]] });
  const ev = await tb.until(() => { const e = tb.all('E').flatMap((m) => m.body.e); return e.length >= 3 ? e : null; });
  check(ev && ev.map((e) => e[4][0]).join() === '60,62,64' && ev.every((e) => e[1] === sa && e[3] === 'note'), 'events arrive in order, from Ann');
  check(ev && ev.every((e, i) => i === 0 || e[2] > ev[i - 1][2]) && ev.every((e) => Number.isInteger(e[0])), 'events carry a rising sequence and a server tick');
  await wait(200);
  check(!ta.find('E'), 'the sender doesn\'t get her own events back');

  // Signals reach only their target.
  ta.send('G', { to: sb, data: { t: 'offer', sdp: 'x' } });
  const sig = await tb.until(() => tb.find('G'));
  check(sig && sig.body.from === sa && sig.body.data.t === 'offer', 'a SIGNAL reaches its target with the sender\'s slot');
  await wait(200);
  check(!late.find('G'), 'and nobody else');

  // Claims: the room decides; one owner; freed when they leave.
  ta.send('I', { i: 1, k: 'claim', key: 'seat.4' });
  const c1 = await ta.until(() => ta.find('R', (r) => r.r === 1));
  check(c1 && c1.body.ok === 1, 'Ann claims seat 4');
  tb.send('I', { i: 7, k: 'claim', key: 'seat.4' });
  const c2 = await tb.until(() => tb.find('R', (r) => r.r === 7));
  check(c2 && c2.body.ok === 0 && c2.body.why === 'taken' && c2.body.by === sa, 'Ben is told it\'s taken, by Ann');
  check(!!(await tb.until(() => tb.find('D', (d) => d.d.some((x) => x[1] === 'claim.seat.4' && x[2] === sa)))), 'everyone hears Ann has seat 4');
  tb.send('I', { i: 7, k: 'claim', key: 'seat.4' });
  await wait(300);
  check(tb.all('R').filter((r) => r.body.r === 7).length === 2 && tb.all('R').filter((r) => r.body.r === 7).every((r) => r.body.ok === 0), 'a repeated request gets the same answer');
  check((await joined(r8, { name: 'Eve' })).find('W').body.claims['seat.4'] === sa, 'a newcomer\'s welcome lists the claims');
  ta.send('I', { i: 2, k: 'claim', key: 'seat.4' });
  check((await ta.until(() => ta.find('R', (r) => r.r === 2)))?.body.ok === 1, 'claiming your own seat again is fine');
  ta.ws.close(1000);
  check(!!(await tb.until(() => tb.find('D', (d) => d.d.some((x) => x[1] === 'claim.seat.4' && x[2] === -1)))), 'Ann leaves → seat 4 is free for everyone');
  tb.send('I', { i: 8, k: 'claim', key: 'seat.4' });
  check((await tb.until(() => tb.find('R', (r) => r.r === 8)))?.body.ok === 1, 'now Ben gets it');
  tb.send('I', { i: 9, k: 'release', key: 'seat.4' });
  check((await tb.until(() => tb.find('R', (r) => r.r === 9)))?.body.ok === 1, 'and can release it');
  tb.send('I', { i: 10, k: 'fly', key: 'x' });
  check((await tb.until(() => tb.find('R', (r) => r.r === 10)))?.body.ok === 0, 'an unknown intent is refused');

  // Away: others hear it.
  tb.send('A', { on: 1 });
  check(!!(await late.until(() => late.find('D', (d) => d.d.some((x) => x[1] === 'away' && x[2] === 1 && x[3] === sb)))), 'AWAY reaches the others');
  [tb, late, late2].forEach((c) => c.ws.close(1000));

  // ---- 3.2c: ghosts, AFK, floods, storage, /health ----
  const r9 = room('ghost');
  const watcher = await joined(r9, { name: 'Watch' });
  const ghost = await joined(r9, { name: 'Ghost' });
  const gslot = ghost.find('W').body.slot;
  // The watcher keeps talking; the ghost goes silent (no messages, no keepalive).
  const keep = setInterval(() => { try { watcher.ws.send('ping'); watcher.send('P', { c: 1 }); } catch (e) {} }, 1000);
  await ghost.until(() => ghost.closed, GHOST_MS + 6000);
  check(ghost.closed?.code === 4004, `a silent socket is closed as a ghost (4004, got ${ghost.closed?.code})`);
  check(!!(await watcher.until(() => watcher.find('D', (d) => d.d.some((x) => x[1] === 'away' && x[2] === 1 && x[3] === gslot)))), 'others see the ghost as away first');
  check(!!(await watcher.until(() => watcher.find('L', (x) => x.slot === gslot), GRACE_MS + 4000)), 'then it leaves after the grace period');
  await watcher.until(() => watcher.closed, AFK_MS + 6000);
  clearInterval(keep);
  check(watcher.closed?.code === 4004 && /away/.test(watcher.closed?.reason || ''), `no movement for the AFK time → 4004 "away" (got ${watcher.closed?.code} ${watcher.closed?.reason})`);

  const r10 = room('flood');
  const flooder = await joined(r10, { name: 'Flood' });
  for (let i = 0; i < 500; i++) { try { flooder.send('P', { c: i }); } catch (e) { break; } }
  check(!!(await flooder.until(() => flooder.closed)) && flooder.closed.code === 4005, `500 messages at once → 4005 (got ${flooder.closed?.code})`);

  // Storage in "main": a 10 Hz world write load writes few rows (debounced).
  const mw = await joined('main', { name: 'Writer' });
  const before = (await debug('main')).usage.rows;
  const t0 = Date.now();
  while (Date.now() - t0 < 6000) { mw.send('K', { w: { tst: [Date.now() % 1000] } }); mw.state(Math.floor((Date.now() - t0) / 100) + 1, (Date.now() - t0) / 1000); await wait(100); }
  for (let i = 0; i < 6; i++) { mw.ws.send('ping'); await wait(500); }
  const rows = (await debug('main')).usage.rows - before;
  check(rows <= 6, `6 s of 10 Hz world writes → ${rows} rows written (≤ 30 per minute)`);
  check((await debug('main')).storageKeys.includes('kv:tst'), 'the world key is saved');
  for (let i = 0; i < 5; i++) { mw.ws.send('ping'); await wait(500); }
  const idle = await debug('main');
  check(!idle.ticking && idle.deadlines.ghost > Date.now(), 'an idle room stops ticking and sets one ghost-check alarm');
  mw.ws.close(1000);
  const h1 = await (await fetch(`${base}/health`)).json();
  const h2 = await (await fetch(`${base}/health`)).json();
  check(h1.main && typeof h1.main.requestsToday === 'number', `/health shows room main's usage (${h1.main?.requestsToday} requests today)`);
  check(h2.cachedMs > 0, 'a second /health within 60 s is answered from memory');

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
