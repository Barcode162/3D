# The restructure (J1–J8): the governing spec

**Status:** in progress since 2026-10-24 (approved by the user: "go").
**Baseline:** `main` at `bbe782f` (the last build before the restructure;
build `87bd110` was the last game deploy).
**Progress:** `PLAN.md` → 4. Now (the job being done), 7. Done (jobs
finished), and `git log --grep '^J[1-8]\.'` (one commit per piece).

This file governs until J8 is done (the user's rule of 2026-10-24). Where
any older instruction (`CLAUDE.md`, `docs/REFERENCE.md`, `docs/DECISIONS.md`,
`docs/AUTOMATION.md`, `docs/STUDIO.md`, a tool's header) disagrees with this
file, this file wins, and the older text gets fixed in the job that owns it
(below). It is written to be executed by any Claude session, at any effort
setting, **without the chat it came from**: every piece names what it
produces and how it is checked. When this spec is silent on a detail, take
the safer option, write the choice down in the piece's commit message and
in the job's notes (`docs/records/known-issues.md` if it leaves something
open), and ask the user only if the choice is theirs (Charter → permissions).

---

## 1. Why (what was wrong, verified 2026-10-24)

The project works and is live, but it grew one request at a time; each
addition brought its own instructions, its own copy of data and its own way
of syncing. Verified facts (the full list with owners:
`docs/records/known-issues.md`):

1. **No single source of truth.** At least five files give instructions and
   contradict each other (branch names, testing, what "go" means). Two
   programs parse `PLAN.md` (`tools/plan.mjs`, `dev/plan.js`). Three lists of
   "projects" (`toolbelt/tools.json`, the site's folder scan, the old hub
   boards). `homepage/` is uploaded by two workflows.
2. **Docs describe the past.** E.g. the docs say the repo is public; it is
   private (GitHub API, 2026-10-24). `docs/REFERENCE.md` still says "go"
   works the retired hub board.
3. **The user's words are used as the spec.** Plan items and rules are
   lightly reworded chat. Nothing turns them into an engineering spec
   (files, data, acceptance), so each session re-interprets them.
4. **Nothing enforces the process.** The last session worked on its own
   branch instead of `work-go`; one of its notes never reached `main`.
5. **The safety net is broken.** Since the new light (L1) the browser smoke
   test fails on GitHub's runners (screenshot timeouts, a rotation count),
   so 12 browser suites are skipped on every game deploy; the last four
   step deploys (R3–R6) ended red. Dev's "Last deploy" is always red.
6. **Silent, gated data hops.** Dev's plan is a copy uploaded at the very
   end of the hub workflow, after nine browser tests: a failed or cancelled
   run leaves Dev stale with no warning. Closing a step never moves the next
   item into Now; the Done line waits a whole step for its build code.
7. **Channels that carry nothing.** Every "go" pulls the Studio store, the
   UI Editor store and Dev notes; none has ever carried anything from the
   user (world v1–v3 all by Claude, UI store v0, zero notes).
8. **No reality check on the roadmap.** Realistic items sit next to very
   hard ones with no verdict.

## 2. What the user asked for (summary; word for word in Appendix A)

- One simple workflow: the user says what they want in chat, in any words;
  Claude plans, organises, builds, checks, ships and documents everything.
- Real paperwork: a handbook like Unity's (every system, spec, data field,
  page, button, action), a playbook that can't be misread.
- Self-sufficient: any AI session, from anywhere, can pick up the project
  and operate the engine (and, later, the script language) with no special
  instructions from the user.
- Guardrails: requests are checked against what's possible and allowed;
  nothing happens outside the guidelines; never assume the user knows
  anything.
- Zero errors per session: every error gets its root cause found and the
  way of working changed so it can't recur.
- A procedure for every scenario, adaptable (not one fixed routine).
- Freeze features; the world, light, audio and engine will be remade later,
  item by item, under the new system.
- Never restructure again: the structure must hold and grow.

## 3. Principles (every piece below follows them)

1. **One source per fact.** Everything else links to it or is generated
   from it, with a check that fails when they disagree.
2. **One job per document.** Charter = what's allowed; Playbook = how we
   work (the only place that gives instructions); Handbook = how things work
   (describes, never instructs); Plan = what's next; Records = what
   happened (append-only). The archive is history, never instructions.
