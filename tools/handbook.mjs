#!/usr/bin/env node
// The Handbook's generated pages (restructure J6): written from the files, so
// they can't go stale; docs/handbook/generated/ is never edited by hand.
//   node tools/handbook.mjs write     write every generated page
//   node tools/handbook.mjs check     the generated pages are current, every folder is
//                                     described (docs/handbook/folders.json), every link in the
//                                     Handbook leads to a file (exit 1 if not)
// Pages: map.md (every folder: what, files, languages, who uploads it and where),
// systems.md (every system page under docs/handbook/systems/, by chapter, with its status),
// commands.md (the front door's commands, from tools/lib/commands.mjs),
// data-world.md, data-ui.md, data-lists.md, data-protocol.md (the data reference, J6.3)
// and inventory.md (every thing in every place, its model; tools/lib/handbook_data.mjs).
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { execSync } from 'node:child_process';
import { dirname, join, extname } from 'node:path';
import { worldPage, uiPage, listsPage, protocolPage, inventoryPage } from './lib/handbook_data.mjs';

const OUT = 'docs/handbook/generated';
const BANNER = (src) => `<!-- generated: node tools/handbook.mjs write (from ${src}); never edit by hand -->\n`;
const json = (f) => JSON.parse(readFileSync(f, 'utf8'));
// The repo's files as they are now: tracked or new (not ignored), and still on disk.
const tracked = () => [...new Set(execSync('git ls-files --cached --others --exclude-standard', { encoding: 'utf8' }).split('\n').filter(Boolean))].filter((f) => existsSync(f));
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

