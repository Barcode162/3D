#!/usr/bin/env node
// The UI kit (docs/archive/UI_PLAN.md R1): the theme and screen files in ui/system/
// are the truth; the CSS the pages load is written from them.
//   node tools/uikit.mjs css      write ui/themes.css and looped.js's theme list from ui/system/themes/
//   node tools/uikit.mjs check    exit 1 if a theme or screen file is wrong, or themes.css is out of date
//   node tools/uikit.mjs list     the themes and screens, one line each
//   node tools/uikit.mjs status   the UI Editor's live version vs the one the repo has
//   node tools/uikit.mjs pull     the UI Editor's published version → ui/system/ (then css); says what changed
//   node tools/uikit.mjs push --note ".."   the repo's themes and screens → a new version by Claude
//                                 (refused if someone published since the last pull; needs HUB_KEY)
// The published version the repo matches: ui/system/live-version.json (commit it).
// UI_URL overrides the store (toolbelt/ui-editor/api.php).
// Files:
//   ui/system/themes/index.json  {order, default (what a page wears when it names none: the bar's menus),
//                                house (the project's own theme, R2: the Toolbelt, the editor, Compare)}
//   ui/system/tokens.json        every token once: its group, kind and what it does
//   ui/system/themes/<id>.json   {format, id, name, about, scheme, click, tokens:{"bg": "#0A0908", ...},
//                                other?: {scheme, tokens:{...}}}  the theme's other half (light for a dark
//                                theme, or dark): only the tokens that differ; the page follows the
//                                phone's setting unless <html data-scheme="light|dark"> says otherwise
//   ui/system/screens/<id>.json  {format, id, name, about, root: <part>} (R1.3)
import { readFileSync, writeFileSync, readdirSync, existsSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';

export const SYS = 'ui/system';
const readJson = (p) => JSON.parse(readFileSync(p, 'utf8'));

export function loadTokens() { return readJson(join(SYS, 'tokens.json')); }
export function loadThemes() {
  const dir = join(SYS, 'themes'), index = readJson(join(dir, 'index.json'));
  return { index, themes: index.order.map((id) => readJson(join(dir, `${id}.json`))) };
}
export function loadScreens() {
  const dir = join(SYS, 'screens');
  if (!existsSync(dir)) return [];
  return readdirSync(dir).filter((f) => f.endsWith('.json') && f !== 'index.json').sort().map((f) => ({ file: f, ...readJson(join(dir, f)) }));
}

// What a value of each kind looks like (the editor shows a matching control).
const KIND = {
  color: (v) => /^#[0-9a-f]{3,8}$/i.test(v) || /^rgba?\([^)]*\)$/.test(v) || v === 'transparent',
  length: (v) => /^-?[\d.]+(px|em|rem|%|s|ms)?$/.test(v),
  number: (v) => /^-?[\d.]+$/.test(v),
  font: (v) => v.length > 0 && !/[;{}]/.test(v),
  paint: (v) => v.length > 0 && !/[;{}]/.test(v),   // gradients, shadows, filters: any CSS value
  choice: () => true,
};

export function themeProblems(tokens, theme) {
  const out = [], at = `theme "${theme.id}"`;
  if (theme.format !== 1) out.push(`${at}: format is not 1`);
  if (!/^[a-z][a-z0-9-]*$/.test(theme.id || '')) out.push(`${at}: id must be lower-case words`);
  if (!theme.name) out.push(`${at}: no name`);
  if (!['dark', 'light'].includes(theme.scheme)) out.push(`${at}: scheme must be dark or light`);
  for (const t of tokens.tokens) {
    const v = theme.tokens?.[t.id];
    if (v === undefined) { out.push(`${at}: token "${t.id}" is missing`); continue; }
    if (typeof v !== 'string') { out.push(`${at}: token "${t.id}" must be text`); continue; }
    const ok = t.kind === 'choice' ? t.choices.includes(v) : KIND[t.kind]?.(v.trim());
    if (!ok) out.push(`${at}: token "${t.id}" = "${v}" is not a ${t.kind}`);
  }
  for (const k of Object.keys(theme.tokens || {})) if (!tokens.tokens.some((t) => t.id === k)) out.push(`${at}: unknown token "${k}" (add it to tokens.json)`);
  if (theme.other !== undefined) {
    const o = theme.other;
    if (!o || !['dark', 'light'].includes(o.scheme) || o.scheme === theme.scheme) out.push(`${at}: other.scheme must be the opposite of scheme`);
    for (const [k, v] of Object.entries(o?.tokens || {})) {
      const t = tokens.tokens.find((x) => x.id === k);
      if (!t) { out.push(`${at}: other has an unknown token "${k}"`); continue; }
      const ok = typeof v === 'string' && (t.kind === 'choice' ? t.choices.includes(v) : KIND[t.kind]?.(v.trim()));
      if (!ok) out.push(`${at}: other token "${k}" = "${v}" is not a ${t.kind}`);
    }
  }
  return out;
}

export function themesCss(tokens, index, themes) {
  const lines = [
    '/* Looped themes. WRITTEN BY tools/uikit.mjs FROM ui/system/themes/: edit those',
    '   files (or use the UI Editor), then run `node tools/uikit.mjs css`. Never edit',
    '   this file by hand: `node tools/uikit.mjs check` fails when it is out of date.',
    '   A theme is tokens only (ui/system/tokens.json says what each one does);',
    '   looped.css builds every control from them. Pick one with',
    '   <html data-look="...">; looped.js does it from the player\'s choice. */',
  ];
  for (const th of themes) {
    const sel = th.id === index.default ? `:root, :root[data-look="${th.id}"]` : `:root[data-look="${th.id}"]`;
    lines.push('', `${sel} {`, `  /* ---- ${th.name}: ${th.about} ---- */`, `  color-scheme: ${th.scheme};`);
    let group = null;
    for (const t of tokens.tokens) {
      if (t.group !== group) { group = t.group; }
      lines.push(`  --${t.id}: ${th.tokens[t.id]};`);
    }
    lines.push('}');
    if (th.other && Object.keys(th.other.tokens || {}).length) {
      // The other half: when the page asks for it, or the phone is set to it and the page doesn't say.
      const body = [`  color-scheme: ${th.other.scheme};`, ...tokens.tokens.filter((t) => t.id in th.other.tokens).map((t) => `  --${t.id}: ${th.other.tokens[t.id]};`)];
      const base = th.id === index.default ? [':root:not([data-look])', `:root[data-look="${th.id}"]`] : [`:root[data-look="${th.id}"]`];
      lines.push(`${base.map((b) => `${b}[data-scheme="${th.other.scheme}"]`).join(', ')} {`, ...body, '}');
      lines.push(`@media (prefers-color-scheme: ${th.other.scheme}) {`, `  ${base.map((b) => `${b}:not([data-scheme])`).join(', ')} {`, ...body.map((l) => '  ' + l), '  }', '}');
    }
  }
  return lines.join('\n') + '\n';
}

// looped.js keeps the theme list (names, the key click) between two marker lines.
export function looksJs(themes, index) {
  // The default theme first: looped.js takes the first one as its default.
  if (index) themes = [...themes].sort((a, b) => (b.id === index.default) - (a.id === index.default));
  return themes.map((t) => `    ${/^[a-z]+$/.test(t.id) ? t.id : JSON.stringify(t.id)}: { name: ${JSON.stringify(t.name)}, about: ${JSON.stringify(t.about)}, click: ${JSON.stringify(t.click)} }`).join(',\n');
}
const MARK_A = '    // LOOKS: written by tools/uikit.mjs from ui/system/themes/', MARK_B = '    // end LOOKS';
export function withLooks(js, themes, index = loadThemes().index) {
  const a = js.indexOf(MARK_A), b = js.indexOf(MARK_B);
  if (a < 0 || b < 0) throw new Error('ui/looped.js: the LOOKS marker lines are missing');
  return js.slice(0, a + MARK_A.length) + '\n' + looksJs(themes, index) + '\n' + js.slice(b);
}

export function loadParts() { return readJson(join(SYS, 'parts.json')); }
const ICONS = () => new Set([...readFileSync('ui/icons.svg', 'utf8').matchAll(/id="i-([a-z-]+)"/g)].map((m) => m[1]));

// A screen's tree: every part a known type, every prop known and of its kind,
// children only on containers.
export function screenProblems(parts, screen, icons = ICONS()) {
  const out = [], at = `screen "${screen.id}"`, defs = Object.fromEntries(parts.parts.map((p) => [p.type, p]));
  if (screen.format !== 1) out.push(`${at}: format is not 1`);
  if (!/^[a-z][a-z0-9-]*$/.test(screen.id || '')) out.push(`${at}: id must be lower-case words`);
  if (screen.file && screen.file !== `${screen.id}.json`) out.push(`${at}: the file must be called ${screen.id}.json`);
  if (!screen.name) out.push(`${at}: no name`);
  const walk = (part, path) => {
    const where = `${at} part ${path || 'root'}`, d = defs[part?.type];
    if (!d) { out.push(`${where}: unknown type "${part?.type}"`); return; }
    for (const [k, v] of Object.entries(part)) {
      if (k === 'type' || k === 'id') continue;
      if (k === 'children') {
        if (!d.children) out.push(`${where}: a ${part.type} can't hold parts`);
        else if (!Array.isArray(v)) out.push(`${where}: children must be a list`);
        else v.forEach((c, i) => walk(c, path === '' ? String(i) : `${path}.${i}`));
        continue;
      }
      const p = d.props[k];
      if (!p) { out.push(`${where}: a ${part.type} has no "${k}"`); continue; }
      const ok = p.kind === 'text' ? typeof v === 'string' : p.kind === 'number' ? typeof v === 'number' && isFinite(v)
        : p.kind === 'bool' ? typeof v === 'boolean' : p.kind === 'choice' ? p.choices.includes(v)
        : p.kind === 'icon' ? v === '' || icons.has(v) : p.kind === 'list' ? Array.isArray(v) && v.every((x) => typeof x === 'string') : false;
      if (!ok) out.push(`${where}: "${k}" = ${JSON.stringify(v)} is not a ${p.kind}${p.kind === 'icon' ? ' in ui/icons.svg' : ''}`);
    }
  };
  walk(screen.root, '');
  return out;
}

export function problems() {
  const out = [];
  const tokens = loadTokens(), { index, themes } = loadThemes();
  const ids = new Set();
  for (const t of tokens.tokens) {
    if (ids.has(t.id)) out.push(`tokens.json: "${t.id}" is listed twice`); ids.add(t.id);
    if (!tokens.groups.some((g) => g.id === t.group)) out.push(`tokens.json: "${t.id}" has an unknown group "${t.group}"`);
    if (!KIND[t.kind]) out.push(`tokens.json: "${t.id}" has an unknown kind "${t.kind}"`);
    if (!t.what) out.push(`tokens.json: "${t.id}" says nothing about what it does`);
  }
  if (!index.order.includes(index.default)) out.push('themes/index.json: the default theme is not in the order');
  if (index.house && !index.order.includes(index.house)) out.push('themes/index.json: the house theme is not in the order');
  for (const th of themes) out.push(...themeProblems(tokens, th));
  const parts = loadParts(), types = new Set();
  for (const p of parts.parts) {
    if (types.has(p.type)) out.push(`parts.json: "${p.type}" is listed twice`); types.add(p.type);
    for (const [k, d] of Object.entries(p.props)) if (!['text', 'number', 'bool', 'choice', 'icon', 'list'].includes(d.kind)) out.push(`parts.json: ${p.type}.${k} has an unknown kind "${d.kind}"`);
  }
  const screens = loadScreens(), order = readJson(join(SYS, 'screens', 'index.json')).order;
  for (const sc of screens) {
    out.push(...screenProblems(parts, sc));
    if (!order.includes(sc.id)) out.push(`screens/index.json: "${sc.id}" is not in the order`);
  }
  for (const id of order) if (!screens.some((sc) => sc.id === id)) out.push(`screens/index.json: "${id}" has no file`);
  if (readFileSync('ui/themes.css', 'utf8') !== themesCss(tokens, index, themes)) out.push('ui/themes.css is out of date: run `node tools/uikit.mjs css`');
  const js = readFileSync('ui/looped.js', 'utf8');
  try { if (withLooks(js, themes) !== js) out.push('ui/looped.js theme list is out of date: run `node tools/uikit.mjs css`'); } catch (e) { out.push(e.message); }
  return out;
}

// ---- The UI Editor's store ----
const STORE = process.env.UI_URL || 'https://joshuavelasco.com/toolbelt/ui-editor/api.php';
const MARK = join(SYS, 'live-version.json');
const mark = () => (existsSync(MARK) ? readJson(MARK) : { version: 0 });
async function storeGet(q) { const r = await fetch(STORE + q, { cache: 'no-store' }); if (!r.ok) throw new Error(`${q}: HTTP ${r.status}`); return r.json(); }
export function repoBundle() {
  const files = {}, { themes } = loadThemes();
  for (const t of themes) files[`themes/${t.id}.json`] = t;
  for (const sc of loadScreens()) { const { file, ...rest } = sc; files[`screens/${sc.id}.json`] = rest; }
  return { format: 1, files };
}
// Writes a bundle's files into ui/system/ (new themes and screens join the end
// of their index), then the CSS. Returns the paths that changed.
export function writeBundle(bundle) {
  const changed = [];
  for (const [path, content] of Object.entries(bundle.files)) {
    const m = /^(themes|screens)\/([a-z][a-z0-9-]*)\.json$/.exec(path);
    if (!m || content.id !== m[2]) continue;
    const file = join(SYS, path), text = JSON.stringify(content, null, 2) + '\n';
    if (!existsSync(file) || readFileSync(file, 'utf8') !== text) { mkdirSync(join(SYS, m[1]), { recursive: true }); writeFileSync(file, text); changed.push(path); }
    const ix = join(SYS, m[1], 'index.json'), index = readJson(ix);
    if (!index.order.includes(m[2])) { index.order.push(m[2]); writeFileSync(ix, JSON.stringify(index, null, 2) + '\n'); }
  }
  return changed;
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const cmd = process.argv[2] ?? 'check';
  const flag = (n) => { const i = process.argv.indexOf('--' + n); return i > 0 ? process.argv[i + 1] : undefined; };
  if (cmd === 'status') {
    const live = await storeGet('?live');
    console.log(`UI Editor live: v${live.version}${live.author ? ` by ${live.author}` : ''}${live.note ? ': ' + live.note : ''}; the repo matches v${mark().version}`);
    process.exit(0);
  }
  if (cmd === 'pull') {
    const live = await storeGet('?live');
    if (!live.version) { console.log('Nothing published from the UI Editor yet.'); process.exit(0); }
    if (live.version === mark().version) { console.log(`Up to date (v${live.version}).`); process.exit(0); }
    const b = await storeGet('?bundle=' + live.version);
    const changed = writeBundle(b);
    const bad = problems().filter((p) => !/out of date/.test(p));
    if (bad.length) { console.log('The published version has problems (files written, CSS not):\n' + bad.join('\n')); process.exit(1); }
    const tokens = loadTokens(), { index, themes } = loadThemes();
    writeFileSync('ui/themes.css', themesCss(tokens, index, themes));
    writeFileSync('ui/looped.js', withLooks(readFileSync('ui/looped.js', 'utf8'), themes));
    writeFileSync(MARK, JSON.stringify({ version: live.version, author: live.author, at: live.at, note: live.note }, null, 2) + '\n');
    console.log(`Pulled v${live.version} by ${live.author}${live.note ? ' ("' + live.note + '")' : ''}: ` + (changed.length ? 'changed ' + changed.join(', ') : 'no file changed'));
    process.exit(0);
  }
  if (cmd === 'push') {
    const KEY = process.env.HUB_KEY;
    if (!KEY) { console.log('No HUB_KEY in the environment.'); process.exit(2); }
    const bad = problems();
    if (bad.length) { console.log(bad.join('\n')); process.exit(1); }
    const r = await fetch(STORE, { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY },
      body: JSON.stringify({ a: 'publish', base: mark().version, note: flag('note') || 'From the repo', bundle: repoBundle() }) });
    const j = await r.json();
    if (!j.ok) { console.log(`Refused: ${j.error}`); process.exit(1); }
    writeFileSync(MARK, JSON.stringify({ version: j.live.version, author: j.live.author, at: j.live.at, note: j.live.note }, null, 2) + '\n');
    console.log(`Published v${j.live.version} (commit ${MARK}).`);
    process.exit(0);
  }
  if (cmd === 'css') {
    const tokens = loadTokens(), { index, themes } = loadThemes();
    const bad = themes.flatMap((t) => themeProblems(tokens, t));
    if (bad.length) { console.log(bad.join('\n')); process.exit(1); }
    writeFileSync('ui/themes.css', themesCss(tokens, index, themes));
    writeFileSync('ui/looped.js', withLooks(readFileSync('ui/looped.js', 'utf8'), themes));
    console.log(`wrote ui/themes.css and looped.js's list: ${themes.length} themes, ${tokens.tokens.length} tokens each`);
  } else if (cmd === 'check') {
    const bad = problems();
    console.log(bad.length ? bad.join('\n') : 'ok  uikit: themes, tokens, screens and themes.css agree');
    process.exit(bad.length ? 1 : 0);
  } else if (cmd === 'list') {
    const { index, themes } = loadThemes();
    for (const t of themes) console.log(`theme  ${t.id.padEnd(12)} ${t.name}${t.id === index.default ? ' (default)' : ''}`);
    for (const s of loadScreens()) console.log(`screen ${s.id.padEnd(12)} ${s.name}`);
  } else {
    console.log(readFileSync(new URL(import.meta.url), 'utf8').split('\n').slice(1, 11).join('\n'));
    process.exit(2);
  }
}
