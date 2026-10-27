// R-WORLD-3 · Things are placed with plain numbers, and their kinds exist.
import { worldFiles } from './_world.mjs';
export default {
  id: 'R-WORLD-3',
  area: 'world',
  rule: 'A thing has a kind that exists, "at" as three numbers and "turn" as one number.',
  why: 'Incident 2026-10-26 (J72 deploy red): turns written as lists failed the schema after the push.',
  when: ['assets/world/**'],
  check(ctx) {
    const spots = [];
    const w = worldFiles(ctx);
    const num = (v) => typeof v === 'number' && Number.isFinite(v);
    for (const p of w.places) for (const t of ctx.json(p).things || []) {
      const where = `${p} → ${t.id}`;
      if (!w.kinds.has(t.kind)) spots.push({ where, fix: `its kind "${t.kind}" isn't in any assets/world/kinds file: add the kind or fix the name` });
      if (t.at !== undefined && !(Array.isArray(t.at) && t.at.length === 3 && t.at.every(num))) spots.push({ where, fix: '"at" must be [x, y, z], three numbers' });
      if (t.turn !== undefined && !num(t.turn)) spots.push({ where, fix: '"turn" must be one number (degrees about up); a tilt needs its own model' });
    }
    return spots;
  },
};
