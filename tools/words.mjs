#!/usr/bin/env node
// The words tool (J116.3), built when its sources changed, then run:
//   node tools/words.mjs place <name>   assets/world/places/<name>.looped → <name>.json (never edited by hand)
//   node tools/words.mjs show <name>    where each thing went, and the place's rules
//   node tools/words.mjs check          every place in words has a current world file
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync, readdirSync } from 'node:fs';

const out = 'build/native/words_tool';
const words = readdirSync('looped/words').filter((f) => f.endsWith('.cpp') && f !== 'rig.cpp').map((f) => `looped/words/${f}`);
const srcs = ['tools/native/words_tool.cpp', ...words, 'looped/world/loop.cpp', 'looped/json.cpp', 'looped/gltf.cpp'];
const deps = [...srcs, ...readdirSync('looped/words').filter((f) => f.endsWith('.h')).map((f) => `looped/words/${f}`)];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stdout.write('building the words tool… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.log('done');
}
const args = process.argv.slice(2);
process.exit(spawnSync(out, args.length ? args : ['check'], { stdio: 'inherit' }).status ?? 1);
