# Known issues

Open problems, each with the job that fixes it. A line is closed by the
piece that fixes it (its id goes in the line, then the line moves to the
bottom section). Found 2026-10-24 unless dated otherwise. The restructure's
spec: `docs/plan/restructure.md`.

## Open

| Id | Problem | Fixed by |
|---|---|---|
| KI-1 | Five files give instructions and contradict each other: `CLAUDE.md` (branch `work-go`, quick check only), `docs/REFERENCE.md` ("go" = hub inbox and cards, branch `work`, run every test locally first, "Runs queue, they don't cancel"), `docs/DECISIONS.md` ("NEXT.md is all a session reads", "keep PROGRESS.md current", "push straight to main after every step"), `docs/AUTOMATION.md`, `docs/STUDIO.md` ("this file is the plan", `NEXT.md`). | J3.6, J6.6 |
| KI-2 | Dev's plan is a copy uploaded at the very end of `hub.yml`, after nine browser tests: a failed or cancelled run leaves Dev stale (Dev showed R5 for 2 h on 2026-10-24). | J5.2 |
| KI-3 | Finishing a step never moves the next item into Now, and the Done line waits for the next step to get its build code. (Stopgap 2026-10-24: `plan.mjs seal`.) | J4.1, J4.3 |
| KI-4 | `PLAN.md` → 3. The chapters: the "Now" lines are stale ("R5 replaces them", "Dev (R4) next"); the plan tool has no command to update them. | J4.1 |
| KI-5 | The browser smoke test fails on GitHub since the new light (R5: screenshot timeout 90 s; R6: "rotation should resize exactly once each way"), so 12 browser suites are skipped at every game deploy; R3–R6 deploys all ended red. | J2.1 (the tests open the bar with `?light=classic` on GitHub: `TEST_LIGHT`; closes when a GitHub run passes the smoke test) |
| KI-9 | `homepage/` is uploaded by both `deploy.yml` and `hub.yml`. | J5.3 |
| KI-10 | Two separate parsers of `PLAN.md` (`tools/plan.mjs`, `dev/plan.js`). | J4.1 |
| KI-11 | Three lists of projects: `toolbelt/tools.json` (Dev), the site's hand-written list + folder scan (`homepage/site/layout.php` `site_projects()`), the old hub boards (`hub/hub.php`). | J5.5 |
| KI-12 | `hub/hub.php`, `tools/hub.mjs`, `tools/hub_test.mjs` still carry the retired board (cards, sections, projects, places/spots, `hub/seed.json`, `hub/projects/*`). | J5.4 |
| KI-13 | The docs say the repo is public (`docs/DECISIONS.md` → Hosting; `docs/REFERENCE.md` → "this public repo"); it is private (GitHub API, 2026-10-24). | J6.6 |
| KI-14 | The last session (R3–R6) worked on its own branch (`claude/hub-consolidation-ui-plan-bi2jz8`), not `work-go`; one of its notes (a `docs/DECISIONS.md` line) never reached `main` (brought home in J1.1). Nothing stops a session from using any branch. | J4.3, J4.5 |
| KI-15 | `tools/piece.mjs` and `tools/step.mjs` hard-code an old session's attribution lines (overridable with `PIECE_ATTRIBUTION`). | J4.3 |
| KI-16 | Stale pointers: `tools/step.mjs`'s header says it writes `NEXT.md`; `NEXT.md` is a stub; `docs/manual/README.md` points to `docs/PLAN.md`. | J3.6, J6.6 |
| KI-17 | Three channels are pulled at every "go" but have never carried anything from the user: the Studio store (world v1–v3 all published by Claude), the UI Editor store (v0), Dev notes (none). Their role is undecided. | J5.1 |
| KI-19 | Duplicate pictures: `homepage/site/img/jukebox.jpg` = `ui/system/img/jukebox.jpg`, `homepage/site/img/counter.jpg` = `ui/system/img/door.jpg`. | J6.6 |
| KI-20 | The roadmap has no feasibility verdicts; tag `L1.8` is used twice (Done `8722f75` and a road item). | J4.1, J7.1 |
| KI-21 | `docs/audit/*.json` (seven large audits of old paths like `app/main.cpp`, `engine/`): status unknown, probably history. | J6.6 |
| KI-22 | The world's two copies (the Studio store read live by the game and the room; the repo's built-in fallback) are not documented anywhere as one rule. | J5.1 |
| KI-23 | `tools/piece.mjs` and `tools/step.mjs` push to `work-go` by default; `work-go` has been stale since 2026-10-23. | J4.3 |
| KI-24 | The Studio test failed on GitHub on 2026-10-22 (run 179, before the new light) at one check, "S3.4 reopening the Studio: your draft is still there (undefined vs -2)", after a 4-minute wait; every run since stopped before it (KI-5), so its state today is unknown. It may be a real bug (a draft lost when the Studio reopens) or slow-machine timing. Allowed to fail on GitHub until settled. | J2 follow-up (the first J2 run), then a fix |
| KI-25 | Three browser tests have never run on GitHub: touching things, travel between places, voice chat. Added in J2.2 and allowed to fail until their first GitHub results are in. | J2 follow-up (the first J2 run) |
| KI-27 | `tools/account_test.mjs` was built for the old multiplayer relay (`server/relay.php`, retired 2026-10-18); no list runs it (`tools/lib/checks.mjs` → `NOT_LISTED`). Sign-up and profiles have no browser test on GitHub (`hub_test` and `gate_test` cover `account.php`'s API). | J7.1 (a job to rewrite it for the room server) |
| KI-28 | GitHub's tests use the classic light (J2.1), so the new light is covered only by the shaders' compile check (`wgsl`), the piece pictures (taken here with the new light) and the phone. A GitHub machine with a graphics card would cover it, at a cost: the user's call. | J7.1 (verdict; the user decides) |

## Closed

| Id | Problem | Closed by |
|---|---|---|
| KI-7 | Dev's "Last deploy" only reports the game workflow, and it is always red (KI-5), so it says nothing; the hub workflow reports nothing. | J2.3: both workflows report their real state (`tools/ci_status.mjs` → `private/dev/ci-game.json`, `ci-site.json`): started, live, then every failed test by name and the ones allowed to fail; Dev → Overview shows two rows with their age. |
| KI-8 | `deploy.yml` comments are stale ("a failing smoke blocks the deploy"; the smoke runs after the upload). | J2.2/J2.3: the test section's comments rewritten (tests run after the upload, every test runs after any failure, each with a time limit). |
| KI-6 | Seven browser suites were allowed to fail in silence. | J2.2: each test now either counts or names its open known issue in its step name ("allowed to fail: KI-…"), checked by `tools/workflows_test.mjs`; J2.3 shows every failure in Dev. |
| KI-26 | GitHub never ran the albedo, shader, plan, touch, travel and voice checks; one failing step skipped every later test (R6's run skipped 12); no test had a time limit (the Studio test once took 31 min to fail). | J2.2: one list of checks (`tools/lib/checks.mjs`), every check in exactly one workflow (`tools/workflows_test.mjs`, in the quick check), every test runs after any failure, each with a time limit. |
| KI-18 | 12 stale side branches and `tmp-capability-check`; this environment can't save tags or delete branches. | The user's decision, 2026-10-24 #4: leave them. |
