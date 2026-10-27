// R-PLAN-1 · The plan changes only through its tool.
export default {
  id: 'R-PLAN-1',
  area: 'plan',
  rule: 'PLAN.md changes only together with docs/plan/plan.json (node tools/plan.mjs writes both).',
  why: 'CLAUDE.md: the Plan is edited only with node tools/plan.mjs; PLAN.md is made from plan.json.',
  when: ['PLAN.md', 'docs/plan/plan.json'],
  check(ctx) {
    if (ctx.changed.includes('PLAN.md') && !ctx.changed.includes('docs/plan/plan.json'))
      return [{ where: 'PLAN.md', fix: 'edited by hand: undo it and make the change with node tools/plan.mjs' }];
    return [];
  },
};
