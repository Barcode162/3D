# Bar3D: guide for Claude sessions (short version, 2026-10-21)

A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**: a VRChat-style 3D
hangout bar. Engine in `looped/`, the bar in `bar/`, page in `bar/web/`,
multiplayer room in `cloud/`. **The plan is `PLAN.md`**: one doc, eight fixed
parts (North star, your rules, the 5 chapters, Now, the road, Parked, Done,
where things are). The long guide (layout, every tool, iOS limits) is
**`docs/REFERENCE.md`**: open the part you need, not all of it.

## The user's one word: "go" (user, 2026-10-21)
The user only says **"go"**. They work from an iPhone (Safari), are a beginner
in graphics and C++, and want one whole step done per go, fast, with breakage
easy to pinpoint. So on "go":
1. Read **`PLAN.md` → 4. Now** (the step, its pieces; part 7 is the build log). Open
   other docs only when a piece needs them. No menu, no plan reply, no
   questions unless truly blocked (then: ask once, with a recommendation).
   **World first:** `node tools/studio.mjs pull` (the world the user published
   from the Studio → `assets/world/`; commit it as "World: from the studio vN"
   before any piece) and `node tools/uikit.mjs pull` (themes and screens the
   user published from the UI Editor → `ui/system/`; commit as "UI: from the
   editor vN"). Never edit either store by hand.
2. Work out of a branch `work-go` (`git fetch origin main && git checkout -B
   work-go origin/main` if it doesn't exist). Do the step's pieces in order.
   **One piece = one commit**, message starting with its tag (`E3.2 ...`).
   Per piece only the **quick** check (`node tools/check.mjs quick`: build,
   C++ tests, shaders; the piece runner's default) plus the pictures. Push
   each commit: `git push -u origin HEAD:work-go` (the container is temporary).
3. End of the step: `git fetch origin main && git merge origin/main`, push
   `main` **once**. CI runs every test at deploy. CI deploys in ~6 min; the
   `build xxxxxxx` label on screen shows the live commit.
   If the step changed files in `assets/world/`: once the deploy is live,
   `node tools/studio.mjs push --note "<pieces>"` (publishes them as a new
   version by Claude; refused if the user published since: pull, merge, push).
4. Update `PLAN.md` in the same push, **only through `node tools/plan.mjs`**:
   tick the pieces, one Done line (build sha, the pieces, the commit range),
   move the next road item into Now with its pieces. Never add a part, a
   chapter or another plan file. Gotchas → `docs/DECISIONS.md`.
5. Reply to the user, short: the build code, then one line per piece:
   where to stand, what they should see, and "if not, say: broke E3.2".
6. Hub (if `HUB_KEY` is set): one `node tools/hub.mjs post --build <sha>
   --title ... --text ...` at the end. Nothing else on the hub during a go.
7. Don't sit watching CI. If a run goes red, fix it, then tell the user.

## Speed over tests (user, 2026-10-23)
This cloud machine has no graphics card, so any test that runs the game in a
browser (studio, smoke, touch, travel, mp, ...) is slow: 20-30 min each, an
hour+ per piece with the new lighting. The user won't wait on them. So:
- Never run the browser tests here, never wait on them, never block a piece
  or a step on them. The quick check + the pictures are the gate.
- CI runs the whole set at every deploy. A red CI run: fix it after,
  without stopping the current step (and only the part that's really broken).
- The phone check is the real visual test. A slower, safer run only when
  the user asks or for a "broke <piece>" that needs it.
- Tracking never gets skipped: one piece = one commit, the Build log, the
  gallery, the PLAN.md ticks. That's what lets us fix things later.

## Automation first (user, 2026-10-22)
Anything done twice becomes a tool; Claude runs the tools, not the steps by
hand, and improves a tool whenever it's missing or clumsy
(`docs/AUTOMATION.md`). Each piece: `node tools/piece.mjs <TAG> "<title>"
--check "<phone check>"` (build, quick check, before/after
pictures into the `gallery` branch, commit, push). Each step's end: `node
tools/step.mjs finish <STEP> "<what>" --bench`. Before/after is the norm.

## Pinpointing breakage
- Every piece is its own commit with its own phone check, so "broke E3.2"
  names the exact commit. `git log --grep '^E3.2'` finds it.
- **"broke <piece>"**: reproduce with tests/screenshots, fix it (or `git revert`
  that piece's commit), push `main`, give the new build code.
- **"undo"**: `git revert` the last step's whole range (`PLAN.md` → Done), push
  `main`; the bar is back to the build before.
- `node tools/check.mjs` prints PASS/FAIL per system and, for a failure, its
  log file and first error lines (`build/check/<name>.log`).
  `node tools/check.mjs all` runs everything; `node tools/check.mjs room chess`
  only those.

## Other words the user may use
- **"park: <idea>"**: one line to `PLAN.md` → Parked, under its chapter (`node tools/plan.mjs park`). No work.
- **"talk"**: conversation only, no tools.
- **"status"**: read `PLAN.md` (Now, the chapters' lines), answer in a few lines.
- A new idea in chat: one line to Parked under its chapter; never onto the road on your own.
- A new rule from the user ("always…", "never…"): one dated line in `PLAN.md` → Your rules.

## Rules that always hold
- No outside engines or libraries (approved exceptions: three.js for Meridian
  and the platformer). C++20, 2-space indent, `namespace engine` in `looped/`;
  `looped/` never includes `bar/`. Dawn C++ API, callbacks not Asyncify.
  Log with `LOG_INFO` / `LOG_ERROR`.
- Every third-party asset needs a license + credit entry. No copyrighted music.
- No PRs. Don't commit secrets. Explain in plain words; say what the user will
  see on the phone. Performance is only judged on the phone (`?perf=1`,
  Send report → `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`).
- Build: `source /opt/emsdk/emsdk_env.sh; emcmake cmake -B build -G Ninja`
  (once), `cmake --build build`. Setup: `bash tools/setup.sh` (the
  SessionStart hook runs it).
