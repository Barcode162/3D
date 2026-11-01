#!/usr/bin/env node
// The labs' data (J116, Looped rebuilt):
//   node tools/labs.mjs bar [--bake] [--quick]   the bar for the new engine (labs/web/next/data/) from build/bake/bar
//                                        (--lights: only the lamps and the rig, after the rig changed)
//                                        (--quick: a fast, grainy bake, a few minutes; the full one about half an hour)
//   node tools/labs.mjs light [--bake]   the light lab's pictures and numbers (labs/web/light/data/) from
//                                        build/bake/lab-page, and the draw lab's baked room (labs/web/draw/data/);
//                                        --bake bakes it first (about 8 minutes the first time, 1 with the references kept)
// The full-quality settings live here, so the page always shows the same bake.
import { execSync } from 'node:child_process';
import { copyFileSync, mkdirSync, readdirSync, existsSync, rmSync, readFileSync, writeFileSync } from 'node:fs';
import { gzipSync } from 'node:zlib';

const [cmd, ...rest] = process.argv.slice(2);
if (cmd === 'bar') {  // the bar on the new engine (J116.6): its bake into labs/web/next/data/
  const from = rest.includes('--quick') ? 'build/bake/bar-quick' : 'build/bake/bar';
  if (rest.includes('--bake')) {
    const quick = rest.includes('--quick');
    execSync(`node tools/bake.mjs bake bar ${from}`, {
      stdio: 'inherit',
      env: { ...process.env, NOREF: '1', ...(quick ? { SAMPLES: '16', CORNER_SAMPLES: '16', PROBE_SAMPLES: '16', DIRECT_RAYS: '8' }
                                                  : { SAMPLES: '128', CORNER_SAMPLES: '96', PROBE_SAMPLES: '48', DIRECT_RAYS: '32' }) },
    });
  }
  const to = 'labs/web/next/data';
  if (!rest.includes('--lights')) {
    rmSync(to, { recursive: true, force: true });
    mkdirSync(`${to}/tex`, { recursive: true });
    writeFileSync(`${to}/place.bin.gz`, gzipSync(readFileSync(`${from}/next/place.bin`), { level: 9 }));  // the page unpacks it
    for (const f of readdirSync(`${from}/next/tex`)) copyFileSync(`${from}/next/tex/${f}`, `${to}/tex/${f}`);
  }
  // The lights alive (J118.2): the rig plays the lamps, named as the bake orders them.
  writeFileSync(`${to}/lamps.json`, execSync('node tools/bake.mjs lamps bar', { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }));
  copyFileSync('assets/world/rigs/bar.looped', `${to}/rig.looped`);
  console.log(`the bar for the new engine: ${to} (${readdirSync(`${to}/tex`).length - 1} pictures, the lamps and the rig)`);
  process.exit(0);
}
if (cmd !== 'light') {
  console.log('usage: node tools/labs.mjs light [--bake] | bar [--bake] [--quick] [--lights]');
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
for (const f of ["scene.bin", "repair.bin"]) if (existsSync(`${from}/${f}`)) copyFileSync(`${from}/${f}`, `${drawTo}/${f}`);  // the physics lab reads them here too
console.log(`the light lab's data: ${readdirSync(to).join(', ')}; the draw lab's: ${readdirSync(drawTo).join(', ')}`);