3. **Generated over hand-written** wherever code or data already knows the
   answer (tool usage, data fields, checks, controls, folder map).
4. **One pipeline for every change:** intake → spec → pieces → checks →
   ship → verify live → records → reply. Tools enforce it; locks stop
   bypasses, for Claude and for any other AI or person.
5. **Every state visible, with its age.** Dev and the status command show
   the truth and how old each part of it is.
6. **Every failure is loud and recorded,** and every recorded failure
   changes a procedure, tool or check.
7. **The user's words are kept word for word as input;** Claude's specs are
   separate, precise, and link back to them.
8. **Scope before work:** every request gets a feasibility verdict first.
9. **Check a capability before promising it.** Before relying on an outside
   action (a new kind of push, a deletion, a publish, an API), confirm it is
   allowed in this environment, by documentation or by a dry run that
   leaves nothing behind if it fails (incident 2026-10-24-branch-cleanup).
10. **Plain words for the user;** precise words in specs; one glossary.

## 4. The target structure

### 4.1 The five books (final layout, reached by J8)

```
CLAUDE.md            entry for every AI session: who the user is, the order of
                     authority, the first action, links; short (≤ 80 lines)
README.md            the project's front page for people: what it is, the folders,
                     where to read
docs/charter/        THE CHARTER (the user's book)
  charter.md           what the project is, North star and rules word for word
                       (+ what each means in practice), reality limits, the
                       permission list (allowed / ask first / never; unlisted → ask)
  glossary.md          every term, one meaning
docs/playbook/       THE PLAYBOOK (the only instructions)
  README.md            situation → procedure index; the fallback for the unexpected
  session.md intake.md job.md reply.md incident.md failures.md revert.md
  ways.md change.md    (J3 sets the final list)
docs/handbook/       THE HANDBOOK (reference, how-to, explanations)
  README.md map.md tech.md data-map.md checks.md commands.md (generated)
  systems/<chapter>/<system>.md   ui/<page>.md   howto/<recipe>.md
  generated/           data reference etc.; never edited by hand
docs/plan/           THE PLAN
  restructure.md       this spec (until J8; then archived)
  plan.json            the plan's data (J4.1): jobs, pieces, questions, parked
  jobs/J<n>.md         one spec per job
  README.md            the readable plan, generated from plan.json
docs/records/        THE RECORDS (append-only)
  known-issues.md      open problems, each with the job that fixes it
  incidents/<date>-<slug>.md   decisions/<NNNN>-<slug>.md
  requests/<YYYY-MM>.md (the user's words)   sessions/<YYYY-MM>.md
docs/archive/        history only; every file starts with a banner saying so
```

Until J4.1, `PLAN.md` (root) stays the plan and is edited only through
`node tools/plan.mjs`. J4.1 replaces it with `docs/plan/plan.json` and the
generated `docs/plan/README.md`, and removes the root `PLAN.md`.

### 4.2 Order of authority

Charter > Playbook > Handbook > Plan > Records > Archive. If two disagree,
the higher wins and the lower is fixed in the same session (and recorded as
an incident if it caused an error). During J1–J8 this spec sits just under
the Charter.

### 4.3 Numbering

- **Jobs** are `J1, J2, …`, numbered in the order they are created, never
  reused. **Pieces** are `J<n>.<k>`. One piece = one commit whose subject
  starts with its id (`J4.3 The ship command …`), so "broke J4.3" names the
  exact commit (`git log --grep '^J4\.3 '`).
- Old tags (E3.2, S7.1, L1.7, R6.1, W1, EN4 …) stay valid in history. When
  J4.1 converts the road, each old road item becomes a new job with a
  `formerly` field (e.g. `J14 formerly EN4`).
- The tag `L1.8` was used twice (Done `8722f75` and a road item); J4.1's
  conversion keeps both apart through the new numbers.

### 4.4 Branches

