// Claude's side of the project hub (joshuavelasco.com/hub, hub/hub.php).
// Only `post` and `posts` (the news) are in use: the board commands below
// (inbox, queue, cards, places…) belong to the retired board and leave in J5.4.
// Needs the Claude key in the environment as HUB_KEY (the owner makes it in
// the hub: Account → Make a new Claude key).
//   node tools/hub.mjs projects              every project (the hub holds one board per project)
//   node tools/hub.mjs inbox                 cards where the owner wrote (answers, messages, "what you want")
//                                            since Claude last did: read these first on "go"
//   node tools/hub.mjs queue                 the "Ready for Claude" cards, in order
//   node tools/hub.mjs board [section]       every card (or one section's)
//   node tools/hub.mjs card <id>             one card in full
//   node tools/hub.mjs report <id> [--stage doing|check|done|review|ready|idea]
//        [--report "what I did, how to check"] [--question "..."] [--build abc1234]
//        (the report and the question also go on the card's conversation)
//   node tools/hub.mjs say <id> "message"    a message on the card's conversation (clears its inbox mark)
//   node tools/hub.mjs add <section> "<title>" ["<plan>"]
//   node tools/hub.mjs set <id> [--title ..] [--section ..] [--plan-file f]   edit a card
//   node tools/hub.mjs section <id> "<about>"   a section's "what goes here" line
//   node tools/hub.mjs links                 rewrite every UNLOCKS line from the NEEDS lines
//   node tools/hub.mjs check                 the board against docs/HUB_MAP.md (exit 1 on errors)
//   node tools/hub.mjs snapshot [file]       the board as Markdown in the repo (default docs/hub/<project>.md)
//   node tools/hub.mjs migrate-notes         move YOUR NOTES lines out of plans into the cards' "what you want"
// Hub v3 (docs/LOOPED.md C): the map, the news, one-tap answers.
//   report <id> ... --question "..." --choices "Arcade|Lounge"   the owner answers with one tap
//   node tools/hub.mjs spots                 every place on the map, with its line and talk
//   node tools/hub.mjs spot <id> [--about ".."] [--status live|building|planned|idea]
//   node tools/hub.mjs spotsay <id> "message"   answer in a place's talk
//   node tools/hub.mjs place <card> <spot>   put a card on a place
//   node tools/hub.mjs post --build abc1234 --title ".." --text ".." [--shots a.png,b.png]   a news post
//        (pictures are made JPEG, 1000 px at most, and uploaded first; Home shows it,
//         the owner's Publish switch puts it on joshuavelasco.com/news)
//   node tools/hub.mjs posts                 the news posts
// Every command takes --project <id> (default HUB_PROJECT, else bar3d).
// HUB_URL overrides the address (tests).
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { basename, extname } from 'node:path';
import { dirname } from 'node:path';
const URL_ = process.env.HUB_URL || 'https://joshuavelasco.com/hub/hub.php';
const KEY = process.env.HUB_KEY || '';
if (!KEY) {
  console.log('No HUB_KEY: make one in the hub (Account → Make a new Claude key) and add it to the environment.');
  process.exit(2);
}
const argv = process.argv.slice(2);
const PROJECT = (() => { const i = argv.indexOf('--project'); return i >= 0 ? argv.splice(i, 2)[1] : process.env.HUB_PROJECT || 'bar3d'; })();
const call = async (body) => {
  body.project ??= PROJECT;
  const r = await fetch(URL_, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY } });
  const j = await r.json();
  if (!j.ok) { console.log('Hub: ' + (j.error || r.status)); process.exit(1); }
  return j;
};
const [cmd, ...args] = argv;
const flag = (name) => { const i = args.indexOf('--' + name); return i >= 0 ? args[i + 1] : undefined; };
const line = (c) => `${c.id}  [${c.stage}] ${c.section}: ${c.title}${c.question ? '  (question open)' : ''}${c.unread ? '  (owner wrote)' : ''}`;
const indent = (t) => t.replace(/\n/g, '\n          ');
const when = (at) => new Date(at * 1000).toISOString().slice(0, 16).replace('T', ' ');
// The owner's words since Claude last wrote, oldest first.
const fresh = (c) => {
  const t = c.thread || [];
  let i = t.length;
  while (i > 0 && t[i - 1].role === 'owner') i--;
  return t.slice(i);
};
const show = (c) => {
  let out = line(c);
  if (c.notes) out += '\n    want: ' + indent(c.notes);
  if (c.plan) out += '\n    plan: ' + indent(c.plan);
  for (const m of fresh(c)) out += `\n    ${m.kind === 'answer' ? 'answer' : 'says'}: ` + indent(m.text);
  return out;
};

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
    if ((c.plan || '').length > 5900) warn(`${name}: plan near the 6000-character limit`);
    if (field(c.plan, 'YOUR NOTES') !== null) warn(`${name}: YOUR NOTES in the plan (run: node tools/hub.mjs migrate-notes)`);
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

