// Bar3D realtime server (Cloudflare Workers + Durable Objects).
//
// Every phone keeps one WebSocket open to its room: one Durable Object per
// room name (cloud/room.js), so everyone in "main" talks to the same little
// server. The protocol is docs/PROTOCOL.md (codec: cloud/proto.js).
//
//   wss://<worker>/room/<name>   the room socket (name: a-z, 0-9, -; 1-32)
//   GET /health                  {"ok":1,...} (3.2c: cached 60 s)
//
// Only pages from our own sites may connect: the Origin header must match
// one of ALLOWED_ORIGINS exactly (or be localhost / 127.0.0.1 on any port).
// Requests without an Origin (Node test tools) are allowed: this check stops
// other websites' pages, it isn't a login.
export { Room } from './room.js';

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

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === '/health' || url.pathname === '/') {
      return json({ ok: 1, server: 'bar3d-cloud', proto: Number(env.PROTO) || 1 });
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
