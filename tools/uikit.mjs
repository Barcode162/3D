#!/usr/bin/env node
// The UI kit (docs/UI_PLAN.md R1): the theme and screen files in ui/system/
// are the truth; the CSS the pages load is written from them.
//   node tools/uikit.mjs css      write ui/themes.css and looped.js's theme list from ui/system/themes/
//   node tools/uikit.mjs check    exit 1 if a theme or screen file is wrong, or themes.css is out of date
//   node tools/uikit.mjs list     the themes and screens, one line each
// Files:
//   ui/system/tokens.json        every token once: its group, kind and what it does
//   ui/system/themes/<id>.json   {format, id, name, about, scheme, click, tokens:{"bg": "#0A0908", ...}}
//   ui/system/screens/<id>.json  {format, id, name, about, root: <part>} (R1.3)
import { readFileSync, writeFileSync, readdirSync, existsSync } from 'node:fs';
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
  return readdirSync(dir).filter((f) => f.endsWith('.json')).sort().map((f) => ({ file: f, ...readJson(join(dir, f)) }));
}

// What a value of each kind looks like (the editor shows a matching control).
const KIND = {
  color: (v) => /^#[0-9a-f]{3,8}$/i.test(v) || /^rgba?\([^)]*\)$/.test(v) || v === 'transparent',
  length: (v) => /^-?[\d.]+(px|em|rem|%)?$/.test(v),
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
  }
  return lines.join('\n') + '\n';
}

// looped.js keeps the theme list (names, the key click) between two marker lines.
export function looksJs(themes) {
  return themes.map((t) => `    ${/^[a-z]+$/.test(t.id) ? t.id : JSON.stringify(t.id)}: { name: ${JSON.stringify(t.name)}, about: ${JSON.stringify(t.about)}, click: ${JSON.stringify(t.click)} }`).join(',\n');
}
const MARK_A = '    // LOOKS: written by tools/uikit.mjs from ui/system/themes/', MARK_B = '    // end LOOKS';
export function withLooks(js, themes) {
  const a = js.indexOf(MARK_A), b = js.indexOf(MARK_B);
  if (a < 0 || b < 0) throw new Error('ui/looped.js: the LOOKS marker lines are missing');
  return js.slice(0, a + MARK_A.length) + '\n' + looksJs(themes) + '\n' + js.slice(b);
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
  for (const th of themes) out.push(...themeProblems(tokens, th));
  if (readFileSync('ui/themes.css', 'utf8') !== themesCss(tokens, index, themes)) out.push('ui/themes.css is out of date: run `node tools/uikit.mjs css`');
  const js = readFileSync('ui/looped.js', 'utf8');
  try { if (withLooks(js, themes) !== js) out.push('ui/looped.js theme list is out of date: run `node tools/uikit.mjs css`'); } catch (e) { out.push(e.message); }
  return out;
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const cmd = process.argv[2] ?? 'check';
  if (cmd === 'css') {
    const tokens = loadTokens(), { index, themes } = loadThemes();
    const bad = themes.flatMap((t) => themeProblems(tokens, t));
    if (bad.length) { console.log(bad.join('\n')); process.exit(1); }
    writeFileSync('ui/themes.css', themesCss(tokens, index, themes));
    writeFileSync('ui/looped.js', withLooks(readFileSync('ui/looped.js', 'utf8'), themes));
    console.log(`wrote ui/themes.css and looped.js's list: ${themes.length} themes, ${tokens.tokens.length} tokens each`);
  } else if (cmd === 'check') {
    const bad = problems();
    console.log(bad.length ? bad.join('\n') : 'ok  uikit: themes, tokens and themes.css agree');
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
