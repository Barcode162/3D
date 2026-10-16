// Bar3D realtime server (Cloudflare Workers + Durable Objects).
//
// Every phone keeps one WebSocket open to its room (one Durable Object per
// room name, so everyone in "main" talks to the same little server). The
// room forwards each player's state to the others the moment it arrives,
// keeps the shared world (glasses, jukebox, mirrors...), delivers messages
// (voice chat handshakes, quick events), stamps everything with its own
// clock, and checks login tokens with the accounts server on IONOS.
// It does the same job as server/relay.php, without polling.
//
// Connect: wss://<worker>/room/<name>?id=<player id>
// Client -> server (JSON text):
//   {"hello":1,"name":"Josh","tok":"<login token>"}    once, right after connecting
//   {"t":12345.6,"s":[...numbers]}                     your state (t = your clock, ms)
//   {"w":{"g3":[...],"jb":[...]}}                      shared world changes
//   {"m":[[to, text], ...]}                            messages (to = id or "*")
//   {"ping":<your clock>}                              clock sync
// (one message may carry several of these fields)
// Server -> client:
//   {"welcome":1,"now":s,"players":[{id,name,t,s,acct,av}],"world":[[key,by,[..]]]}
//   {"p":{id,name,t,s,acct,av}}       someone's new state
//   {"left":id}                        someone left
//   {"w":[[key,by,[..]]]}              world changes by others
//   {"m":[from, text]}                 a message for you
//   {"pong":<your clock>,"now":s}      clock sync reply (s = server seconds)
// GET /health -> {"ok":1,"server":"bar3d-cloud"}

const MAX_PLAYERS = 16;
const MAX_VALUES = 20;
const MAX_KEYS = 96;
const MAX_MSG = 12000;
const WORLD_KEEP_MS = 30 * 60 * 1000;

const cors = {
  'Access-Control-Allow-Origin': '*',
  'Cache-Control': 'no-store',
  'Content-Type': 'application/json',
};

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === '/health' || url.pathname === '/') {
      return new Response(JSON.stringify({ ok: 1, server: 'bar3d-cloud' }), { headers: cors });
    }
    const m = /^\/room\/([a-z0-9-]{1,32})$/.exec(url.pathname);
    if (!m) return new Response('not found', { status: 404, headers: cors });
    if (request.headers.get('Upgrade') !== 'websocket') {
      return new Response('expected a websocket', { status: 426, headers: cors });
    }
    const room = env.ROOMS.get(env.ROOMS.idFromName(m[1]));
    return room.fetch(request);
  },
};

function cleanNumbers(list) {
  if (!Array.isArray(list) || list.length > MAX_VALUES) return null;
  const out = [];
  for (const v of list) {
    if (typeof v !== 'number' || !isFinite(v)) return null;
    out.push(v);
  }
  return out;
}