// The board as Markdown, so the repo has its own copy (docs/HUB_MAP.md §7).
function snapshot(b) {
  const out = [`# Hub board: ${PROJECT}`, '',
    `Copied from the hub by \`node tools/hub.mjs snapshot\` on ${when(Date.now() / 1000)} UTC. The hub is the live`,
    'version; this file is the copy in the repo (read it when the hub is out of reach).', ''];
  const stages = { idea: 'Idea', review: 'Review', ready: 'Ready for Claude', doing: 'In progress', check: 'Check on phone', done: 'Done' };
  for (const s of b.sections) {
    const cards = b.cards.filter((c) => c.section === s.id);
    out.push(`## ${s.name}${s.advanced ? ' (advanced)' : ''}`, '', s.about ? `_${s.about}_` : '', '');
    for (const c of cards) {
      out.push(`### ${c.title}`, '', `\`${c.id}\` · ${stages[c.stage] || c.stage}${c.build ? ` · build ${c.build}` : ''}`, '');
      if (c.notes) out.push('**What the owner wants:** ' + c.notes.replace(/\n+/g, ' / '), '');
      if (c.plan) out.push('```', c.plan, '```', '');
      if (c.report) out.push('**Claude\'s report:** ' + c.report.replace(/\n+/g, ' / '), '');
      if (c.question) out.push('**Open question:** ' + c.question.replace(/\n+/g, ' / '), '');
    }
  }
  return out.filter((l, i, a) => !(l === '' && a[i - 1] === '')).join('\n') + '\n';
}

