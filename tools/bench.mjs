#!/usr/bin/env node
// Benchmarks, kept forever in the gallery branch (bench.jsonl) so every
// change in speed is visible over time (docs/archive/AUTOMATION.md):
//   node tools/bench.mjs              native benches + the bar at every spot
//   node tools/bench.mjs native       only the native benches (fast)
//   node tools/bench.mjs --no-save    print only
// Native benches: every tools/native/bench_*.cpp, built with g++ -O2 (and
// em++ + node when emsdk is there: wasm speed is what the phone runs), each
// printing lines "name value unit". The bar: headless SwiftShader, so
// frame times only compare runs on this machine; draws and triangles are
// exact. Prints each number against the previous run.
import { execSync, spawnSync } from 'node:child_process';
import { readdirSync, mkdirSync } from 'node:fs';
import { openCapture, SPOT_NAMES } from './lib/capture.mjs';
import { openGallery, readJsonl, appendJsonl, commitGallery, writeReadme } from './lib/gallery.mjs';

const argv = process.argv.slice(2);
const onlyNative = argv.includes('native');
const save = !argv.includes('--no-save');
const sh = (c) => execSync(c, { encoding: 'utf8' }).trim();
const EMSDK = 'source /opt/emsdk/emsdk_env.sh >/dev/null 2>&1;';
const results = { native: {}, wasm: {}, spots: {} };

mkdirSync('build/native', { recursive: true });
const benches = readdirSync('tools/native').filter((f) => /^bench_.*\.cpp$/.test(f));
for (const f of benches) {
  const name = f.replace(/\.cpp$/, '');
  // The bench lists its sources on its first line: "// sources: a.cpp b.cpp".
  const first = sh(`head -1 tools/native/${f}`);
  const srcs = (first.match(/sources:(.*)/)?.[1] ?? '').trim();
  const parse = (out, into) => {
    for (const l of out.split('\n')) {
      const m = l.match(/^(\S+)\s+([-\d.e+]+)\s*(\S*)/);
      if (m) into[m[1]] = `${+m[2]}${m[3] ? ' ' + m[3] : ''}`;
    }
  };
  const g = spawnSync('bash', ['-c', `g++ -std=c++20 -O2 -I. tools/native/${f} ${srcs} -o build/native/${name} && build/native/${name}`], { encoding: 'utf8' });
  if (g.status !== 0) { console.log(`✗ ${name} (g++):\n${g.stdout}${g.stderr}`); continue; }
  parse(g.stdout, results.native);
  const w = spawnSync('bash', ['-c', `${EMSDK} command -v em++ >/dev/null && em++ -std=c++20 -O2 -I. tools/native/${f} ${srcs} -sENVIRONMENT=node -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -o build/native/${name}.js && node build/native/${name}.js`], { encoding: 'utf8' });
  if (w.status === 0) parse(w.stdout, results.wasm);
}

if (!onlyNative) {
  const cap = await openCapture();
  for (const spot of SPOT_NAMES) {
    const r = await cap.shot(spot, 'perf=1', null, +(process.env.WAIT ?? 6000));
    const p = r.perf;
    if (p) results.spots[spot] = { avg: p.avg, cpu: p.cpu, gpu: p.gpu, draws: p.draws?.main ?? p.draws, tris: p.tris?.main ?? p.tris };
  }
  await cap.close();
}

openGallery();
const prev = readJsonl('bench.jsonl').at(-1);
const flat = (r) => {
  const o = {};
  for (const [k, v] of Object.entries(r.native ?? {})) o[`native ${k}`] = v;
  for (const [k, v] of Object.entries(r.wasm ?? {})) o[`wasm ${k}`] = v;
  for (const [s, v] of Object.entries(r.spots ?? {})) for (const [k, x] of Object.entries(v ?? {})) o[`${s} ${k}`] = typeof x === 'object' ? JSON.stringify(x) : x;
  return o;
};
const now = flat(results), before = prev ? flat(prev) : {};
console.log('\n=== bench ===');
for (const [k, v] of Object.entries(now)) {
  const a = parseFloat(before[k]), b = parseFloat(v);
  const d = isFinite(a) && isFinite(b) && a !== 0 ? ` (${b >= a ? '+' : ''}${(100 * (b - a) / a).toFixed(1)}%)` : '';
  console.log(`${k.padEnd(40)} ${String(v).padStart(14)}${d}`);
}
if (save) {
  const sha = sh('git rev-parse --short HEAD');
  appendJsonl('bench.jsonl', { sha, date: new Date().toISOString().slice(0, 16).replace('T', ' '), ...results });
  writeReadme();
  commitGallery(`bench ${sha}`);
}
