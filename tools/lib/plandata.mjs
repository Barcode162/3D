// The plan as data (restructure J4.1). Two files are the source:
//   docs/plan/plan.json     the plan: the chapters, Now, the road, Parked, Done, part 8
//   docs/charter/rules.json the user's North star and rules, word for word
// PLAN.md is written from them (render) and never edited by hand; Dev reads
// the two JSON files. parse() turns a PLAN.md into the data (used once, to
// convert, and by the test that proves nothing is lost).
import { readFileSync, writeFileSync } from 'node:fs';

export const PLAN_JSON = 'docs/plan/plan.json';
export const RULES_JSON = 'docs/charter/rules.json';
export const PLAN_MD = 'PLAN.md';
export const PARTS = ['1. North star', '2. Your rules', '3. The chapters', '4. Now', '5. The road', '6. Parked', '7. Done', '8. Where things are'];
export const CHAPTERS = { world: 'The world', site: 'The site', engine: 'The engine', tools: 'The tools', backbone: 'The backbone' };
const IDS = Object.keys(CHAPTERS), NAMES = Object.values(CHAPTERS);
const idOf = (name) => IDS[NAMES.indexOf(name)];

// Every road and parked item's verdict (J7.1; the Playbook's six, intake.md
// step 3): what it is, and why, in one line tied to the Charter's limits.
// "after" names the jobs that must come first.
export const VERDICTS = { now: 'Doable now', after: 'Doable after', cost: 'Doable at a cost', experiment: 'Needs an experiment', impossible: 'Not possible', rule: 'Against a rule' };
const KIND_OF = Object.fromEntries(Object.entries(VERDICTS).map(([k, v]) => [v, k]));
export const verdictText = (v) => `${VERDICTS[v.kind]}${v.kind === 'after' ? ' ' + v.after : ''} — ${v.why}`;
// The line under an item in PLAN.md: "  *Formerly* A1 · *Verdict:* Doable after J20 — why".
const verdictLine = (v, formerly) => `  ${formerly ? `*Formerly* ${formerly} · ` : ''}*Verdict:* ${verdictText(v)}`;
const VERDICT_RE = new RegExp(`^  (?:\\*Formerly\\* (\\S+) · )?\\*Verdict:\\* (${Object.values(VERDICTS).join('|')})(?: (.+?))? — (.*)$`);
export function parseVerdict(line) {
  const m = VERDICT_RE.exec(line);
  if (!m) return null;
  const kind = KIND_OF[m[2]];
  if ((kind === 'after') !== Boolean(m[3])) return null;
  return { formerly: m[1] ?? null, verdict: { kind, ...(kind === 'after' ? { after: m[3] } : {}), why: m[4] } };
}

