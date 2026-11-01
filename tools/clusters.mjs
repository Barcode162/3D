#!/usr/bin/env node
// The bar's models in clusters (J116.6, looped/draw/clusters.h), built when its sources changed, then run:
//   node tools/clusters.mjs [place] [limit px]   every model's clusters; the triangles drawn from the standard spots
//   node tools/clusters.mjs spec [place]        every model against the model spec (looped/draw/model_spec.h): what breaks it and why
import { execSync, spawnSync } from 'node:child_process';
import { statSync, existsSync, mkdirSync } from 'node:fs';
import { LIGHT_SOURCES } from './lib/native_sources.mjs';

const out = 'build/native/clusters_tool';
const srcs = `tools/native/clusters_tool.cpp looped/draw/clusters.cpp looped/draw/model_spec.cpp looped/draw/model_measure.cpp ${LIGHT_SOURCES}`.split(' ');
const deps = [...srcs, 'tools/native/light_tool.cpp', 'looped/draw/clusters.h', 'looped/draw/model_spec.h'];
mkdirSync('build/native', { recursive: true });
const stale = !existsSync(out) || deps.some((f) => statSync(f).mtimeMs > statSync(out).mtimeMs);
if (stale) {
  process.stdout.write('building the clusters tool… ');
  execSync(`g++ -std=c++20 -O2 -I. ${srcs.join(' ')} -o ${out}`, { stdio: 'inherit' });
  console.log('done');
}
const args = process.argv.slice(2);
const spec = args[0] === 'spec';
if (spec) args.shift();
process.exit(spawnSync(out, args, { stdio: 'inherit', env: { SIMPLIFY: '0', ...(spec ? { SPEC: '1' } : {}), ...process.env } }).status ?? 1);
