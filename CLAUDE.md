# Bar3D: guide for Claude sessions (short version, 2026-10-21)

A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**: a VRChat-style 3D
hangout bar. Engine in `looped/`, the bar in `bar/`, page in `bar/web/`,
multiplayer room in `cloud/`. The long guide (layout, every tool, hub details,
iOS limits) is **`docs/REFERENCE.md`**: open the part you need, not all of it.

## The user's one word: "go" (user, 2026-10-21)
The user only says **"go"**. They work from an iPhone (Safari), are a beginner
in graphics and C++, and want one whole step done per go, fast, with breakage
easy to pinpoint. So on "go":
1. Read **`NEXT.md`** only (the next step, its pieces, the build log). Open
   other docs only when a piece needs them. No menu, no plan reply, no
   questions unless truly blocked (then: ask once, with a recommendation).
2. Work out of a branch `work-go` (`git fetch origin main && git checkout -B
   work-go origin/main` if it doesn't exist). Do the step's pieces in order.
   **One piece = one commit**, message starting with its tag (`E3.2 ...`),
   each passing `node tools/check.mjs` before the commit. Push each commit:
   `git push -u origin HEAD:work-go` (the container is temporary).
3. End of the step: `node tools/check.mjs` (green), `git fetch origin main &&
   git merge origin/main`, push `main` **once**. CI deploys in ~6 min; the
   `build xxxxxxx` label on screen shows the live commit.
4. Update `NEXT.md` in the same push: tick the pieces, add one Build log line
   (build sha, the pieces, the commit range), write the **next** step's pieces.
   Tick the step in `docs/ENGINE.md` → Progress. Gotchas → `docs/DECISIONS.md`.
5. Reply to the user, short: the build code, then one line per piece:
   where to stand, what they should see, and "if not, say: broke E3.2".
6. Hub (if `HUB_KEY` is set): one `node tools/hub.mjs post --build <sha>
   --title ... --text ...` at the end. Nothing else on the hub during a go.
7. Don't sit watching CI. If a run goes red, fix it, then tell the user.

## Pinpointing breakage
- Every piece is its own commit with its own phone check, so "broke E3.2"
  names the exact commit. `git log --grep '^E3.2'` finds it.
- **"broke <piece>"**: reproduce with tests/screenshots, fix it (or `git revert`
  that piece's commit), push `main`, give the new build code.
- **"undo"**: `git revert` the last step's whole range (Build log in
  `NEXT.md`), push `main`; the bar is back to the build before.
- `node tools/check.mjs` prints PASS/FAIL per system and, for a failure, its
  log file and first error lines (`build/check/<name>.log`).
  `node tools/check.mjs all` runs everything; `node tools/check.mjs room chess`
  only those.

## Other words the user may use
- **"park: <idea>"**: one line to Parked in `PROGRESS.md`. No work.
- **"talk"**: conversation only, no tools.
- **"status"**: read `NEXT.md` and `docs/ENGINE.md` → Progress, answer in a few lines.
- A new idea in chat: one line to Parked; never onto the plan on your own.

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
