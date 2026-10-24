# REFERENCE.md's old instructions (archived 2026-10-24)

> **History, not instructions.** These sections of `docs/REFERENCE.md` told
> sessions how to work before the restructure; `docs/playbook/` replaced them
> (J3.7). Kept to read, never to follow.

## Start of every session (the user often starts fresh chats to save tokens)
0. **The plan is `PLAN.md`** (2026-10-24): one doc, eight fixed parts; the
   old plans (PROGRESS, VISION, the Alive plan, LOOPED...) are in
   `docs/archive/`, to read, never to plan from.
1. Read **`PLAN.md`** (North star, your rules, Now) and **`docs/DECISIONS.md`**
   (what was chosen, why, and what failed).
2. The SessionStart hook (`.claude/settings.json` → `tools/setup.sh`)
   installs Emscripten. If it didn't run, run `bash tools/setup.sh`.
3. Don't build or run headless tests until the user says "go" (tokens).

## The hub: the user's board (2026-10-20, v2)
The user plans in **joshuavelasco.com/hub** (`hub/`, server `hub/hub.php`): one
board per project (Bar3D is `bar3d`, the default), a card per piece of the
world, in player-facing sections (Advanced tab: the backend), with stages Idea
→ Waiting for your review → **Ready for Claude** → In progress → Check on phone
→ Done. Each card has the user's own words ("What you want", the `notes`
field: never overwrite), Claude's structured plan, and a conversation (Talk:
questions, answers, reports, messages). Claude reads and updates it with
`node tools/hub.mjs` (needs `HUB_KEY` in the environment; see the file's top).
- **"go"** means: first `node tools/hub.mjs inbox` (what the user wrote since
  you last did: answers, messages, edited "What you want"): fold each into the
  card's plan (`set <id> --plan-file`) and reply with `say <id> "..."` or a new
  `--question`. Then `node tools/hub.mjs queue` and work the Ready cards in that
  order. For each: `report <id> --stage doing` when starting; when deployed,
  `--stage check --build <sha> --report "<what changed + How to check it>"`.
- A decision only the user can make: `--question "<question, with your
  recommendation>" --choices "<rec>|<other>"` on the card (the hub shows it as
  a bubble with one-tap answers), leave it in its stage, go on to the next card.
- **Hub v3 (2026-10-20, docs/LOOPED.md C): no cards on screen.** Home = what
  needs you (questions, reviews, checks as one-tap bubbles) + the **news feed**;
  Explore = the bar's map of **places** (`hub/projects/bar3d-spots.json`:
  world, beyond, engine "under the floor", tools; cards land on a place by
  its `spot` or the rules there; `hub.mjs place <card> <spot>`); the old board
  pages are gone (R6.6), Dev is the control room. `hub.mjs inbox` also lists places the
  user wrote about (answer with `spotsay`).
- **After every deploy, one news post:** `node tools/hub.mjs post --build <sha>
  --title "<plain words>" --text "<what changed, for anyone>" --shots
  build/after/door.png,...` (screenshots of what changed + a hero shot). It's
  private; the user's Publish switch puts it on joshuavelasco.com/news.
- New ideas from the user in chat: `add ideas "<title>"` (not the master list).
- **What goes where** is `docs/HUB_MAP.md`: the sorting questions, one card per
  deliverable, the card format (WHAT / NOW / STEPS / NEEDS / NOT HERE / DETAILS).
  Write only NEEDS by hand; `node tools/hub.mjs links` writes every UNLOCKS line.
  After any board change run `node tools/hub.mjs check` (0 errors before you stop).
  Keep the user's own words in YOUR NOTES; never overwrite them.
- (2026-10-24) The board's cards were carried into `PLAN.md`; the board is
  retired when Dev (R4) is live. Its last snapshot: `docs/archive/hub-board-bar3d.md`.

## The routine (user, 2026-10-19: fewer, deeper passes; save tokens)
- **"start"**: read the docs, show a short menu: the master list + Parked, one
  line each. No builds, no tests. The user picks one.
