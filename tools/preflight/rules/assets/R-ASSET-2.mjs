// R-ASSET-2 · Every sound and music folder has its credits.
export default {
  id: 'R-ASSET-2',
  area: 'assets',
  rule: 'Every assets folder with sound or music in it has a CREDITS.md naming where it came from and its licence.',
  why: 'Charter: no copyrighted music, no asset without a licence that allows it.',
  when: ['assets/**'],
  check(ctx) {
    const spots = [];
    for (const d of ctx.list('assets')) {
      const dir = `assets/${d}`;
      if (!ctx.isDir(dir)) continue;
      const media = ctx.list(dir).filter((f) => /\.(mp3|m4a|ogg|wav|mid|flac)$/i.test(f));
      if (media.length && !ctx.exists(`${dir}/CREDITS.md`)) spots.push({ where: `${dir}/`, fix: `${media.length} sound file(s) and no CREDITS.md: add one (source, author, licence)` });
    }
    return spots;
  },
};
