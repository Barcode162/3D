#!/usr/bin/env node
// The light tool, built when its sources changed, then run (docs/archive/AUTOMATION.md):
//   node tools/light.mjs stats [place]          the place's geometry and ray tree
//   node tools/light.mjs gi [place]             builds the bounced light on the CPU, timed by phase
//   node tools/light.mjs map [place] [y] [out]  a top-down map of the bounced light (PNG)
//   node tools/light.mjs rays [place]           raw ray speed
//   node tools/light.mjs ref [place] [out] [spot…]  the reference pictures (J116.1): every light path, full detail
// SIMPLIFY=0.05 merges corners 5 cm apart first (what the game does; the
// default, except for fit and ref, which need every detail). SIMPLIFY=0: none.
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync } from 'node:fs';

import { LIGHT_SOURCES } from './lib/native_sources.mjs';

const out = 'build/native/light_tool';
const srcs = `tools/native/light_tool.cpp ${LIGHT_SOURCES}`.split(' ');
const deps = [...srcs, 'looped/bake/light_paths.h', 'looped/radiosity.h', 'looped/bvh.h', 'looped/scene_geometry.h', 'bar/bar_scene.h'];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stdout.write('building the light tool… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.log('done');
}
const args = process.argv.slice(2);
process.exit(spawnSync(out, args.length ? args : ['stats'], { stdio: 'inherit', env: { SIMPLIFY: args[0] === 'fit' || args[0] === 'ref' ? '0' : '0.05', ...process.env } }).status ?? 1);
