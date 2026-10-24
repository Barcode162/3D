// The locks (restructure J4.5) refuse what they should and let the rest
// through: Claude Code's guard (tools/lib/guard.mjs) and the git hooks
// (tools/githooks/), fed made-up calls. Nothing is pushed or committed.
// Usage: node tools/locks_test.mjs
import { spawnSync, execSync } from 'node:child_process';
import { readFileSync } from 'node:fs';

let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const guard = (call) => spawnSync('node', ['tools/lib/guard.mjs'], { input: typeof call === 'string' ? call : JSON.stringify(call), encoding: 'utf8', env: { ...process.env, CLAUDE_PROJECT_DIR: process.cwd() } }).status;
const bash = (command) => guard({ tool_name: 'Bash', tool_input: { command } });
const edit = (file) => guard({ tool_name: 'Edit', tool_input: { file_path: `${process.cwd()}/${file}` } });

for (const c of ['git push -u origin HEAD:job/J4', 'git fetch origin main && git merge origin/main', 'node tools/project.mjs ship "x"',
  'git log main', 'git config core.hooksPath tools/githooks', 'echo git push origin main', "cat > f <<'X'\nconst a = ['git push --force'];\nX"])
  check(bash(c) === 0, `guard lets through: ${c.split('\n')[0]}`);
for (const c of ['git push origin HEAD:main', 'cd x && git push origin main', 'X=1 git -C . push origin main', 'git push --force origin HEAD:job/J4',
  'git push origin +HEAD:job/J4', 'git push origin :job/J3', 'git push --delete origin job/J3', 'git commit --no-verify -m x', 'git config core.hooksPath /dev/null'])
  check(bash(c) === 2, `guard refuses: ${c}`);
for (const f of ['PLAN.md', 'docs/plan/plan.json', 'docs/charter/rules.json', 'ui/themes.css']) check(edit(f) === 2, `guard refuses a hand edit of ${f}`);
check(edit('tools/ship.mjs') === 0, 'guard lets through an edit of an ordinary file');
check(guard('not json') === 0, 'guard lets a call it cannot read through (the git hooks still hold)');

const settings = JSON.parse(readFileSync('.claude/settings.json', 'utf8'));
check(settings.hooks.PreToolUse.some((h) => /Bash/.test(h.matcher) && /Edit/.test(h.matcher) && h.hooks.some((x) => /tools\/lib\/guard\.mjs/.test(x.command))), '.claude/settings.json runs the guard before Bash and edits');
check(/core\.hooksPath tools\/githooks/.test(readFileSync('tools/setup.sh', 'utf8')), 'tools/setup.sh turns the git hooks on');

const z = '0'.repeat(40), head = execSync('git rev-parse HEAD', { encoding: 'utf8' }).trim();
const parent = execSync('git rev-parse HEAD~1', { encoding: 'utf8' }).trim();
const push = (line, ship) => spawnSync('bash', ['tools/githooks/pre-push'], { input: line + '\n', encoding: 'utf8', env: { ...process.env, SHIP_OK: ship ? '1' : '' } }).status;
check(push(`refs/heads/x ${head} refs/heads/job/J9 ${z}`) === 0, 'pre-push lets a job branch through');
check(push(`refs/heads/x ${head} refs/heads/main ${parent}`) === 1, 'pre-push refuses main without the ship');
check(push(`refs/heads/x ${head} refs/heads/main ${parent}`, true) === 0, 'pre-push lets the ship move main forward');
check(push(`refs/heads/x ${parent} refs/heads/main ${head}`, true) === 1, 'pre-push refuses rewriting main, even from the ship');
check(push(`(delete) ${z} refs/heads/main ${head}`, true) === 1, 'pre-push refuses deleting main');
// The freeze's lock (J7.1): only the files restructure §7 allows, while the rule stands.
const { outside, frozenOn } = await import('./lib/freeze.mjs');
check(outside(['docs/plan/plan.json', 'tools/plan.mjs', 'dev/plan.js', 'PLAN.md', 'bar/test_hooks.cpp', '.github/workflows/deploy.yml'], 'J7.1 x').length === 0, 'the freeze lets docs, tools, Dev, the plan, test switches and workflows through');
check(JSON.stringify(outside(['docs/plan/plan.json', 'looped/drive/tire.h', 'bar/projects/racing/cars.h', 'assets/world/x.json'], 'J7.1 x')) === JSON.stringify(['looped/drive/tire.h', 'bar/projects/racing/cars.h', 'assets/world/x.json']), 'the freeze refuses the engine, the bar and the world');
check(outside(['assets/world/x.json'], 'World: from the studio v4').length === 0 && outside(['ui/system/x.json'], 'UI: from the editor v1').length === 0, 'the user\'s own store changes (World:, UI:) pass');
const rulesNow = JSON.parse(readFileSync('docs/charter/rules.json', 'utf8'));
check(frozenOn(rulesNow) === rulesNow.rules.some((r) => r.text.startsWith('**Freeze:')) && !frozenOn({ rules: [{ text: '**Other:** x' }] }), 'the lock is on exactly while the Freeze rule stands');
check(/freeze\.mjs/.test(readFileSync('tools/githooks/commit-msg', 'utf8')), 'the commit-msg hook runs the freeze\'s lock');
const hook = readFileSync('tools/githooks/pre-commit', 'utf8');
check(/plan\.mjs check/.test(hook) && /PRIVATE KEY/.test(hook) && /HUB_KEY/.test(hook), 'pre-commit checks the plan, secret patterns and the HUB_KEY value');
console.log(failed ? `LOCKS TEST: ${failed} FAILED` : 'LOCKS TEST PASSED');
process.exit(failed ? 1 : 0);
