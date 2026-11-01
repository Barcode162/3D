#!/usr/bin/env node
// The bar's models in clusters (J116.6, looped/draw/clusters.h), built when its sources changed, then run:
//   node tools/clusters.mjs [place] [limit px]   every model's clusters; the triangles drawn from the standard spots
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync } from 'node:fs';
import { LIGHT_SOURCES } from './lib/native_sources.mjs';

const out = 'build/native/clusters_tool';
const srcs = `tools/native/clusters_tool.cpp looped/draw/clusters.cpp ${LIGHT_SOURCES}`.split(' ');
const deps = [...srcs, 'tools/native/light_tool.cpp', 'looped/draw/clusters.h'];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stdout.write('building the clusters tool… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.log('done');
}
process.exit(spawnSync(out, process.argv.slice(2), { stdio: 'inherit', env: { SIMPLIFY: '0', ...process.env } }).status ?? 1);
