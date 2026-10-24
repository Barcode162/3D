// Compiles every WGSL shader (looped/shaders/*.wgsl and the projects') with
// the browser's real WebGPU compiler and prints each error and warning with
// its line, so a shader mistake is found in seconds instead of from the
// game's log (docs/archive/AUTOMATION.md):
//   node tools/wgsl_check.mjs [file.wgsl ...]
// Exits 1 on any error. In tools/check.mjs (game).
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { launchBrowser } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

let files = process.argv.slice(2);
if (!files.length) {
  const dirs = ['looped/shaders', ...readdirSync('bar/projects', { withFileTypes: true })
    .filter((d) => d.isDirectory()).map((d) => join('bar/projects', d.name))];
  for (const d of dirs) {
    try { for (const f of readdirSync(d)) if (f.endsWith('.wgsl')) files.push(join(d, f)); } catch {}
  }
}
// WebGPU needs a secure page: any page of the local test server will do.
const site = serveSite();
const browser = await launchBrowser();
const page = await browser.newPage();
await page.goto(`${site.url}/wgsl-check-blank`).catch(() => {});
const sources = files.map((f) => ({ file: f, code: readFileSync(f, 'utf8') }));
const results = await page.evaluate(async (sources) => {
  const adapter = await navigator.gpu?.requestAdapter();
  if (!adapter) return { error: 'no WebGPU adapter' };
  const device = await adapter.requestDevice();
  const out = [];
  for (const s of sources) {
    const module = device.createShaderModule({ code: s.code, label: s.file });
    const info = await module.getCompilationInfo();
    out.push({ file: s.file, messages: info.messages.map((m) => ({ type: m.type, line: m.lineNum, col: m.linePos, text: m.message })) });
  }
  return { out };
}, sources);
await browser.close();
site.close();
if (results.error) { console.log(results.error); process.exit(1); }
let errors = 0;
for (const r of results.out) {
  const bad = r.messages.filter((m) => m.type === 'error');
  errors += bad.length;
  console.log(`${bad.length ? 'FAIL' : 'ok  '} ${r.file}`);
  const lines = sources.find((s) => s.file === r.file).code.split('\n');
  for (const m of r.messages) {
    console.log(`     ${m.type} ${m.line}:${m.col} ${m.text}`);
    if (m.line > 0) console.log(`       | ${lines[m.line - 1]?.trim()}`);
  }
}
console.log(errors ? `${errors} SHADER ERROR(S)` : 'ALL SHADERS COMPILE');
process.exit(errors ? 1 : 0);
