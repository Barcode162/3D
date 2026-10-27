// R-WORLD-2 · Every kind has a state with a live loop.
import { worldFiles } from './_world.mjs';
export default {
  id: 'R-WORLD-2',
  area: 'world',
  rule: 'Every kind has a state that plays a live loop.',
  why: 'Incident 2026-10-26 (lounge): 17 new kinds with no states; the world reported each one.',
  when: ['assets/world/kinds/**'],
  check(ctx) {
    const spots = [];
    const w = worldFiles(ctx);
    // As the world checks it (looped/world/world.cpp check): the kinds things
    // use; a kind marked "still" (it never moves, and says why) is fine.
    const used = new Set(w.places.flatMap((p) => (ctx.json(p).things || []).map((t) => t.kind)));
    for (const [id, k0] of w.kinds) {
      const k = w.resolve(id);
      if (!used.has(id) || k.still) continue;
      const states = Object.values(k.states || {});
      if (!states.length) { spots.push({ where: `${k0.file} → kind ${id}`, fix: 'no states: give it "states": {"on": {"loop": "<a loop>"}} and "start": "on" (or "is" a kind that has them)' }); continue; }
      for (const [name, s] of Object.entries(k.states || {}))
        if (s.loop && !(k.loops || {})[s.loop]) spots.push({ where: `${k0.file} → kind ${id}, state ${name}`, fix: `its loop "${s.loop}" isn't in the kind's loops: add it or name one that is` });
    }
    return spots;
  },
};
