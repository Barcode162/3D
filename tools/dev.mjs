#!/usr/bin/env node
// Claude's side of Dev (dev/api.php): what the user left in Dev → Plan.
//   node tools/dev.mjs notes          the notes and ideas not read yet (read these at the start of a go)
//   node tools/dev.mjs notes --all    every note
//   node tools/dev.mjs read <id>...   mark notes as read once they're in the plan (park → plan.mjs park; a rule → plan.mjs rule)
// Needs HUB_KEY (the hub's Claude key). DEV_URL overrides the store.
const URL_ = process.env.DEV_URL || 'https://joshuavelasco.com/dev/api.php';
const KEY = process.env.HUB_KEY || '';
const [cmd, ...rest] = process.argv.slice(2);
if (!KEY) { console.log('No HUB_KEY in the environment.'); process.exit(2); }
async function post(body) {
  const r = await fetch(URL_, { method: 'POST', headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY }, body: JSON.stringify(body) });
  try { return { status: r.status, ...(await r.json()) }; } catch { return { ok: 0, status: r.status, error: `Dev's store did not answer (HTTP ${r.status}; is Dev deployed yet?)` }; }
}
if (cmd === 'notes') {
  const j = await post({ a: 'notes' });
  if (!j.ok) { console.log(j.error || `HTTP ${j.status}`); process.exit(1); }
  const list = rest.includes('--all') ? j.notes : j.notes.filter((n) => !n.read);
  if (!list.length) console.log('Nothing new in Dev.');
  for (const n of list.reverse()) console.log(`${n.id}  [${n.kind}] ${new Date(n.at * 1000).toISOString().slice(0, 16)} ${n.by}: ${n.text}`);
} else if (cmd === 'read') {
  const j = await post({ a: 'read', ids: rest });
  console.log(j.ok ? `marked ${rest.length} as read` : j.error);
} else {
  console.log('usage: node tools/dev.mjs notes [--all] | read <id>...'); process.exit(2);
}