function split(md) {
  const heads = [...md.matchAll(/^## (.*)$/gm)];
  const head = md.slice(0, heads[0].index);
  const parts = heads.map((h, i) => md.slice(h.index + h[0].length + 1, i + 1 < heads.length ? heads[i + 1].index : md.length));
  return { head, names: heads.map((h) => h[1]), parts };
}
// "- " items with their "  " continuation lines.
function items(body) {
  const out = [];
  for (const l of body.split('\n')) {
    if (l.startsWith('- ')) out.push(l.slice(2));
    else if (/^  \S/.test(l) && out.length) out[out.length - 1] += '\n' + l;
  }
  return out;
}
const introOf = (body) => body.split('\n').filter((l) => /^\*[^*]/.test(l)).join('\n');
// fn(the item's line, its verdict line parsed or null).
function byChapter(body, fn) {
  const out = Object.fromEntries(IDS.map((c) => [c, []]));
  let cur = null, last = null;
  const flush = () => { if (last) out[cur].push(fn(last.line, last.v)); last = null; };
  for (const l of body.split('\n')) {
    const h = /^### (.*)$/.exec(l);
    if (h) { if (cur) flush(); cur = idOf(h[1]); continue; }
    if (!cur) continue;
    if (l.startsWith('- ')) { flush(); last = { line: l.slice(2), v: null }; }
    else if (last && l.startsWith('  ')) last.v = parseVerdict(l);
  }
  if (cur) flush();
  return out;
}

export function parse(md) {
  const { head, names, parts } = split(md);
  if (JSON.stringify(names) !== JSON.stringify(PARTS)) throw new Error('PLAN.md: the parts are not the eight fixed ones');
  const [north, rules, chapters, now, road, parked, done, where] = parts;
  const nowHead = /^\*\*([A-Z]+\d+) · ([^*]+)\*\* \(([^)]+)\)\. (.*?) · (N\d+(?: N\d+)*)$/m.exec(now);
  const piece = (l) => { const m = /^\[( |x)\] \*\*([A-Z]+\d+(?:\.\d+)?)\*\* (.*?)(?: \*Check:\* (.*))?$/.exec(l); return { id: m[2], text: m[3], check: m[4] ?? '', done: m[1] === 'x' }; };
  return {
    rules: {
      format: 1,
      about: 'The user\'s North star and rules, word for word (the source; PLAN.md parts 1-2 and the Charter show them). Only the user changes them: node tools/plan.mjs rule.',
      northIntro: introOf(north),
      north: items(north).map((t) => { const m = /^\*\*(N\d+) ([^*]+)\*\* ?([\s\S]*)$/.exec(t); return { id: m[1], title: m[2], text: m[3] }; }),
      rulesIntro: introOf(rules),
      rules: items(rules).map((t) => { const m = /^\((\d{4}-\d\d-\d\d)\) ([\s\S]*)$/.exec(t); return { date: m[1], text: m[2] }; }),
    },
    plan: {
      format: 1,
      about: 'The plan (the source; PLAN.md is written from it). Edited only with node tools/plan.mjs. Dev reads it.',
      head: head.replace(/\n+$/, ''),
      chaptersIntro: introOf(chapters),
      chapters: items(chapters).map((t) => { const m = /^\*\*(The [a-z]+):\*\* ([\s\S]*?)(?: \*Now:\* ([\s\S]*))?$/.exec(t); return { id: idOf(m[1]), name: m[1], what: m[2], now: m[3] ?? '' }; }),
      now: nowHead ? { id: nowHead[1], title: nowHead[2], chapter: nowHead[3], about: nowHead[4], serves: nowHead[5].split(' '),
        pieces: now.split('\n').filter((l) => l.startsWith('- ')).map((l) => piece(l.slice(2))) } : null,
      roadIntro: introOf(road),
      upNext: (/^\*\*Up next, in order:\*\* (.*)$/m.exec(road) || [])[1] ?? '',
      road: byChapter(road, (l, v) => {
        const m = /^\[( |x)\] \*\*([^*]+)\*\* ([\s\S]*) · (N\d+(?: N\d+)*)$/.exec(l);
        return { id: m[2], ...(v?.formerly ? { formerly: v.formerly } : {}), text: m[3], serves: m[4].split(' '), done: m[1] === 'x', ...(v ? { verdict: v.verdict } : {}) };
      }),
      parkedIntro: introOf(parked),
      parked: byChapter(parked, (l, v) => (v ? { text: l, verdict: v.verdict } : l)),
      doneIntro: introOf(done),
      done: items(done).map((t) => { const m = /^(?:`([0-9a-f]{7})`|\(this push\)) ([\s\S]*)$/.exec(t); return { build: m[1] ?? null, text: m[2] }; }),
      where: where.replace(/^\n+|\n+$/g, ''),
    },
  };
}

export function render({ plan, rules }) {
  const out = [plan.head, ''];
  const part = (n, ...lines) => out.push(`## ${PARTS[n]}`, ...lines, '');
  part(0, rules.northIntro, ...rules.north.map((s) => `- **${s.id} ${s.title}** ${s.text}`));
  part(1, rules.rulesIntro, ...rules.rules.map((r) => `- (${r.date}) ${r.text}`));
  part(2, plan.chaptersIntro, ...plan.chapters.map((c) => `- **${c.name}:** ${c.what}${c.now ? ` *Now:* ${c.now}` : ''}`));
  const n = plan.now;
  part(3, ...(n ? [`**${n.id} · ${n.title}** (${n.chapter}). ${n.about} · ${n.serves.join(' ')}`,
    ...n.pieces.map((p) => `- [${p.done ? 'x' : ' '}] **${p.id}** ${p.text}${p.check ? ` *Check:* ${p.check}` : ''}`)] : []));
  const chapters = (map, line) => IDS.flatMap((c) => [`### ${CHAPTERS[c]}`, ...map[c].map(line), '']);
  out.push(`## ${PARTS[4]}`, plan.roadIntro, `**Up next, in order:** ${plan.upNext}`, '',
    ...chapters(plan.road, (r) => `- [${r.done ? 'x' : ' '}] **${r.id}** ${r.text} · ${r.serves.join(' ')}${r.verdict ? '\n' + verdictLine(r.verdict, r.formerly) : ''}`));
  out.push(`## ${PARTS[5]}`, plan.parkedIntro, '', ...chapters(plan.parked, (t) => (typeof t === 'string' ? `- ${t}` : `- ${t.text}\n${verdictLine(t.verdict)}`)));
  part(6, plan.doneIntro, ...plan.done.map((d) => `- ${d.build ? '`' + d.build + '`' : '(this push)'} ${d.text}`));
  out.push(`## ${PARTS[7]}`, plan.where, '');
  return out.join('\n');
}

export const load = () => ({ plan: JSON.parse(readFileSync(PLAN_JSON, 'utf8')), rules: JSON.parse(readFileSync(RULES_JSON, 'utf8')) });
export function write(data) {
  writeFileSync(PLAN_JSON, JSON.stringify(data.plan, null, 1) + '\n');
  writeFileSync(RULES_JSON, JSON.stringify(data.rules, null, 1) + '\n');
  writeFileSync(PLAN_MD, render(data));
}
