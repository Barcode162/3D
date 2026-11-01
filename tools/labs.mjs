#!/usr/bin/env node
// The labs' data (J116, Looped rebuilt):
//   node tools/labs.mjs light [--bake]   the light lab's pictures and numbers (labs/web/light/data/) from
//                                        build/bake/lab-page, and the draw lab's baked room (labs/web/draw/data/);
//                                        --bake bakes it first (about 8 minutes the first time, 1 with the references kept)
// The full-quality settings live here, so the page always shows the same bake.
import { execSync } from 'node:child_process';
import { copyFileSync, mkdirSync, readdirSync, existsSync, rmSync } from 'node:fs';

const [cmd, ...rest] = process.argv.slice(2);
if (cmd !== 'light') {
  console.log('usage: node tools/labs.mjs light [--bake]');
  process.exit(2);
}
const from = 'build/bake/lab-page', to = 'labs/web/light/data';
if (rest.includes('--bake') || !existsSync(`${from}/summary.json`)) {
  // (the reference pictures in <from>/ref are kept: they're slow and don't change with the bake)
  execSync(`node tools/bake.mjs bake lab ${from}`, {
    stdio: 'inherit',
    env: { ...process.env, WIDTH: '400', HEIGHT: '225', SAMPLES: '512', REF_SAMPLES: '1024', CORNER_SAMPLES: '256' },
  });
}
rmSync(to, { recursive: true, force: true });
mkdirSync(to, { recursive: true });
copyFileSync(`${from}/summary.json`, `${to}/summary.json`);
for (const f of readdirSync(`${from}/lab`)) copyFileSync(`${from}/lab/${f}`, `${to}/${f}`);
for (const f of readdirSync(from)) if (f.endsWith('.png') && !f.endsWith('-error.png') && f !== 'atlas.png') copyFileSync(`${from}/${f}`, `${to}/${f}`);
// The baked room for the draw lab (J116.4).
const drawTo = 'labs/web/draw/data';
rmSync(drawTo, { recursive: true, force: true });
mkdirSync(drawTo, { recursive: true });
if (existsSync(`${from}/scene.bin`)) copyFileSync(`${from}/scene.bin`, `${drawTo}/scene.bin`);
console.log(`the light lab's data: ${readdirSync(to).join(', ')}; the draw lab's: ${readdirSync(drawTo).join(', ')}`);
