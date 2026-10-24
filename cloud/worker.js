// Bar3D realtime server (Cloudflare Workers + Durable Objects).
//
// Every phone keeps one WebSocket open to its room: one Durable Object per
// room name (cloud/room.js), so everyone in "main" talks to the same little
// server. The protocol is docs/handbook/protocol.md (codec: cloud/proto.js).
//
//   wss://<worker>/room/<name>   the room socket (name: a-z, 0-9, -; 1-32)
//   GET /health                  {"ok":1,...} plus room "main"'s usage today,
//                                remembered for 60 s (so polling it is cheap)
//
// Only pages from our own sites may connect: the Origin header must match
// one of ALLOWED_ORIGINS exactly (or be localhost / 127.0.0.1 on any port).
// Requests without an Origin (Node test tools) are allowed: this check stops
// other websites' pages, it isn't a login.
export { Room } from './room.js';
export { Ledger } from './ledger.js';

const HEALTH_CACHE_MS = 60000;

const json = (body, status = 200) => new Response(JSON.stringify(body), {
  status,
  headers: { 'Content-Type': 'application/json', 'Cache-Control': 'no-store', 'Access-Control-Allow-Origin': '*' },
});

export function originAllowed(origin, env) {
  if (!origin) return true;
  const allowed = String(env.ALLOWED_ORIGINS || '').split(',').map((s) => s.trim()).filter(Boolean);
  if (allowed.includes(origin)) return true;
  return /^http:\/\/(localhost|127\.0\.0\.1)(:\d{1,5})?$/.test(origin);
}

// /health: the Cache API does nothing on workers.dev addresses, so the answer
// is kept in this Worker instance's memory instead (one Durable Object
// request per minute at most, per instance).
let health = null, healthAt = 0;

async function healthAnswer(env) {
  const now = Date.now();
  if (health && now - healthAt < HEALTH_CACHE_MS) return { ...health, cachedMs: now - healthAt };
  let main = null;
  try {
    const room = env.ROOMS.get(env.ROOMS.idFromName('main'));
    const r = await room.fetch(new Request('https://room/stats', { headers: { 'X-Room': 'main' } }));
    main = await r.json();
  } catch (e) {
    main = { error: String(e && e.message || e) };
  }
  health = { ok: 1, server: 'bar3d-cloud', proto: Number(env.PROTO) || 1, main };
  healthAt = now;
  return { ...health, cachedMs: 0 };
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === '/health' || url.pathname === '/') return json(await healthAnswer(env));
    // The Ledger: players may only read their own wallet (X-Public); the
    // test-only door /bank-internal exists with DEV = "1" only.
    if (url.pathname === '/bank' || (url.pathname === '/bank-internal' && env.DEV === '1')) {
      if (!originAllowed(request.headers.get('Origin'), env)) return json({ ok: 0, error: 'origin not allowed' }, 403);
      const headers = new Headers(request.headers);
      if (url.pathname === '/bank') headers.set('X-Public', '1');
      else headers.delete('X-Public');
      const ledger = env.LEDGER.get(env.LEDGER.idFromName('main'));
      return ledger.fetch(new Request(request, { headers }));
    }
    const m = /^\/room\/([a-z0-9-]{1,32})(\/debug)?$/.exec(url.pathname);
    if (!m) return json({ ok: 0, error: 'not found' }, 404);
    if (m[2] && env.DEV !== '1') return json({ ok: 0, error: 'not found' }, 404);
    if (!originAllowed(request.headers.get('Origin'), env)) return json({ ok: 0, error: 'origin not allowed' }, 403);
    if (!m[2] && request.headers.get('Upgrade') !== 'websocket') return json({ ok: 0, error: 'expected a websocket' }, 426);
    const headers = new Headers(request.headers);
    headers.set('X-Room', m[1]);
    const room = env.ROOMS.get(env.ROOMS.idFromName(m[1]));
    return room.fetch(new Request(request, { headers }));
  },
};
