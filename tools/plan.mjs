#!/usr/bin/env node
// The plan's keeper (restructure J4.1). The plan is data: docs/plan/plan.json
// (chapters, Now, the road, Parked, Done) and docs/charter/rules.json (the
// user's North star and rules, word for word). Claude changes them only
// through this tool; every save writes PLAN.md (the readable plan, never
// edited by hand), the Charter's rules block and CLAUDE.md's "Never" list, and
// refuses anything that would break the plan's shape (eight parts, five
// chapters, every road item serving a North star line).
//   node tools/plan.mjs check                    the shape, and PLAN.md / the Charter / CLAUDE.md match the data
//   node tools/plan.mjs now                      the job in Now and its pieces
//   node tools/plan.mjs tick <PIECE>             tick a piece in Now (piece.mjs does it)
//   node tools/plan.mjs add <chapter> <ID> "<what>" --serves N3[,N4] --verdict <kind> --why "<one line>" [--after "J9, J12"] [--formerly <old tag>]
//                                                a road item, with its verdict (intake.md step 3)
//   node tools/plan.mjs park <chapter> "<idea>" [--verdict <kind> --why "<one line>" [--after …]] [--from <ID>]
//                                                a line in Parked (the user's "park:"; --from: a road item the user put off)
//   node tools/plan.mjs verdict <ID | start of a parked line> <kind> "<why>" [--after "J9, J12"]   give or change an item's verdict
//   node tools/plan.mjs nextid                   the next free job number (J<n>: never reused)
//   node tools/plan.mjs rule "<rule>" [--replaces "<bold name>"]   a dated rule (only on the user's word, with
//                                                its request); --replaces takes out the rule whose bold name starts so
//   node tools/plan.mjs start <ID> --piece "<ID.1>|<what>|<phone check>" [--piece ...]
//                                                the road item becomes Now (the old Now must be all ticked)
//   node tools/plan.mjs edit <ID> "<what>" [--serves N1,N3]   rewrite an open road item (the user changed it)
//   node tools/plan.mjs pieces [--about "<what the job is>"] --piece "<ID.1>|<what>|<check>" ...   replace Now's pieces (only before any is ticked)
//   node tools/plan.mjs drop <PIECE> ["<parked line>" --park <chapter>]   take an open piece out of Now (the user said not now)
//   node tools/plan.mjs next "J5 → J6 → …"      the road's "Up next, in order" line (start drops the started id)
//   node tools/plan.mjs done --sha <sha> --what "<pieces: what>" --range <a..b>
//                                                a Done line on top (step.mjs does it)
//   node tools/plan.mjs seal <sha>               the Done line still marked "(this push)" gets its build code
//   node tools/plan.mjs chapter <chapter> "<now line>"   a chapter's "Now:" line (where it stands today)
//   node tools/plan.mjs render [--check]         write PLAN.md and the blocks from the data (--check: only compare)
//   node tools/plan.mjs relink                   every doc path in the plan that moved (J6.6, tools/lib/docmoves.mjs) → its new place
//   node tools/plan.mjs specs                    every job's spec as JSON {id: text} (docs/plan/jobs/<ID>.md, and
//                                                J1–J8's sections of docs/archive/restructure.md; a spec under a job's old
//                                                tag counts for it too); plan.yml sends it to Dev
// Chapters: world, site, engine, tools, backbone. Verdict kinds: now, after
// (with --after), cost, experiment, impossible, rule.
import { readFileSync, writeFileSync, readdirSync } from 'node:fs';
import { load, write, render, parseVerdict, VERDICTS, PARTS, CHAPTERS, PLAN_MD } from './lib/plandata.mjs';
export { PARTS, CHAPTERS };

