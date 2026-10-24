# A job: from spec to live

**When:** "go", or a wish that intake turned into a job.

**Before you start:** the job has a spec (`docs/plan/jobs/J<n>.md`; for
J1–J8, `docs/plan/restructure.md`) and is in Now (`node tools/plan.mjs now`).
If Now's job is finished, start the next one: `node tools/plan.mjs start
J<n> --piece "J<n>.1|<what>|<phone check>" …` with the pieces from its spec.

## Steps

1. **The branch.** `git fetch origin main && git checkout -B job/J<n>
   origin/main` (or, if `origin/job/J<n>` has pieces already, check it out and
   carry on from there).
2. **Each piece, in order** (one commit each): see A piece, below.
3. **Ship** when every piece is ticked: see Ship, below.
4. **Verify live** (never say "live" before this): the build code on the live
   bar matches (`EXPECT_SHA=<sha> node tools/check_live.mjs`, or the bar's
   `config.json`), and Dev's deploy rows show the build. GitHub's tests run
   after the game is live (about an hour): a red result → [failures.md](failures.md)
   → A deploy is red.
5. **The world store** (only if the job changed `assets/world/`): once the
   deploy is live, `node tools/studio.mjs push --note "<pieces>"` (refused if
   the user published since: pull, merge, push again).
6. **Reply** ([reply.md](reply.md) → Done).

## A piece

1. Make the change, inside the job's scope and the freeze (`docs/plan/restructure.md` §7).
2. Run **only the checks covering what changed** and save it, with one
   command:
   ```
   PIECE_BRANCH=job/J<n> node tools/piece.mjs J<n>.<k> "<what changed, in words>" --check "<phone check>" [--no-shots | --pages <page> | --compare "x=0"]
   ```
   It finds the checks for the changed files (`node tools/check.mjs changed`,
   browser tests included when they cover the change), builds only if the
   change needs it, takes before/after pictures (the bar's spots, or
   `--pages` for a site page; `--no-shots` when nothing on screen changes),
   ticks the piece in the plan, commits "J<n>.<k> …", pushes the job branch,
   and records the piece in the gallery.
3. A check fails → nothing is committed; fix it and run again
   ([failures.md](failures.md) → A check fails).
4. The commit message carries the attribution lines of this session
   (taken from the session itself; `PIECE_ATTRIBUTION` overrides them).

## Ship

1. Move the next job into Now first (its pieces from its spec), so Dev shows
   it right after the push: `node tools/plan.mjs start J<n+1> --piece …`.
2. The session line ([session.md](session.md) → End), committed with the
   finish.
3. ```
   node tools/project.mjs ship "<what the job did>" [--news "<friendly lines>"]
   ```
   It refuses unless: on the job's branch, nothing else unsaved, every commit
   belongs to the job, every piece ticked, the plan's files agree. Then it
   runs the checks for everything the job changed, merges `origin/main`
   (stops on a conflict: [failures.md](failures.md) → Conflicts), writes the
   Done line, commits "Notes: J<n> built (…)" with the plan and the session
   line, pushes the job branch and `main` **once** (the only way `main`
   moves: the locks refuse any other push to it), then waits up to 12 min
   for Dev's reports and says exactly what is live and what isn't confirmed.
4. News: only for something people can see or do, in friendly words
   (`--news`); the user publishes it from Dev → Publish. Restructure jobs:
   `--no-news`.

**Done when:** the build is live (checked), the plan shows the job in Done
and the next one in Now, and the user has the reply.

**If something goes wrong:** [failures.md](failures.md); any error →
[incident.md](incident.md).

**Records:** the commits (one per piece), the Done line, the gallery, the
session line.
