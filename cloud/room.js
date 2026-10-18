// One Bar3D room (a Cloudflare Durable Object): the authority for who is in
// the room and in which slot. Protocol: docs/PROTOCOL.md (codec in proto.js).
//
// Built so far (restructure 3.2a): slots and resume tokens, HELLO → WELCOME,
// guest first then an async login check → PROFILE, close codes 4001-4004,
// PING/PONG, the 5 s hello timeout, a 30 s grace when a socket drops, the
// storage schemaVersion, and the one-alarm scheduler. 3.2b adds STATE/TICK,
// events, signals and the legacy world; 3.2c ghosts, rate limits, persistence.
//
// Hibernation: the room may be evicted from memory between messages. Who is
// connected is rebuilt from the sockets' attachments; players in their grace
// period are kept in storage (key `grace`), so nothing lives only in memory.
import { PROTO, MAX_SLOTS, decodeText, decodeBinary } from './proto.js';

const TICK_MS = 50;
const HELLO_MS = 5000;
const SCHEMA_VERSION = 1;  // docs/PROTOCOL.md §9: bump when stored keys change shape
const TOKEN_CACHE_MS = 10 * 60 * 1000;
const TOKEN_TIMEOUT_MS = 2000;
const PERSISTENT_ROOM = 'main';
const MAX_SOCKETS = MAX_SLOTS * 2;  // players + sockets still saying hello

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

