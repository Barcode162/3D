// R-DOCS-2 · Every folder has its line in the Handbook's folder list.
export default {
  id: 'R-DOCS-2',
  area: 'docs',
  rule: 'Every folder (two levels down) has its one line in docs/handbook/folders.json.',
  why: 'J100 (the piano samples): assets/piano had no line and the ship stopped at the handbook check.',
  when: ['**'],
  check(ctx) {
    const described = ctx.json('docs/handbook/folders.json').folders;
    const spots = [];
    const folders = new Set();
    for (const f of ctx.changed) {
      if (!ctx.exists(f)) continue;  // deleted
      const parts = f.split('/').slice(0, -1);
      for (let i = 1; i <= Math.min(2, parts.length); i++) folders.add(parts.slice(0, i).join('/'));
    }
    for (const d of folders) if (!described[d] && !d.startsWith('docs/archive') && !d.startsWith('build'))
      spots.push({ where: `${d}/`, fix: 'add a one-line "what it is" for it to docs/handbook/folders.json' });
    return spots;
  },
};
