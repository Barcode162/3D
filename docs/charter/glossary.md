# Glossary

One meaning per word, used the same way in every book, in commits and in
replies to the user. A new word gets its line here before it's used. Where
the code or data uses another name for the same thing, it's given in
*italics*.

## Words the user can say

- **"go"**: continue the plan: do the job in Now (or start the next one).
- **"broke J4.3"**: the change made by piece J4.3 broke something; find it,
  fix it or undo it.
- **"undo"**: put the last job's changes back the way they were (its whole
  commit range is in Done).
- **"status"**: a short answer: what's in Now, what's next, what's live.
- **"park: <idea>"**: keep the idea in Parked, no work.
- **"talk"**: conversation only, nothing is changed.
- Anything else, in any words: a request; it goes through intake.

## Everything else, A to Z

- **admin**: a bar account allowed into Dev and the tools; the **owner**
  (the user) is the first admin.
- **archive**: `docs/archive/`: history kept to read; never instructions.
- **Bar3D**: the hangout bar, J Production's first world
  (joshuavelasco.com/bar). **Bar3D classic**: the old public build kept at
  /classic/.
- **books (the five)**: Charter, Playbook, Handbook, Plan, Records: every
  document belongs to exactly one, and only the Playbook gives instructions.
- **build**, **build code**: one version of the game; its code is the first
  7 letters of the commit it was built from (e.g. `87bd110`), shown on
  screen as "build 87bd110".
- **chapter**: one of the five fixed areas every job and idea belongs to:
  the world, the site, the engine, the tools, the backbone.
- **Charter**: `docs/charter/`: what the project is, the user's words, the
  limits, the permissions. Only the user changes it.
- **check**: an automatic test. The **quick check** (build, C++ tests,
  shaders, data and plan checks; minutes, no browser) runs for every piece;
  the **full set** (with the browser tests) runs on GitHub at every deploy.
- **CI**: GitHub's automatic runs (GitHub Actions) that build, test and
  deploy when `main` changes.
- **clock**: the shared time every loop in the world follows, the same for
  every player.
- **commit**: one saved change in the code's history. One piece = one commit.
- **decision**: a choice that holds until changed, with its reasons and
  the options not taken (`docs/records/decisions/`).
- **deploy**: putting a new version on the live servers (CI does it after a
  push to `main`). The game deploy and the site deploy are separate.
- **Dev**: joshuavelasco.com/dev, the control room (owner and admins only):
  Overview, Projects, Plan, Publish, People.
- **Dials**: the tool page showing the world's numbers (light, sound, feel)
  as dials.
- **Done**: the list of finished jobs, newest first, each with its build
  code and commit range.
- **draft**: something saved but not published: a news draft, the Studio's
  draft of the world.
- **Ember**: the house theme (amber, warm, frosted); the other ten themes
  are options.
- **experiment**: a short, time-boxed try to find out whether something is
  possible, before promising it; it changes nothing live.
- **freeze**: the period (J1 to J8) when no features, world, engine or
  language changes are made.
- **gallery**: the `gallery` branch on GitHub: before/after pictures and
  numbers for every piece.
- **Handbook**: `docs/handbook/` (J6): how everything works: every system,
  data field, page, button and command, plus how-to recipes.
- **highlight**: the one news item the user pushes to the front page, from
  Dev → Publish.
- **incident**: the record of one error: what happened, the root cause, the
  fix to the way we work (`docs/records/incidents/`).
- **J Production**: the user's company; Bar3D, Looped and the script
  language are its projects.
- **job**: a numbered unit of work in the plan (`J1`, `J2`, …), made of
  pieces. Numbers are never reused.
- **job branch**: `job/J<n>`, where a job's pieces are saved before the job
  ships.
- **kind**: a world file entry saying what a type of thing is (its model,
  seats, taps, states, loops). Many things share one kind.
- **known issue**: an open problem, written down with the job that fixes
  it (`docs/records/known-issues.md`).
- **lane**: one moving channel of a loop (move, turn, glow, sound…). *The
  world files call it a "piece"; the Studio shows "Lanes".*
- **ledger**: the server that keeps credits and owned things.
- **live**: what people get right now at joshuavelasco.com.
- **loop**: the never-ending animation of a thing's state, timed by the
  clock.