// The system pages (J6.2): one template, ten parts, in this order.
const PARTS = ['What it is', 'Status', 'What you can do with it', 'How it works', 'Data', 'Commands', 'Limits', 'Checks', 'Known traps', 'History'];
const CHAPTERS = ['engine', 'world', 'site', 'tools', 'backbone'];
function systemPages() {
  return tracked().concat(untracked()).filter((f) => /^docs\/handbook\/systems\/[a-z]+\/[a-z0-9-]+\.md$/.test(f)).sort();
}
const untracked = () => execSync('git ls-files -o --exclude-standard docs/handbook', { encoding: 'utf8' }).split('\n').filter(Boolean);
function systemInfo(f) {
  const text = readFileSync(f, 'utf8');
  const title = (text.match(/^# (.*)$/m) || [, f])[1];
  const status = ((text.split('## Status')[1] || '').trim().match(/^\*\*(Live|Partial|Planned)/) || [, '?'])[1];
  const what = ((text.split('## What it is')[1] || '').trim().split('\n')[0] || '');
  return { title, status, what, parts: [...text.matchAll(/^## (.*)$/gm)].map((m) => m[1]) };
}
function systemsPage() {
  const pages = systemPages();
  let out = BANNER('docs/handbook/systems/') + '# The systems\n\nEvery system of J Production, one page each, by chapter (the five chapters of the plan). Each page has the same ten parts: ' + PARTS.join(' · ') + '.\n';
  for (const ch of CHAPTERS) {
    const mine = pages.filter((f) => f.split('/')[3] === ch);
    out += `\n## The ${ch}\n\n| System | Status | What it is |\n|---|---|---|\n`;
    for (const f of mine) { const s = systemInfo(f); out += `| [${cell(s.title)}](../systems/${ch}/${f.split('/').pop()}) | ${s.status} | ${cell(s.what)} |\n`; }
  }
  return out;
}
// Every command of the front door (tools/lib/commands.mjs), with its usage.
async function commandsPageAsync() {
  const { COMMANDS } = await import('./lib/commands.mjs');
  return BANNER('tools/lib/commands.mjs') + '# The commands\n\nEvery tool runs through one front door: `node tools/project.mjs <command>` (Claude runs them; the Playbook says when). `node tools/project.mjs help` prints the same list.\n\n| Command | What it does | Usage | Runs |\n|---|---|---|---|\n' +
    COMMANDS.map((c) => `| \`${c.name}\` | ${cell(c.summary)} | \`node tools/project.mjs ${cell(c.usage)}\` | ${c.own ? 'project.mjs itself' : '`' + c.run.join(' ') + '`'} |`).join('\n') + '\n';
}
const COMMANDS_PAGE = await commandsPageAsync();
const PROTOCOL_PAGE = await protocolPage(BANNER);
const PAGES = { 'map.md': mapPage, 'systems.md': systemsPage, 'commands.md': () => COMMANDS_PAGE,
  'data-world.md': () => worldPage(BANNER), 'data-ui.md': () => uiPage(BANNER), 'data-lists.md': () => listsPage(BANNER),
  'data-protocol.md': () => PROTOCOL_PAGE, 'inventory.md': () => inventoryPage(BANNER) };

function problems() {
  const out = [], files = tracked(), described = json('docs/handbook/folders.json').folders;
  for (const d of foldersNeeded(files)) if (!described[d] && !d.startsWith('docs/archive/')) out.push(`folder ${d}/ has no line in docs/handbook/folders.json`);
  for (const d of Object.keys(described)) if (d !== '.' && !files.some((f) => f.startsWith(d + '/')) && !(d === OUT)) out.push(`docs/handbook/folders.json describes ${d}/, which holds no file`);
  for (const [name, make] of Object.entries(PAGES)) {
    const file = join(OUT, name);
    if (!existsSync(file) || readFileSync(file, 'utf8') !== make()) out.push(`${file} is not current (node tools/handbook.mjs write)`);
  }
  for (const f of systemPages()) {
    const s = systemInfo(f);
    if (!CHAPTERS.includes(f.split('/')[3])) out.push(`${f}: not in one of the five chapters`);
    if (s.parts.join('|') !== PARTS.join('|')) out.push(`${f}: its parts must be exactly ${PARTS.join(', ')}`);
    if (s.status === '?') out.push(`${f}: its Status must start with **Live**, **Partial** or **Planned**`);
  }
  // No file points at a doc that isn't there (J6.6). Not checked: history (the archive, the
  // records, the restructure's spec) and the stores' copies (they keep old paths until
  // their next publish, KI-31).
  const SKIP = /^(docs\/archive\/|docs\/records\/|docs\/plan\/restructure\.md$|assets\/world\/|ui\/system\/|tools\/lib\/docmoves\.mjs$)/;  // (docmoves.mjs: the map of the old paths)
  const TEXT = /\.(md|cpp|h|js|mjs|php|html|py|wgsl|yml|json|sh|css|txt|toml)$/;
  for (const f of files.concat(untracked())) {
    if (SKIP.test(f) || !TEXT.test(f) || !existsSync(f)) continue;
    const text = readFileSync(f, 'utf8');
    for (const m of text.matchAll(/(?<![\w/.-])docs\/[A-Za-z0-9_./-]*[A-Za-z0-9_/]/g)) {
      const next = text[m.index + m[0].length] || '';
      if ('<{$*'.includes(next) && next) continue;  // a pattern (docs/plan/jobs/J<n>.md, docs/plan/**)
      if (!existsSync(m[0])) out.push(`${f}: points at ${m[0]}, which isn't there`);
    }
  }
  // Every relative link in the Handbook leads to a file.
  for (const f of files.concat(untracked()).filter((x) => x.startsWith('docs/handbook/') && x.endsWith('.md'))) {
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
  console.log(bad.length ? bad.join('\n') : 'ok  handbook: the generated pages are current, every folder is described, every link leads to a file, no file points at a missing doc');
  process.exit(bad.length ? 1 : 0);
} else {
  console.log(readFileSync(new URL(import.meta.url), 'utf8').split('\n').slice(1, 9).join('\n')); process.exit(2);
}
