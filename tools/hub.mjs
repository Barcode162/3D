// Claude's side of the project hub (joshuavelasco.com/hub, hub/hub.php).
// Needs the Claude key in the environment as HUB_KEY (the owner makes it in
// the hub: Account → Make a new Claude key).
//   node tools/hub.mjs queue                 the "Ready for Claude" cards, in order
//   node tools/hub.mjs board [section]       every card (or one section's)
//   node tools/hub.mjs card <id>             one card in full
//   node tools/hub.mjs report <id> [--stage doing|check|done|review|ready|idea]
//        [--report "what I did, how to check"] [--question "..."] [--build abc1234]
//   node tools/hub.mjs add <section> "<title>" ["<plan>"]
//   node tools/hub.mjs set <id> [--title ..] [--section ..] [--plan-file f]   edit a card
//   node tools/hub.mjs section <id> "<about>"   a section's "what goes here" line
//   node tools/hub.mjs links                 rewrite every UNLOCKS line from the NEEDS lines
//   node tools/hub.mjs check                 the board against docs/HUB_MAP.md (exit 1 on errors)
// HUB_URL overrides the address (tests).
import { readFileSync } from 'node:fs';
const URL_ = process.env.HUB_URL || 'https://joshuavelasco.com/hub/hub.php';
const KEY = process.env.HUB_KEY || '';
if (!KEY) {
  console.log('No HUB_KEY: make one in the hub (Account → Make a new Claude key) and add it to the environment.');
  process.exit(2);
}
const call = async (body) => {
  const r = await fetch(URL_, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY } });
  const j = await r.json();
  if (!j.ok) { console.log('Hub: ' + (j.error || r.status)); process.exit(1); }
  return j;
};
const [cmd, ...args] = process.argv.slice(2);
const flag = (name) => { const i = args.indexOf('--' + name); return i >= 0 ? args[i + 1] : undefined; };
const line = (c) => `${c.id}  [${c.stage}] ${c.section}: ${c.title}${c.question ? '  (question open)' : ''}`;