export class Room {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    this.dev = env.DEV === '1';
    this.proto = Number(env.PROTO) || PROTO;
    this.graceMs = Number(env.GRACE_MS) || 30000;
    this.connectLimit = Number(env.CONNECT_LIMIT_PER_MIN) || 10;
    this.name = '';
    this.players = new Map();    // slot → {slot, name, look, acct, av, resume, ws|null, graceUntil}
    this.deadlines = new Map();  // job name → ms; the one alarm is set to the earliest
    this.alarmAt = 0;            // what the alarm is set to now (0 = none)
    this.ipHits = new Map();     // ip → connect times in the last minute
    this.tokens = new Map();     // login token → {acct, name, av, until}
    // A plain "ping" text is answered by the runtime without waking the room
    // (the idle keepalive, §2.5.9). It isn't a protocol message.
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair('ping', 'pong'));
    for (const ws of ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (!a) continue;
      if (a.room) this.name = a.room;
      if (a.slot >= 0) this.players.set(a.slot, { ...a, ws, graceUntil: 0 });
    }
    ctx.blockConcurrencyWhile(() => this.load());
  }

  async load() {
    const s = this.ctx.storage;
    const version = await s.get('schemaVersion');
    if (version !== SCHEMA_VERSION && (version !== undefined || (await s.list({ limit: 1 })).size)) {
      console.log(`room: storage schema ${version} != ${SCHEMA_VERSION}, wiped to defaults`);
      await s.deleteAll();
    }
    const grace = (await s.get('grace')) || {};
    for (const p of Object.values(grace)) {
      if (!this.players.has(p.slot)) this.players.set(p.slot, { ...p, ws: null });
    }
    this.alarmAt = (await s.getAlarm()) || 0;
    this.reschedule();
  }

  // ---- HTTP: the WebSocket upgrade (the Worker already checked the Origin) ----
  async fetch(request) {
    const url = new URL(request.url);
    this.name = request.headers.get('X-Room') || this.name;
    if (this.dev && url.pathname.endsWith('/debug')) {
      const keys = await this.ctx.storage.list();
      return Response.json({ storageKeys: [...keys.keys()], slots: [...this.players.keys()], alarmAt: this.alarmAt });
    }
    if (request.headers.get('Upgrade') !== 'websocket') return new Response('expected a websocket', { status: 426 });
    const ip = (this.dev && request.headers.get('X-Test-IP')) || request.headers.get('CF-Connecting-IP') || 'local';
    if (!this.allowConnect(ip)) return new Response('too many connections, wait a minute', { status: 429 });
    // Sockets that haven't said HELLO count too (they're closed after 5 s).
    if (this.ctx.getWebSockets().length >= MAX_SOCKETS) return new Response('room busy', { status: 503 });
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

  // ---- Messages ----
  async webSocketMessage(ws, data) {
    const a = ws.deserializeAttachment();
    if (!a) return;
    if (typeof data !== 'string') {
      const d = decodeBinary(data);
      if (!d.ok || a.slot < 0) return;  // STATE/PROP handling: 3.2b
      return;
    }
    const d = decodeText(data, 'toServer');
    if (!d.ok) return;  // 3.2c: counted against the rate limit (4005)
    if (a.slot < 0) {
      if (d.letter === 'H') await this.hello(ws, a, d.body);
      return;
    }
    if (d.letter === 'P') this.send(ws, text('P', { c: d.body.c, s: Date.now() }));
  }

  async hello(ws, a, h) {
    if (h.proto < this.proto - 1 || h.proto > this.proto) {
      this.close(ws, 4002, `protocol mismatch: the room speaks ${this.proto}`);
      return;
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
    const wasInGrace = resumed && this.players.get(slot).graceUntil > 0;
    const p = { slot, name: h.name, look: h.look, acct: '', av: 0, resume: randomHex(16), proto: h.proto, graceUntil: 0 };
    if (resumed) {  // keep what the login check already found
      const old = this.players.get(slot);
      Object.assign(p, { acct: old.acct, av: old.av, name: old.acct ? old.name : h.name });
    }
    p.ws = ws;
    this.players.set(slot, p);
    ws.serializeAttachment({ room: this.name, ...this.publicFields(p), resume: p.resume, proto: p.proto });
    if (wasInGrace) await this.saveGrace();
    this.updateHelloDeadline();

    this.send(ws, text('W', {
      proto: this.proto, slot, resume: p.resume, serverMs: Date.now(), tick: this.tick(), tickHz: 1000 / TICK_MS,
      room: this.name || 'main', roster: [...this.players.values()].map((q) => this.player(q)), claims: {}, sessions: {},
    }));
    if (!resumed) this.broadcast(text('J', this.player(p)), slot);

    // Guest first; if the login checks out, everyone gets the real profile.
    if (h.ticket) {
      const who = await this.checkToken(h.ticket);
      const now = this.players.get(slot);
      if (who && now && now.ws === ws) {
        Object.assign(now, { acct: who.acct, name: who.name, av: who.av });
        ws.serializeAttachment({ room: this.name, ...this.publicFields(now), resume: now.resume, proto: now.proto });
        this.broadcast(text('F', { slot, name: now.name, acct: now.acct, av: now.av }));
      }
    }
  }

  publicFields(p) { return { slot: p.slot, name: p.name, look: p.look, acct: p.acct, av: p.av }; }

  player(p) {
    const out = { slot: p.slot, name: p.name, look: p.look };
    if (p.acct) out.acct = p.acct;
    if (p.av) out.av = p.av;
    if (p.graceUntil > 0) out.away = 1;
    return out;
  }

  tick() { return Math.floor(Date.now() / TICK_MS) % 0x100000000; }

  // ---- Leaving ----
  async webSocketClose(ws, code, reason) {
    this.close(ws, code === 1000 || (code >= 3000 && code <= 4999) ? code : 1000, reason);  // reciprocate
    await this.gone(ws, code);
  }

  async webSocketError(ws) { await this.gone(ws, 1006); }

  async gone(ws, code) {
    const a = ws.deserializeAttachment();
    this.updateHelloDeadline();
    if (!a || a.slot < 0) return this.checkEmpty();
    const p = this.players.get(a.slot);
    if (!p || p.ws !== ws) return this.checkEmpty();  // replaced: the slot lives on
    if (code === 1000) {  // a deliberate goodbye: free the slot now
      await this.leave(a.slot, 'left');
      return;
    }
    p.ws = null;
    p.graceUntil = Date.now() + this.graceMs;
    await this.saveGrace();
    this.reschedule();
  }

  async leave(slot, why) {
    if (!this.players.delete(slot)) return;
    this.broadcast(text('L', { slot, why }));
    await this.saveGrace();
    await this.checkEmpty();
  }

  async checkEmpty() {
    if (this.players.size || this.ctx.getWebSockets().some((w) => w.readyState === 1)) return;
    // Only "main" keeps anything; other rooms leave no trace.
    if (this.name && this.name !== PERSISTENT_ROOM) {
      await this.ctx.storage.deleteAll();
      this.deadlines.clear();
      this.alarmAt = 0;
    }
  }

  async saveGrace() {
    const grace = {};
    for (const p of this.players.values()) {
      if (p.graceUntil > 0) grace[p.slot] = { ...this.publicFields(p), resume: p.resume, graceUntil: p.graceUntil };
    }
    if (Object.keys(grace).length) await this.ctx.storage.put('grace', grace);
    else await this.ctx.storage.delete('grace');
    await this.markSchema();
  }

  async markSchema() {
    if (this.name === PERSISTENT_ROOM) await this.ctx.storage.put('schemaVersion', SCHEMA_VERSION);
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
    const now = Date.now();
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.slot < 0 && now >= a.openedMs + HELLO_MS) this.close(ws, 4004, 'no hello within 5 s');
    }
    for (const p of [...this.players.values()]) {
      if (p.graceUntil > 0 && now >= p.graceUntil) await this.leave(p.slot, 'connection lost');
    }
    this.updateHelloDeadline();
    this.reschedule();
  }

  // ---- Helpers ----
  send(ws, data) { try { ws.send(data); } catch (e) {} }

  broadcast(data, exceptSlot = -1) {
    for (const p of this.players.values()) if (p.ws && p.slot !== exceptSlot) this.send(p.ws, data);
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
      who = { acct: clip(tok.slice(5), 32), name: clip(tok.slice(5), 24) || 'Tester', av: 1 };
    } else if (/^[0-9a-f]{48}$/.test(tok) && this.env.ACCOUNT_URL) {
      try {
        const r = await fetch(this.env.ACCOUNT_URL, {
          method: 'POST',
          headers: { 'Content-Type': 'text/plain' },
          body: JSON.stringify({ a: 'me', token: tok }),
          signal: AbortSignal.timeout(TOKEN_TIMEOUT_MS),
        });
        const j = await r.json();
        if (j && j.ok && j.me) who = { acct: clip(String(j.me.id), 32), name: clip(String(j.me.name), 24) || 'Guest', av: Math.max(0, j.me.avatar | 0) };
      } catch (e) {}
    }
    if (this.tokens.size > 500) this.tokens.clear();
    this.tokens.set(tok, { who, until: now + TOKEN_CACHE_MS });
    return who;
  }
}
