// Lint: JavaScript inside EM_ASM / EM_JS must reach our own objects with
// quoted names (window['bar3dAudio']['play']), because the Release build's
// Closure compiler renames unquoted ones (window.bar3dAudio.play) and the
// call silently breaks only on the live site. Flags `window.x`, `Module.x`
// and `['x'].y` inside those blocks. Browser APIs (document.getElementById,
// el.textContent...) are fine: Closure knows them.
// Usage: node tools/lint_emasm.mjs   (exit 1 on problems)
import { readFileSync, readdirSync, statSync } from 'node:fs';
import { join } from 'node:path';

const BUILTINS = new Set(['length', 'push', 'pop', 'shift', 'forEach', 'map', 'filter', 'slice', 'indexOf',
  'call', 'apply', 'bind', 'toString', 'test', 'exec', 'then', 'catch', 'set', 'get', 'subarray',
  'location', 'navigator', 'document', 'performance', 'localStorage', 'devicePixelRatio', 'innerWidth',
  'innerHeight', 'addEventListener', 'removeEventListener', 'setTimeout', 'clearTimeout', 'requestAnimationFrame',
  'AudioContext', 'webkitAudioContext', 'open', 'close', 'focus', 'getSelection', 'matchMedia', 'screen']);

function files(dir) {
  return readdirSync(dir).flatMap((f) => {
    const p = join(dir, f);
    return statSync(p).isDirectory() ? files(p) : /\.(cpp|h)$/.test(f) ? [p] : [];
  });
}

// The { ... } JavaScript block after each EM_ASM*( or EM_JS(..., in order.
function blocks(src) {
  const out = [];
  const re = /\bEM_(ASM\w*|JS)\s*\(/g;
  let m;
  while ((m = re.exec(src))) {
    const open = src.indexOf('{', m.index);
    if (open < 0) break;
    let depth = 0, i = open;
    for (; i < src.length; i++) {
      if (src[i] === '{') depth++;
      else if (src[i] === '}' && --depth === 0) break;
    }
    out.push({ start: open, body: src.slice(open, i + 1) });
    re.lastIndex = i;
  }
  return out;
}

let problems = 0;
for (const file of [...files('engine'), ...files('app')]) {
  const src = readFileSync(file, 'utf8');
  for (const { start, body } of blocks(src)) {
    const checks = [/\b(?:window|Module)\.(\w+)/g, /\]\s*\.(\w+)/g];
    for (const re of checks) {
      let m;
      while ((m = re.exec(body))) {
        if (BUILTINS.has(m[1])) continue;
        const line = src.slice(0, start + m.index).split('\n').length;
        console.log(`${file}:${line}: unquoted name in EM_ASM: ${m[0]} (write ['${m[1]}'])`);
        problems++;
      }
    }
  }
}
console.log(problems ? `${problems} problem(s)` : 'EM_ASM lint: ok');
process.exit(problems ? 1 : 0);