if (cmd === 'projects') {
  const r = await call({ a: 'projects' });
  for (const p of r.projects) console.log(`${p.id}  ${p.name}: ${p.counts.ready} ready, ${p.counts.doing} doing, ${p.counts.check} to check, ${p.questions} questions${p.link ? '  ' + p.link : ''}`);
} else if (cmd === 'inbox') {
  const q = await call({ a: 'inbox' });
  if (!q.cards.length && !(q.spots || []).length) console.log('Nothing new from the owner.');
  for (const c of q.cards) console.log(show(c));
  for (const sp of q.spots || []) {
    const t = sp.thread || [];
    let i = t.length;
    while (i > 0 && t[i - 1].role === 'owner') i--;
    console.log(`place ${sp.id}  (the owner wrote in its talk)` + t.slice(i).map((m) => '\n    says: ' + indent(m.text)).join(''));
  }
} else if (cmd === 'spots') {
  const r = await call({ a: 'spots' });
  for (const sp of r.spots) console.log(`${sp.id.padEnd(9)} [${sp.layer}] ${sp.icon} ${sp.name} (${sp.status}): ${sp.about}${sp.unread ? '  (owner wrote)' : ''}`);
} else if (cmd === 'spot') {
  const body = { a: 'spot', id: args[0] };
  if (flag('about') !== undefined) body.about = flag('about');
  if (flag('status') !== undefined) body.status = flag('status');
  const r = await call(body);
  console.log(`${r.spot.id}: ${r.spot.about || ''} (${r.spot.status || ''})`);
} else if (cmd === 'spotsay') {
  const r = await call({ a: 'spotsay', id: args[0], text: args[1] });
  console.log(`${r.spot.id}: said.`);
} else if (cmd === 'place') {
  const r = await call({ a: 'save', card: { id: args[0], spot: args[1] } });
  console.log(line(r.card) + '  → ' + r.card.spot);
} else if (cmd === 'post') {
  const build = flag('build') || '';
  const names = [];
  for (const file of (flag('shots') || '').split(',').map((x) => x.trim()).filter(Boolean)) {
    // JPEG, at most 1000 px tall or wide (python3 + PIL), else as it is.
    let data, type = 'jpeg';
    try {
      data = execFileSync('python3', ['-c', 'import sys,io;from PIL import Image;im=Image.open(sys.argv[1]).convert("RGB");im.thumbnail((1000,1000));b=io.BytesIO();im.save(b,"JPEG",quality=84);sys.stdout.buffer.write(b.getvalue())', file], { maxBuffer: 1 << 24 });
    } catch (e) {
      data = readFileSync(file);
      type = extname(file).toLowerCase() === '.png' ? 'png' : 'jpeg';
    }
    const name = basename(file, extname(file)).toLowerCase().replace(/[^a-z0-9_-]/g, '-').slice(0, 40);
    const r = await call({ a: 'shot', build, name, image: `data:image/${type};base64,` + data.toString('base64') });
    names.push(r.file);
  }
  const r = await call({ a: 'post', post: { build, title: flag('title') || '', text: flag('text') || '', shots: names } });
  console.log(`post ${r.post.id}: ${r.post.title} (${r.post.shots.length} pictures, private until the owner publishes)`);
} else if (cmd === 'posts') {
  const r = await call({ a: 'posts' });
  for (const p of r.posts) console.log(`${p.id}  ${when(p.at)}  ${p.build || '-------'}  ${p.published ? 'PUBLIC ' : 'private'}  ${p.title}  (${p.shots.length} pictures)`);
} else if (cmd === 'queue') {
  const q = await call({ a: 'queue' });
  if (!q.cards.length) console.log('Nothing ready for Claude.');
  for (const c of q.cards) console.log(show(c));
} else if (cmd === 'say') {
  const r = await call({ a: 'say', id: args[0], text: args[1] });
  console.log(line(r.card));
} else if (cmd === 'snapshot') {
  const b = await call({ a: 'board' });
  const file = args[0] || `docs/hub/${PROJECT}.md`;
  mkdirSync(dirname(file), { recursive: true });
  writeFileSync(file, snapshot(b));
  console.log(`${file}: ${b.cards.length} cards`);
} else if (cmd === 'migrate-notes') {
  const b = await call({ a: 'board' });
  for (const c of b.cards) {
    const notes = field(c.plan, 'YOUR NOTES');
    if (notes === null) continue;
    const plan = c.plan.replace(/^YOUR NOTES:.*\n?/m, '').trim();
    const card = { id: c.id, plan };
    if (!c.notes) card.notes = notes;
    else if (!c.notes.includes(notes)) { console.log('skip (has its own notes): ' + line(c)); continue; }
    await call({ a: 'save', card });
    console.log('moved: ' + line(c));
  }
} else if (cmd === 'board') {
  const b = await call({ a: 'board' });
  for (const c of b.cards.filter((c) => !args[0] || c.section === args[0])) console.log(line(c));
} else if (cmd === 'card') {
  const b = await call({ a: 'board' });
  const c = b.cards.find((x) => x.id === args[0]);
  if (!c) console.log('No such card.');
  else {
    console.log(show(c));
    if (c.report) console.log('    report: ' + indent(c.report));
    if (c.question) console.log('    question: ' + indent(c.question));
    for (const m of (c.thread || []).slice(-10)) console.log(`    ${when(m.at)} ${m.who} (${m.kind}): ` + indent(m.text));
  }
} else if (cmd === 'report') {
  const body = { a: 'report', id: args[0] };
  for (const f of ['stage', 'report', 'question', 'build']) if (flag(f) !== undefined) body[f] = flag(f);
  if (flag('choices') !== undefined) body.choices = flag('choices').split('|').map((x) => x.trim()).filter(Boolean);
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
  console.log('Usage: node tools/hub.mjs [--project id] projects | inbox | queue | board [section] | card <id> | report <id> --stage .. --report .. | say <id> ".." | add <section> "<title>" | set <id> .. | section <id> ".." | links | check | snapshot [file] | migrate-notes');
}
