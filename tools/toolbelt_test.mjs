// The Toolbelt's list (toolbelt/tools.json, docs/UI_PLAN.md R1.1): every
// tool has an id, a group that exists, a name, a line of what it does, an
// icon that exists, and a way in (a page or a command) unless it's "soon";
// pages inside this repo exist. No browser.  Usage: node tools/toolbelt_test.mjs
import { readFileSync, existsSync } from 'node:fs';

let failed = 0;
const check = (ok, what) => { if (!ok) { console.log(`FAIL ${what}`); failed++; } };
const data = JSON.parse(readFileSync('toolbelt/tools.json', 'utf8'));
const icons = new Set([...readFileSync('ui/icons.svg', 'utf8').matchAll(/id="i-([a-z-]+)"/g)].map((m) => m[1]));
const groups = new Set(data.groups.map((g) => g.id));
const projects = new Set((data.projects || []).map((p) => p.id));
check(projects.size > 0 && projects.size === data.projects.length, 'projects listed once each');
for (const p of data.projects || []) check(!!p.name && !!p.about && !!p.status, `project "${p.id}": a name, a line and a status`);
const ids = new Set();
// Where a site path lives in this repo (the rest, like /bar/ and /classic/, is built or kept elsewhere).
const LOCAL = [['/toolbelt/', 'toolbelt/'], ['/studio/', 'studio/'], ['/hub/', 'hub/'], ['/ui/', 'ui/']];
check(data.format === 1, 'format is 1');
for (const t of data.tools) {
  const at = `tool "${t.id}"`;
  check(/^[a-z][a-z0-9-]*$/.test(t.id || ''), `${at}: id is lower-case words`);
  check(!ids.has(t.id), `${at}: id is used once`); ids.add(t.id);
  check(groups.has(t.group), `${at}: group "${t.group}" exists`);
  check(projects.has(t.project), `${at}: project "${t.project}" exists`);
  check(typeof t.name === 'string' && t.name.length > 0 && t.name.length <= 24, `${at}: a short name`);
  check(typeof t.what === 'string' && t.what.length > 0 && t.what.length <= 110, `${at}: one line of what it does`);
  check(icons.has(t.icon), `${at}: icon "${t.icon}" is in ui/icons.svg`);
  check(t.status === undefined || t.status === 'soon', `${at}: status is "soon" or nothing`);
  if (t.status !== 'soon') check(!!(t.href || t.run), `${at}: has a page or a command`);
  if (t.href && t.status !== 'soon') {
    const m = LOCAL.find(([p]) => t.href.startsWith(p));
    if (m) {
      const rest = t.href.slice(m[0].length).split('?')[0];
      const file = m[1] + (rest === '' || rest.endsWith('/') ? rest + 'index.html' : rest);
      check(existsSync(file) || existsSync(file.replace(/index\.html$/, 'index.php')), `${at}: page ${t.href} exists (${file})`);
    }
  }
  if (t.run && t.status !== 'soon') check(existsSync(t.run.split(' ')[1]), `${at}: command file ${t.run.split(' ')[1]} exists`);
}
console.log(failed ? `${failed} problem(s)` : `ok  toolbelt: ${data.tools.length} tools in ${groups.size} groups`);
process.exit(failed ? 1 : 0);
