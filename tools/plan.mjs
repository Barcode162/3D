#!/usr/bin/env node
// The World Plan's keeper (PLAN.md, the one doc of truth). Claude edits
// PLAN.md only through this tool, so its shape never changes: eight parts in
// order, five chapters, every road item tagged and serving a North star line.
//   node tools/plan.mjs check                    the shape; exit 1 with what's wrong
//   node tools/plan.mjs now                      part 4: the step "go" does, its pieces
//   node tools/plan.mjs tick <PIECE>             tick a piece in Now (piece.mjs does it)
//   node tools/plan.mjs add <chapter> <TAG> "<what>" --serves N3[,N4]   a road item
//   node tools/plan.mjs park <chapter> "<idea>"  a line in Parked (the user's "park:")
//   node tools/plan.mjs rule "<rule>" [--replaces "<bold name>"]   a dated line in Your rules (only on the
//                                                user's word); --replaces takes out the rule whose bold name starts so
//   node tools/plan.mjs start <TAG> --piece "<TAG.1>|<what>|<phone check>" [--piece ...]
//                                                the road item becomes Now (the old Now must be all ticked)
//   node tools/plan.mjs edit <TAG> "<what>" [--serves N1,N3]   rewrite an open road item (the user changed it)
//   node tools/plan.mjs pieces [--about "<what the step is>"] --piece "<TAG.1>|<what>|<check>" ...   replace Now's pieces (only before any is ticked)
//   node tools/plan.mjs drop <PIECE> ["<parked line>" --park <chapter>]   take an open piece out of Now (the user said not now)
//   node tools/plan.mjs next "R5 → R6 → …"      the road's "Up next, in order" line (start drops the started tag)
//   node tools/plan.mjs done --sha <sha> --what "<pieces: what>" --range <a..b>
//                                                a Done line on top (step.mjs does it)
//   node tools/plan.mjs charter [--check]        the Charter's North star and rules block, copied word for word
//                                                from parts 1-2 (--check: exit 1 if docs/charter/charter.md is stale)
//   node tools/plan.mjs seal <sha>               the Done line still marked "(this push)" gets its build code
//                                                (once its deploy is live; step.mjs does it at the next finish)
// Chapters: world, site, engine, tools, backbone.
import { readFileSync, writeFileSync } from 'node:fs';

export const FILE = 'PLAN.md';
export const PARTS = ['1. North star', '2. Your rules', '3. The chapters', '4. Now', '5. The road', '6. Parked', '7. Done', '8. Where things are'];
export const CHAPTERS = { world: 'The world', site: 'The site', engine: 'The engine', tools: 'The tools', backbone: 'The backbone' };
const NAMES = Object.values(CHAPTERS);
const TAG = '[A-Z]{1,3}\\d+(?:\\.\\d+)?';

export function read(file = FILE) { return readFileSync(file, 'utf8'); }

// The text of one part (without its heading), and where it sits.
function part(text, n) {
  const head = `\n## ${PARTS[n]}\n`, a = text.indexOf(head);
  if (a < 0) return null;
  const from = a + head.length, b = text.indexOf('\n## ', from);
  return { from, to: b < 0 ? text.length : b + 1, body: text.slice(from, b < 0 ? text.length : b + 1) };
}
function chapterBlock(body, name) {
  const head = `### ${name}\n`, a = body.indexOf(head);
  if (a < 0) return null;
  const from = a + head.length, b = body.indexOf('\n### ', from);
  return { from, to: b < 0 ? body.length : b + 1 };
}
const tagsIn = (s) => [...s.matchAll(new RegExp(`\\*\\*(${TAG})\\*\\*`, 'g'))].map((m) => m[1]);

