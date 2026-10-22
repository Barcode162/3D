// Starts the Cloudflare room (cloud/) locally with `wrangler dev` for the
// browser tests, and a tiny protocol client ("probe") that can look at and
// write to a room the way a phone does. Needs `npm ci --prefix cloud`.
import { spawn } from 'node:child_process';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { decodeText, encodeText } from '../../cloud/proto.js';

const wait = (ms) => new Promise((r) => setTimeout(r, ms));

// → {ws: 'ws://127.0.0.1:PORT', http, stop()}. ROOM_URL=ws://... uses a
// server that's already running instead.
export async function startRoom(port = 8791, vars = {}) {
  if (process.env.ROOM_URL) {
    const ws = process.env.ROOM_URL.replace(/^http/, 'ws');
    return { ws, http: ws.replace(/^ws/, 'http'), stop: async () => {} };
  }
  const stateDir = mkdtempSync(join(tmpdir(), 'bar3d-room-'));
  const args = ['wrangler', 'dev', '--port', String(port), '--ip', '127.0.0.1', '--persist-to', stateDir, '--var', 'DEV:1', '--var', 'WORLD_URL:'];
  for (const [k, v] of Object.entries(vars)) args.push('--var', `${k}:${v}`);
  const server = spawn('npx', args, { cwd: new URL('../../cloud/', import.meta.url).pathname, stdio: ['ignore', 'pipe', 'pipe'], detached: true });
  // Never leave a server behind if the test crashes.
  process.on('exit', () => { try { process.kill(-server.pid, 'SIGKILL'); } catch (e) {} });
  let log = '';
  server.stdout.on('data', (d) => (log += d));
  server.stderr.on('data', (d) => (log += d));
  for (let i = 0; i < 120 && !/Ready on/.test(log); i++) {
    await wait(500);
    if (server.exitCode !== null) break;
  }
  if (!/Ready on/.test(log)) { console.log(log); throw new Error('wrangler dev did not start'); }
  return {
    ws: `ws://127.0.0.1:${port}`,
    http: `http://127.0.0.1:${port}`,
    log: () => log,
    stop: async () => {
      try { process.kill(-server.pid, 'SIGTERM'); } catch (e) {}
      await wait(500);
      try { rmSync(stateDir, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 }); } catch (e) {}
    },
  };
}

// A protocol client: joins `room`, records every text message.
export async function probe(wsBase, room, name = 'Probe') {
  const ws = new WebSocket(`${wsBase}/room/${room}`);
  ws.binaryType = 'arraybuffer';
  const p = { ws, got: [] };
  ws.onmessage = (e) => {
    if (typeof e.data !== 'string') return;
    const d = decodeText(e.data, 'toClient');
    if (d.ok) p.got.push(d);
  };
  await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = () => reject(new Error('probe could not connect')); });
  p.send = (letter, body) => ws.send(encodeText(letter, body, 'toServer'));
  p.find = (letter, pred = () => true) => p.got.find((m) => m.letter === letter && pred(m.body));
  p.until = async (fn, ms = 5000) => { for (let t = 0; t < ms; t += 50) { const v = fn(); if (v) return v; await wait(50); } return fn(); };
  p.send('H', { proto: 1, build: 'probe', name, look: 0 });
  p.welcome = (await p.until(() => p.find('W')))?.body;
  // Keepalive like a phone's, so the room doesn't close the probe as a ghost.
  const keep = setInterval(() => { try { ws.send('ping'); } catch (e) {} }, 4000);
  p.close = () => { clearInterval(keep); try { ws.close(1000); } catch (e) {} };
  return p;
}