// The card format (docs/HUB_MAP.md §5): labelled lines in the plan; refs are "(#id)".
const LABELS = ['YOUR NOTES', 'WHAT', 'NOW', 'STEPS', 'LATER', 'NEEDS', 'UNLOCKS', 'NOT HERE', 'DETAILS', 'HOME'];
const field = (plan, label) => {
  const m = (plan || '').match(new RegExp('^' + label + ':(.*)$', 'm'));
  return m ? m[1].trim() : null;
};
const refs = (text) => [...(text || '').matchAll(/\(#([0-9a-f]{10})\)/g)].map((m) => m[1]);
const needsOf = (c) => refs(field(c.plan, 'NEEDS'));
function withUnlocks(c, cards) {
  if (field(c.plan, 'WHAT') === null) return c.plan;  // not in the card format yet
  const by = cards.filter((o) => o.id !== c.id && needsOf(o).includes(c.id));
  const text = 'UNLOCKS: ' + (by.length ? by.map((o) => `${o.title} (#${o.id})`).join('; ') : 'nothing yet');
  if (/^UNLOCKS:.*$/m.test(c.plan)) return c.plan.replace(/^UNLOCKS:.*$/m, text);
  return c.plan.replace(/^(NEEDS:.*)$/m, `$1\n${text}`);
}
function checkBoard(b) {
  const out = [];
  const err = (text) => out.push({ error: true, text });
  const warn = (text) => out.push({ error: false, text });
  const ids = new Map(b.cards.map((c) => [c.id, c]));
  const sections = new Set(b.sections.map((s) => s.id));
  const titles = new Map();
  for (const c of b.cards) {
    const name = `${c.id} "${c.title}"`;
    const t = c.title.toLowerCase().replace(/[^a-z0-9]+/g, ' ').trim();
    if (titles.has(t)) err(`${name}: same title as ${titles.get(t)}`); else titles.set(t, c.id);
    if (!sections.has(c.section)) err(`${name}: unknown section ${c.section}`);
    if (field(c.plan, 'WHAT') === null) { err(`${name}: no WHAT line (not in the card format)`); continue; }
    if (field(c.plan, 'NEEDS') === null) err(`${name}: no NEEDS line (write "nothing" if none)`);
    for (const r of [...refs(c.plan)]) if (!ids.has(r)) err(`${name}: points at a card that's gone (#${r})`);
    for (const r of needsOf(c)) {
      if (r === c.id) err(`${name}: needs itself`);
      const o = ids.get(r);
      if (o && ['ready', 'doing'].includes(c.stage) && o.stage !== 'done') warn(`${name} is ${c.stage} but waits on "${o.title}" (${o.stage})`);
      if (o && c.stage === 'done' && o.stage !== 'done') warn(`${name} is done but needs "${o.title}" (${o.stage})`);
    }
    if (withUnlocks(c, b.cards) !== c.plan) err(`${name}: UNLOCKS out of date (run: node tools/hub.mjs links)`);
    if (c.section === 'ideas') {
      const home = field(c.plan, 'HOME');
      if (home === null) err(`${name}: Ideas card without a HOME line`);
      else if (sections.has(home.split(/[ (]/)[0])) warn(`${name}: has a home (${home}); move it there`);
    }
    if (/^bug:/i.test(c.title) && c.section === 'tools' && !/hub|test|deploy|report/i.test(c.title))
      warn(`${name}: a bug goes in the section of the broken thing`);
    if ((c.plan || '').length > 3900) warn(`${name}: plan near the 4000-character limit`);
  }
  // Loops in NEEDS (A needs B needs A) can never start.
  const state = new Map();
  const visit = (id, path) => {
    if (state.get(id) === 1) { err('NEEDS loop: ' + [...path, id].map((x) => ids.get(x)?.title).join(' → ')); return; }
    if (state.get(id) === 2 || !ids.has(id)) return;
    state.set(id, 1);
    for (const r of needsOf(ids.get(id))) visit(r, [...path, id]);
    state.set(id, 2);
  };
  for (const c of b.cards) visit(c.id, []);
  return out;
}

if (cmd === 'queue') {
  const q = await call({ a: 'queue' });
  if (!q.cards.length) console.log('Nothing ready for Claude.');
  for (const c of q.cards) console.log(line(c) + (c.plan ? '\n    plan: ' + c.plan.replace(/\n/g, '\n          ') : ''));
} else if (cmd === 'board') {
  const b = await call({ a: 'board' });
  for (const c of b.cards.filter((c) => !args[0] || c.section === args[0])) console.log(line(c));
} else if (cmd === 'card') {
  const b = await call({ a: 'board' });
  const c = b.cards.find((x) => x.id === args[0]);
  console.log(c ? JSON.stringify(c, null, 2) : 'No such card.');
} else if (cmd === 'report') {
  const body = { a: 'report', id: args[0] };
  for (const f of ['stage', 'report', 'question', 'build']) if (flag(f) !== undefined) body[f] = flag(f);
  const r = await call(body);
  console.log(line(r.card));
} else if (cmd === 'add') {
  const r = await call({ a: 'save', card: { section: args[0], title: args[1], plan: args[2] || '' } });
  console.log(line(r.card));
} else if (cmd === 'set') {
  const card = { id: args[0] };
  if (flag('title') !== undefined) card.title = flag('title');
  if (flag('section') !== undefined) card.section = flag('section');
  if (flag('plan-file') !== undefined) card.plan = readFileSync(flag('plan-file'), 'utf8').trim();
  const r = await call({ a: 'save', card });
  console.log(line(r.card));
} else if (cmd === 'section') {
  const r = await call({ a: 'section', id: args[0], about: args[1] });
  console.log(r.section.id + ': ' + r.section.about);
} else if (cmd === 'links') {
  const b = await call({ a: 'board' });
  for (const c of b.cards) {
    const plan = withUnlocks(c, b.cards);
    if (plan !== c.plan) { await call({ a: 'save', card: { id: c.id, plan } }); console.log('links: ' + line(c)); }
  }
} else if (cmd === 'check') {
  const b = await call({ a: 'board' });
  const problems = checkBoard(b);
  for (const p of problems) console.log((p.error ? 'ERROR ' : 'warn  ') + p.text);
  console.log(`${b.cards.length} cards, ${problems.filter((p) => p.error).length} errors, ${problems.filter((p) => !p.error).length} warnings.`);
  process.exit(problems.some((p) => p.error) ? 1 : 0);
} else {
  console.log('Usage: node tools/hub.mjs queue | board [section] | card <id> | report <id> --stage .. --report .. | add <section> "<title>" | set <id> .. | section <id> ".." | links | check');
}