- **Looped**: J Production's engine (C++ → WebAssembly → WebGPU). *Its
  code is in `looped/`, namespace `engine`.*
- **`main`**: the branch the live site is built from.
- **Meridian**: the user's own driving game, opened from the racing rigs
  (uses three.js, an approved exception).
- **news**: posts on joshuavelasco.com/news. Claude writes drafts; the user
  publishes them from Dev → Publish.
- **North star**: the user's five lines (N1–N5) for where the project is
  going (Charter §2).
- **note**: a message the user leaves in Dev → Plan for the next session.
- **order of authority**: which book wins when two disagree: Charter >
  Playbook > Handbook > Plan > Records > Archive.
- **owner**: the user: the first admin, the only one who changes the
  Charter, publishes news and pushes highlights.
- **park**, **Parked**: an idea kept for later, not on the plan yet.
- **part**: an ability a thing has (a model, a seat, a light, a sound, a
  tap area…).
- **permission list**: Charter §5: allowed, ask first, never; anything not
  listed: ask.
- **phone check**: what the user looks at on the phone to confirm a piece
  worked: where to stand, what to see.
- **piece**: one change inside a job (`J4.3`), saved as one commit, with its
  own phone check. (Not a chess piece, not a loop's lane.)
- **place**: a world file listing things and where they sit (the bar, a
  home).
- **Plan**: what's next: the jobs, their order, Parked, Done. Today
  `PLAN.md`; from J4.1 `docs/plan/plan.json`, shown on Dev.
- **Playbook**: `docs/playbook/` (J3): the procedures: what to do in every
  situation. The only place that gives instructions.
- **project**: one of J Production's works (Bar3D, Looped, the script
  language, future ones), each with its own tools in Dev.
- **proposal**: a world change Claude leaves in the Studio for the user to
  preview, then apply or discard.
- **publish**: make something live: a world version from the Studio, a
  theme from the UI Editor, a news post from Dev.
- **Records**: `docs/records/`: what happened: requests, incidents,
  sessions, decisions, known issues. Added to, never rewritten.
- **request**: what the user asked for, kept word for word in the Records.
- **restructure**: jobs J1–J8 (from 2026-10-24) that give the project its
  five books, honest checks, enforced procedures and clean data flows;
  spec: `docs/archive/restructure.md`.
- **room server**: the multiplayer server (on Cloudflare) every player in
  the bar connects to.
- **rule (a thing's)**: a "when → then" line in a kind: what changes a
  thing's state (tapped → in use). Always written "a thing's rule".
- **rule (the user's)**: a decision that always holds, in the user's words
  (Charter §2). Always written "the user's rule".
- **script language**: J Production's own small language for what things
  in the world do (states, rules, loops). Planned, not built.
- **session**: one working conversation with Claude, from its start to its
  end.
- **ship**: finish a job: merge, push `main` once, check it's live, record
  it, reply.
- **spec**: the precise description of a job (goal, scope, pieces,
  acceptance), written by Claude, linked to the user's words.
- **state**: what a thing is doing now (idle, in use, playing…).
- **status**: the one-screen summary of the truth now (Dev, and
  `node tools/project.mjs status` from J4).
- **store**: a server folder that keeps data outside the code: the Studio
  store (the live world), the UI store (themes and screens), Dev's store
  (notes, highlights, the plan copy), the news-and-keys service.
- **Studio**: the tool for building and changing the world on the phone.
- **theme**: a set of colours, fonts and shapes every page can wear.
- **thing**: anything in the world, with a stable id (a stool, a glass, a
  lamp).
- **Toolbelt**: the list of every tool, grouped by project.
- **UI Editor**: the tool for themes and screens.
- **verdict**: the answer to "is this possible?" given before any work:
  doable now · after something else · at a cost (the user decides) · needs
  an experiment · not possible (and the closest thing that is) · against
  a rule.
- **version**: a numbered published state of a store (world v3, UI v0).
- **WebAssembly**: the fast code format browsers run; Looped's C++ is
  compiled to it.
- **WebGPU**: the browser's way to use the phone's graphics chip; Looped
  draws with it.
- **world**: everything in the bar: places, kinds, things, materials,
  light, sound.
- **World notes**: the tool page showing what each thing in the world does,
  its states and rules.
