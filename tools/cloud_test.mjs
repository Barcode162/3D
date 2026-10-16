// Tests the Cloudflare room server (cloud/worker.js) with two fake players:
// join, states, shared world, messages, clock sync, leaving.
//   (cd cloud && npx wrangler dev --port 8787) then: node tools/cloud_test.mjs [ws://127.0.0.1:8787]
const base = process.argv[2] ?? 'ws://127.0.0.1:8787';
const room = 'test' + Math.floor(Math.random() * 1e6);
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
function player(id) {
  const ws = new WebSocket(`${base}/room/${room}?id=${id}`);
  const got = [];
  ws.onmessage = (e) => got.push(JSON.parse(e.data));
  return { ws, got, open: new Promise((r) => (ws.onopen = r)), send: (o) => ws.send(JSON.stringify(o)) };
}
let failed = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failed++; };

const a = player('aaaa1111');
await a.open;
a.send({ hello: 1, name: 'Alice', ping: 1 });
a.send({ t: 1, s: [1, 2, 3] });
a.send({ w: { jb: [4, 5] } });
await wait(300);
check(a.got.some((m) => m.welcome), 'Alice gets a welcome');
check(a.got.some((m) => m.pong === 1 && m.now > 1e9), 'clock sync reply');
const b = player('bbbb2222');
await b.open;
const t0 = performance.now();
b.send({ hello: 1, name: 'Bob' });
await wait(300);
const welcome = b.got.find((m) => m.welcome);
check(welcome && welcome.players.some((p) => p.id === 'aaaa1111' && p.s[2] === 3), 'Bob sees Alice\'s state on joining');
check(welcome && welcome.world.some((e) => e[0] === 'jb' && e[2][1] === 5), 'Bob gets the shared world');
const before = b.got.length;
a.send({ t: 2, s: [7, 8, 9] });
let latency = -1;
for (let i = 0; i < 100 && latency < 0; i++) {
  await wait(5);
  if (b.got.slice(before).some((m) => m.p && m.p.s[0] === 7)) latency = performance.now() - t0;
}
check(b.got.slice(before).some((m) => m.p && m.p.id === 'aaaa1111' && m.p.s[0] === 7), 'state forwarded right away');
b.send({ m: [['aaaa1111', 'v:hello'], ['*', 'e:[1]']] });
b.send({ w: { g3: [1, 1, 1] } });
await wait(300);
check(a.got.some((m) => m.m && m.m[0] === 'bbbb2222' && m.m[1] === 'v:hello'), 'direct message delivered');
check(a.got.some((m) => m.m && m.m[1] === 'e:[1]'), 'message to everyone delivered');
check(a.got.some((m) => m.w && m.w[0][0] === 'g3'), 'world change forwarded');
check(!b.got.some((m) => m.w && m.w.some((e) => e[0] === 'g3')), 'no echo of your own world change');
b.ws.close();
await wait(400);
check(a.got.some((m) => m.left === 'bbbb2222'), 'leaving is announced');
a.ws.close();
console.log(failed ? `${failed} FAILED` : 'CLOUD TEST PASSED');
process.exit(failed ? 1 : 0);