- **"plan"**: a short plan for the picked item: what changes, what they'll see
  on the phone, open questions. No code yet. The user says "go" or edits it.
- **"go"**: build it in steps (each saved to `work`); test and deploy **once at the end** (not after
  each small step), then the "How to check it worked" list.
- **"park: <idea>"**: one line to `PLAN.md` → Parked, under its chapter. No work, no debate.
- **"talk"**: conversation only, no tools.
- **"wrap up"**: write decisions and progress into the docs, commit, push.
- Never add ideas to the master list on your own; they go to Parked.

## Working with the user
- The user works **only from an iPhone 15 Pro Max (iOS 26, Safari)** through the
  Claude app. They can't run anything locally or open devtools.
- They're a beginner in graphics and C++. Explain in plain language, keep
  replies short, and say what they'll see on the phone after each change.
- After every deploy, give the user a short **"How to check it worked"**
  list: the build code, then 2-4 specific things to look at, each saying
  where to stand or look and what they should see (and what would mean it
  failed). They can't always tell what changed, so be concrete.
- Testing loop: push to `main` (once per "go", see below) → GitHub Actions builds and force-pushes the site
  to the `gh-pages` branch (Pages serves it) → user opens
  **https://joshuavelasco.com/bar/** in Safari (private since 2026-10-20: sign-in,
  invites; github.io is only a signpost, docs/LOOPED.md A). The `build xxxxxxx` label in
  the top-right shows which commit is live, so the user can confirm they have
  the newest version.

## Never lose work, deploy in batches (user, 2026-10-20)
The user works through many hub cards, then tests many systems at once on the
phone. So: **save every step, deploy once per "go"**, and only the systems
that changed get rebuilt (docs/DECISIONS.md → Deploys by system).
- The container is temporary. **Commit after every working step and push it to
  the `work` branch** (`git push -u origin HEAD:work`), without being asked.
  `work` never deploys; it only keeps the work safe. No PRs.
- **At the end of "go"** (every Ready card done, or the user asks): run the
  tests for each system you touched (table below), merge into `main`
  (`git fetch origin main && git merge origin/main`, re-test if it brought
  changes), push `main` **once**. Then every finished card goes to Check with
  that one build code, and the chat gets one combined "How to check it" list,
  grouped by system.
- **What CI rebuilds:** `deploy.yml` compares `main` with the live build and
  skips the game when only the hub, `docs/` or Markdown notes changed;
  `hub.yml` deploys the hub alone (~2 min). Runs queue, they don't cancel.
- **Tests per system** (before the push to `main`):
  hub → `hub_test.mjs` · server PHP → `php -l` + `hub_test.mjs` + `gate_test.mjs` ·
  cloud room/ledger → `proto_test`, `room_test`, `ledger_test`, `mp_test` ·
  game (looped/, bar/, looped/shaders/, bar/web/, assets/) → build + `smoke.mjs` +
  `tools/native/run.sh` + the feature's own test (arcade, rhythm, chess...) ·
  docs only → nothing.
- **Two sessions at once:** each pushes to its own `work-<topic>` branch and
  merges `origin/main` right before its one push to `main`.
- An urgent one-line fix the user is waiting on may go straight to `main`.
- Update `PLAN.md` (through `tools/plan.mjs`) in the same commit whenever a
  step is finished or the plan changes. Record new decisions or gotchas in
  `docs/DECISIONS.md`; new ideas from the user go to `PLAN.md` → Parked. A fresh session only knows what's
  written in the repo.
- **Don't make the user wait (user, 2026-10-21).** The deploy puts the game
  live ~6 minutes after the push (build + room tests + upload); the game's
  long tests run after it's live. So: run every relevant test locally
  first, push, and give the "How to check it" list **right away** (the build
  code on screen says when it's live). Don't sit in chat watching CI: start a
  background watcher on the Actions run, keep working, and if a run goes red,
  fix it at once and tell the user. `check_live.mjs` when it's up.