export const FILE = PLAN_MD;
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
  const roadLines = road.split('\n');
  for (const [i, l] of roadLines.entries()) {
    if (!l.startsWith('- ')) continue;
    const m = new RegExp(`^- \\[( |x)\\] \\*\\*(${TAG})\\*\\* .+ · (N\\d+(?: N\\d+)*)$`).exec(l);
    if (!m) { out.push(`The road: an item must be "- [ ] **TAG** what · N1" (${l.slice(0, 70)})`); continue; }
    if (m[1] === ' ' && !parseVerdict(roadLines[i + 1] || '')) out.push(`The road: ${m[2]} has no verdict line ("  *Verdict:* Doable now — why"; intake.md step 3)`);
    if (seen.has(m[2])) out.push(`The road: ${m[2]} is listed twice`); seen.add(m[2]);
    for (const n of m[3].split(' ')) if (!stars.has(n)) out.push(`The road: ${m[2]} serves ${n}, which is not a North star line`);
  }
  const next = /\*\*Up next, in order:\*\*(.*)/.exec(road);
  if (next) for (const t of next[1].match(new RegExp(TAG, 'g')) || []) if (!seen.has(t) && !tagsIn(now).includes(t)) out.push(`The road: "Up next" names ${t}, which isn't on the road`);
  const done = part(text, 6).body;
  for (const l of done.split('\n').filter((l) => l.startsWith('- '))) if (!/^- (`[0-9a-f]{7}`|\(this push\)) /.test(l)) out.push(`Done: a line must start with its build code (${l.slice(0, 50)})`);
  return out;
}

// The Charter (docs/charter/charter.md) shows parts 1-2 word for word between
// two markers, from docs/charter/rules.json (through the rendered PLAN.md).
export const CHARTER = 'docs/charter/charter.md';
const MARK_A = '<!-- generated: node tools/plan.mjs charter (from docs/charter/rules.json, word for word; never edit by hand) -->';
const MARK_B = '<!-- end generated -->';
export function charterBlock(text) {
  const items = (n) => part(text, n).body.split('\n').reduce((out, l) => { if (l.startsWith('- ')) out.push(l); else if (/^  \S/.test(l) && out.length) out[out.length - 1] += '\n' + l; return out; }, []);
  return `${MARK_A}\n### North star\n${items(0).join('\n')}\n\n### Your rules (newest first)\n${items(1).join('\n')}\n${MARK_B}`;
}
// CLAUDE.md (read first by every session) shows the Charter's "Never" list
// word for word between two markers; the Charter is its source.
export const ENTRY = 'CLAUDE.md';
const NEVER_A = '<!-- generated: node tools/plan.mjs charter (the Charter §5 "Never" list, word for word; never edit by hand) -->';
export function withNever(entry, charterDoc) {
  const a = charterDoc.indexOf('**Never:**'), b = charterDoc.indexOf('**Anything not listed');
  if (a < 0 || b < a) throw new Error(`${CHARTER} has no "Never:" list`);
  const list = charterDoc.slice(a + '**Never:**'.length, b).trim();
  const x = entry.indexOf(NEVER_A), y = entry.indexOf(MARK_B, x);
  if (x < 0 || y < x) throw new Error(`${ENTRY} has no generated "Never" block (the two markers)`);
  return entry.slice(0, x) + `${NEVER_A}\n${list}\n` + entry.slice(y);
}
export function withCharter(doc, text) {
  const a = doc.indexOf(MARK_A), b = doc.indexOf(MARK_B);
  if (a < 0 || b < a) throw new Error(`${CHARTER} has no generated block (the two markers)`);
  return doc.slice(0, a) + charterBlock(text) + doc.slice(b + MARK_B.length);
}

