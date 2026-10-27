# A job: from spec to live

**When:** "go", or a wish that intake turned into a job.

**Before you start:** the job has a spec (`docs/plan/jobs/J<n>.md`; for
J1–J8, `docs/archive/restructure.md`) and is in Now (`node tools/plan.mjs now`).
If Now's job is finished, start the next one: `node tools/plan.mjs start
J<n> --piece "J<n>.1|<what>|<phone check>" …` with the pieces from its spec.

## Steps

0. **Research first** (rule "Research, then build"): before the first code
   change, add a **Research** section to the job's plan: everything the job
   touches, found by searching the whole repo (engine, bar, tools, builds,
   workflows, assets) for every name it will change; how it works now (read
   the code); how established engines solve it; the risks and the earlier
   incidents (`docs/records/incidents/`) that apply; the order of the work.
   Commit it as `Notes: J<n> research`, tell the user in plain words, and
   build on their go.
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
5. **The world** ships with the game: the Studio is parked (J114, request
   2026-10-27 #3), the bar plays the world built in (`assets/world/`), so a
   world change needs nothing after the deploy. (The store's live world only
   with the page option `?liveworld=1`; no `studio.mjs push`.)
6. **Reply** ([reply.md](reply.md) → Done).

## A piece

1. Make the change, inside the job's scope and the freeze (`docs/archive/restructure.md` §7).
2. Save it **without tests or pictures** (the user tests on the phone:
   rule "The user tests"), with one command:
   ```
   PIECE_BRANCH=job/J<n> node tools/piece.mjs J<n>.<k> "<what changed, in words>" --check "<phone check>" --test none --no-shots
   ```
   It runs the preflight checklist first (`node tools/preflight.mjs`: a
   broken rule names the rule, the spot and the fix; fix it, or add a rule
   when a miss had no rule yet), then finds the checks for the changed files (`node tools/check.mjs changed`,
   browser tests included when they cover the change), builds only if the
   change needs it, takes before/after pictures (the bar's spots, or
   `--pages` for a site page; `--no-shots` when nothing on screen changes),
   ticks the piece in the plan, commits "J<n>.<k> …", pushes the job branch,
   and records the piece in the gallery.
3. A change to a page's controls (a button, a tab, a field) changes its
   Handbook page in the same piece (`docs/handbook/ui/`, the control's exact
   label first): `tools/ui_labels_test.mjs` compares the pages a test can
   open; for the Studio and the bar's menus the piece tool refuses the
   commit without their page (or `--docs-unchanged "<why>"`).
4. A check fails → nothing is committed; fix it and run again
   ([failures.md](failures.md) → A check fails).
5. The commit message carries the attribution lines of this session
   (taken from the session itself; `PIECE_ATTRIBUTION` overrides them).

## Ship

1. Move the next job into Now first (its pieces from its spec), so Dev shows
   it right after the push: `node tools/plan.mjs start J<n+1> --piece …`.
2. The session line ([session.md](session.md) → End), committed with the
   finish.
3. ```
   node tools/project.mjs ship "<what the job did>" --skip-check "the user tests" [--news "<friendly lines>"]
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
