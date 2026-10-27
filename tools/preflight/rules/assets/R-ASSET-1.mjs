// R-ASSET-1 · Every model has its credits line.
export default {
  id: 'R-ASSET-1',
  area: 'assets',
  rule: 'Every model in assets/models is named in assets/models/CREDITS.md (its script, its textures\' licence).',
  why: 'Charter: no asset without a licence that allows it; the credits are where that is shown.',
  when: ['assets/models/**'],
  check(ctx) {
    const credits = ctx.read('assets/models/CREDITS.md');
    // `name.glb`, `prefix_*` and `neon_*.glb` in backticks.
    const pats = [...credits.matchAll(/`([^`]+)`/g)].map((m) => m[1].replace(/\.glb$/, ''))
      .map((p) => new RegExp('^' + p.replace(/[.+^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*') + '$'));
    const spots = [];
    for (const f of ctx.list('assets/models').filter((x) => x.endsWith('.glb'))) {
      const name = f.replace(/\.glb$/, '');
      if (!pats.some((re) => re.test(name))) spots.push({ where: `assets/models/${f}`, fix: 'add it to a row of assets/models/CREDITS.md: the script that builds it and its textures' });
    }
    return spots;
  },
};
