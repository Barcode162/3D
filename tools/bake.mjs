#!/usr/bin/env node
// The baker (J116.3), built when its sources changed, then run:
//   node tools/bake.mjs unwrap [place]   every still model laid out flat: pieces, sizes, the atlas
// Full detail (no simplifying): the light is stored over the real surfaces.
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync } from 'node:fs';
import { LIGHT_SOURCES } from './lib/native_sources.mjs';

const out = 'build/native/bake_tool';
const srcs = `tools/native/bake_tool.cpp looped/bake/unwrap.cpp ${LIGHT_SOURCES}`.split(' ');
const deps = [...srcs, 'tools/native/light_tool.cpp', 'looped/bake/unwrap.h', 'looped/bake/light_paths.h', 'looped/bvh.h', 'looped/scene_geometry.h', 'bar/bar_scene.h'];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stdout.write('building the baker… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.log('done');
}
const args = process.argv.slice(2);
process.exit(spawnSync(out, args.length ? args : ['unwrap'], { stdio: 'inherit', env: { SIMPLIFY: '0', ...process.env } }).status ?? 1);
