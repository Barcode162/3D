// One Bar3D room (a Cloudflare Durable Object): the authority for who is in
// the room, in which slot, who holds what (claims), the order of events and
// the shared world. Protocol: docs/PROTOCOL.md (codec in proto.js).
//
// What it does:
// - Slots and resume tokens, HELLO → WELCOME, guest first then an async login
//   check → PROFILE, close codes 4001-4005, PING/PONG, a 5 s hello timeout,
//   a 30 s grace when a socket drops (restructure 3.2a).
// - STATE in (binary), a 20 Hz TICK out only while something moved, stopping
//   after 2 s of quiet so the room can sleep (3.2b).
// - EVENTS stamped with the server tick and a sequence number, fanned out to
//   the others; SIGNAL forwarded to one player (voice chat handshakes).
// - The legacy shared world (KV): every write gets a server sequence number
//   and is echoed to everyone, the writer included, so every phone ends on
//   the same value (3.2b; replaced by RoomState fields in Phase 5).
// - Claims (`claim` / `release` intents): the room decides who has a seat,
//   a machine or a glass; one owner per key; freed when the owner leaves.
// - Ghost and AFK checks, rate limits (4005), debounced storage of changed
//   world keys only, usage counters for /health (3.2c).
//
// Hibernation: the room may be evicted from memory between messages. Who is
// connected, their last STATE and their claims are rebuilt from the sockets'
// attachments; players in their grace period and the world are in storage.
import { PROTO, MAX_SLOTS, BIN, decodeText, decodeBinary, propBytes } from './proto.js';
import { checkWrite } from './rules.js';
import * as darts from './games/darts.js';
import * as jukebox from './games/jukebox.js';
import * as race from './games/race.js';
import { Glasses, eyesOf } from './games/glasses.js';
import { Sim } from './sim/sim.js';
import simWasm from './sim/room_sim.wasm';
// The bar's layout comes from its world files (docs/ENGINE.md), bundled in.
import barPlace from '../assets/world/places/bar.json';
import barKinds from '../assets/world/kinds/bar.json';
import homePlace from '../assets/world/places/home.json';
import homeKinds from '../assets/world/kinds/home.json';

const TICK_MS = 50;
const IDLE_STOP_MS = 2000;       // the ticker stops after this long without changes
const HELLO_MS = 5000;
const GHOST_MS = 15000;          // no message and no keepalive this long: a ghost
const AFK_MS = 30 * 60 * 1000;   // no movement, voice, intent or event this long: away
const GHOST_CHECK_IDLE_MS = 60000;  // how often an idle room checks (one alarm)
const FLUSH_MS = 2500;           // world writes are saved at most this often
const USAGE_SAVE_MS = 60000;     // usage counters are saved with a flush at most this often
const WORLD_FORGET_MS = 30 * 60 * 1000;  // an empty room forgets its world after this
const SCHEMA_VERSION = 1;        // docs/PROTOCOL.md §9: bump when stored keys change shape
const TOKEN_CACHE_MS = 10 * 60 * 1000;
const TOKEN_TIMEOUT_MS = 2000;
const PERSISTENT_ROOM = 'main';
// Rooms that keep their world while empty, and the private bar's room: only
// logins account.php says may play get in (docs/LOOPED.md A5).
const PERSISTENT_ROOMS = new Set(['main', 'looped']);
const PRIVATE_ROOMS = new Set(['looped']);
const MAX_SOCKETS = MAX_SLOTS * 2;  // players + sockets still saying hello
const MAX_KV_KEYS = 160;  // bar keys (~40) + thing states t<net> (one per thing that ever changed)
const THING_REACH = 6;     // metres: you touch things near you
const MAX_CLAIMS_PER_PLAYER = 8;
const CLAIM_KEY = /^[A-Za-z0-9_.-]{1,40}$/;
// Rate limits per socket (PROTOCOL §7): messages, bytes, intents.
const MSG_RATE = 25, MSG_BURST = 60;
const BYTE_RATE = 65536, BYTE_BURST = 131072;
const INTENT_RATE = 10, INTENT_BURST = 20;
const BAD_FRAME_COST = 10;
// Free-plan daily quotas (Workers Free, checked 2026-10-18): Durable Object
// requests (20 incoming WebSocket messages = 1 request).
const DAILY_REQUESTS = 100000;
const SLOW_DOWN_AT = 0.7;

function randomHex(bytes) {
  const b = new Uint8Array(bytes);
  crypto.getRandomValues(b);
  return Array.from(b, (x) => x.toString(16).padStart(2, '0')).join('');
}

function text(letter, body) { return letter + JSON.stringify(body); }

// Cut a string to at most `bytes` UTF-8 bytes (the protocol counts bytes).
function clip(s, bytes) {
  const enc = new TextEncoder();
  while (enc.encode(s).length > bytes) s = s.slice(0, -1);
  return s;
}

function hexToBytes(hex) {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.substr(i * 2, 2), 16);
  return out;
}
function bytesToHex(b) { return Array.from(b, (x) => x.toString(16).padStart(2, '0')).join(''); }

function today() { return new Date().toISOString().slice(0, 10); }

