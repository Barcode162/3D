// Claude's side of the project hub (joshuavelasco.com/hub, hub/hub.php).
// Needs the Claude key in the environment as HUB_KEY (the owner makes it in
// the hub: Account → Make a new Claude key).
//   node tools/hub.mjs queue                 the "Ready for Claude" cards, in order
//   node tools/hub.mjs board [section]       every card (or one section's)
//   node tools/hub.mjs card <id>             one card in full
//   node tools/hub.mjs report <id> [--stage doing|check|done|review|ready|idea]
//        [--report "what I did, how to check"] [--question "..."] [--build abc1234]
//   node tools/hub.mjs add <section> "<title>" ["<plan>"]
// HUB_URL overrides the address (tests).
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
} else {
  console.log('Usage: node tools/hub.mjs queue | board [section] | card <id> | report <id> --stage .. --report .. | add <section> "<title>"');
}
