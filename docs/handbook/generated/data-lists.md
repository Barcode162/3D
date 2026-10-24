<!-- generated: node tools/handbook.mjs write (from toolbelt/tools.json and .github/workflows/deploy.yml); never edit by hand -->
# Data: the lists and the game's config

## Projects (`toolbelt/tools.json` → `projects`)

The one list of projects ([projects](../systems/site/projects.md)).

| Id | Name | Status | About | On the site |
|---|---|---|---|---|
| `bar3d` | Bar3D | Live | The hangout bar: the game, its world and the tools that build it. | featured → `/play/` |
| `looped` | Looped | In progress | Our engine, from scratch: C++ to WebAssembly to WebGPU. Its tools build the looks and screens every project uses. | featured → `/engine/` |
| `script` | The script language | Planned | Our own small, safe language for things that live in the world (T1 on the road). | featured |
| `workshop` | The workshop | Tools | How Claude builds, checks, pictures and ships every piece. | no |

## Tools (`toolbelt/tools.json` → `tools`)

| Id | Name | Project | Group | Opens / runs | What |
|---|---|---|---|---|---|
| `studio` | Studio | `bar3d` | Make | `/studio/` | Build, move and light anything in the world. Every version kept. |
| `ui-editor` | UI Editor | `looped` | Make | `/toolbelt/ui-editor/` | Themes and screens for the site, Dev, the bar's menus and the engine. |
| `screens` | Screens | `looped` | Make | `/ui/screen.html` | See any screen file drawn in the current theme. |
| `compare` | Compare themes | `looped` | Make | `/ui/compare.html` | One screen in every theme side by side, light and dark. |
| `style` | Style page | `looped` | Make | `/ui/` | Every control in every theme. |
| `dials` | Dials | `bar3d` | Tune and check | `/hub/dials.html` | Every number in the world as a dial: light, sound, feel. |
| `world` | World notes | `bar3d` | Tune and check | `/hub/world.html` | What each thing in the world does, its states and rules. |
| `bar` | The live bar | `bar3d` | Tune and check | `/bar/` | Walk the floor. Add ?perf=1 for the frame meter and Send report. |
| `perf` | Perf report | `bar3d` | Tune and check | `/bar/report.php?code=` | Open a phone report by its six-letter code. |
| `tests` | Tests and deploys | `workshop` | Tune and check | `https://github.com/Barcode162/3D/actions` | Every test run and deploy on GitHub. |
| `gallery` | Gallery | `bar3d` | Tune and check | `https://github.com/Barcode162/3D/tree/gallery` | Before and after pictures of every piece. |
| `classic` | Bar3D classic | `bar3d` | Tune and check | `/classic/` | The old public build, kept as it was. |
| `flags` | Engine flags | `looped` | Tune and check |  (soon) | Switch engine features on and off, like ?light=classic or ?bounce=0. |
| `project` | Front door | `workshop` | Run by Claude | `node tools/project.mjs status` | Every tool through one command; status shows the whole truth on one screen. |
| `piece` | Piece runner | `workshop` | Run by Claude | `node tools/piece.mjs <TAG> "<title>"` | One piece: build, check, pictures, commit, push. |
| `step` | Step finisher | `workshop` | Run by Claude | `node tools/step.mjs finish <STEP> "<what>"` | The end of a step: full check, deploy, build log. |
| `plan` | Plan keeper | `workshop` | Run by Claude | `node tools/plan.mjs check` | Keeps PLAN.md in its eight parts and five chapters; ticks pieces, writes Done. |
| `dev-notes` | Your notes | `workshop` | Run by Claude | `node tools/dev.mjs notes` | What you left in Dev → Plan, read at the start of every go. |
| `check` | Checks | `workshop` | Run by Claude | `node tools/check.mjs quick` | PASS or FAIL per system, with the first error lines. |
| `studio-cli` | World sync | `bar3d` | Run by Claude | `node tools/studio.mjs pull` | Brings the world you published into the repo, and back. |
| `uikit` | UI kit | `looped` | Run by Claude | `node tools/uikit.mjs pull` | Brings what you publish in the UI Editor into the site; writes the theme CSS. |
| `bench` | Benchmarks | `looped` | Run by Claude | `node tools/bench.mjs` | Engine speed numbers, each against the last run. |
| `light` | Light tool | `looped` | Run by Claude | `node tools/light.mjs` | Checks and bakes the world's light. |

## The game's config (`/bar/config.json`, written by `deploy.yml`)

The page reads it at start-up.

| Field | What it is |
|---|---|
| `build` | the commit that was built, 7 letters (the `build xxxxxxx` label on screen) |
| `proto` | the room protocol version this build speaks |
| `ws` | the room server's address (Cloudflare, `CF_WS`); empty: no multiplayer |
| `room` | the room the bar joins (`main`) |
| `accountsUrl` | where accounts live (`account.php`) |
| `reportUrl` | where phone reports go (`report.php`) |
