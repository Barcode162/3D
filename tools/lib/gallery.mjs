// The gallery: an orphan branch `gallery` (never merged, never deployed)
// that keeps every piece's before/after pictures, its stats and the
// benchmark history, so nothing about how the bar changed is ever lost.
// Checked out as a worktree in build/gallery. Layout:
//   pieces.jsonl           one line per piece (tools/piece.mjs)
//   bench.jsonl            one line per benchmark run (tools/bench.mjs)
//   latest/<spot>.png      the newest picture of each spot (the next "before")
//   <TAG>/<spot>.jpg       a piece's before/after, side by side
//   README.md              the gallery page (newest first), regenerated
import { execSync } from 'node:child_process';
import { existsSync, readFileSync, writeFileSync, appendFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';

export const GALLERY = 'build/gallery';
const sh = (cmd, opts = {}) => execSync(cmd, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'], ...opts }).trim();

export function openGallery() {
  if (!existsSync(join(GALLERY, '.git'))) {
    let remote = false;
    try { remote = sh('git ls-remote --heads origin gallery').length > 0; } catch {}
    if (remote) {
      sh('git fetch -q origin gallery');
      try { sh(`git worktree add -f ${GALLERY} gallery`); } catch { sh(`git worktree add -f -B gallery ${GALLERY} origin/gallery`); }
    } else {
      try { sh('git worktree prune'); } catch {}
      sh(`git worktree add --orphan -b gallery ${GALLERY}`);
    }
  } else {
    try { sh('git fetch -q origin gallery', {}); sh('git merge -q --ff-only origin/gallery', { cwd: GALLERY }); } catch {}
  }
  mkdirSync(join(GALLERY, 'latest'), { recursive: true });
  return GALLERY;
}

export function readJsonl(name) {
  const p = join(GALLERY, name);
  if (!existsSync(p)) return [];
  return readFileSync(p, 'utf8').split('\n').filter(Boolean).map((l) => JSON.parse(l));
}
export function appendJsonl(name, rec) { appendFileSync(join(GALLERY, name), JSON.stringify(rec) + '\n'); }

// The gallery page: pieces newest first, each with its pictures and numbers.
export function writeReadme() {
  const pieces = readJsonl('pieces.jsonl').reverse();
  const bench = readJsonl('bench.jsonl');
  const out = ['# Bar3D gallery', '',
    'Every piece of work, newest first: what it is, its before/after pictures',
    '(left before, right after) and its numbers. Made by `tools/piece.mjs`;',
    'benchmarks by `tools/bench.mjs` (see `bench.jsonl`).', ''];
  if (bench.length) {
    const b = bench[bench.length - 1];
    out.push(`Latest benchmark (${b.date}, ${b.sha}): ` +
      Object.entries(b.native ?? {}).map(([k, v]) => `${k} ${v}`).join(', '), '');
  }
  for (const p of pieces) {
    out.push(`## ${p.tag} ${p.title}`, '',
      `\`${p.sha}\` · ${p.date} · ${p.files} files, +${p.added} −${p.removed} · tests: ${p.tests.join(', ') || 'none'}` +
      (p.perf ? ` · headless frame ${p.perf.avg} ms, ${p.perf.draws} draws` : ''), '');
    if (p.check) out.push(`Phone check: ${p.check}`, '');
    for (const img of p.images ?? []) out.push(`![${p.tag} ${img}](${p.tag}/${img})`, '');
  }
  writeFileSync(join(GALLERY, 'README.md'), out.join('\n') + '\n');
}

export function commitGallery(message) {
  sh('git add -A', { cwd: GALLERY });
  try { sh(`git commit -q -m ${JSON.stringify(message)}`, { cwd: GALLERY }); } catch { return false; }
  for (let i = 0; i < 4; i++) {
    try { sh('git push -q -u origin gallery', { cwd: GALLERY }); return true; } catch (e) {
      if (i === 3) { console.log('gallery push failed: ' + e.message.split('\n')[0]); return false; }
      execSync(`sleep ${2 ** (i + 1)}`);
    }
  }
}