export function problems(text) {
  const out = [];
  if (!/^# J Production · The World Plan\n/.test(text)) out.push('the first line must be "# J Production · The World Plan"');
  const heads = [...text.matchAll(/^## (.*)$/gm)].map((m) => m[1]);
  if (JSON.stringify(heads) !== JSON.stringify(PARTS)) out.push(`the parts must be exactly, in order: ${PARTS.join(' / ')} (found: ${heads.join(' / ')})`);
  if (out.length) return out;
  const north = part(text, 0).body, stars = new Set([...north.matchAll(/\*\*(N\d+) /g)].map((m) => m[1]));
  if (!stars.size) out.push('North star: no N1, N2... lines');
  for (const [n, kind] of [[2, 'bullets'], [4, 'headings'], [5, 'headings']]) {
    const body = part(text, n).body;
    const found = kind === 'bullets' ? [...body.matchAll(/^- \*\*(The [a-z]+):\*\*/gm)].map((m) => m[1]) : [...body.matchAll(/^### (.*)$/gm)].map((m) => m[1]);
    if (JSON.stringify(found) !== JSON.stringify(NAMES)) out.push(`${PARTS[n]}: the chapters must be exactly ${NAMES.join(', ')} (found: ${found.join(', ') || 'none'})`);
  }
  const now = part(text, 3).body;
  if (!new RegExp(`^\\*\\*${TAG} · `, 'm').test(now)) out.push('Now: no "**TAG · title**" step line');
  for (const l of now.split('\n').filter((l) => l.startsWith('- '))) {
    if (!new RegExp(`^- \\[( |x)\\] \\*\\*${TAG}\\*\\* .+`).test(l)) out.push(`Now: a piece line must be "- [ ] **TAG.n** what" (${l.slice(0, 60)})`);
  }
  const road = part(text, 4).body, seen = new Set(tagsIn(now).filter((t) => t.includes('.')).map((t) => t.split('.')[0]));
  for (const l of road.split('\n').filter((l) => l.startsWith('- '))) {
    const m = new RegExp(`^- \\[( |x)\\] \\*\\*(${TAG})\\*\\* .+ · (N\\d+(?: N\\d+)*)$`).exec(l);
    if (!m) { out.push(`The road: an item must be "- [ ] **TAG** what · N1" (${l.slice(0, 70)})`); continue; }
    if (seen.has(m[2])) out.push(`The road: ${m[2]} is listed twice`); seen.add(m[2]);
    for (const n of m[3].split(' ')) if (!stars.has(n)) out.push(`The road: ${m[2]} serves ${n}, which is not a North star line`);
  }
  const next = /\*\*Up next, in order:\*\*(.*)/.exec(road);
  if (next) for (const t of next[1].match(new RegExp(TAG, 'g')) || []) if (!seen.has(t) && !tagsIn(now).includes(t)) out.push(`The road: "Up next" names ${t}, which isn't on the road`);
  const done = part(text, 6).body;
  for (const l of done.split('\n').filter((l) => l.startsWith('- '))) if (!/^- (`[0-9a-f]{7}`|\(this push\)) /.test(l)) out.push(`Done: a line must start with its build code (${l.slice(0, 50)})`);
  return out;
}

function save(text, file = FILE) {
  const bad = problems(text);
  if (bad.length) throw new Error('refused, PLAN.md would break its shape:\n' + bad.join('\n'));
  writeFileSync(file, text);
}
function chapterName(c) {
  const name = CHAPTERS[c] || NAMES.find((n) => n.toLowerCase() === String(c).toLowerCase());
  if (!name) throw new Error(`"${c}" is not a chapter: ${Object.keys(CHAPTERS).join(', ')}`);
  return name;
}
function addLine(text, n, chapter, line) {
  const p = part(text, n), block = chapterBlock(p.body, chapterName(chapter));
  let before = p.body.slice(0, block.to).replace(/\n*$/, '\n');
  return text.slice(0, p.from) + before + line + '\n' + (p.body.slice(block.to).startsWith('\n') ? '' : '\n') + p.body.slice(block.to) + text.slice(p.to);
}

// The Charter (docs/charter/charter.md) shows parts 1-2 word for word between
// two markers; PLAN.md stays their source until J4.1 (restructure D3).
export const CHARTER = 'docs/charter/charter.md';
const MARK_A = '<!-- generated: node tools/plan.mjs charter (from PLAN.md parts 1-2, word for word; never edit by hand) -->';
const MARK_B = '<!-- end generated -->';
export function charterBlock(text) {
  const items = (n) => part(text, n).body.split('\n').reduce((out, l) => { if (l.startsWith('- ')) out.push(l); else if (/^  \S/.test(l) && out.length) out[out.length - 1] += '\n' + l; return out; }, []);
  return `${MARK_A}\n### North star\n${items(0).join('\n')}\n\n### Your rules (newest first)\n${items(1).join('\n')}\n${MARK_B}`;
}
export function withCharter(doc, text) {
  const a = doc.indexOf(MARK_A), b = doc.indexOf(MARK_B);
  if (a < 0 || b < a) throw new Error(`${CHARTER} has no generated block (the two markers)`);
  return doc.slice(0, a) + charterBlock(text) + doc.slice(b + MARK_B.length);
}

export function tick(text, piece) {
  const re = new RegExp(`^- \\[ \\] \\*\\*${piece.replace('.', '\\.')}\\*\\*`, 'm');
  const p = part(text, 3);
  if (!re.test(p.body)) return null;
  return text.slice(0, p.from) + p.body.replace(re, `- [x] **${piece}**`) + text.slice(p.to);
}
export function addDone(text, { sha, what, range }) {
  let t = text.replace('- (this push) ', `- \`${sha}\` `);
  const p = part(t, 6), first = p.body.indexOf('\n- ');
  const at = first < 0 ? p.body.length : first + 1;
  const body = p.body.slice(0, at) + `- (this push) ${what}${range ? ' → ' + range : ''}\n` + p.body.slice(at);
  return t.slice(0, p.from) + body + t.slice(p.to);
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const [cmd, ...rest] = process.argv.slice(2);
  const flags = (n) => { const out = []; rest.forEach((a, i) => { if (a === '--' + n) out.push(rest[i + 1]); }); return out; };
  const pos = rest.filter((a, i) => !a.startsWith('--') && !(i > 0 && rest[i - 1].startsWith('--')));
  try {
    let text = read();
    if (cmd === 'check' || !cmd) {
      const bad = problems(text);
      console.log(bad.length ? bad.join('\n') : 'ok  plan: PLAN.md keeps its eight parts and five chapters');
      process.exit(bad.length ? 1 : 0);
    } else if (cmd === 'now') {
      console.log(part(text, 3).body.trim());
    } else if (cmd === 'tick') {
      const t = tick(text, pos[0]);
      if (!t) { console.log(`${pos[0]} is not an open piece in Now`); process.exit(1); }
      save(t); console.log(`ticked ${pos[0]}`);
    } else if (cmd === 'add') {
      const [chapter, tag, what] = pos, serves = (flags('serves')[0] || '').split(',').filter(Boolean);
      if (!serves.length) throw new Error('--serves N1[,N2]: which North star line does it serve? (none: park it instead)');
      if (new RegExp(`\\*\\*${tag}\\*\\*`).test(text)) throw new Error(`${tag} is already in the plan`);
      save(addLine(text, 4, chapter, `- [ ] **${tag}** ${what} · ${serves.join(' ')}`)); console.log(`added ${tag} to ${chapterName(chapter)}`);
    } else if (cmd === 'park') {
      save(addLine(text, 5, pos[0], `- ${pos[1]}`)); console.log(`parked under ${chapterName(pos[0])}`);
    } else if (cmd === 'rule') {
      let p = part(text, 1);
      const old = flags('replaces')[0];
      if (old) {
        const re = new RegExp(`^- \\(\\d{4}-\\d\\d-\\d\\d\\) \\*\\*${old.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}.*\\n`, 'm');
        if (!re.test(p.body)) throw new Error(`no rule starts with "**${old}"`);
        text = text.slice(0, p.from) + p.body.replace(re, '') + text.slice(p.to); p = part(text, 1);
      }
      const at = p.body.indexOf('\n- ') + 1, day = new Date().toISOString().slice(0, 10);
      save(text.slice(0, p.from) + p.body.slice(0, at) + `- (${day}) ${pos[0]}\n` + p.body.slice(at) + text.slice(p.to)); console.log(old ? `rule "${old}" replaced` : 'rule added');
    } else if (cmd === 'start') {
      const tag = pos[0], now = part(text, 3);
      if (/^- \[ \]/m.test(now.body)) throw new Error('Now still has open pieces: tick them (or take them out by hand with the user) first');
      const road = part(text, 4), m = new RegExp(`^- \\[ \\] \\*\\*${tag.replace('.', '\\.')}\\*\\* (.+) · (N\\d+(?: N\\d+)*)\\n`, 'm').exec(road.body);
      if (!m) throw new Error(`${tag} is not an open road item`);
      const chapter = NAMES.find((n) => { const b = chapterBlock(road.body, n); return b && road.body.slice(b.from, b.to).includes(m[0]); });
      const pieces = flags('piece').map((s) => { const [t, w, c] = s.split('|'); return `- [ ] **${t}** ${w}${c ? ` *Check:* ${c}` : ''}`; });
      if (!pieces.length) throw new Error('give the step its pieces: --piece "TAG.1|what|phone check"');
      const title = m[1].split(':')[0].replace(/\*\*/g, '');
      let t = text.slice(0, road.from) + road.body.replace(m[0], '').replace(new RegExp(`(\\*\\*Up next, in order:\\*\\* )${tag}(?: →|,) `), '$1') + text.slice(road.to);
      const n2 = part(t, 3);
      t = t.slice(0, n2.from) + `**${tag} · ${title}** (${chapter.toLowerCase()}). ${m[1]} · ${m[2]}\n${pieces.join('\n')}\n\n` + t.slice(n2.to);
      save(t); console.log(`${tag} is now in Now with ${pieces.length} pieces`);
    } else if (cmd === 'edit') {
      const [tag, what] = pos, road = part(text, 4);
      const re = new RegExp(`^(- \\[ \\] \\*\\*${tag.replace('.', '\\.')}\\*\\* ).+( · N\\d+(?: N\\d+)*)$`, 'm');
      if (!re.test(road.body)) throw new Error(`${tag} is not an open road item`);
      const serves = flags('serves')[0];
      save(text.slice(0, road.from) + road.body.replace(re, (_, a, b) => a + what + (serves ? ' · ' + serves.split(',').join(' ') : b)) + text.slice(road.to)); console.log(`${tag} rewritten`);
    } else if (cmd === 'pieces') {
      const now = part(text, 3);
      if (flags('piece').length && /^- \[x\]/m.test(now.body)) throw new Error('Now already has ticked pieces: its pieces can only be added to, not replaced');
      const pieces = flags('piece').map((s) => { const [t, w, c] = s.split('|'); return `- [ ] **${t}** ${w}${c ? ` *Check:* ${c}` : ''}`; });
      if (!pieces.length && !flags('about').length) throw new Error('--piece "TAG.1|what|phone check" ... (or --about alone)');
      if (!pieces.length) pieces.push(...now.body.split('\n').filter((l) => l.startsWith('- ')));
      let head = now.body.split('\n').filter((l) => !l.startsWith('- ') && l.trim()).join('\n');
      const about = flags('about')[0];
      if (about) head = head.replace(/^(\*\*[^*]+\*\* \([a-z ]+\)\.) .*$/m, `$1 ${about}`);
      save(text.slice(0, now.from) + head + '\n' + pieces.join('\n') + '\n\n' + text.slice(now.to)); console.log(`Now has ${pieces.length} pieces`);
    } else if (cmd === 'drop') {
      // A piece the user no longer wants in this step: out of Now (into Parked with --park <chapter> "<line>").
      const tag = pos[0], now = part(text, 3);
      const re = new RegExp(`^- \\[ \\] \\*\\*${tag.replace('.', '\\.')}\\*\\* .*\\n`, 'm');
      if (!re.test(now.body)) throw new Error(`${tag} is not an open piece in Now`);
      let t = text.slice(0, now.from) + now.body.replace(re, '') + text.slice(now.to);
      const pk = flags('park');
      if (pk.length) t = addLine(t, 5, pk[0], `- ${pos[1]}`);
      save(t); console.log(`${tag} dropped from Now${pk.length ? ', parked under ' + chapterName(pk[0]) : ''}`);
    } else if (cmd === 'next') {
      const p = part(text, 4);
      if (!/\*\*Up next, in order:\*\*/.test(p.body)) throw new Error('the road has no "Up next" line');
      save(text.slice(0, p.from) + p.body.replace(/\*\*Up next, in order:\*\*.*/, `**Up next, in order:** ${pos[0]}`) + text.slice(p.to)); console.log('Up next set');
    } else if (cmd === 'charter') {
      const doc = read(CHARTER), want = withCharter(doc, text);
      if (rest.includes('--check')) {
        console.log(doc === want ? 'ok  charter: the North star and rules block matches PLAN.md' : `${CHARTER} is stale: run node tools/plan.mjs charter`);
        process.exit(doc === want ? 0 : 1);
      }
      writeFileSync(CHARTER, want); console.log(doc === want ? 'charter already current' : 'charter block written');
    } else if (cmd === 'seal') {
      if (!/^[0-9a-f]{7}$/.test(pos[0] || '')) throw new Error('seal <sha>: the 7-letter build code');
      const p = part(text, 6);
      if (!p.body.includes('- (this push) ')) throw new Error('Done has no "(this push)" line to seal');
      save(text.slice(0, p.from) + p.body.replace('- (this push) ', `- \`${pos[0]}\` `) + text.slice(p.to)); console.log(`sealed with ${pos[0]}`);
    } else if (cmd === 'done') {
      save(addDone(text, { sha: flags('sha')[0], what: flags('what')[0], range: flags('range')[0] })); console.log('Done line added');
    } else {
      console.log(read(new URL(import.meta.url).pathname).split('\n').slice(1, 22).join('\n')); process.exit(2);
    }
  } catch (e) { console.log(e.message); process.exit(1); }
}