// Writes the data, PLAN.md and the two generated blocks, after the checks.
export function save(data) {
  const text = render(data), bad = problems(text);
  if (bad.length) throw new Error('refused, the plan would break its shape:\n' + bad.join('\n'));
  write(data);
  const charter = withCharter(read(CHARTER), text);
  writeFileSync(CHARTER, charter);
  writeFileSync(ENTRY, withNever(read(ENTRY), charter));
}
// Whether PLAN.md and the blocks match the data (nobody edited them by hand).
export function stale(data = load()) {
  const text = render(data), out = [];
  if (read(FILE) !== text) out.push(`${FILE} differs from the data (run node tools/plan.mjs render)`);
  const charter = withCharter(read(CHARTER), text);
  if (read(CHARTER) !== charter) out.push(`${CHARTER}'s rules block differs from docs/charter/rules.json`);
  if (read(ENTRY) !== withNever(read(ENTRY), charter)) out.push(`${ENTRY}'s "Never" list differs from the Charter`);
  return out;
}
function chapterId(c) {
  const id = CHAPTERS[c] ? c : Object.keys(CHAPTERS).find((k) => CHAPTERS[k].toLowerCase() === String(c).toLowerCase());
  if (!id) throw new Error(`"${c}" is not a chapter: ${Object.keys(CHAPTERS).join(', ')}`);
  return id;
}
const roadItems = (plan) => Object.values(plan.road).flat();
// A verdict from the flags: --verdict <kind> --why "<one line>" [--after "J9, J12"].
function verdictFrom(kind, why, after) {
  if (!VERDICTS[kind]) throw new Error(`--verdict: one of ${Object.keys(VERDICTS).join(', ')} (${Object.values(VERDICTS).join(', ')})`);
  if (!why || /\n| — /.test(why)) throw new Error('--why "<one line>": the reason, tied to the Charter\'s limits (no line breaks, no " — ")');
  if ((kind === 'after') !== Boolean(after)) throw new Error(kind === 'after' ? '--after "J9, J12": which jobs come first' : '--after only goes with --verdict after');
  if (after && !/^J\d+(, J\d+)*$/.test(after)) throw new Error('--after: job numbers, "J9" or "J9, J12"');
  return { kind, ...(after ? { after } : {}), why };
}
// The highest job number anywhere in the plan (Now, the road, Done), +1.
export function nextId(plan) {
  const ids = [plan.now?.id, ...roadItems(plan).map((r) => r.id), ...plan.done.map((d) => d.text)].join(' ');
  return `J${Math.max(8, ...[...ids.matchAll(/\bJ(\d+)\b/g)].map((m) => +m[1])) + 1}`;
}
const newPieces = (specs) => specs.map((s) => { const [id, text, check] = s.split('|'); return { id, text, check: check ?? '', done: false }; });

