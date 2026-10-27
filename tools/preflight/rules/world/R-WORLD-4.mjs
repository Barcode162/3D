// R-WORLD-4 · Every model the world names is in assets/models.
import { worldFiles } from './_world.mjs';
export default {
  id: 'R-WORLD-4',
  area: 'world',
  rule: 'Every model a kind or thing names is a file in assets/models.',
  why: 'A missing model leaves a hole in the bar and an error in the log.',
  when: ['assets/world/**', 'assets/models/**'],
  check(ctx) {
    const spots = [];
    const w = worldFiles(ctx);
    const have = new Set(ctx.list('assets/models'));
    const want = (file, where) => { if (file && !have.has(file)) spots.push({ where, fix: `assets/models/${file} isn't there: build it (tools/assets/) or fix the name` }); };
    for (const [id, k] of w.kinds) want(k.parts?.model?.file, `${k.file} → kind ${id}`);
    for (const p of w.places) for (const t of ctx.json(p).things || []) want(t.parts?.model?.file, `${p} → ${t.id}`);
    return spots;
  },
};
