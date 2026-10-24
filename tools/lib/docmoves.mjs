// Where the old notes went (restructure J6.6): docs/X.md → its new place. The
// Handbook's generated pages show texts from the stores' files (which keep the
// old paths until their next publish, KI-31) with the new paths.
import { readdirSync } from 'node:fs';
export const MOVES = {
  'docs/PROTOCOL.md': 'docs/handbook/protocol.md', 'docs/DRIVING_REWORK.md': 'docs/plan/jobs/W20.md',
  'docs/manual/': 'docs/handbook/manual/', 'docs/audit/': 'docs/archive/audit/', 'docs/PLAN.md': 'PLAN.md',
  'docs/plan/restructure.md': 'docs/archive/restructure.md',  // J8.2
};
for (const f of readdirSync('docs/archive').filter((x) => /^[A-Z_-]+\.md$/.test(x) && x !== 'README.md')) MOVES[`docs/${f}`] = `docs/archive/${f}`;
const RE = new RegExp(Object.keys(MOVES).sort((a, b) => b.length - a.length).map((k) => k.replace(/[.*+?^${}()|[\]\\/]/g, '\\$&')).join('|'), 'g');
export const fixDocRefs = (text) => String(text ?? '').replace(RE, (m) => MOVES[m]);
