// Compares screenshots: node tools/imgdiff.mjs a.png b.png   (or two folders:
// every PNG with the same name). A pixel differs when any channel differs by
// more than THRESHOLD (default 16 of 255). Prints the % of differing pixels
// per image; exits 1 if any is above LIMIT % (default 1). Used for "looks
// the same" checks (docs/RESTRUCTURE.md → Visual check).
import { readdirSync, statSync } from 'node:fs';
import { join, basename } from 'node:path';
import { readPng } from './lib/png.mjs';

const threshold = +(process.env.THRESHOLD ?? 16);
const limit = +(process.env.LIMIT ?? 1);
const [a, b] = process.argv.slice(2);
if (!a || !b) {
  console.log('usage: node tools/imgdiff.mjs before.png after.png | beforeDir afterDir');
  process.exit(2);
}

export function diffPercent(pa, pb) {
  const A = readPng(pa), B = readPng(pb);
  if (A.width !== B.width || A.height !== B.height) return 100;
  let diff = 0;
  for (let i = 0; i < A.width * A.height; i++) {
    const k = i * 4;
    if (Math.abs(A.data[k] - B.data[k]) > threshold || Math.abs(A.data[k + 1] - B.data[k + 1]) > threshold ||
        Math.abs(A.data[k + 2] - B.data[k + 2]) > threshold) diff++;
  }
  return (100 * diff) / (A.width * A.height);
}

const pairs = statSync(a).isDirectory()
  ? readdirSync(a).filter((f) => f.endsWith('.png')).map((f) => [join(a, f), join(b, f)])
  : [[a, b]];
let failed = false;
for (const [pa, pb] of pairs) {
  let pct;
  try { pct = diffPercent(pa, pb); } catch (e) { console.log(`${basename(pa)}: ${e.message}`); failed = true; continue; }
  const bad = pct > limit;
  failed ||= bad;
  console.log(`${bad ? 'DIFF' : 'ok  '} ${basename(pa)}: ${pct.toFixed(3)}% of pixels differ`);
}
process.exit(failed ? 1 : 0);
