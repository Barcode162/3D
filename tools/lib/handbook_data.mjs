// The Handbook's data reference and the world inventory (restructure J6.3),
// written from the files by tools/handbook.mjs: what each world field, theme
// token, screen part, list entry, config field and room message is, and every
// thing in every place with the model that holds it.
import { readFileSync, readdirSync } from 'node:fs';

const json = (f) => JSON.parse(readFileSync(f, 'utf8'));
const cell = (s) => String(s ?? '').replace(/\|/g, '\\|').replace(/\n/g, ' ');
const code = (s) => (s === undefined || s === '' ? '' : '`' + String(s).replace(/`/g, '') + '`');

// One schema record ({label, about, fields}) as a table.
function fieldTable(rec) {
  const rows = Object.entries(rec.fields || {}).map(([k, f]) => {
    const range = f.min !== undefined || f.max !== undefined ? `${f.min ?? ''} … ${f.max ?? ''}${f.step ? ` (step ${f.step})` : ''}` : '';
    const type = f.type === 'choice' ? `choice: ${f.choices}` : f.type;
    return `| ${code(k)} | ${cell(f.label)} | ${cell(type)} | ${cell(f.unit)} | ${cell(range)} | ${f.live ? 'yes' : ''} | ${cell(f.about)} |`;
  });
  return rows.length ? '| Field | Label | Type | Unit | Range | Live | What |\n|---|---|---|---|---|---|---|\n' + rows.join('\n') + '\n' : '';
}

export function worldPage(banner) {
  const s = json('assets/world/schema.json');
  let out = banner('assets/world/schema.json') + '# Data: the world\'s files\n\n' + cell(s.about) + '\n\n' +
    '"Live": the Studio changes it without rebuilding the scene. The files: `assets/world/places/*.json`, `kinds/*.json`, `catalog.json`, `materials.json`, `songs.json` ([the world model](../systems/engine/world-model.md)).\n';
  for (const key of ['place', 'thing', 'kind', 'state', 'loop', 'piece', 'catalog', 'materials']) {
    const rec = s[key];
    out += `\n## ${rec.label} (\`${key}\`)\n\n${cell(rec.about)}\n\n` + fieldTable(rec);
  }
  out += '\n## Parts (`parts`)\n\nThe abilities a kind (or a thing) can have; each engine system reads its own.\n';
  for (const [k, rec] of Object.entries(s.parts)) out += `\n### ${rec.label} (\`${k}\`)\n\n${cell(rec.about)}\n\n` + fieldTable(rec);
  out += `\n## Songs (\`songs\`)\n\n${cell(s.songs.about)}\n\n` + fieldTable({ fields: s.songs.item.fields });
  out += '\n## Choice lists (`choices`)\n\n| List | Choices |\n|---|---|\n' +
    Object.entries(s.choices).map(([k, v]) => `| ${code(k)} | ${v.map(code).join(', ')} |`).join('\n') + '\n';
  return out;
}

export function uiPage(banner) {
  const t = json('ui/system/tokens.json'), p = json('ui/system/parts.json');
  const groups = Object.fromEntries(t.groups.map((g) => [g.id, g.name]));
  let out = banner('ui/system/tokens.json and ui/system/parts.json') + '# Data: themes and screens\n\n' +
    'What a theme sets and what a screen is made of ([the UI kit](../systems/engine/ui-kit.md)). Both files come from the UI store (`node tools/project.mjs ui pull`); never edited by hand.\n\n' +
    `## Theme tokens\n\n${cell(t.about)}\n\n| Token | Group | Kind | What it does |\n|---|---|---|---|\n` +
    t.tokens.map((x) => `| ${code(x.id)} | ${cell(groups[x.group] || x.group)} | ${cell(x.kind)} | ${cell(x.what)} |`).join('\n') + '\n';
  out += `\n## Screen parts\n\n${cell(p.about)}\n\n| Part | Name | What it is | Holds parts | Props (kind, default) |\n|---|---|---|---|---|\n` +
    p.parts.map((x) => `| ${code(x.type)} | ${cell(x.name)} | ${cell(x.what)} | ${x.children ? 'yes' : ''} | ${cell(Object.entries(x.props || {}).map(([k, v]) => `${k} (${v.kind}${v.default !== undefined ? ', ' + JSON.stringify(v.default) : ''})`).join('; '))} |`).join('\n') + '\n';
  const screens = readdirSync('ui/system/screens').filter((f) => f.endsWith('.json')).sort();
  const themes = readdirSync('ui/system/themes').filter((f) => f.endsWith('.json')).sort();
  out += `\n## The screens and themes there are\n\n- Screens (${screens.length}): ${screens.map((f) => code(f.replace(/\.json$/, ''))).join(', ')}\n- Themes (${themes.length}): ${themes.map((f) => code(f.replace(/\.json$/, ''))).join(', ')}\n`;
  return out;
}

// What each field of the game's config means (a new field shows its raw value until it's added here).
const CONFIG = {
  build: 'the commit that was built, 7 letters (the `build xxxxxxx` label on screen)',
  proto: 'the room protocol version this build speaks',
  ws: 'the room server\'s address (Cloudflare, `CF_WS`); empty: no multiplayer',
  room: 'the room the bar joins (`main`)',
  accountsUrl: 'where accounts live (`account.php`)',
  reportUrl: 'where phone reports go (`report.php`)',
};

export function listsPage(banner) {
  const tb = json('toolbelt/tools.json');
  const groups = Object.fromEntries(tb.groups.map((g) => [g.id, g.name]));
  const deploy = readFileSync('.github/workflows/deploy.yml', 'utf8');
  const cfg = (deploy.match(/json\.dump\(\{([\s\S]*?)\},\s*\n?\s*open\("build\/site\/config\.json"/) || [, ''])[1];
  const cfgKeys = [...cfg.matchAll(/"([A-Za-z]+)":\s*([^,}]+(?:\[[^\]]*\])?)/g)].map((m) => [m[1], m[2].trim()]);
  let out = banner('toolbelt/tools.json and .github/workflows/deploy.yml') + '# Data: the lists and the game\'s config\n\n' +
    `## Projects (\`toolbelt/tools.json\` → \`projects\`)\n\nThe one list of projects ([projects](../systems/site/projects.md)).\n\n| Id | Name | Status | About | On the site |\n|---|---|---|---|---|\n` +
    tb.projects.map((p) => `| ${code(p.id)} | ${cell(p.name)} | ${cell(p.status)} | ${cell(p.about)} | ${p.site ? `${p.site.place}${p.site.href ? ' → ' + code(p.site.href) : ''}` : 'no'} |`).join('\n') + '\n' +
    `\n## Tools (\`toolbelt/tools.json\` → \`tools\`)\n\n| Id | Name | Project | Group | Opens / runs | What |\n|---|---|---|---|---|---|\n` +
    tb.tools.map((t) => `| ${code(t.id)} | ${cell(t.name)} | ${code(t.project)} | ${cell(groups[t.group] || t.group)} | ${t.href ? code(t.href) : t.run ? code(t.run) : ''}${t.status === 'soon' ? ' (soon)' : ''} | ${cell(t.what)} |`).join('\n') + '\n' +
    '\n## The game\'s config (`/bar/config.json`, written by `deploy.yml`)\n\nThe page reads it at start-up.\n\n| Field | What it is |\n|---|---|\n' +
    cfgKeys.map(([k, v]) => `| ${code(k)} | ${cell(CONFIG[k] || 'set by deploy.yml: ' + v)} |`).join('\n') + '\n';
  return out;
}

export async function protocolPage(banner) {
  const { PROTO, BIN, TO_SERVER, TO_CLIENT, MAX_SLOTS, MAX_TEXT_TO_SERVER, MAX_SIGNAL_TO_SERVER, MAX_TO_CLIENT } = await import('../../cloud/proto.js');
  const table = (dir) => Object.entries(dir).map(([letter, m]) => {
    const fields = Object.entries(m.spec).map(([k, v]) => (Array.isArray(v) && v[1] ? k : `${k}?`)).join(', ');
    return `| ${code(letter)} | ${cell(m.name)} | ${cell(fields)} |`;
  }).join('\n');
  return banner('cloud/proto.js') + '# Data: the room protocol\n\n' +
    `The messages between a phone and the room, read from the codec itself (\`cloud/proto.js\`; the game's twin is \`looped/netproto.*\`, both checked on \`proto/vectors.json\`). The full spec with every byte: \`docs/PROTOCOL.md\` ([the protocol](../systems/engine/net-codec-and-protocol.md)).\n\n` +
    `- Protocol version: **${PROTO}** (the room also accepts the one before)\n- Slots per room: ${MAX_SLOTS}\n- Largest text to the room: ${MAX_TEXT_TO_SERVER} bytes (a voice signal: ${MAX_SIGNAL_TO_SERVER}); largest to a phone: ${MAX_TO_CLIENT}\n\n` +
    '## Binary messages\n\n| Type | Byte |\n|---|---|\n' + Object.entries(BIN).map(([k, v]) => `| ${code(k)} | ${code('0x' + v.toString(16).padStart(2, '0'))} |`).join('\n') + '\n' +
    '\n## Text messages: phone → room\n\nOne type letter + a JSON object; `?` marks an optional field.\n\n| Letter | Message | Fields |\n|---|---|---|\n' + table(TO_SERVER) + '\n' +
    '\n## Text messages: room → phone\n\n| Letter | Message | Fields |\n|---|---|---|\n' + table(TO_CLIENT) + '\n';
}

// Every thing in every place: its kind, what it sits on, the model that holds it.
export function inventoryPage(banner) {
  const places = readdirSync('assets/world/places').filter((f) => f.endsWith('.json')).sort();
  const kinds = {};
  for (const f of readdirSync('assets/world/kinds').filter((x) => x.endsWith('.json'))) for (const k of json(`assets/world/kinds/${f}`).kinds) kinds[k.id] = k;
  const chain = (id) => { const out = []; for (let k = kinds[id], n = 0; k && n < 16; k = kinds[k.is], n++) out.push(k); return out; };
  const modelOf = (thing) => thing.parts?.model?.file || chain(thing.kind).map((k) => k.parts?.model?.file).find(Boolean) || '';
  const isRoom = (id) => chain(id).some((k) => k.id === 'room');
  const used = new Set();
  let out = banner('assets/world/places/*.json and kinds/*.json') + '# The world inventory\n\n' +
    'Every thing in every place of the built-in world (the repo\'s copy; the live version may differ: `node tools/project.mjs world status`): its kind, what it sits on, and the model file that holds it today. ' +
    '**Room model**: the thing is a whole room or wall set (its model is room-sized, so smaller things baked into it can\'t be moved one by one). **No model**: a spot, a light or a helper with no 3D model of its own.\n';
  for (const f of places) {
    const p = json(`assets/world/places/${f}`), things = p.things || [];
    out += `\n## ${f.replace(/\.json$/, '')} (${things.length} things)\n\n| Thing | Kind | Sits on | Model | Note |\n|---|---|---|---|---|\n`;
    for (const t of things) {
      used.add(t.kind);
      const m = modelOf(t);
      out += `| ${code(t.id)} | ${code(t.kind)} | ${t.parent ? code(t.parent) : ''} | ${m ? code(m) : ''} | ${isRoom(t.kind) ? 'room model' : m ? '' : 'no model'} |\n`;
    }
  }
  const unplaced = Object.keys(kinds).filter((id) => !used.has(id) && !Object.values(kinds).some((k) => k.is === id)).sort();
  const bases = Object.keys(kinds).filter((id) => !used.has(id) && Object.values(kinds).some((k) => k.is === id)).sort();
  out += `\n## Kinds not placed anywhere\n\nKinds that no place uses (they can be added from the Studio's Library): ${unplaced.map(code).join(', ') || 'none'}.\n\n` +
    `Base kinds (others build on them; not placed themselves): ${bases.map(code).join(', ') || 'none'}.\n`;
  return out;
}