// For the piece runner and the step finisher.
export function tickPiece(id) {
  const data = load(), p = data.plan.now?.pieces.find((x) => x.id === id && !x.done);
  if (!p) return false;
  p.done = true; save(data); return true;
}
// The Done line still "(this push)" gets its build code once its finish commit
// ("Notes: J<n> built …") is in the history: the next piece seals it, so Dev
// never shows a shipped job as "going live now" (J8.1's drills).
export function sealShipped(log) {
  const data = load(), d = data.plan.done.find((x) => !x.build), id = d && (/\bJ\d+/.exec(d.text) || [])[0];
  if (!id) return null;
  const hit = log.split('\n').map((l) => l.split('\t')).find(([, subject]) => subject?.startsWith(`Notes: ${id} built`));
  if (!hit) return null;
  d.build = hit[0].slice(0, 7); save(data); return d.build;
}
export function addDoneLine({ sha, what, range }) {
  const data = load();
  for (const d of data.plan.done) if (!d.build) d.build = sha;
  data.plan.done.unshift({ build: null, text: `${what}${range ? ' → ' + range : ''}` });
  save(data);
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const [cmd, ...rest] = process.argv.slice(2);
  const flags = (n) => { const out = []; rest.forEach((a, i) => { if (a === '--' + n) out.push(rest[i + 1]); }); return out; };
  const pos = rest.filter((a, i) => !a.startsWith('--') && !(i > 0 && rest[i - 1].startsWith('--')));
  try {
    const data = load(), { plan, rules } = data;
    if (cmd === 'check' || !cmd) {
      const bad = [...problems(render(data)), ...stale(data)];
      console.log(bad.length ? bad.join('\n') : 'ok  plan: the data keeps its eight parts and five chapters; PLAN.md, the Charter and CLAUDE.md match it');
      process.exit(bad.length ? 1 : 0);
    } else if (cmd === 'now') {
      console.log(part(render(data), 3).body.trim());
    } else if (cmd === 'render') {
      if (rest.includes('--check')) { const bad = stale(data); console.log(bad.length ? bad.join('\n') : 'ok  render: PLAN.md and the blocks match the data'); process.exit(bad.length ? 1 : 0); }
      save(data); console.log('PLAN.md and the blocks written');
    } else if (cmd === 'tick') {
      if (!tickPiece(pos[0])) { console.log(`${pos[0]} is not an open piece in Now`); process.exit(1); }
      console.log(`ticked ${pos[0]}`);
    } else if (cmd === 'add') {
      const [chapter, id, what] = pos, serves = (flags('serves')[0] || '').split(',').filter(Boolean);
      if (!serves.length) throw new Error('--serves N1[,N2]: which North star line does it serve? (none: park it instead)');
      if (roadItems(plan).some((r) => r.id === id) || plan.now?.id === id) throw new Error(`${id} is already in the plan`);
      const verdict = verdictFrom(flags('verdict')[0], flags('why')[0], flags('after')[0]), formerly = flags('formerly')[0];
      plan.road[chapterId(chapter)].push({ id, ...(formerly ? { formerly } : {}), text: what, serves, done: false, verdict });
      save(data); console.log(`added ${id} to ${CHAPTERS[chapterId(chapter)]}`);
    } else if (cmd === 'park') {
      const kind = flags('verdict')[0], from = flags('from')[0];
      if (from) {  // a road item the user put off: it leaves the road (and Up next) for Parked
        const ch = Object.keys(plan.road).find((c) => plan.road[c].some((r) => r.id === from && !r.done));
        if (!ch) throw new Error(`--from ${from}: not an open road item`);
        plan.road[ch] = plan.road[ch].filter((r) => r.id !== from);
        if (new RegExp(`\\b${from}\\b`).test(plan.upNext)) throw new Error(`--from ${from}: it's in Up next; take it out of the order first (next "…")`);
      }
      plan.parked[chapterId(pos[0])].push(kind ? { text: pos[1], verdict: verdictFrom(kind, flags('why')[0], flags('after')[0]) } : pos[1]);
      save(data); console.log(`parked under ${CHAPTERS[chapterId(pos[0])]}${from ? ` (${from} off the road)` : ''}`);
    } else if (cmd === 'verdict') {
      const [which, kind, why] = pos, verdict = verdictFrom(kind, why, flags('after')[0]);
      const item = roadItems(plan).find((r) => r.id === which && !r.done);
      if (item) item.verdict = verdict;
      else {
        const hits = Object.values(plan.parked).flatMap((list) => list.map((t, i) => ({ list, i, text: typeof t === 'string' ? t : t.text }))).filter((x) => x.text.startsWith(which));
        if (hits.length !== 1) throw new Error(`"${which}" is ${hits.length ? 'the start of ' + hits.length + ' parked lines' : 'not an open road item nor the start of a parked line'}`);
        hits[0].list[hits[0].i] = { text: hits[0].text, verdict };
      }
      save(data); console.log(`${which}: ${VERDICTS[kind]}`);
      // A verdict must agree with what the user already decided (incident 2026-10-24-verdict-without-spec).
      const spec = [which, item?.formerly].filter(Boolean).map((t) => `docs/plan/jobs/${t}.md`).find((f) => { try { readFileSync(f); return true; } catch { return false; } });
      if (spec) console.log(`  its spec: ${spec}: the verdict must agree with its decisions`);
    } else if (cmd === 'nextid') {
      console.log(nextId(plan));
    } else if (cmd === 'rule') {
      const old = flags('replaces')[0];
      if (old) {
        const i = rules.rules.findIndex((r) => r.text.startsWith(`**${old}`));
        if (i < 0) throw new Error(`no rule starts with "**${old}"`);
        rules.rules.splice(i, 1);
      }
      rules.rules.unshift({ date: new Date().toISOString().slice(0, 10), text: pos[0] });
      save(data); console.log(old ? `rule "${old}" replaced` : 'rule added');
    } else if (cmd === 'start') {
      const id = pos[0];
      if (plan.now?.pieces.some((p) => !p.done)) throw new Error('Now still has open pieces: tick them (or take them out with the user) first');
      const ch = Object.keys(plan.road).find((c) => plan.road[c].some((r) => r.id === id && !r.done));
      if (!ch) throw new Error(`${id} is not an open road item`);
      const pieces = newPieces(flags('piece'));
      if (!pieces.length) throw new Error('give the job its pieces: --piece "ID.1|what|phone check"');
      const item = plan.road[ch].find((r) => r.id === id);
      plan.road[ch] = plan.road[ch].filter((r) => r !== item);
      plan.upNext = plan.upNext.replace(new RegExp(`^${id}(?: →|,) `), '');
      plan.now = { id, title: item.text.split(':')[0].replace(/\*\*/g, ''), chapter: CHAPTERS[ch].toLowerCase(), about: item.text, serves: item.serves, pieces };
      save(data); console.log(`${id} is now in Now with ${pieces.length} pieces`);
    } else if (cmd === 'edit') {
      const [id, what] = pos, item = roadItems(plan).find((r) => r.id === id && !r.done);
      if (!item) throw new Error(`${id} is not an open road item`);
      item.text = what; const serves = flags('serves')[0]; if (serves) item.serves = serves.split(',');
      save(data); console.log(`${id} rewritten`);
    } else if (cmd === 'pieces') {
      if (!plan.now) throw new Error('nothing in Now');
      const pieces = newPieces(flags('piece'));
      if (pieces.length && plan.now.pieces.some((p) => p.done)) throw new Error('Now already has ticked pieces: its pieces can only be added to, not replaced');
      if (!pieces.length && !flags('about').length) throw new Error('--piece "ID.1|what|phone check" ... (or --about alone)');
      if (pieces.length) plan.now.pieces = pieces;
      if (flags('about').length) plan.now.about = flags('about')[0];
      save(data); console.log(`Now has ${plan.now.pieces.length} pieces`);
    } else if (cmd === 'drop') {
      const id = pos[0], i = plan.now?.pieces.findIndex((p) => p.id === id && !p.done) ?? -1;
      if (i < 0) throw new Error(`${id} is not an open piece in Now`);
      plan.now.pieces.splice(i, 1);
      const pk = flags('park'); if (pk.length) plan.parked[chapterId(pk[0])].push(pos[1]);
      save(data); console.log(`${id} dropped from Now${pk.length ? ', parked under ' + CHAPTERS[chapterId(pk[0])] : ''}`);
    } else if (cmd === 'next') {
      plan.upNext = pos[0]; save(data); console.log('Up next set');
    } else if (cmd === 'done') {
      addDoneLine({ sha: flags('sha')[0], what: flags('what')[0], range: flags('range')[0] }); console.log('Done line added');
    } else if (cmd === 'seal') {
      if (!/^[0-9a-f]{7}$/.test(pos[0] || '')) throw new Error('seal <sha>: the 7-letter build code');
      const d = plan.done.find((x) => !x.build);
      if (!d) throw new Error('Done has no "(this push)" line to seal');
      d.build = pos[0]; save(data); console.log(`sealed with ${pos[0]}`);
    } else if (cmd === 'chapter') {
      plan.chapters.find((c) => c.id === chapterId(pos[0])).now = pos[1]; save(data); console.log(`${CHAPTERS[chapterId(pos[0])]}: Now line set`);
    } else if (cmd === 'relink') {
      const { fixDocRefs } = await import('./lib/docmoves.mjs');
      const before = JSON.stringify(plan) + JSON.stringify(rules);
      const walk = (v) => (typeof v === 'string' ? fixDocRefs(v) : Array.isArray(v) ? v.map(walk) : v && typeof v === 'object' ? Object.fromEntries(Object.entries(v).map(([k, x]) => [k, walk(x)])) : v);
      Object.assign(plan, walk(plan));
      Object.assign(rules, walk(rules));  // a doc path inside a rule (only the path changes, never the user's words; J8.2)
      if (JSON.stringify(plan) + JSON.stringify(rules) === before) console.log('nothing to relink'); else { save(data); console.log('the plan\'s doc paths point to where the docs are now'); }
    } else if (cmd === 'specs') {
      const specs = {}, dir = 'docs/plan/jobs';
      const restructure = read('docs/archive/restructure.md');
      for (const m of restructure.matchAll(/^### (J\d+) · [^\n]*\n[\s\S]*?(?=^#{2,3} |(?![\s\S]))/gm)) specs[m[1]] = m[0].trim();
      for (const f of readdirSync(dir).filter((x) => /^[A-Z]+\d+(\.\d+)?\.md$/.test(x))) specs[f.slice(0, -3)] = read(`${dir}/${f}`).trim();
      // A spec written under a job's old tag (docs/plan/jobs/W20.md) is its renumbered job's too (J7.1: "Formerly").
      for (const r of roadItems(plan)) if (r.formerly && specs[r.formerly] && !specs[r.id]) specs[r.id] = specs[r.formerly];
      console.log(JSON.stringify(specs));
    } else if (cmd === 'charter') {
      const bad = stale(data);
      if (rest.includes('--check')) { console.log(bad.length ? bad.join('\n') : 'ok  charter: its rules block matches docs/charter/rules.json, and CLAUDE.md\'s "Never" list matches the Charter'); process.exit(bad.length ? 1 : 0); }
      save(data); console.log(bad.length ? 'charter blocks written' : 'charter already current');
    } else {
      console.log(read(new URL(import.meta.url).pathname).split('\n').slice(1, 30).join('\n')); process.exit(2);
    }
  } catch (e) { console.log(e.message); process.exit(1); }
}
