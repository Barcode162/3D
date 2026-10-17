// Voice chat test: two players through a local PHP relay (needs `php`).
// Chromium's fake microphone stands in for a real one. Checks that the
// players connect phone-to-phone (WebRTC), that Alice's mic turns on and
// her voice reaches Bob, and that a piano key Alice plays arrives at Bob
// over the direct data channel. Bob sits on a bar stool; Alice must see him
// sitting. Saves build/voice-alice.png.
// Usage: NODE_PATH=$(npm root -g) node tools/voice_test.mjs
import { createServer } from 'node:http';
import { spawn } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { mkdtempSync, copyFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { extname, join, resolve } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const siteDir = resolve('build/site');
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
const site = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  const file = path === '/' ? 'index.html' : path;
  try {
    const body = await readFile(join(siteDir, file));
    res.writeHead(200, { 'content-type': types[extname(file)] ?? 'application/octet-stream' });
    res.end(body);
  } catch {
    res.writeHead(404).end();
  }
}).listen(0);
const port = site.address().port;
const relayDir = mkdtempSync(join(tmpdir(), 'relay-'));
copyFileSync('server/relay.php', join(relayDir, 'relay.php'));
const relayPort = port + 1;
const php = spawn('php', ['-S', `127.0.0.1:${relayPort}`, '-t', relayDir], { stdio: 'ignore' });
const relay = `http://127.0.0.1:${relayPort}/relay.php`;

const browser = await playwright.chromium.launch({
  args: ['--enable-unsafe-webgpu', '--enable-features=Vulkan', '--use-vulkan=swiftshader',
         '--use-webgpu-adapter=swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist',
         '--use-fake-ui-for-media-stream', '--use-fake-device-for-media-stream', '--autoplay-policy=no-user-gesture-required'],
});
const context = await browser.newContext({ viewport: { width: 430, height: 932 }, permissions: ['microphone'] });
async function open(name, extra) {
  const page = await context.newPage();
  await page.goto(`http://localhost:${port}/?skipintro&room=voice&name=${name}&server=${encodeURIComponent(relay)}&${extra}`);
  return page;
}
const waitLog = (page, re, ms = 90000) =>
  page.waitForFunction((src) => new RegExp(src).test(document.getElementById('log').textContent), re.source,
                       { timeout: ms, polling: 250 }).then(() => true, () => false);
// Alice looks at bar stool 3 (x about -4.6), Bob sits on it.
const alice = await open('Alice', 'cam=' + encodeURIComponent('-4.4,1.66,-1.6,-10,-12'));
const bob = await open('Bob', 'sit=3');
await waitLog(alice, /Everything loaded/, 180000);
await waitLog(bob, /Everything loaded/, 180000);
for (const p of [alice, bob]) await p.mouse.click(5, 920);  // unlocks sound
const results = [];
const check = (name, ok) => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}`); };
check('Alice and Bob connect directly (WebRTC)', await waitLog(alice, /Voice: connected to Bob/, 60000));
check('Bob sees the connection too', await waitLog(bob, /Voice: connected to Alice/, 30000));
await alice.evaluate(() => window.bar3dVoice.micStart());
await alice.evaluate(() => window.bar3dSettings.set('micGate', -100));  // the fake mic's beep always passes
check("Alice's mic is on", await alice.evaluate(() => window.bar3dVoice.micOn()));
await bob.waitForTimeout(4000);
// The fake mic beeps on and off: take the loudest of 3 s of readings.
const level = await bob.evaluate(async () => {
  const n = window.bar3dNet;
  const idx = n.indexOf(n.peers()[0]);
  let best = 0;
  for (let i = 0; i < 30; i++) {
    best = Math.max(best, window.bar3dVoice.peerLevel(idx));
    await new Promise((r) => setTimeout(r, 100));
  }
  return best;
});
const stats = await bob.evaluate(async () => {
  const out = [];
  for (const pc of window.bar3dVoice._pcs()) {
    (await pc.getStats()).forEach((r) => { if (r.type === 'inbound-rtp' || r.type === 'outbound-rtp') out.push(r.type + ' bytes=' + (r.bytesReceived ?? r.bytesSent) + ' level=' + r.audioLevel); });
    out.push('transceivers: ' + pc.getTransceivers().map((t) => t.direction + '/' + t.currentDirection + ' send=' + !!t.sender.track).join(','));
  }
  return out.join('\n');
});
const astats = await alice.evaluate(async () => {
  const out = [];
  for (const pc of window.bar3dVoice._pcs()) {
    (await pc.getStats()).forEach((r) => { if (r.type === 'inbound-rtp' || r.type === 'outbound-rtp') out.push(r.type + ' bytes=' + (r.bytesReceived ?? r.bytesSent)); });
    out.push('transceivers: ' + pc.getTransceivers().map((t) => t.direction + '/' + t.currentDirection + ' send=' + !!t.sender.track).join(','));
  }
  return out.join('\n');
});
const samples = await alice.evaluate(async () => {
  const out = [];
  for (let i = 0; i < 20; i++) { out.push(Math.round(window.bar3dVoice.inputDb())); await new Promise((r) => setTimeout(r, 150)); }
  return out.join(' ');
});
console.log('Alice mic dB over 3 s:', samples);
console.log('Bob ctx:', await bob.evaluate(() => window.bar3dAudio.context().state));
console.log('Bob stats:\n' + stats + '\nAlice stats:\n' + astats);
check(`Bob hears Alice (level ${level.toFixed(2)})`, level > 0.05);
const viaChannel = await alice.evaluate(() => window.bar3dVoice.sendData(window.bar3dNet.peers()[0], 'e:["pk",[60,0.8,1]]'));
check('Piano notes go over the direct data channel', viaChannel);
await alice.bringToFront();
await alice.waitForTimeout(3000);
await alice.screenshot({ path: 'build/voice-alice.png' });
const log = await alice.$eval('#log', (el) => [...el.children].map((c) => c.textContent).join('\n'));
console.log(log.split('\n').filter((l) => /Voice|Multiplayer/.test(l)).join('\n'));
await browser.close();
site.close();
php.kill();
const passed = results.every(Boolean);
console.log(passed ? 'VOICE TEST PASSED' : 'VOICE TEST FAILED');
process.exit(passed ? 0 : 1);
