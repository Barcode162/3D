// Keeps Looped an engine of its own (J86): nothing in looped/ includes a
// file from bar/ or names the bar's page objects (bar3d…); the engine talks
// to its page only through window.loopedHost (looped/host.h).
//   node tools/lint_engine.mjs
import { readdirSync, readFileSync, statSync } from 'node:fs';
import { join } from 'node:path';

const bad = [];
const walk = (dir) => {
  for (const name of readdirSync(dir)) {
    const p = join(dir, name);
    if (statSync(p).isDirectory()) { walk(p); continue; }
    if (!/\.(cpp|h|wgsl|js)$/.test(name)) continue;
    readFileSync(p, 'utf8').split('\n').forEach((line, i) => {
      if (/#include\s+"bar\//.test(line)) bad.push(`${p}:${i + 1}: includes a bar file: ${line.trim()}`);
      if (/bar3d/.test(line)) bad.push(`${p}:${i + 1}: names the bar ("bar3d…"); use window.loopedHost: ${line.trim()}`);
    });
  }
};
walk('looped');
if (bad.length) { console.log(bad.join('\n')); console.log(`FAIL  engine lint: ${bad.length} line(s)`); process.exit(1); }
console.log('ok  engine lint: looped/ never includes bar/ and never names the bar\'s page objects');
