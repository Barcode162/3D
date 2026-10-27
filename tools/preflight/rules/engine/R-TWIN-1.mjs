// R-TWIN-1 · Numbers that live in two places say the same thing.
// A list of twins: change one, change the others. Add a line when a new
// number has to match across files.
const TWINS = [
  { name: 'kMaxLights', at: [['looped/scene_renderer.h', /kMaxLights = (\d+);/], ['looped/shaders/scene.wgsl', /const kMaxLights = (\d+)u;/], ['looped/shaders/gi.wgsl', /const kMaxLights = (\d+)u;/]] },
  { name: 'kMaxGridCells', at: [['looped/scene_renderer.h', /kGridSide = (\d+);/, (v) => v * v], ['looped/shaders/scene.wgsl', /const kMaxGridCells = (\d+)u;/], ['looped/shaders/gi.wgsl', /const kMaxGridCells = (\d+)u;/]] },
];
export default {
  id: 'R-TWIN-1',
  area: 'engine',
  rule: 'A number kept in several files (the light limits, the light grid) is the same in all of them.',
  why: 'The C++ and the shaders share the lighting block byte for byte: a mismatch draws garbage or nothing.',
  when: TWINS.flatMap((t) => t.at.map(([f]) => f)),
  check(ctx) {
    const spots = [];
    for (const t of TWINS) {
      const vals = t.at.map(([f, re, map]) => { const m = ctx.read(f).match(re); return m ? (map ? map(+m[1]) : +m[1]) : null; });
      if (vals.some((v) => v === null) || new Set(vals).size > 1)
        spots.push({ where: t.name, fix: `${t.at.map(([f], i) => `${f} says ${vals[i]}`).join(', ')}: make them match` });
    }
    return spots;
  },
};
