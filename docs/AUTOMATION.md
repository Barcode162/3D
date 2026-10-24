# Automation: the tools that do the work (user, 2026-10-22)

**The rule:** anything done twice becomes a tool. Claude doesn't do steps
by hand; Claude builds and improves the tools that do them, then runs the
tools. The tools are reused and grow as the project grows. When a tool is
missing or clumsy, fixing the tool is part of the piece.

## The everyday tools
| Tool | What it does |
|---|---|
| `node tools/piece.mjs <TAG> "<title>" --test <systems> --check "<phone check>" [--compare "x=0"]` | one piece, end to end: build → its tests → pictures of every spot → before/after of every spot that changed (and of the feature's switch) → commit `<TAG> <title>` → push work-go → numbers in the gallery |
| `node tools/step.mjs status` | the pieces of the step so far, with their phone checks |
| `node tools/step.mjs finish <STEP> "<what>" [--bench]` | the end of a step: full check → merge main → push main once → Done line in PLAN.md → benchmark → hub post → the reply for the user |
| `node tools/bench.mjs [native]` | benchmarks (native C++ and wasm benches `tools/native/bench_*.cpp`, the bar at every spot), saved forever, each number shown against the last run |
| `node tools/beforeafter.mjs "<option A>" "<option B>" [spots]` | side-by-side pictures of two page options, any time |
| `node tools/piece.mjs baseline` | pictures of the current build as the next "before" |
| `node tools/piece.mjs <TAG> "<title>" --pages a,b` | a site or tool piece: phone-width before/after pictures of those pages (repo paths, e.g. `toolbelt/ui-editor/index.html?mode=theme`) instead of the bar |
| `node tools/uikit.mjs css / check / pull / push` | the UI kit: `ui/themes.css` written from `ui/system/` (never by hand), the check that they agree, the UI Editor's published version into the repo and back |
| `node tools/plan.mjs check / now / tick / add / park / rule / start / done` | the World Plan's keeper: the only way Claude edits `PLAN.md`; refuses anything that breaks its eight parts or five chapters (piece.mjs ticks, step.mjs writes Done through it) |
| `toolbelt/tools.json` | the Toolbelt: every tool, one entry each (a new tool = one line there) |

## The gallery (branch `gallery`, worktree `build/gallery`)
Never merged or deployed. Keeps, for every piece: its before/after pictures
(`<TAG>/<spot>.jpg`, `<TAG>/<spot>-switch.jpg`), its numbers
(`pieces.jsonl`: files, lines, tests, headless frame time, draws,
brightness per spot), every finished step (`steps.jsonl`) and every
benchmark (`bench.jsonl`). `README.md` there is the gallery page, newest
first; on GitHub it shows the pictures. **Before/after is the norm: every
piece that changes the picture has one.**

## Adding to the tools
- A new benchmark: `tools/native/bench_<name>.cpp`, first line
  `// sources: <files to compile with it>`, prints `name value unit` lines.
- A new spot: `tools/lib/spots.mjs` (every piece then pictures it too).
- A new check system: `tools/check.mjs` CHECKS and RULES.
- A new feature switch: read it with `hooks.feature("name")` (bar/test_hooks.h),
  document it in its `docs/manual/<track>.md` section; `--compare "name=0"`
  gives its before/after.
