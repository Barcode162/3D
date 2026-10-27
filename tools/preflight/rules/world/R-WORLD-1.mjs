// R-WORLD-1 · Every thing in a place has its own id.
import { worldFiles } from './_world.mjs';
export default {
  id: 'R-WORLD-1',
  area: 'world',
  rule: 'Every thing in a place has its own id.',
  why: 'Incident 2026-10-26 (lounge): a second "bookshelf" hid the first, and its tap spot was gone.',
  when: ['assets/world/places/**'],
  check(ctx) {
    const spots = [];
    for (const p of worldFiles(ctx).places) {
      const seen = new Set();
      for (const t of ctx.json(p).things || []) {
        if (seen.has(t.id)) spots.push({ where: `${p} → ${t.id}`, fix: 'two things share this id: rename one (ids name the thing everywhere, so pick a new one)' });
        seen.add(t.id);
      }
    }
    return spots;
  },
};
