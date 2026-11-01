#!/usr/bin/env node
// The words tool (J116.3), built when its sources changed, then run:
//   node tools/words.mjs place <name>   assets/world/places/<name>.looped → <name>.json (never edited by hand)
//   node tools/words.mjs show <name>    where each thing went, and the place's rules
//   node tools/words.mjs print <name>   the world file the words make, without writing it
//   node tools/words.mjs check          every place in words has a current world file
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync, readdirSync } from 'node:fs';

const out = 'build/native/words_tool';
const words = readdirSync('looped/words').filter((f) => f.endsWith('.cpp') && f !== 'rig.cpp').map((f) => `looped/words/${f}`);
const srcs = ['tools/native/words_tool.cpp', ...words, 'looped/world/loop.cpp', 'looped/json.cpp', 'looped/gltf.cpp', 'looped/draw/clusters.cpp', 'looped/draw/model_spec.cpp', 'looped/draw/model_measure.cpp'];
const deps = [...srcs, 'looped/draw/clusters.h', 'looped/draw/model_spec.h', ...readdirSync('looped/words').filter((f) => f.endsWith('.h')).map((f) => `looped/words/${f}`)];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stderr.write('building the words tool… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.error('done');
}
const args = process.argv.slice(2);
const COMMANDS = ['place', 'show', 'print', 'check'];  // the native tool's (tools/native/words_tool.cpp)
if (args.length && !COMMANDS.includes(args[0])) { console.log(`node tools/words.mjs ${COMMANDS.join(' | ')}`); process.exit(2); }
process.exit(spawnSync(out, args.length ? args : ['check'], { stdio: 'inherit' }).status ?? 1);