- One branch per job: `job/J<n>`, made from `origin/main`. Pieces are
  pushed there (`PIECE_BRANCH=job/J<n>` for `tools/piece.mjs` and
  `tools/step.mjs` until J4's tools take over).
- `main` gets exactly one push per job (the job's finish), except an urgent
  fix (Playbook).
- **This environment refuses tag pushes and branch deletions (HTTP 403,
  verified 2026-10-24).** So: no tags; job branches stay after shipping; the
  12 old side branches stay until the user decides (known issue KI-18).

### 4.5 The user's workflow (what stays simple)

The user says anything, in any words ("go" = continue the plan). Claude:
(1) says in one or two lines what it understood and the verdict (possible,
after X, at a cost, needs an experiment, not possible, against a rule);
(2) does the work through the Playbook; (3) replies with the build code and,
per piece, where to look, what to see, and "if not, say: broke J<n>.<k>".
A question goes to the user only when the decision is theirs: one question,
Claude's recommendation first.

## 5. Decisions (made by Claude under "go"; the user can change any)

- **D1 Five books** as in 4.1, using the user's own words "playbook" and
  "handbook".
- **D2 The plan becomes data** (`docs/plan/plan.json`, J4.1) with one writer
  (the plan tool) and one schema; the readable plan and Dev both come from
  it. Reason: two regex parsers of a Markdown file drifted.
- **D3 North star and rules move to `docs/charter/rules.json`** in J4.1 (the
  source); `charter.md` shows them through a generated block. Until then
  `PLAN.md` parts 1–2 are the source and `charter.md` carries a generated
  copy (`node tools/plan.mjs charter` writes it; the quick check fails if
  it's stale).
- **D4 Code folders stay where they are.** Moving `looped/`, `bar/`,
  `cloud/`, the web folders would risk the live site for no visible gain.
  Only stray top-level files are tidied (J3.6/J6.6), and after J6 a check
  refuses unknown top-level entries.
- **D5 Deploys stay "live first, tests after"** (the user's speed rule,
  2026-10-21), but every result is reported honestly (J2.3).
- **D6 Dev keeps the note box** (read at session start) but it becomes part
  of the status screen, not a separate ritual.
- **D7 The repo is private,** so Dev cannot read GitHub directly; plan data
  reaches Dev through a dedicated, test-free upload (J5.2).
- **D8 No news posts for J1–J8:** there's nothing new for people to see.
  `tools/step.mjs finish … --no-news`.
- **D9 The hub service keeps its address** (`hub/hub.php` still serves news,
  screenshots, Claude's key and the Dials' tuning), but its retired board
  code is removed (J5.4) and the docs call it "the news-and-keys service".
- **D10 Verifying "live" uses our own endpoints** (the bar's `config.json`
  / `version.txt` through `tools/check_live.mjs`, and the deploy status
  files of J2.3), not the GitHub API (tools in the container can't use it).

## 6. The jobs

Each job: one branch, its pieces in order (one commit each), the quick
check per piece (`node tools/check.mjs quick`), a finish that merges main
and pushes `main` once, then the next job moves into Now
(`node tools/plan.mjs start J<n+1> --piece …` with the pieces below)
**before** the finish commit, so Dev shows the new Now right after.

Finishing a job until J4 replaces it:
```
PIECE_BRANCH=job/J<n> PIECE_ATTRIBUTION="<this session's two lines>" \
  node tools/step.mjs finish J<n> "<what>" --no-news
```

### J1 · Freeze and the Charter
- **J1.1** Close R6 (sealed with `87bd110`), the freeze and the other new
  rules in `PLAN.md` → Your rules, J1–J8 on the road with J1 in Now, this
  spec, `docs/records/known-issues.md`, the incidents found while starting,
  the stray note from the last session's branch brought home, and a
  "restructure in progress" pointer at the top of `CLAUDE.md`.
  *Accept:* `node tools/plan.mjs check` ok; Dev's parser reads J1 as Now
  with 3 pieces; every known issue names its fixing job.
- **J1.2** `docs/charter/charter.md`: (a) what J Production and the project
  are; (b) North star and the user's rules, word for word (generated block,
  D3), each rule followed by "In practice:" lines Claude writes and the
  user confirms; (c) reality limits: the phone (iPhone 15 Pro Max, Safari,
  WebGPU on iOS 26, memory, heat, no SharedArrayBuffer), hosting (IONOS
  shared PHP: no long-running processes or websockets; Cloudflare Workers
  and Durable Objects; GitHub Pages only as a signpost), CI (GitHub runners:
  no GPU, software drawing is slow), the cloud machine (no GPU, temporary,
  tag pushes and branch deletions refused), what Claude can't do (see or
  hear the phone, judge speed or feel, make hand-crafted art; models only
  through code and Blender scripts; session limits), money, law (13+;
  teens and adults apart; payments; the 18+ casino with earned credits),
  licences; (d) the permission list: **allowed** inside a job the user asked
  for; **ask first** (with the exact list: deleting anything outside the
  repo, changing North star/rules, adding an outside library, engine or
  asset, spending money, hosting/DNS/secrets, accounts and credits, making
  something public, changing the structure); **never** (secrets in git,
  copyrighted material, impersonation, weakening age separation,
  force-pushing or rewriting `main`, skipping or disabling a test to get
  green, PRs); **anything not listed: ask.** (e) How the Charter changes:
  only the user.
  *Accept:* `node tools/plan.mjs charter --check` ok; the user said yes (or
  their changes are in).
- **J1.3** `docs/charter/glossary.md`: every term used in the books and in
  replies (job, piece, build, ship, live, deploy, check, store, publish,
  draft, thing, kind, place, part, state, loop, rule, clock, room server,
  ledger, Studio, Dev, Dials, UI Editor, theme, screen, gallery, incident,
  known issue, verdict, freeze …), one plain meaning each, alphabetical.
  *Accept:* every term in the Charter's and this spec's headings is defined.

### J2 · Honest checks
- **J2.1** The browser tests run on GitHub again: the tests open the bar
  with the light switch they need (`?light=classic` exists: `bar/test_hooks.*`)
  through one shared place (one env var read by the shared test helpers, set
  in `deploy.yml`), so software drawing is fast enough; the phone keeps the
  full light. *Accept:* the smoke test passes on a GitHub run (link in the
  commit or the known issue); time-box one session: if it still fails, the
  known issue records exactly why.
- **J2.2** Every test counts: one list of checks (`tools/lib/checks.mjs`,
  read by `tools/check.mjs`); `tools/workflows_test.mjs` (in the quick
  check) fails unless every check runs in exactly one workflow (or
  `NOT_ON_CI` says why), every test file is listed (or `NOT_LISTED` says
  why), every test step runs after any failure with a time limit, and a test
  allowed to fail names its open known issue in its step name. Tests that
  passed on GitHub before (run 164, 2026-10-21) count; tests with a known
  problem or never run there are allowed to fail with their known issue
  (KI-24, KI-25) until their GitHub results are in. No test is deleted or
  weakened to get green.
- **J2.3** Both workflows report their real state to Dev's store through
  `tools/ci_status.mjs`: `private/dev/ci-game.json` and
  `private/dev/ci-site.json` = `{workflow, build, live, tests:
  running|passed|failed|cancelled, failed: [step ids], allowed: [step ids
  that failed but may], at, url}`, written at the start, after the upload
  (the game), and at the end (always, from `toJSON(steps)`). `dev/api.php`
  `{"a":"ci"}` returns `{game, site}` (the old `ci.json` reads as the game's
  until the next game deploy); Dev's Overview shows both rows, with their
  age. Stale comments in `deploy.yml` fixed.

### J3 · The Playbook
(2026-10-24: J3.1 added for the user's request #5 (test only what changed, push
only what the user wants); the pieces below are J3.2–J3.7 in `PLAN.md`.)
Every procedure file uses one template: **When** (the trigger) · **Before
you start** · **Steps** (numbered; each names the exact command or file) ·
**Done when** · **If something goes wrong** (→ which procedure) ·
**Records** (what to write down). Commands that arrive in J4 are written as
they will be and marked "(J4)"; J4.6 removes the marks.
- **J3.1** `session.md`: the start (status, stores, notes, Now, the user's
  message → intake) and the end (everything committed and pushed, records
  written, the session line with its error count, the reply).
- **J3.2** `intake.md`: every kind of message (a wish, a change, a
  question, a problem or "broke X", "undo", "status", "park:", a new rule,
  a priority change, "talk", "go", something impossible or against a rule,
  something unclear, a Studio/UI Editor publish, a Dev note, a phone report
  code, "stop"/"pause") → its procedure; recording the request word for
  word; the feasibility check with its six verdicts (doable now · after X ·
  at a cost (the user decides) · needs an experiment (time-boxed) · not
  possible (why, and the closest thing that is) · against a rule (which));
  the capability check (principle 9); the job spec template
  (`docs/plan/jobs/TEMPLATE.md`: Goal, From the user (links), Scope / Not in
  scope, Design, Pieces with phone checks, Acceptance, Risks, Verdict).
- **J3.3** `job.md` and `reply.md`: the lifecycle (spec → pieces → checks →
  pictures → commit → ship → verify live → records → reply) and the reply
  formats (understood + verdict; done: build code + per piece where to look
  / what to see / "if not, say: broke …"; one question with a
  recommendation; an error: what broke, the impact, what was done, the fix
  to the system).
- **J3.4** `incident.md`, `failures.md`, `revert.md`: the zero-error loop
  (stop, contain, record, root cause, fix the system, prove the fix); every
  failure scenario (a check fails, CI red, deploy failed, live broken or
  down, a store conflict, a merge conflict, a tool crash, a missing key,
  blocked network, a full disk, an interrupted session and how to resume,
  two sessions at once, docs ≠ reality, a leaked secret, data at risk) and
  **the unexpected** (stop → what is at risk → contain → is a procedure
  covering it? → if not: safest option, ask if it's the user's call, then
  write the missing procedure); "broke J<n>.<k>" and "undo".
- **J3.5** `ways.md` and `change.md`: ways of working (a normal job, an
  urgent fix, an experiment, content through the Studio or the UI Editor, a
  multi-session job, talk only, an audit) and how the structure itself may
  change (a decision record the user approves; adding a tool, system, page,
  procedure or project: where it goes, which checks cover it).
- **J3.6** One voice: `CLAUDE.md` rewritten as the short entry (4.1, 4.2,
  first action, the Charter's "never" list as a generated block); every
  instruction section of `docs/REFERENCE.md`, `docs/DECISIONS.md`,
  `docs/AUTOMATION.md`, `docs/STUDIO.md` and the tool headers either moved
  into the Playbook or marked "history" (their facts move to the Handbook in
  J6); `NEXT.md` deleted. *Accept:* `grep` finds no instruction to use
  `work-go`, the hub board, `NEXT.md` or "start/plan/wrap up" outside
  `docs/archive/` and this spec.

### J4 · Tools and locks
- **J4.1** (Built 2026-10-24 with two choices: `PLAN.md` stays at the root as
  the generated readable plan, since everything already points there; road
  items keep their ids until J7.1 gives each its verdict and its J number.)
  The plan as data: `docs/plan/plan.json` (schema in
  `docs/plan/plan.schema.json`: chapters with their "now" lines; jobs
  `{id, title, chapter, serves, status: proposed|planned|now|live|done|parked|dropped,
  verdict, formerly, requests, spec, pieces:[{id,title,check,status,commit}],
  ship:{build,range,date}}`; questions `{id, text, options, recommended,
  asked, answer}`; `next` order), `docs/charter/rules.json` (D3), the plan
  tool rewritten over them (same commands + `job`, `question`, `seal`), the
  readable `docs/plan/README.md` generated, every road and parked item and
  every Done line converted without loss (a test compares counts and text),
  `dev/plan.js` reading `plan.json` (same piece, so Dev never breaks), root
  `PLAN.md` removed, all references updated.
- **J4.2** One front door: `node tools/project.mjs <command>` with a
  registry (`tools/lib/commands.mjs`: name, one-line summary, usage, module)
  and `help`; `status` prints the whole truth on one screen, each part with
  its age and a 5-second network limit: git (branch, ahead/behind, dirty),
  the job in Now and its pieces, open questions, unread Dev notes, news
  drafts, the live build, both deploys (J2.3), the world and UI stores vs
  the repo, open incidents and known issues, the docs check, the freeze.
- **J4.3** `project.mjs ship`: refuses unless on `job/J<n>` for the job in
  Now, the tree is clean, every commit since `main` starts with one of the
  job's piece ids (or `J<n> ` for the finish), the quick check passed on
  this exact HEAD (recorded by `check.mjs`), the plan and docs checks pass;
  then merges `origin/main` (stops on a conflict), re-checks if the merge
  brought changes, pushes `main`, writes the Done record, waits for the
  deploy reports (J2.3) and the live build (D10), and prints the user's
  reply. Replaces `tools/step.mjs finish` (kept as a thin alias). The commit
  attribution comes from the session (`CLAUDE_CODE_REMOTE_SESSION_ID`), not
  a hard-coded line.
- **J4.4** Records tools: `project.mjs request` (word for word, with date
  and session), `incident new`, `decision new`, `session end` (the session
  line: asks, pieces, ships, errors with links), templates in
  `docs/records/*/TEMPLATE.md`.
- **J4.5** Locks: git hooks in `tools/githooks/` (installed by
  `tools/setup.sh` through `core.hooksPath`): pre-commit (plan and docs
  checks, generated files unchanged by hand, no secret patterns), pre-push
  (no push to `main` except from `ship`, never a forced push to `main`);
  Claude Code hooks in `.claude/settings.json` (use the `update-config`
  skill): SessionStart adds `project.mjs status --brief` to the session's
  context; PreToolUse refuses `git push` to `main` outside `ship` and edits
  of generated files; Stop warns about uncommitted or unpushed work.
- **J4.6** The Playbook and the tools agree: every command the Playbook
  names exists in the registry, every registry command is in the Playbook
  or the Handbook; "(J4)" marks removed. Runs in the quick check.

### J5 · Data flows and Dev
- **J5.1** `docs/handbook/data-map.md`: every store (the repo; the Studio
  store `private/studio` and its live/built-in world semantics: the live
  game and the room read the store, the repo is the built-in fallback and
  Claude's working copy; the UI store `private/ui`; Dev's store
  `private/dev`; the news-and-keys service `private/hub`; accounts; the
  room server; the ledger; phone reports; the `gallery` branch; the phone's
  local saves): who writes, who reads, how it syncs, what wins on a
  conflict, how staleness shows. Decides each user channel's role (KI-17).
- **J5.2** The plan reaches Dev within about a minute: a new workflow
  `.github/workflows/plan.yml` (on `docs/plan/**`, `docs/charter/**`,
  `docs/records/**`; no tests; uploads the plan data, rules, and an
  incidents index with `{sha, at}`); the `PLAN.md` upload leaves `hub.yml`;
  Dev shows "as of <sha>, N min ago".
- **J5.3** Exactly one deployer per folder: `docs/handbook/deploy-map.json`
  (path → workflow → target, or "not deployed") and a check that every
  tracked file matches exactly one entry and that the workflows agree;
  `deploy.yml` stops uploading `homepage/`.
- **J5.4** The retired board out of `hub/hub.php`, `tools/hub.mjs`,
  `tools/hub_test.mjs` (cards, sections, boards/projects, places/spots,
  seed); news, screenshots, the key and the Dials' tuning stay; server data
  is left untouched.
- **J5.5** One list of projects: `toolbelt/tools.json` → `projects` is the
  source; Dev and the site's projects page both read it.
- **J5.6** Dev shows the truth: Overview = the job in Now and its next
  piece, both deploys, questions waiting for the user, news drafts, errors
  this week; Plan = jobs from `plan.json` (tap one for its spec), parked,
  done; the note box stays.

### J6 · The Handbook
- **J6.1** `map.md` (every folder: what, language, deployed by, URL) and
  `tech.md` (every language and outside tool: where, why, ours or not,
  runtime or build-time).
- **J6.2** System pages, one template (What it is · Status: live / partial
  / planned · What you can do with it · How it works · Data · Commands ·
  Limits · Checks · Known traps · History), about 40, under
  `systems/<chapter>/` (engine: frame and core, world model, rendering,
  light, materials, post, audio, physics, driving, skeleton and animation,
  input, net codec and protocol, loading and jobs, profiler and phone
  reports, UI kit, text, the script language (planned); world: the bar and
  its rooms, darts, chess, piano, 4K Mania, racing and Meridian, arcade,
  bartending, jukebox and music, avatars, voice, menus and HUD, seats and
  taps; site: pages, news, highlights, projects, sign-in and portal;
  tools: Dev, Studio, Dials, World notes, UI Editor and its pages, Toolbelt,
  command-line tools, gallery, checks; backbone: hosting, accounts and the
  door, room server, ledger, stores and keys, CI and deploys, classic).
- **J6.3** Data reference generated from the files (`assets/world/schema.json`
  and the world files, theme tokens, screen parts, `tools.json`,
  `config.json`, the room protocol) into `generated/`; plus the world
  inventory: every thing in `assets/world/places/*.json`, which model file
  holds it today (flags things baked into room-sized models), and what is
  not placed.
- **J6.4** Every page and control: `ui/<page>.md` with a table whose first
  column is the control's exact label; a CI crawl compares the labels of
  pages a test can open (Dev `?demo`, UI Editor, Dials, the site, Toolbelt)
  with the tables; for the Studio and the bar's menus, the piece tool
  refuses a commit that changes the page without its handbook page (or
  `--docs-unchanged "<why>"`). `commands.md` generated from the registry.
- **J6.5** How-to recipes (about 25) mapping common requests to exact steps:
  move/add/remove a thing, change a light or a mood, a material, a sound, a
  thing's loop or rule, a new kind, a new place, publish to the Studio
  store, a new page, a theme, a screen, a game, a test, a tool, a check, a
  phone report, an urgent fix, a revert …
- **J6.6** The old notes sorted: every section of `docs/DECISIONS.md`,
  `docs/REFERENCE.md`, `docs/AUTOMATION.md`, `docs/STUDIO.md`,
  `docs/ENGINE.md`, `docs/PROTOCOL.md`, `docs/manual/*`, `docs/audit/*`
  goes to its Handbook page (still true) or to `docs/archive/` (history,
  with a banner); `docs/DRIVING_REWORK.md` becomes the spec of its future
  job; the decisions still in force become `docs/records/decisions/`;
  duplicate images resolved (KI-19); a docs check (links resolve, generated
  files current, no references to missing files) joins the quick check.

### J7 · The plan rebuilt with a reality check
- **J7.1** Every job converted from the old road and Parked gets a verdict
  (3.8 / J3.2's six) with a one-line reason tied to the Charter's limits.
- **J7.2** The world remake as proposed jobs: every object a real, separate
  model (today many are baked into room-sized models), placement, light,
  sound, in a sensible order.
- **J7.3** The script language's realistic scope
  (`systems/engine/script-language.md`, status planned): a small, safe
  language for what things in the world do (states, rules, loops, events),
  designed for AI and people to read and write, every error with a line
  number and a fix hint; not a replacement for C++/JS. Its first jobs.
- **J7.4** The order: Claude's recommendation with reasons; the user
  approves or changes it (one question).

### J8 · Proof, then unfreeze
- **J8.1** Cold-start drills: about 20 realistic requests in the user's
  style (a wish, a bug, "undo", "status", an impossible ask, a rule, a
  Studio publish, a red deploy …), each given to a fresh AI session (a
  subagent; approved by the user's "go" on 2026-10-24) with only the repo:
  it must name the right procedure, verdict, files and commands without
  breaking a permission. The answer key is written first
  (`docs/records/drills/<date>.md`); every miss fixes the books; rerun until
  all pass.
- **J8.2** Final audit: docs check green, every flow in the data map
  verified live, Dev matches the repo, no instruction outside the Playbook,
  no open drift; this spec archived.
- **J8.3** Unfreeze (a rule line) and the first real job runs through the
  new system end to end.

## 7. The freeze (until J8.3)

Allowed to change: `docs/**`, `tools/**`, `.github/workflows/**`,
`.claude/**`, `CLAUDE.md`, `README.md`, `NEXT.md` (deletion), `PLAN.md`
(through the plan tool), `dev/**`, `hub/**` (J5.4), `homepage/projects/`
(J5.5), `toolbelt/tools.json`, test-only switches in `bar/test_hooks.*`
(J2.1). Not allowed without the user: anything else in `looped/`, `bar/`,
`cloud/`, `assets/`, `studio/`, `ui/`, `server/`. An urgent fix for
something live that broke is allowed (Playbook → urgent fix; before J3:
the smallest change, its own commit, recorded as an incident).

## 8. Facts about this environment (verified 2026-10-24)

- The repo `Barcode162/3D` is **private**.
- From the cloud session: branch pushes work; **tag pushes and branch
  deletions are refused** (HTTP 403 from the session's git proxy).
- No `gh` CLI for Claude; the GitHub tools (MCP) read runs and logs.
- `HUB_KEY` is set (Claude's key for the Dev, Studio, UI and hub stores).
- No graphics card here or on GitHub's runners; the browser tests are slow
  here (20–30 min each) and must not be run here (user rule 2026-10-23).
- Smoke on GitHub, last runs: R5 `page.screenshot` timeout (90 s), R6
  "rotation should resize exactly once each way"; 12 suites skipped after.
- Stores: world live v3 (Claude), repo matches; UI store v0; Dev notes: 0;
  news: 12 posts, 6 drafts (the user's to publish).

## 9. Risks

- **J2 may not get every suite green on GitHub's slow machines.** Then the
  honest result is a known issue per suite with the reason; never a fake
  green.
- **J4.1 touches what Dev reads.** The reader switches in the same piece;
  `tools/dev_test.mjs` and a parse of the real plan must pass first.
- **J6 is large.** It may take several sessions; every piece is committed
  and pushed as it's done, so nothing is lost when a session ends.
- **Session limits.** Any session can continue from `PLAN.md` → Now, this
  spec and the job branch; the first action is always the status.

---

## Appendix A: the user's words (2026-10-24, word for word)

> Okay I built a shit ton but, instead of it feeling like a real project it
> seems like Claude uses a lot of my actual words to work and there's a lot
> of non correct instructions or multiple instructions going and pointing at
> this and that, we need to rework every piece data flowing. It works its
> live everything is functional but there's a definite sort of problem. When
> I want to work on things for example the plan in the Dev page didn't
> update and I just overall want to talk about it. Only speak after
> understanding the whole thing

> Go ahead and do this. I want to pinpoint specifically this. We have things
> that work already but aren't structurally like a full project would
> because we worked day after day adding this and that one at a time while I
> assume normal deving isn't like that, that's because I have ai working
> with my brain and that's it. Because it's a huge project and all I was
> thinking let's make an. Engine that can have the actual properties of a
> game engine and an interface and everything but instead of having to
> manually do anything at all, I'd have you Claude execute everything which
> is why I wanted to make myself a full programming language for this engine
> so everything is easy and I also wanted to really have everything
> documented properly but documents that contain what it is how every single
> thing is supposed to be used and all of the specs and every piece of data,
> so that we can have a real handbook for this engine sort of like unity so
> that I can instead use this chat and from here use the full engine just by
> knowing how it works and asking you for things. The point of the tools was
> to make it easier and the point of the plan and all that was so that we
> can have a real playbook that isn't just some random thing that can get
> misinterpreted. I think we would have to think about making everything
> fully self sufficient so that me and my ai chats can come from anywhere
> and start using this no problem ofmissinterpretation. Maybe we should sort
> of freeze everything and keep it as a more raw structure of a plan and the
> actual world and everything will get fully remade every item right now
> isn't a real 3d object but a full room and there's a lot of things that
> aren't placed and everything from the lighting from the engine audio and
> everything else so we just have to mainly it's sort of like I'm making
> something that is continually getting upgraded. I want to have this simple
> absolute simple workflow. I go on this chat and tell you what I want you
> can figure everything else out like planning, and managing and organizing
> and everything the way it's most efficient for you and for the new
> language and just overall it's a big project I need to have managed by
> you, so right now the full structure uses a bunch of different languages
> and maybe some already existing underlying engines and such. I want to be
> able to officially on paper say I have something real not just a small
> thing I can barely navigate through a website that works somewhat but most
> of the dev side and stuff is still a bunch of gibberish when looking at it
> from my point of view. I just want to keep working on my project but
> really have organization and genuine paperwork because when you have that
> and each button is explained and every action possible is on paper you
> can't go wrong since you deal with language perfectly and words so let's
> have some real, boring but real instructions we can get actual results
> just from me asking for things. Don't assume I ever know anything and
> always make sure what I'm asking is within the scope of what's possible.
> We need to make sure we aren't delusional and we have guidelines that tell
> us what is okay and not. Nothing has the right to happen if it's not apart
> of that, and another thing is I want to be able to have ai actually build
> and work with my thing with a margin of 0 errors per session of there is a
> single one for any reason we figure out the root and update how we go
> about things. Everything we do should be as expected anything not expected
> should have a way to actually take care of it aka we have a full procedure
> every time BUT a procedure doesn't mean it's the same always, we should
> have any type of scenarios and any way of working, I am genuinely
> extremely adamant about proper work in going to be using Ultracode for
> this part. Make sure it's perfect. Think about everything get it planned
> and let me know when you're ready to start and what you plan to do because
> it's an important job and I don't want to mess with restructure ever
> again and yes I want ai to be able to operate my engine and language by
> itself without me saying anything like any special instructions that's
> not my job

> Go remember I'm using opus 5.5 high for the execution

What "go" approved (Claude's plan reply, 2026-10-24): the plan (J1–J8 as
above); fresh AI helpers testing the books in J8; saving the 12 old side
branches as tags and deleting them (not possible here: see 4.4 and KI-18).