export class Room {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    this.dev = env.DEV === '1';
    this.proto = Number(env.PROTO) || PROTO;
    this.graceMs = Number(env.GRACE_MS) || 30000;
    this.ghostMs = Number(env.GHOST_MS) || GHOST_MS;
    this.afkMs = Number(env.AFK_MS) || AFK_MS;
    this.ghostCheckMs = Number(env.GHOST_CHECK_MS) || GHOST_CHECK_IDLE_MS;
    this.usageSavedAt = 0;
    this.songTestSeconds = this.dev ? Number(env.SONG_SECONDS) || 0 : 0;  // tests: every song this long
    this.connectLimit = Number(env.CONNECT_LIMIT_PER_MIN) || 10;
    this.name = '';
    this.players = new Map();    // slot → {slot, name, look, acct, av, resume, ws|null, graceUntil, away}
    this.states = new Map();     // slot → {seq, raw (avatar block + activity bytes)}
    this.claims = new Map();     // key → slot
    this.kv = new Map();         // key → {seq, by, values}
    this.dirtyKv = new Set();    // keys changed since the last save
    this.dirtyPlayers = new Set();  // slots whose STATE changed since the last TICK
    this.pendingEvents = [];     // stamped events waiting for the next TICK
    this.deadlines = new Map();  // job name → ms; the one alarm is set to the earliest
    this.alarmAt = 0;            // what the alarm is set to now (0 = none)
    this.ipHits = new Map();     // ip → connect times in the last minute
    this.tokens = new Map();     // login token → {who, until}
    this.buckets = new Map();    // socket → rate-limit buckets
    this.lastMsg = new Map();    // slot → ms of their last message
    this.lastActive = new Map(); // slot → ms of their last movement, intent or event
    this.intentResults = new Map();  // slot → Map(request id → RESULT text)
    this.games = {};             // games the room runs: darts → {owner slot, session}
    this.race = race.empty();    // the racing rigs (cloud/games/race.js), shared as world key rc
    // The place's physics (every glass) and its things (E7.3): the bar, or
    // the test home for rooms named home*. Remade once the room knows its name.
    this.simFiles = {
      'places/bar.json': JSON.stringify(barPlace), 'kinds/bar.json': JSON.stringify(barKinds),
      'places/home.json': JSON.stringify(homePlace), 'kinds/home.json': JSON.stringify(homeKinds),
    };
    this.place = '';
    this.worldVersion = 0;       // the Studio's live world in use (0: built in)
    this.worldCheckedMs = 0;
    this.usePlace();
    this.lastSimMs = 0;
    this.lastSeq = 0;
    this.ticker = null;
    this.lastChangeMs = 0;
    this.lastGhostCheck = 0;
    this.emptySince = 0;
    this.stateHz = 0;            // 0 = normal; 10 when the daily quota runs low
    this.usage = { day: today(), messages: 0, alarms: 0, rows: 0, bad: 0, upgrades: 0, wiped: 0, refused: 0 };
    // A plain "ping" text is answered by the runtime without waking the room
    // (the idle keepalive, §2.5.9). It isn't a protocol message.
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair('ping', 'pong'));
    const now = Date.now();
    for (const ws of ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (!a) continue;
      if (a.room) this.name = a.room;
      this.usePlace();
      if (a.slot < 0) continue;
      this.players.set(a.slot, { ...a, ws, graceUntil: 0, away: a.away || 0 });
      if (a.st) this.states.set(a.slot, { seq: a.st.seq, raw: hexToBytes(a.st.raw) });
      for (const key of a.claims || []) this.claims.set(key, a.slot);
      if (a.darts) this.games.darts = { owner: a.slot, session: a.darts };
      this.lastMsg.set(a.slot, now);
      this.lastActive.set(a.slot, now);
    }
    ctx.blockConcurrencyWhile(() => this.load());
  }

  // Which place this room is (from its name), and its world made for it.
  usePlace(force = false) {
    const place = /^home/.test(this.name) ? 'home' : 'bar';
    if (place === this.place && !force) return;
    this.place = place;
    this.sim = new Sim(simWasm, { ...this.simFiles, place });
    this.glasses = new Glasses(this.sim);
    this.thingsSynced = false;  // the stored thing states handed to this.sim yet
    if (this.kv.size) this.glasses.restore(this.kv);
  }

  async load() {
    const s = this.ctx.storage;
    const version = await s.get('schemaVersion');
    if (version !== SCHEMA_VERSION && (version !== undefined || (await s.list({ limit: 1 })).size)) {
      console.log(`room: storage schema ${version} != ${SCHEMA_VERSION}, wiped to defaults`);
      await s.deleteAll();
      this.usage.wiped++;
    }
    this.schemaMarked = (await s.get('schemaVersion')) === SCHEMA_VERSION;
    const grace = (await s.get('grace')) || {};
    for (const p of Object.values(grace)) {
      if (!this.players.has(p.slot)) this.players.set(p.slot, { ...p, ws: null, away: 1 });
      for (const key of p.claims || []) this.claims.set(key, p.slot);
    }
    const kv = await s.list({ prefix: 'kv:' });
    for (const [k, v] of kv) {
      // Keys from before the things (the lights `lt`, mirrors `m<n>`): gone.
      if (/^(lt|m\d)$/.test(k.slice(3))) { await s.delete(k); continue; }
      this.kv.set(k.slice(3), v);
    }
    for (const e of this.kv.values()) this.lastSeq = Math.max(this.lastSeq, e.seq);
    this.emptySince = (await s.get('emptySince')) || 0;
    const usage = await s.get('usage');
    if (usage && usage.day === this.usage.day) Object.assign(this.usage, usage);
    this.alarmAt = (await s.getAlarm()) || 0;
    await this.useLiveWorld();
    // Glasses where they last came to rest; ones in a hand stay in it.
    this.glasses.restore(this.kv);
    for (const [key, slot] of this.claims) {
      const m = /^glass\.(\d+)$/.exec(key);
      if (m) this.glasses.grab(+m[1], slot, this.eyes(slot));
    }
    this.scheduleSongEnd();
    const rc = this.kv.get('rc');
    this.race = race.restore(rc ? rc.values : null);
    this.setDeadline('race', race.deadline(this.race));
    this.reschedule();
  }

  // The world the Studio published (docs/STUDIO.md 7.6): the same files the
  // phones load, so the room runs the same things and rules. Checked when the
  // room starts and, at most every 30 s, when someone joins. Any problem: keep
  // what we have (the world built in at first).
  async useLiveWorld() {
    const url = this.env.WORLD_URL;
    if (!url || Date.now() - this.worldCheckedMs < 30000) return;
    this.worldCheckedMs = Date.now();
    try {
      const live = await (await fetch(url + '?live', { signal: AbortSignal.timeout(4000) })).json();
      const v = live && live.version | 0;
      if (v < 1 || v === this.worldVersion) return;
      const b = await (await fetch(url + '?bundle=' + v, { signal: AbortSignal.timeout(8000), cf: { cacheTtl: 31536000 } })).json();
      if (!b || !b.files || (b.format || 1) > 1) return;
      const files = { ...this.simFiles };
      for (const [path, content] of Object.entries(b.files)) files[path] = JSON.stringify(content);
      this.simFiles = files;
      this.worldVersion = v;
      this.usePlace(true);
      console.log(`room: world v${v} (live, by ${b.author})`);
    } catch (e) {
      console.log('room: the live world could not be loaded: ' + e);
    }
  }

  // Sequence numbers for world writes, claims and events: always rising, even
  // across restarts (milliseconds × 1000 + a counter).
  nextSeq() {
    this.lastSeq = Math.max(this.lastSeq + 1, Date.now() * 1000);
    return this.lastSeq;
  }

  // ---- HTTP: the WebSocket upgrade (the Worker already checked the Origin) ----
  async fetch(request) {
    const url = new URL(request.url);
    this.name = request.headers.get('X-Room') || this.name;
    this.usePlace();
    if (request.headers.get('Upgrade') === 'websocket') await this.useLiveWorld();
    if (url.pathname === '/stats') return Response.json(this.stats());
    if (this.dev && url.pathname.endsWith('/debug')) {
      const keys = await this.ctx.storage.list();
      return Response.json({ storageKeys: [...keys.keys()], slots: [...this.players.keys()], alarmAt: this.alarmAt,
        deadlines: Object.fromEntries(this.deadlines), ticking: !!this.ticker, claims: Object.fromEntries(this.claims),
        usage: this.usage });
    }
    if (request.headers.get('Upgrade') !== 'websocket') return new Response('expected a websocket', { status: 426 });
    const ip = (this.dev && request.headers.get('X-Test-IP')) || request.headers.get('CF-Connecting-IP') || 'local';
    if (!this.allowConnect(ip)) return new Response('too many connections, wait a minute', { status: 429 });
    // Sockets that haven't said HELLO count too (they're closed after 5 s).
    if (this.ctx.getWebSockets().length >= MAX_SOCKETS) return new Response('room busy', { status: 503 });
    this.count('upgrades');
    const pair = new WebSocketPair();
    this.ctx.acceptWebSocket(pair[1]);
    pair[1].serializeAttachment({ room: this.name, slot: -1, openedMs: Date.now() });
    this.updateHelloDeadline();
    return new Response(null, { status: 101, webSocket: pair[0] });
  }

  allowConnect(ip) {
    const now = Date.now();
    const hits = (this.ipHits.get(ip) || []).filter((t) => now - t < 60000);
    if (hits.length >= this.connectLimit) { this.ipHits.set(ip, hits); return false; }
    hits.push(now);
    this.ipHits.set(ip, hits);
    if (this.ipHits.size > 1000) this.ipHits.clear();  // memory cap; resets the window
    return true;
  }

  // ---- Usage (for /health and the "slow down" switch) ----
  count(what, n = 1) {
    const day = today();
    if (this.usage.day !== day) this.usage = { day, messages: 0, alarms: 0, rows: 0, bad: 0, upgrades: 0, wiped: 0, refused: 0 };
    if (!(what in this.usage)) this.usage[what] = 0;
    this.usage[what] += n;
  }

  // Durable Object requests used today (estimate): 20 incoming messages count
  // as one, plus every alarm and connection.
  requestsToday() { return Math.ceil(this.usage.messages / 20) + this.usage.alarms + this.usage.upgrades; }

  stats() {
    return { room: this.name || PERSISTENT_ROOM, players: [...this.players.values()].filter((p) => p.ws).length,
      usage: this.usage, requestsToday: this.requestsToday(), dailyRequests: DAILY_REQUESTS, stateHz: this.stateHz || 20 };
  }

  checkQuota() {
    if (this.stateHz || this.requestsToday() < DAILY_REQUESTS * SLOW_DOWN_AT) return;
    this.stateHz = 10;
    this.broadcast(text('D', { d: [[this.nextSeq(), 'net.stateHz', 10, -1]] }));
  }

  // ---- Rate limits ----
  // Token buckets per socket; false (and the socket closed with 4005) when over.
  spend(ws, bytes, cost = 1) {
    const now = Date.now();
    let b = this.buckets.get(ws);
    if (!b) { b = { msgs: MSG_BURST, bytes: BYTE_BURST, intents: INTENT_BURST, at: now }; this.buckets.set(ws, b); }
    const dt = (now - b.at) / 1000;
    b.at = now;
    b.msgs = Math.min(MSG_BURST, b.msgs + dt * MSG_RATE) - cost;
    b.bytes = Math.min(BYTE_BURST, b.bytes + dt * BYTE_RATE) - bytes;
    b.intents = Math.min(INTENT_BURST, b.intents + dt * INTENT_RATE);
    if (b.msgs >= 0 && b.bytes >= 0) return true;
    this.close(ws, 4005, 'too many messages');
    return false;
  }

  spendIntent(ws) {
    const b = this.buckets.get(ws);
    if (!b) return true;
    b.intents -= 1;
    if (b.intents >= 0) return true;
    this.close(ws, 4005, 'too many requests');
    return false;
  }

  // ---- Messages ----
  async webSocketMessage(ws, data) {
    const a = ws.deserializeAttachment();
    if (!a) return;
    this.count('messages');
    const size = typeof data === 'string' ? data.length : data.byteLength;
    if (!this.spend(ws, size)) return;
    const now = Date.now();
    if (a.slot >= 0) this.lastMsg.set(a.slot, now);
    if (typeof data !== 'string') {
      const d = decodeBinary(data);
      if (!d.ok) return this.badFrame(ws);
      if (a.slot < 0) return;
      if (d.type === 'state') this.onState(ws, a.slot, d.msg, new Uint8Array(data));
      // PROP needs a lease (Phase 5); until then glasses go through the world KV.
      this.maybeCheckGhosts(now);
      return;
    }
    const d = decodeText(data, 'toServer');
    if (!d.ok) return this.badFrame(ws);
    if (a.slot < 0) {
      if (d.letter === 'H') await this.hello(ws, a, d.body);
      return;
    }
    switch (d.letter) {
      case 'P': this.send(ws, text('P', { c: d.body.c, s: Date.now() })); break;
      case 'K': this.onKv(a.slot, d.body.w); break;
      case 'E': this.onEvents(a.slot, d.body.e); break;
      case 'G': this.onSignal(a.slot, d.body); break;
      case 'I': if (this.spendIntent(ws)) this.onIntent(ws, a.slot, d.body); break;
      case 'A': this.onAway(a.slot, d.body.on); break;
      case 'R': await this.onReauth(ws, a.slot, d.body.ticket); break;
      default: break;  // a second HELLO: ignored
    }
    this.maybeCheckGhosts(now);
    this.checkQuota();
  }

  badFrame(ws) {
    this.count('bad');
    this.spend(ws, 0, BAD_FRAME_COST);
  }

  // A STATE: keep the newest per player (by sequence number), mark it for the
  // next TICK, and remember it in the socket's attachment (hibernation).
  onState(ws, slot, msg, bytes) {
    const a = msg.avatar;
    if (Math.abs(a.x) > 40 || Math.abs(a.z) > 40 || a.y < -2 || a.y > 12) { this.count('refused'); return; }  // not in the bar
    const old = this.states.get(slot);
    if (old) {
      const ahead = (msg.seq - old.seq) & 0xffff;
      if (ahead === 0 || ahead >= 0x8000) return;  // older or repeated
    }
    const raw = bytes.slice(3);  // avatar block + activity, exactly as TICK carries them
    const moved = !old || bytesToHex(old.raw) !== bytesToHex(raw);
    this.states.set(slot, { seq: msg.seq, raw });
    if (!moved) return;
    this.lastActive.set(slot, Date.now());
    this.dirtyPlayers.add(slot);
    this.saveAttachment(slot);
    this.startTicker();
  }

  // Legacy shared world: each key gets a server sequence number; everyone,
  // the writer too, hears the result, so the last write wins everywhere.
  // Each write is checked against the world's rules (cloud/rules.js). A
  // refused write isn't stored or passed on; the writer gets the room's
  // current value back (so their copy snaps back), and a refusal counter.
  onKv(slot, w) {
    const out = [], back = [];
    for (const [key, values] of Object.entries(w)) {
      const r = this.checkKv(slot, key, values, false);
      if (!r.ok) {
        this.count('refused');
        const cur = this.kv.get(key);
        if (cur) back.push([cur.seq, key, cur.by, cur.values]);
        continue;
      }
      out.push(this.storeKv(slot, key, r.values));
    }
    if (back.length) this.sendTo(slot, text('K', { w: back.slice(0, 64) }));
    if (!out.length) return;
    this.lastActive.set(slot, Date.now());
    for (let i = 0; i < out.length; i += 64) this.broadcast(text('K', { w: out.slice(i, i + 64) }));
    this.scheduleFlush();
  }

  checkKv(slot, key, values, viaIntent) {
    if (!this.kv.has(key) && this.kv.size >= MAX_KV_KEYS) return { ok: false, why: 'world full' };
    const p = this.players.get(slot);
    return checkWrite(key, values, {
      slot, viaIntent, nowS: Date.now() / 1000, owner: (k) => (k ? this.claims.get(k) : undefined),
      badges: (p && p.badges) || [],
    });
  }

  storeKv(slot, key, values) {
    const entry = { seq: this.nextSeq(), by: slot, values };
    this.kv.set(key, entry);
    this.dirtyKv.add(key);
    if (key === 'jb') this.scheduleSongEnd();
    return [entry.seq, key, slot, values];
  }

  // The jukebox (cloud/games/jukebox.js): when the current song ends, the room
  // starts the next one for everyone. One deadline in the alarm scheduler.
  scheduleSongEnd() {
    const jb = this.kv.get('jb');
    this.setDeadline('song', jb ? jukebox.endsAtMs(jb.values, this.songTestSeconds) : 0);
  }

  nextSong() {
    const jb = this.kv.get('jb');
    if (!jb || !jukebox.endsAtMs(jb.values, this.songTestSeconds)) return;
    if (!this.connectedCount()) {  // nobody's listening: the jukebox goes quiet
      this.storeKv(-1, 'jb', [-1, 0]);
      this.scheduleFlush();
      return;
    }
    const entry = this.storeKv(-1, 'jb', jukebox.next(jb.values, Date.now()));
    this.broadcast(text('K', { w: [entry] }));
    this.scheduleFlush();
  }

  sendTo(slot, data) {
    const p = this.players.get(slot);
    if (p && p.ws) this.send(p.ws, data);
  }

  // Events: stamped and sent to the others with the next TICK.
  onEvents(slot, list) {
    if (!list.length) return;
    const tick = this.tick();
    for (const [type, args] of list) this.pendingEvents.push([tick, slot, this.nextSeq(), type, args]);
    this.lastActive.set(slot, Date.now());
    this.startTicker();
  }

  onSignal(slot, body) {
    const to = this.players.get(body.to);
    if (!to || !to.ws || body.to === slot) return;
    this.send(to.ws, text('G', { from: slot, data: body.data }));
  }

  // Intents: one RESULT per (slot, request id), so a resend after a
  // reconnect gets the same answer instead of acting twice.
  async onIntent(ws, slot, body) {
    let done = this.intentResults.get(slot);
    if (!done) { done = new Map(); this.intentResults.set(slot, done); }
    if (done.has(body.i)) {  // a resend: the same answer (or wait, if it's still being worked out)
      const prev = done.get(body.i);
      if (prev) this.send(ws, prev);
      return;
    }
    done.set(body.i, null);
    this.lastActive.set(slot, Date.now());
    // Most answers are immediate; a winning dart waits for the Ledger's payment.
    const result = await this.runIntent(slot, body);
    const reply = text('R', { r: body.i, ...result });
    done.set(body.i, reply);
    if (done.size > 64) done.delete(done.keys().next().value);
    this.send(ws, reply);
  }

  runIntent(slot, body) {
    const key = body.key;
    if (body.k === 'claim' || body.k === 'release') {
      if (typeof key !== 'string' || !CLAIM_KEY.test(key)) return { ok: 0, why: 'bad key' };
      if (/^glass\./.test(key)) return { ok: 0, why: 'glasses use grab' };
      const owner = this.claims.get(key);
      if (body.k === 'claim') {
        if (owner === slot) return { ok: 1, seq: this.lastSeq };
        if (owner !== undefined) return { ok: 0, why: 'taken', by: owner };
        if (this.claimsOf(slot).length >= MAX_CLAIMS_PER_PLAYER) return { ok: 0, why: 'too many claims' };
        this.claims.set(key, slot);
        const seq = this.nextSeq();
        this.broadcast(text('D', { d: [[seq, 'claim.' + key, slot, slot]] }));
        this.saveAttachment(slot);
        return { ok: 1, seq };
      }
      if (owner !== slot) return { ok: 0, why: 'not yours' };
      this.claims.delete(key);
      this.endGamesOf(slot, key);
      const seq = this.nextSeq();
      this.broadcast(text('D', { d: [[seq, 'claim.' + key, -1, slot]] }));
      this.saveAttachment(slot);
      return { ok: 1, seq };
    }
    if (body.k === 'dart') return this.throwDart(slot, body);
    if (body.k === 'race') return this.raceIntent(slot, body);
    if (body.k === 'grab' || body.k === 'drop' || body.k === 'throw') return this.glassIntent(slot, body);
    if (body.k === 'thing') return this.thingIntent(slot, body);
    if (body.k === 'set') {
      // A world write that only happens if the key still holds `expect`
      // (e.g. taking a drink: the first one to ask gets it).
      const values = body.v, expect = body.expect;
      if (typeof key !== 'string' || !/^[A-Za-z0-9_]{1,16}$/.test(key) || !Array.isArray(values) || values.length > 20) return { ok: 0, why: 'bad key' };
      const cur = this.kv.get(key);
      if (Array.isArray(expect) && JSON.stringify(cur ? cur.values : []) !== JSON.stringify(expect)) {
        return cur && cur.by >= 0 ? { ok: 0, why: 'changed', by: cur.by } : { ok: 0, why: 'changed' };
      }
      const r = this.checkKv(slot, key, values, true);
      if (!r.ok) { this.count('refused'); return { ok: 0, why: r.why.slice(0, 64) }; }
      const entry = this.storeKv(slot, key, r.values);
      this.broadcast(text('K', { w: [entry] }));
      this.scheduleFlush();
      return { ok: 1, seq: entry[0] };
    }
    return { ok: 0, why: 'unknown intent' };
  }

  // Darts (cloud/games/darts.js): only the player the room gave the board to
  // throws; the room adds the wobble, scores it and tells everyone.
  throwDart(slot, body) {
    if (this.claims.get('machine.darts') !== slot) return { ok: 0, why: 'not your turn at the board' };
    if (!Number.isFinite(body.u) || !Number.isFinite(body.v)) return { ok: 0, why: 'bad aim' };
    if (!this.games.darts || this.games.darts.owner !== slot) this.games.darts = { owner: slot, session: darts.newSession() };
    const session = this.games.darts.session;
    const gameId = session.gameId;
    const r = darts.throwDart(session, body.u, body.v);
    this.saveAttachment(slot);
    this.broadcast(text('D', { d: [[this.nextSeq(), 'darts', r, slot]] }), slot);
    this.roomEvent('dartboard', 'hit');  // the board shudders for everyone (E3.5)
    if (!r.won) return { ok: 1, ...r };
    // A finished game pays (logged-in players): the answer carries what the
    // Ledger actually credited (the daily cap can make it less).
    return this.pay(slot, darts.payout(r.thrown), 'darts: 301 in ' + r.thrown + ' darts', 'darts-' + gameId)
      .then((paid) => ({ ok: 1, ...r, ...paid }));
  }

  // Credits a player's account through the Ledger (cloud/ledger.js). Returns
  // {paid: cents, balance, capped} or {guest: 1} / {payError: 1}.
  async pay(slot, cents, reason, ref) {
    const p = this.players.get(slot);
    if (!p || !p.acct || !this.env.LEDGER) return { guest: 1 };
    try {
      const ledger = this.env.LEDGER.get(this.env.LEDGER.idFromName('main'));
      const r = await ledger.fetch(new Request('https://ledger/', {
        method: 'POST', body: JSON.stringify({ a: 'credit', user: p.acct, cents, reason, ref }),
      }));
      const j = await r.json();
      if (!j.ok) return { payError: 1 };
      return { paid: j.duplicate ? 0 : (j.credited | 0), balance: j.balance | 0, capped: j.capped ? 1 : 0 };
    } catch (e) {
      return { payError: 1 };
    }
  }

  // The racing rigs (cloud/games/race.js): garage, ready, finish. Being on a
  // rig is the claim machine.rig0 / machine.rig1.
  raceIntent(slot, body) {
    const r = race.act(this.race, slot, body, Date.now(), (rig) => this.claims.get('machine.rig' + rig) === slot);
    if (r.changed) this.raceChanged();
    const { changed, ...out } = r;
    return out;
  }

  raceChanged() {
    race.maybeStart(this.race, Date.now());
    const entry = this.storeKv(-1, 'rc', race.values(this.race));
    this.broadcast(text('K', { w: [entry] }));
    this.scheduleFlush();
    this.setDeadline('race', race.deadline(this.race));
  }

  // Glasses (cloud/games/glasses.js): grab, put down (`pos`) or throw (`vel`).
  // The room's physics moves them; holding one is the claim `glass.<n>`.
  // Things (docs/ENGINE.md E3): a player touches a thing; the room runs the
  // bar's own rules (the same files and C++ as the phones) and shares every
  // state change it makes as world key t<net> = [state, start (server s)],
  // written by the room (-1), so every phone ends on the room's word.
  thingIntent(slot, body) {
    const n = body.n, e = body.e;
    if (!Number.isInteger(n) || n < 1 || n > 65535 || typeof e !== 'string' || !/^[a-z-]{1,16}$/.test(e)) return { ok: 0, why: 'bad thing' };
    if (e !== 'tap') return { ok: 0, why: 'players can only touch things' };
    this.syncThings();
    const at = this.sim.thingAt(n);
    if (!at) return { ok: 0, why: 'no such thing' };
    const eyes = this.eyes(slot);
    if (!eyes || Math.hypot(eyes.x - at[0], eyes.y - at[1], eyes.z - at[2]) > THING_REACH) return { ok: 0, why: 'too far' };
    const n2 = this.shareThingEvent(n, e);
    return n2 ? { ok: 1, n: n2 } : { ok: 0, why: 'nothing happens' };
  }

  // The stored thing states handed to the room's world (once, after waking).
  syncThings() {
    if (this.thingsSynced) return;
    for (const [key, entry] of this.kv) {
      const m = /^t(\d{1,5})$/.exec(key);
      if (m && entry.values.length === 2) this.sim.thingApply(+m[1], entry.values[0], entry.values[1]);
    }
    this.thingsSynced = true;
  }

  // Runs event `e` on thing `n` in the room's world and shares every change
  // (world keys t<net>, by -1). → how many things changed.
  shareThingEvent(n, e) {
    const changes = this.sim.thingEvent(n, e, Date.now() / 1000);
    if (!changes.length) return 0;
    const entries = changes.map(([net, state, start]) => this.storeKv(-1, 't' + net, [state, start]));
    for (let i = 0; i < entries.length; i += 64) this.broadcast(text('K', { w: entries.slice(i, i + 64) }));
    this.scheduleFlush();
    return entries.length;
  }

  // The room's own games tell things what happened (a dart hit the board).
  roomEvent(id, e) {
    this.syncThings();
    const n = this.sim.thingNet(id);
    if (n) this.shareThingEvent(n, e);
  }

  glassIntent(slot, body) {
    const g = body.g, key = 'glass.' + g;
    const eyes = this.eyes(slot);
    const r = body.k === 'grab' ? this.glasses.grab(g, slot, eyes)
      : this.glasses.letGo(g, slot, eyes, body.k === 'drop' ? { pos: body.pos } : { vel: body.vel });
    if (!r.ok) return r;
    const was = this.claims.get(key);
    if (body.k === 'grab') this.claims.set(key, slot); else this.claims.delete(key);
    const now = this.claims.get(key);
    if (was !== now) this.broadcast(text('D', { d: [[this.nextSeq(), 'claim.' + key, now === undefined ? -1 : now, slot]] }));
    this.saveAttachment(slot);
    this.lastSimMs = this.lastSimMs || Date.now();
    this.startTicker();
    return { ok: 1, seq: this.lastSeq };
  }

  eyes(slot) {
    const st = this.states.get(slot);
    return st ? eyesOf(st.raw) : null;
  }

  // Leaving a machine ends your game on it (the next player starts fresh).
  endGamesOf(slot, key) {
    if (key === 'machine.darts' && this.games.darts && this.games.darts.owner === slot) delete this.games.darts;
    if (/^machine\.rig[01]$/.test(key) && this.race.rigs.some((e) => e && e.slot === slot)) {
      race.leave(this.race, slot);
      this.raceChanged();
    }
  }

  claimsOf(slot) {
    const out = [];
    for (const [k, s] of this.claims) if (s === slot) out.push(k);
    return out;
  }

  // Frees every claim a player holds (they left), telling everyone.
  releaseAll(slot) {
    if (this.glasses.drop(slot).length) { this.lastSimMs = this.lastSimMs || Date.now(); this.startTicker(); }
    const d = [];
    for (const key of this.claimsOf(slot)) {
      this.claims.delete(key);
      this.endGamesOf(slot, key);
      d.push([this.nextSeq(), 'claim.' + key, -1, -1]);
    }
    for (let i = 0; i < d.length; i += 64) this.broadcast(text('D', { d: d.slice(i, i + 64) }));
  }

  onAway(slot, on) {
    const p = this.players.get(slot);
    if (!p || (p.away ? 1 : 0) === on) return;
    p.away = on;
    if (!on) this.lastActive.set(slot, Date.now());
    this.saveAttachment(slot);
    this.broadcast(text('D', { d: [[this.nextSeq(), 'away', on, slot]] }), slot);
  }

  async onReauth(ws, slot, ticket) {
    const who = await this.checkToken(ticket);
    const p = this.players.get(slot);
    if (!p || p.ws !== ws) return;
    if (PRIVATE_ROOMS.has(this.name) && (!who || !who.play)) { this.close(ws, 4006, 'invite only'); return; }
    if (who) Object.assign(p, { acct: who.acct, name: who.name, av: who.av, badges: who.badges || [] });
    else Object.assign(p, { acct: '', av: 0, badges: [] });
    this.saveAttachment(slot);
    const f = { slot, name: p.name };
    if (p.acct) { f.acct = p.acct; f.av = p.av; }
    this.broadcast(text('F', f));
  }

  async hello(ws, a, h) {
    if (h.proto < this.proto - 1 || h.proto > this.proto) {
      this.close(ws, 4002, `protocol mismatch: the room speaks ${this.proto}`);
      return;
    }
    if (PRIVATE_ROOMS.has(this.name)) {
      const who = h.ticket ? await this.checkToken(h.ticket) : null;
      if (ws.readyState !== 1) return;  // it closed while we asked: no ghost
      if (!who || !who.play) { this.close(ws, 4006, 'invite only'); return; }
    }
    // Same resume token → same slot (the old socket, if still open, is replaced).
    let slot = -1, resumed = false;
    if (h.resume) {
      for (const p of this.players.values()) {
        if (p.resume !== h.resume) continue;
        slot = p.slot;
        resumed = true;
        if (p.ws && p.ws !== ws) {
          const old = p.ws;
          p.ws = null;  // so its close doesn't start a grace period
          this.close(old, 4003, 'replaced by a newer connection');
        }
      }
    }
    if (slot < 0) {
      for (let s = 0; s < MAX_SLOTS; s++) if (!this.players.has(s)) { slot = s; break; }
    }
    if (slot < 0) {
      this.close(ws, 4001, `room full (${MAX_SLOTS} players)`);
      return;
    }
    // A room that stood empty for a long time starts with a fresh world
    // (glasses back in place, no song half-played from yesterday).
    if (this.emptySince && Date.now() - this.emptySince > WORLD_FORGET_MS && !this.connectedCount()) await this.forgetWorld();
    if (this.emptySince) { this.emptySince = 0; await this.ctx.storage.delete('emptySince'); }

    const wasInGrace = resumed && this.players.get(slot).graceUntil > 0;
    const p = { slot, name: h.name, look: h.look, acct: '', av: 0, resume: randomHex(16), proto: h.proto, graceUntil: 0, away: 0 };
    if (resumed) {  // keep what the login check already found
      const old = this.players.get(slot);
      Object.assign(p, { acct: old.acct, av: old.av, badges: old.badges || [], name: old.acct ? old.name : h.name });
    } else {
      this.states.delete(slot);
      this.intentResults.delete(slot);
    }
    p.ws = ws;
    this.players.set(slot, p);
    const now = Date.now();
    this.lastMsg.set(slot, now);
    this.lastActive.set(slot, now);
    this.saveAttachment(slot);
    if (wasInGrace) await this.saveGrace();
    this.updateHelloDeadline();

    const welcome = {
      proto: this.proto, slot, resume: p.resume, serverMs: now, tick: this.tick(), tickHz: 1000 / TICK_MS,
      room: this.name || PERSISTENT_ROOM, roster: [...this.players.values()].map((q) => this.player(q)),
      claims: Object.fromEntries(this.claims), sessions: {},
      kv: [...this.kv.entries()].map(([k, e]) => [e.seq, k, e.by, e.values]),
    };
    this.send(ws, text('W', welcome));
    if (this.stateHz) this.send(ws, text('D', { d: [[this.nextSeq(), 'net.stateHz', this.stateHz, -1]] }));
    // Everyone's current pose, so the newcomer sees the room at once.
    const snapshot = this.tickFrame([...this.states.keys()].filter((s) => s !== slot), []);
    if (snapshot) this.send(ws, snapshot);
    if (!resumed) this.broadcast(text('J', this.player(p)), slot);
    else if (wasInGrace) this.broadcast(text('D', { d: [[this.nextSeq(), 'away', 0, slot]] }), slot);

    // Guest first; if the login checks out, everyone gets the real profile.
    if (h.ticket) {
      const who = await this.checkToken(h.ticket);
      const cur = this.players.get(slot);
      if (who && cur && cur.ws === ws) {
        Object.assign(cur, { acct: who.acct, name: who.name, av: who.av, badges: who.badges || [] });
        this.saveAttachment(slot);
        this.broadcast(text('F', { slot, name: cur.name, acct: cur.acct, av: cur.av }));
      }
    }
  }

  connectedCount() { return [...this.players.values()].filter((p) => p.ws).length; }

  publicFields(p) { return { slot: p.slot, name: p.name, look: p.look, acct: p.acct, av: p.av, badges: p.badges || [] }; }

  // What survives hibernation for a connected player: their public fields,
  // resume token, last STATE and claims, in the socket's attachment.
  saveAttachment(slot) {
    const p = this.players.get(slot);
    if (!p || !p.ws) return;
    const st = this.states.get(slot);
    try {
      p.ws.serializeAttachment({ room: this.name, ...this.publicFields(p), resume: p.resume, proto: p.proto, away: p.away || 0,
        st: st ? { seq: st.seq, raw: bytesToHex(st.raw) } : null, claims: this.claimsOf(slot),
        darts: this.games.darts && this.games.darts.owner === slot ? this.games.darts.session : null });
    } catch (e) {}
  }

  player(p) {
    const out = { slot: p.slot, name: p.name, look: p.look };
    if (p.acct) out.acct = p.acct;
    if (p.av) out.av = p.av;
    if (p.graceUntil > 0 || p.away) out.away = 1;
    return out;
  }

  tick() { return Math.floor(Date.now() / TICK_MS) % 0x100000000; }

  // ---- The 20 Hz ticker (only while something changes) ----
  startTicker() {
    this.lastChangeMs = Date.now();
    if (this.ticker) return;
    this.ticker = setInterval(() => this.onTick(), TICK_MS);
    this.setDeadline('ghost', 0);  // checked by the ticker while it runs
  }

  onTick() {
    const now = Date.now();
    const slots = [...this.dirtyPlayers];
    this.dirtyPlayers.clear();
    // The glasses' physics: hands follow their holders, thrown ones fly.
    let props = [], rested = [];
    if (this.glasses.busy()) {
      const dt = this.lastSimMs ? (now - this.lastSimMs) / 1000 : TICK_MS / 1000;
      const out = this.glasses.tick(dt, (s) => this.eyes(s));
      props = out.samples.map((x) => propBytes(x)).filter(Boolean);
      rested = out.rested;
      this.lastChangeMs = now;  // keep ticking while anything moves
    }
    this.lastSimMs = now;
    if (slots.length || props.length) {
      const frame = this.tickFrame(slots, props);
      if (frame) this.broadcastBinary(frame, props.length ? [] : slots);
    }
    // After the TICK with their last poses: the glasses that came to rest
    // (so a phone's last word about each is "resting here").
    for (const r of rested) this.broadcast(text('K', { w: [this.storeKv(-1, 'g' + r.g, r.values)] }));
    if (rested.length) this.scheduleFlush();
    if (this.pendingEvents.length) {
      const events = this.pendingEvents;
      this.pendingEvents = [];
      for (let i = 0; i < events.length; i += 64) {
        const chunk = events.slice(i, i + 64);
        for (const p of this.players.values()) {
          if (!p.ws) continue;
          const mine = chunk.filter((e) => e[1] !== p.slot);  // the sender already played it
          if (mine.length) this.send(p.ws, text('E', { e: mine }));
        }
      }
    }
    this.maybeCheckGhosts(now);
    if (slots.length || now - this.lastChangeMs < IDLE_STOP_MS) return;
    clearInterval(this.ticker);
    this.ticker = null;
    this.armIdleGhostCheck();
  }

  // A TICK frame: tick, server time, the given players' latest blocks, and
  // prop samples (18 bytes each, from propBytes).
  tickFrame(slots, props) {
    const rows = slots.map((s) => [s, this.states.get(s)]).filter(([, st]) => st).slice(0, MAX_SLOTS);
    props = props.slice(0, 32);
    if (!rows.length && !props.length) return null;
    let size = 1 + 4 + 8 + 1 + 1 + props.length * 18;
    for (const [, st] of rows) size += 1 + st.raw.length;
    const out = new Uint8Array(size);
    const dv = new DataView(out.buffer);
    let p = 0;
    out[p++] = BIN.TICK;
    dv.setUint32(p, this.tick(), true); p += 4;
    dv.setFloat64(p, Date.now(), true); p += 8;
    out[p++] = rows.length;
    for (const [s, st] of rows) { out[p++] = s; out.set(st.raw, p); p += st.raw.length; }
    out[p++] = props.length;
    for (const b of props) { out.set(b, p); p += 18; }
    return out;
  }

  // ---- Ghosts and AFK ----
  maybeCheckGhosts(now) {
    if (now - this.lastGhostCheck < 5000) return;
    this.lastGhostCheck = now;
    this.checkGhosts(now);
    // A grace that ran out leaves here too: the alarm can come late on a busy machine.
    for (const p of [...this.players.values()]) {
      if (p.graceUntil > 0 && now >= p.graceUntil) this.leave(p.slot, 'connection lost').catch(() => {});
    }
  }

  checkGhosts(now) {
    for (const p of [...this.players.values()]) {
      if (!p.ws) continue;
      let last = this.lastMsg.get(p.slot) || 0;
      try {
        const auto = this.ctx.getWebSocketAutoResponseTimestamp(p.ws);
        if (auto) last = Math.max(last, auto.getTime());
      } catch (e) {}
      if (now - last > this.ghostMs) {
        this.close(p.ws, 4004, 'no traffic');
        this.lostSocket(p);
      } else if (!p.away && now - (this.lastActive.get(p.slot) || now) > this.afkMs) {
        this.close(p.ws, 4004, 'away, tap to rejoin');
        this.lostSocket(p);
      }
    }
  }

  // While nobody moves, one alarm a minute checks for ghosts (§2.5.9).
  armIdleGhostCheck() {
    this.setDeadline('ghost', this.connectedCount() && !this.ticker ? Date.now() + this.ghostCheckMs : 0);
  }

  // ---- Leaving ----
  async webSocketClose(ws, code, reason) {
    this.close(ws, code === 1000 || (code >= 3000 && code <= 4999) ? code : 1000, reason);  // reciprocate
    await this.gone(ws, code);
  }

  async webSocketError(ws) { await this.gone(ws, 1006); }

  async gone(ws, code) {
    this.buckets.delete(ws);
    const a = ws.deserializeAttachment();
    this.updateHelloDeadline();
    if (!a || a.slot < 0) return this.checkEmpty();
    const p = this.players.get(a.slot);
    if (!p || p.ws !== ws) return this.checkEmpty();  // replaced: the slot lives on
    if (code === 1000) {  // a deliberate goodbye: free the slot now
      await this.leave(a.slot, 'left');
      return;
    }
    this.lostSocket(p);
  }

  // The socket died (or was closed as a ghost): keep the slot for the grace
  // period, shown as away to the others.
  lostSocket(p) {
    if (!p.ws) return;
    p.ws = null;
    p.graceUntil = Date.now() + this.graceMs;
    this.broadcast(text('D', { d: [[this.nextSeq(), 'away', 1, p.slot]] }));
    this.saveGrace();
    this.reschedule();
    this.armIdleGhostCheck();
  }

  async leave(slot, why) {
    if (!this.players.delete(slot)) return;
    this.states.delete(slot);
    this.dirtyPlayers.delete(slot);
    this.lastMsg.delete(slot);
    this.lastActive.delete(slot);
    this.intentResults.delete(slot);
    this.releaseAll(slot);
    this.broadcast(text('L', { slot, why }));
    await this.saveGrace();
    await this.checkEmpty();
  }

  async checkEmpty() {
    this.armIdleGhostCheck();
    if (this.players.size || this.ctx.getWebSockets().some((w) => w.readyState === 1)) return;
    if (this.ticker) { clearInterval(this.ticker); this.ticker = null; }
    // Only "main" keeps anything; other rooms leave no trace.
    if (this.name && !PERSISTENT_ROOMS.has(this.name)) {
      await this.ctx.storage.deleteAll();
      this.schemaMarked = false;
      this.kv.clear();
      this.race = race.empty();
      this.dirtyKv.clear();
      this.deadlines.clear();
      this.alarmAt = 0;
      return;
    }
    await this.flush();
    this.emptySince = Date.now();
    await this.ctx.storage.put('emptySince', this.emptySince);
    this.count('rows');
  }

  async forgetWorld() {
    const keys = [...this.kv.keys()].map((k) => 'kv:' + k);
    this.kv.clear();
    this.race = race.empty();
    this.dirtyKv.clear();
    for (let i = 0; i < keys.length; i += 128) await this.ctx.storage.delete(keys.slice(i, i + 128));
  }

  async saveGrace() {
    const grace = {};
    for (const p of this.players.values()) {
      if (p.graceUntil > 0) grace[p.slot] = { ...this.publicFields(p), resume: p.resume, graceUntil: p.graceUntil, claims: this.claimsOf(p.slot) };
    }
    if (Object.keys(grace).length) await this.ctx.storage.put('grace', grace);
    else await this.ctx.storage.delete('grace');
    this.count('rows');
    await this.markSchema();
  }

  async markSchema() {
    if (!this.schemaMarked) {
      await this.ctx.storage.put('schemaVersion', SCHEMA_VERSION);
      this.schemaMarked = true;
    }
  }

  // ---- Saving the world (debounced: changed keys only, ≤ once per 2 s) ----
  // Every room saves its world (the room can be put to sleep and woken up at
  // any time); rooms other than "main" are erased when the last player leaves.
  scheduleFlush() {
    if (!this.deadlines.has('flush')) this.setDeadline('flush', Date.now() + FLUSH_MS);
  }

  async flush() {
    this.setDeadline('flush', 0);
    const put = {};
    for (const key of this.dirtyKv) {
      const e = this.kv.get(key);
      if (e) put['kv:' + key] = e;
    }
    this.dirtyKv.clear();
    if (Date.now() - this.usageSavedAt > USAGE_SAVE_MS || !this.players.size) {
      put.usage = this.usage;
      this.usageSavedAt = Date.now();
    }
    const keys = Object.keys(put);
    if (!keys.length) return;
    for (let i = 0; i < keys.length; i += 128) {
      const part = {};
      for (const k of keys.slice(i, i + 128)) part[k] = put[k];
      await this.ctx.storage.put(part);
    }
    this.count('rows', keys.length);
    await this.markSchema();
  }

  // ---- The one alarm ----
  // Every timed job is a named deadline; the alarm is set to the earliest.
  // Nothing else calls setAlarm.
  updateHelloDeadline() {
    let first = 0;
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.slot < 0 && ws.readyState < 2) first = first ? Math.min(first, a.openedMs + HELLO_MS) : a.openedMs + HELLO_MS;
    }
    this.setDeadline('hello', first);
  }

  setDeadline(name, ms) {
    if (ms) this.deadlines.set(name, ms);
    else this.deadlines.delete(name);
    this.reschedule();
  }

  reschedule() {
    let grace = 0;
    for (const p of this.players.values()) if (p.graceUntil > 0) grace = grace ? Math.min(grace, p.graceUntil) : p.graceUntil;
    if (grace) this.deadlines.set('grace', grace);
    else this.deadlines.delete('grace');
    const next = this.deadlines.size ? Math.min(...this.deadlines.values()) : 0;
    if (next === this.alarmAt) return;
    this.alarmAt = next;
    if (next) this.ctx.storage.setAlarm(next);
    else this.ctx.storage.deleteAlarm();
  }

  async alarm() {
    this.alarmAt = 0;
    this.count('alarms');
    const now = Date.now();
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.slot < 0 && now >= a.openedMs + HELLO_MS) this.close(ws, 4004, 'no hello within 5 s');
    }
    for (const p of [...this.players.values()]) {
      if (p.graceUntil > 0 && now >= p.graceUntil) await this.leave(p.slot, 'connection lost');
    }
    if ((this.deadlines.get('song') || Infinity) <= now) {
      this.deadlines.delete('song');
      this.nextSong();
    }
    if ((this.deadlines.get('race') || Infinity) <= now) {
      this.deadlines.delete('race');
      if (race.check(this.race, now)) this.raceChanged();
      else this.setDeadline('race', race.deadline(this.race));
    }
    if ((this.deadlines.get('flush') || Infinity) <= now) await this.flush();
    if ((this.deadlines.get('ghost') || Infinity) <= now) {
      this.deadlines.delete('ghost');
      this.checkGhosts(now);
      this.armIdleGhostCheck();
    }
    this.updateHelloDeadline();
    this.reschedule();
  }

  // ---- Helpers ----
  send(ws, data) { try { ws.send(data); } catch (e) {} }

  broadcast(data, exceptSlot = -1) {
    for (const p of this.players.values()) if (p.ws && p.slot !== exceptSlot) this.send(p.ws, data);
  }

  // A TICK goes to everyone except players who'd only get their own block.
  broadcastBinary(frame, slots) {
    for (const p of this.players.values()) {
      if (!p.ws) continue;
      if (slots.length === 1 && slots[0] === p.slot) continue;
      this.send(p.ws, frame);
    }
  }

  close(ws, code, reason) { try { ws.close(code, reason); } catch (e) {} }

  // Who a login token belongs to (accounts on IONOS until 5.1). 2 s timeout,
  // answers cached 10 minutes. Dev builds accept "test:<acct>".
  async checkToken(tok) {
    const now = Date.now();
    const cached = this.tokens.get(tok);
    if (cached && cached.until > now) return cached.who;
    let who = null;
    if (this.dev && tok.startsWith('test:')) {
      const [acct, ...badges] = tok.slice(5).split(':');  // test:<acct>[:badge...]
      who = { acct: clip(acct, 32), name: clip(acct, 24) || 'Tester', av: 1, badges, play: !badges.includes('noplay') };
    } else if (/^[0-9a-f]{48}$/.test(tok) && this.env.ACCOUNT_URL) {
      try {
        const r = await fetch(this.env.ACCOUNT_URL, {
          method: 'POST',
          headers: { 'Content-Type': 'text/plain' },
          body: JSON.stringify({ a: 'me', token: tok }),
          signal: AbortSignal.timeout(TOKEN_TIMEOUT_MS),
        });
        const j = await r.json();
        if (j && j.ok && j.me) {
          who = { acct: clip(String(j.me.id), 32), name: clip(String(j.me.name), 24) || 'Guest', av: Math.max(0, j.me.avatar | 0),
            badges: Array.isArray(j.me.badges) ? j.me.badges.filter((b) => typeof b === 'string').slice(0, 16).map((b) => clip(b, 16)) : [],
            play: j.me.play === 1 };
        }
      } catch (e) {}
    }
    if (this.tokens.size > 500) this.tokens.clear();
    // A failed check isn't remembered (IONOS was slow: try again next time);
    // "not invited yet" only briefly, so a fresh invite works within a minute.
    if (who) this.tokens.set(tok, { who, until: now + (who.play ? TOKEN_CACHE_MS : 30000) });
    return who;
  }
}
