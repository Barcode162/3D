// R-SAFE-2 · No test loses a check.
export default {
  id: 'R-SAFE-2',
  area: 'safety',
  rule: 'A test file keeps at least as many checks as it had on main.',
  why: 'Charter: never skip, disable, weaken or delete a test to make a check green.',
  when: ['tools/**test*.mjs', 'tools/**test*.cpp', 'tools/native/**'],
  check(ctx) {
    const spots = [];
    const count = (t) => (t.match(/\bcheck\(/g) || []).length;
    for (const f of ctx.touched) {
      const before = ctx.git(`git show origin/main:${f}`);
      if (!before) continue;  // a new test
      const now = ctx.exists(f) ? count(ctx.read(f)) : 0;
      if (now < count(before)) spots.push({ where: f, fix: `${count(before) - now} check(s) fewer than on main: put them back (a test changed on purpose: replace each with one at least as strict, and say so to the user)` });
    }
    return spots;
  },
};