export class Room {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    this.players = new Map();  // id -> {name, t, s, acct, av}
    this.world = null;         // key -> [byId, values]; loaded from storage on first use
    this.lastActive = 0;
    // Players who were connected before the object went to sleep.
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.id) this.players.set(a.id, { name: a.name, t: 0, s: [], acct: a.acct || '', av: a.av || 0 });
    }
  }

  async loadWorld() {
    if (this.world) return;
    const saved = await this.ctx.storage.get('world');
    const active = (await this.ctx.storage.get('active')) || 0;
    this.world = new Map(saved && Date.now() - active < WORLD_KEEP_MS ? saved : []);
  }

  now() { return Date.now() / 1000; }

  socketsExcept(id) {
    return this.ctx.getWebSockets().filter((ws) => {
      const a = ws.deserializeAttachment();
      return a && a.id !== id;
    });
  }

  socketOf(id) {
    return this.ctx.getWebSockets().find((ws) => {
      const a = ws.deserializeAttachment();
      return a && a.id === id;
    });
  }

  send(ws, data) {
    try { ws.send(typeof data === 'string' ? data : JSON.stringify(data)); } catch (e) {}
  }

  broadcast(fromId, data) {
    const text = JSON.stringify(data);
    for (const ws of this.socketsExcept(fromId)) this.send(ws, text);
  }

  async fetch(request) {
    const url = new URL(request.url);
    const id = url.searchParams.get('id') || '';
    if (!/^[a-z0-9]{4,16}$/.test(id)) return new Response('bad id', { status: 400 });
    if (!this.players.has(id) && this.players.size >= MAX_PLAYERS) return new Response('room full', { status: 503 });
    const old = this.socketOf(id);  // the same player reconnecting
    if (old) { try { old.close(4000, 'replaced'); } catch (e) {} }
    const pair = new WebSocketPair();
    this.ctx.acceptWebSocket(pair[1]);
    pair[1].serializeAttachment({ id, name: 'Guest', acct: '', av: 0, hello: false });
    return new Response(null, { status: 101, webSocket: pair[0] });
  }

  async webSocketMessage(ws, raw) {
    if (typeof raw !== 'string' || raw.length > 64000) return;
    let msg;
    try { msg = JSON.parse(raw); } catch (e) { return; }
    if (!msg || typeof msg !== 'object') return;
    const a = ws.deserializeAttachment();
    if (!a) return;
    const id = a.id;

    if ('ping' in msg) this.send(ws, { pong: msg.ping, now: this.now() });

    if (msg.hello && !a.hello) {
      a.hello = true;
      a.name = typeof msg.name === 'string' ? msg.name.replace(/[^\x20-\x7E]/g, '').slice(0, 20) || 'Guest' : 'Guest';
      if (typeof msg.tok === 'string' && /^[0-9a-f]{48}$/.test(msg.tok)) {
        const acct = await this.checkToken(msg.tok);
        if (acct) {
          a.acct = acct.id;
          a.av = acct.av;
          a.name = acct.name;
        }
      }
      ws.serializeAttachment(a);
      const p = this.players.get(id) || { t: 0, s: [] };
      Object.assign(p, { name: a.name, acct: a.acct, av: a.av });
      this.players.set(id, p);
      await this.loadWorld();
      const players = [];
      for (const [pid, q] of this.players) {
        if (pid !== id && q.s.length) players.push({ id: pid, name: q.name, t: q.t, s: q.s, acct: q.acct, av: q.av });
      }
      const world = [];
      for (const [key, e] of this.world) if (e[0] !== id) world.push([key, e[0], e[1]]);
      this.send(ws, { welcome: 1, now: this.now(), players, world, name: a.name });
    }
    if (!a.hello) return;

    if (Array.isArray(msg.s)) {
      const s = cleanNumbers(msg.s);
      if (s) {
        const p = this.players.get(id);
        p.t = typeof msg.t === 'number' ? msg.t : 0;
        p.s = s;
        this.broadcast(id, { p: { id, name: p.name, t: p.t, s, acct: p.acct, av: p.av } });
      }
    }

    if (msg.w && typeof msg.w === 'object') {
      await this.loadWorld();
      const changes = [];
      for (const [key, values] of Object.entries(msg.w)) {
        if (!/^[a-z0-9]{1,8}$/.test(key)) continue;
        const v = cleanNumbers(values);
        if (!v) continue;
        if (!this.world.has(key) && this.world.size >= MAX_KEYS) continue;
        this.world.set(key, [id, v]);
        changes.push([key, id, v]);
      }
      if (changes.length) {
        this.broadcast(id, { w: changes });
        this.lastActive = Date.now();
        // Saved so the world survives the room going to sleep.
        this.ctx.storage.put({ world: [...this.world], active: this.lastActive });
      }
    }

    if (Array.isArray(msg.m)) {
      for (const m of msg.m.slice(0, 40)) {
        if (!Array.isArray(m) || m.length !== 2 || typeof m[0] !== 'string' || typeof m[1] !== 'string') continue;
        if (m[1].length > MAX_MSG) continue;
        const text = JSON.stringify({ m: [id, m[1]] });
        if (m[0] === '*') {
          for (const other of this.socketsExcept(id)) this.send(other, text);
        } else {
          const target = this.socketOf(m[0]);
          if (target) this.send(target, text);
        }
      }
    }
  }

  async webSocketClose(ws) { this.gone(ws); }
  async webSocketError(ws) { this.gone(ws); }

  gone(ws) {
    const a = ws.deserializeAttachment();
    if (!a) return;
    // A newer connection of the same player replaced this one: keep them.
    const still = this.ctx.getWebSockets().some((o) => o !== ws && (o.deserializeAttachment() || {}).id === a.id);
    if (still) return;
    this.players.delete(a.id);
    this.broadcast(a.id, { left: a.id });
    if (this.world) this.ctx.storage.put('active', Date.now());
  }

  // Asks the accounts server (IONOS) whose login this is.
  async checkToken(tok) {
    try {
      const r = await fetch(this.env.ACCOUNT_URL, {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: JSON.stringify({ a: 'me', token: tok }),
      });
      const j = await r.json();
      if (j && j.ok && j.me) return { id: String(j.me.id), name: String(j.me.name).slice(0, 40), av: j.me.avatar | 0 };
    } catch (e) {}
    return null;
  }
}
