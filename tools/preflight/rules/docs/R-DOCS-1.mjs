// R-DOCS-1 · A page whose controls changed updates its Handbook page with it.
export default {
  id: 'R-DOCS-1',
  area: 'docs',
  rule: 'A change to the bar\'s menus or the Studio\'s pages comes with its Handbook page.',
  why: 'The menus can\'t be crawled by a quick check, so their Handbook pages (docs/handbook/ui/) say what each control does (J6.4).',
  when: ['bar/web/menu.js', 'studio/*.html', 'studio/*.js', 'studio/*.mjs', 'studio/*.css'],
  check(ctx) {
    const pairs = [[/^bar\/web\/menu\.js$/, 'docs/handbook/ui/menus.md'], [/^studio\/[^/]+\.(html|js|mjs|css)$/, 'docs/handbook/ui/studio.md']];
    const spots = [];
    for (const [re, page] of pairs) {
      const hit = ctx.touched.find((f) => re.test(f));
      if (hit && !ctx.changed.includes(page))
        spots.push({ where: hit, fix: `changed without ${page}: say there what the changed controls do (if no control changed, piece.mjs takes --docs-unchanged "<why>")` });
    }
    return spots;
  },
};
