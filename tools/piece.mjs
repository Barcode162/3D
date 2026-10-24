#!/usr/bin/env node
// One piece of work, done the same way every time (docs/AUTOMATION.md):
//   node tools/piece.mjs <TAG> "<title>" [options]
// builds, runs the piece's own tests, takes pictures of the standard spots,
// keeps a before/after of every spot that changed (and of the feature's own
// switch) in the gallery branch, commits "<TAG> <title>", pushes work-go and
// records the piece's numbers (files, lines, tests, headless frame time,
// brightness) in the gallery's pieces.jsonl.
// Options:
//   --test a,b        check.mjs systems to run first (a failure stops the piece); default
//                     "quick" (build, C++ tests, shaders: minutes, no browser); "changed"
//                     for every system the files touch (slow here: no graphics card);
//                     "none" for none. The slow browser tests run on CI at deploy.
//   --check "..."     the phone check: where to stand, what to see (kept for the reply)
//   --compare "x=0"   also pictures with this page option vs without (the feature's switch)
//   --spots a,b       only these spots (default: all of tools/lib/spots.mjs)
//   --pages a,b       pictures of site pages instead of the bar (repo paths, e.g.
//                     toolbelt/index.html or "ui/screen.html?s=sampler"), at phone width,
//                     before/after against the last picture of the same page
//   --no-shots        skip pictures (code that changes nothing on screen)
//   --branch b        push to this branch (default work-go; also PIECE_BRANCH)
//   --no-build        skip the build
//   --no-push         commit only
//   --dry             do everything but commit and push
//   node tools/piece.mjs baseline    pictures of the current build as the "before"
import { execSync, spawnSync } from 'node:child_process';
import { existsSync, readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { openCapture, SPOT_NAMES } from './lib/capture.mjs';
import { openGallery, appendJsonl, writeReadme, commitGallery, GALLERY } from './lib/gallery.mjs';
import { diffPercentBuffers, exposureStats } from './lib/imgcmp.mjs';
import { launchBrowser } from './lib/browser.mjs';
import { FILE as PLAN_FILE, read as readPlan, tick as tickPlan, problems } from './plan.mjs';
import { serveSite } from './lib/site.mjs';

const sh = (cmd) => execSync(cmd, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'], maxBuffer: 1 << 26 }).trim();
const run = (cmd) => spawnSync('bash', ['-c', cmd], { stdio: 'inherit' }).status === 0;
const EMSDK = 'source /opt/emsdk/emsdk_env.sh >/dev/null 2>&1;';
const ATTRIBUTION = process.env.PIECE_ATTRIBUTION ??
  'Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\nClaude-Session: https://claude.ai/code/session_01Nrk1rpfmMgEQWZYFLjtXXi';

const argv = process.argv.slice(2);
const opt = (name, def = null) => { const i = argv.indexOf('--' + name); return i >= 0 ? argv[i + 1] : def; };
const has = (name) => argv.includes('--' + name);
const [tag, title] = argv;
if (!tag) { console.log(readFileSync(new URL(import.meta.url), 'utf8').split('\n').slice(1, 21).join('\n')); process.exit(2); }
const t0 = Date.now();
const step = (s) => console.log(`\n▶ ${tag}: ${s} (${((Date.now() - t0) / 1000).toFixed(0)} s)`);

function build() {
  step('build');
  if (!run(`${EMSDK} [ -f build/build.ninja ] || emcmake cmake -B build -G Ninja >/dev/null; cmake --build build 2>&1 | grep -E "error|Error|FAILED" ; exit \${PIPESTATUS[0]}`)) {
    console.log(`✗ ${tag}: the build failed`);
    process.exit(1);
  }
}

async function pictures(compare, spots) {
  openGallery();
  const cap = await openCapture();
  const images = [], stats = {};
  let perf = null, errors = [];
  mkdirSync(join(GALLERY, tag), { recursive: true });
  for (const spot of spots) {
    step(`picture: ${spot}`);
    const now = await cap.shot(spot, '');
    errors.push(...now.errors.map((e) => `${spot}: ${e}`));
    if (!perf && now.perf) perf = now.perf;
    stats[spot] = exposureStats(now.png);
    const latest = join(GALLERY, 'latest', `${spot}.png`);
    if (existsSync(latest)) {
      const before = readFileSync(latest);
      const pct = diffPercentBuffers(before, now.png);
      stats[spot].changed = +pct.toFixed(2);
      if (pct > 0.5 && tag !== 'baseline') {
        await cap.sideBySide(before, now.png, 'the build before', `${tag}`, join(GALLERY, tag, `${spot}.jpg`), `${tag} ${title} · ${spot}`);
        images.push(`${spot}.jpg`);
      }
    }
    writeFileSync(latest, now.png);
    if (compare) {
      const off = await cap.shot(spot, compare);
      await cap.sideBySide(off.png, now.png, compare, 'on', join(GALLERY, tag, `${spot}-switch.jpg`), `${tag} ${title} · ${spot} · switch ${compare}`);
      images.push(`${spot}-switch.jpg`);
    }
  }
  await cap.close();
  return { images, stats, perf, errors };
}

// Site and tool pages (no game): a phone-width picture of each, side by side
// with the last picture of the same page (gallery latest/page-<name>.png).
async function pagePictures(pages) {
  openGallery();
  mkdirSync(join(GALLERY, tag), { recursive: true });
  const site = serveSite('.');
  const browser = await launchBrowser();
  const images = [], stats = {}, errors = [];
  for (const path of pages) {
    const name = path.replace(/\/index\.html$/, '').replace(/\.html/, '').replace(/[^a-z0-9]+/gi, '-').replace(/^-|-$/g, '');
    step(`page picture: ${path}`);
    const page = await browser.newPage({ viewport: { width: 390, height: 844 }, deviceScaleFactor: 1 });
    page.on('pageerror', (e) => errors.push(`${path}: ${e}`));
    page.on('response', (r) => { if (r.status() >= 400 && !/favicon/.test(r.url())) errors.push(`${path}: ${r.status()} ${r.url()}`); });
    // Pages use site paths (/ui/...): the repo root is the site root here.
    await page.goto(site.url + path);
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(500);
    const png = await page.screenshot({ fullPage: true });
    await page.close();
    stats[name] = exposureStats(png);
    const latest = join(GALLERY, 'latest', `page-${name}.png`);
    const before = existsSync(latest) ? readFileSync(latest) : null;
    const out = join(GALLERY, tag, `page-${name}.jpg`);
    const show = await browser.newPage({ viewport: { width: 390 * 2 + 12, height: 400 }, deviceScaleFactor: 1 });
    const img = (b) => `data:image/png;base64,${b.toString('base64')}`;
    await show.setContent(`<body style="margin:0;background:#111;color:#eee;font:15px sans-serif">
      <div style="padding:6px 8px;font-weight:bold">${tag} · ${path}</div>
      <div style="display:flex;gap:12px;align-items:flex-start">
      <div style="width:390px"><div style="height:30px;line-height:30px;padding-left:8px;color:#bbb">before</div>${before ? `<img src="${img(before)}">` : '<div style="padding:8px;color:#888">(new page)</div>'}</div>
      <div><div style="height:30px;line-height:30px;padding-left:8px;color:#fc6">after</div><img src="${img(png)}"></div></div></body>`);
    await show.screenshot({ path: out, type: 'jpeg', quality: 72, fullPage: true });
    await show.close();
    images.push(`page-${name}.jpg`);
    writeFileSync(latest, png);
  }
  await browser.close(); site.close();
  return { images, stats, perf: null, errors };
}

if (tag === 'baseline') {
  if (!has('no-build')) build();
  const r = await pictures(null, SPOT_NAMES);
  commitGallery('baseline pictures');
  console.log(r.stats);
  process.exit(0);
}

if (!title) { console.log('usage: node tools/piece.mjs <TAG> "<title>" [--test ...] [--check "..."] [--compare "x=0"]'); process.exit(2); }
const branch = sh('git rev-parse --abbrev-ref HEAD');
const target = opt('branch') ?? process.env.PIECE_BRANCH ?? 'work-go';
if (branch !== target && !has('dry')) console.log(`(note: on ${branch}, pushing HEAD to ${target})`);
if (!has('no-build')) build();

const tests = (opt('test') ?? 'quick').split(',').filter((t) => t && t !== 'none');
if (tests.length) {
  step(`tests: ${tests.join(', ')}`);
  if (!run(`FAST=1 node tools/check.mjs ${tests.join(' ')}`)) { console.log(`✗ ${tag}: tests failed (nothing committed)`); process.exit(1); }
}

let shots = { images: [], stats: {}, perf: null, errors: [] };
if (opt('pages')) {
  shots = await pagePictures(opt('pages').split(','));
  if (shots.errors.length) {
    console.log(`✗ ${tag}: the page logged errors:\n${shots.errors.join('\n')}`);
    if (!has('allow-errors')) process.exit(1);
  }
} else if (!has('no-shots')) {
  const spots = (opt('spots') ?? SPOT_NAMES.join(',')).split(',');
  shots = await pictures(opt('compare'), spots);
  if (shots.errors.length) {
    console.log(`✗ ${tag}: the page logged errors:\n${shots.errors.join('\n')}`);
    if (!has('allow-errors')) process.exit(1);
  }
}

if (has('dry')) { console.log(JSON.stringify(shots.stats, null, 1)); process.exit(0); }
step('commit');
// The piece is ticked in PLAN.md → Now (if it's one of Now's pieces), in this same commit.
{ const t = tickPlan(readPlan(), tag); if (t && !problems(t).length) writeFileSync(PLAN_FILE, t); }
sh('git add -A');
const staged = sh('git diff --cached --numstat');
if (!staged) { console.log(`✗ ${tag}: nothing to commit`); process.exit(1); }
let added = 0, removed = 0, files = 0;
for (const l of staged.split('\n')) { const [a, r] = l.split('\t'); files++; added += +a || 0; removed += +r || 0; }
const body = `${tag} ${title}\n\n${opt('check') ? 'Phone check: ' + opt('check') + '\n\n' : ''}${ATTRIBUTION}`;
writeFileSync('build/piece-msg.txt', body);
sh('git commit -q -F build/piece-msg.txt');
const sha = sh('git rev-parse --short HEAD');
if (!has('no-push')) {
  step(`push ${target}`);
  for (let i = 0; i < 4 && !run(`git push -q -u origin HEAD:${target}`); i++) execSync(`sleep ${2 ** (i + 1)}`);
}

openGallery();
const p = shots.perf;
appendJsonl('pieces.jsonl', {
  tag, title, sha, date: new Date().toISOString().slice(0, 16).replace('T', ' '), files, added, removed, tests,
  check: opt('check') ?? '', images: shots.images, stats: shots.stats,
  perf: p ? { avg: p.avg, cpu: p.cpu, gpu: p.gpu, draws: p.draws?.main ?? p.draws, tris: p.tris?.main ?? p.tris } : null,
  seconds: Math.round((Date.now() - t0) / 1000),
});
writeReadme();
commitGallery(`${tag} ${title} (${sha})`);
console.log(`\n✓ ${tag} ${title} → ${sha} (${files} files, +${added} −${removed}; ${shots.images.length} pictures; ${((Date.now() - t0) / 1000).toFixed(0)} s)`);
