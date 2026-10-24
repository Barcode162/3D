#!/usr/bin/env node
// The Handbook's generated pages (restructure J6): written from the files, so
// they can't go stale; docs/handbook/generated/ is never edited by hand.
//   node tools/handbook.mjs write     write every generated page
//   node tools/handbook.mjs check     the generated pages are current, every folder is
//                                     described (docs/handbook/folders.json), every link in the
//                                     Handbook leads to a file (exit 1 if not)
// Pages: map.md (every folder: what, files, languages, who uploads it and where).
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { execSync } from 'node:child_process';
import { dirname, join, extname } from 'node:path';

const OUT = 'docs/handbook/generated';
const BANNER = (src) => `<!-- generated: node tools/handbook.mjs write (from ${src}); never edit by hand -->\n`;
const json = (f) => JSON.parse(readFileSync(f, 'utf8'));
const tracked = () => execSync('git ls-files', { encoding: 'utf8' }).split('\n').filter(Boolean);
const cell = (s) => String(s ?? '').replace(/\|/g, '\\|').replace(/\n/g, ' ');

const LANG = {
  '.cpp': 'C++', '.h': 'C++', '.wgsl': 'WGSL', '.js': 'JavaScript', '.mjs': 'JavaScript', '.php': 'PHP', '.html': 'HTML',
  '.css': 'CSS', '.json': 'JSON', '.md': 'Markdown', '.py': 'Python', '.sh': 'shell', '.yml': 'YAML', '.txt': 'text',
  '.glb': '3D models', '.mid': 'MIDI', '.mp3': 'audio', '.ogg': 'audio', '.m4a': 'audio', '.wav': 'audio',
  '.png': 'pictures', '.jpg': 'pictures', '.svg': 'pictures', '.woff2': 'fonts', '.ttf': 'fonts', '.otf': 'fonts', '.wasm': 'WebAssembly',
};

// The folders that need a line: every folder holding a tracked file, two levels down.
function foldersNeeded(files) {
  const out = new Set(['.']);
  for (const f of files) {
    const parts = f.split('/').slice(0, -1);
    for (let i = 1; i <= Math.min(2, parts.length); i++) out.add(parts.slice(0, i).join('/'));
  }
  return [...out].sort();
}

function mapPage() {
  const files = tracked(), described = json('docs/handbook/folders.json').folders;
  const deploy = json('docs/handbook/deploy-map.json').entries;
  const entryOf = (f) => deploy.filter((e) => (e.path.endsWith('/') ? f.startsWith(e.path) : f === e.path)).sort((a, b) => b.path.length - a.path.length)[0];
  const rows = Object.keys(described).sort().map((dir) => {
    const inside = files.filter((f) => (dir === '.' ? !f.includes('/') : f.startsWith(dir + '/')));
    const langs = {};
    for (const f of inside) { const l = LANG[extname(f).toLowerCase()] || 'other'; langs[l] = (langs[l] || 0) + 1; }
    const top = Object.entries(langs).sort((a, b) => b[1] - a[1]).slice(0, 3).map(([l]) => l).join(', ');
    const sent = [...new Set(inside.map((f) => entryOf(f)).filter((e) => e && e.workflow).map((e) => `${e.workflow} → ${e.to}`))].join('; ') || 'not uploaded';
    return `| \`${dir === '.' ? '(root)' : dir + '/'}\` | ${cell(described[dir])} | ${inside.length} | ${top || '—'} | ${cell(sent)} |`;
  });
  return BANNER('docs/handbook/folders.json, docs/handbook/deploy-map.json and the files') +
    '# The map: every folder\n\n' +
    'What each folder is, how many files it holds, its main languages, and which workflow uploads it where ' +
    '(`docs/handbook/deploy-map.json`; "not uploaded": it stays in the repo). How the data moves between ' +
    'the places: [data-map.md](../data-map.md).\n\n' +
    '| Folder | What it is | Files | Languages | Uploaded by → to |\n|---|---|---|---|---|\n' + rows.join('\n') + '\n';
}

const PAGES = { 'map.md': mapPage };

function problems() {
  const out = [], files = tracked(), described = json('docs/handbook/folders.json').folders;
  for (const d of foldersNeeded(files)) if (!described[d] && !d.startsWith('docs/archive/')) out.push(`folder ${d}/ has no line in docs/handbook/folders.json`);
  for (const d of Object.keys(described)) if (d !== '.' && !files.some((f) => f.startsWith(d + '/')) && !(d === OUT)) out.push(`docs/handbook/folders.json describes ${d}/, which holds no file`);
  for (const [name, make] of Object.entries(PAGES)) {
    const file = join(OUT, name);
    if (!existsSync(file) || readFileSync(file, 'utf8') !== make()) out.push(`${file} is not current (node tools/handbook.mjs write)`);
  }
  // Every relative link in the Handbook leads to a file.
  for (const f of files.filter((x) => x.startsWith('docs/handbook/') && x.endsWith('.md'))) {
    for (const [, link] of readFileSync(f, 'utf8').matchAll(/\]\(([^)#\s]+)(?:#[^)]*)?\)/g)) {
      if (!/^(https?:|mailto:)/.test(link) && !existsSync(join(dirname(f), link))) out.push(`${f}: the link ${link} leads nowhere`);
    }
  }
  return out;
}

const [cmd] = process.argv.slice(2);
if (cmd === 'write') {
  mkdirSync(OUT, { recursive: true });
  for (const [name, make] of Object.entries(PAGES)) { writeFileSync(join(OUT, name), make()); console.log(`wrote ${join(OUT, name)}`); }
} else if (cmd === 'check') {
  const bad = problems();
  console.log(bad.length ? bad.join('\n') : 'ok  handbook: the generated pages are current, every folder is described, every link leads to a file');
  process.exit(bad.length ? 1 : 0);
} else {
  console.log(readFileSync(new URL(import.meta.url), 'utf8').split('\n').slice(1, 9).join('\n')); process.exit(2);
}
