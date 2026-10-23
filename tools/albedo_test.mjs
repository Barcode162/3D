// Real material brightness (L1.4): assets/models/albedo.json (made by
// tools/albedo_audit.py) covers every model and every material, its fixes
// are sane, and the rules file reads. Run by tools/check.mjs (native).
import { readFileSync, readdirSync } from 'node:fs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const table = JSON.parse(readFileSync('assets/models/albedo.json', 'utf8'));
const rules = JSON.parse(readFileSync('assets/models/albedo_rules.json', 'utf8')).rules;
check(Array.isArray(rules) && rules.every((r) => r.length === 4 && r[1] <= r[2]), 'the rules read (name, low, high, why)');

// A glb's material names, from its JSON chunk.
function materials(path) {
  const b = readFileSync(path);
  const len = b.readUInt32LE(12);
  const gltf = JSON.parse(b.subarray(20, 20 + len).toString('utf8'));
  return (gltf.materials ?? []).map((m, i) => m.name ?? `material${i}`);
}
const models = readdirSync('assets/models').filter((f) => f.endsWith('.glb') && !f.includes('.astc.'));
const missing = [];
for (const f of models) {
  if (!table[f]) { missing.push(f); continue; }
  for (const m of materials(`assets/models/${f}`)) if (!table[f][m]) missing.push(`${f}: ${m}`);
}
check(missing.length === 0, `every model and material is measured${missing.length ? ' (missing: ' + missing.slice(0, 5).join(', ') + '; run python3 tools/albedo_audit.py)' : ''}`);
let fixes = 0, bad = [];
for (const [f, mats] of Object.entries(table)) {
  for (const [m, e] of Object.entries(mats)) {
    if (!Array.isArray(e.albedo) || e.albedo.length !== 3) bad.push(`${f}: ${m} has no albedo`);
    if (e.fix !== undefined) { fixes++; if (!(e.fix > 0.05 && e.fix < 60)) bad.push(`${f}: ${m} fix ${e.fix}`); }
  }
}
check(bad.length === 0, `albedos and fixes are sane${bad.length ? ': ' + bad.slice(0, 5).join(', ') : ''}`);
console.log(`     ${Object.keys(table).length} models, ${fixes} materials moved into real ranges`);
// Real roughness (M0): the rules read, every least roughness is 0..1, and cloth is matte.
const roughRules = JSON.parse(readFileSync('assets/models/rough_rules.json', 'utf8')).rules;
check(Array.isArray(roughRules) && roughRules.every((r) => r.length === 3 && r[1] >= 0 && r[1] <= 1), 'the roughness rules read (name, least, why)');
const roughBad = [];
for (const [f, mats] of Object.entries(table)) {
  for (const [m, e] of Object.entries(mats)) {
    if (e.rough !== undefined && !(e.rough > 0 && e.rough <= 1)) roughBad.push(`${f}: ${m} ${e.rough}`);
    if (/coat|felt|rug|carpet/i.test(m) && e.rough !== undefined && e.rough < 0.9) roughBad.push(`${f}: ${m} is cloth but not matte`);
  }
}
check(roughBad.length === 0, `roughness is real (cloth matte)${roughBad.length ? ': ' + roughBad.slice(0, 5).join(', ') : ''}`);
if (failed) { console.log('ALBEDO TEST FAILED'); process.exit(1); }
console.log('ALBEDO TEST PASSED');
