# Bar3D reference (the long guide; CLAUDE.md is the short one, PLAN.md the plan)

> **History (archived in J6.6, 2026-10-24).** What's still true is in the Handbook
> (`docs/handbook/README.md`); decisions still in force are in `docs/records/decisions/`.

> **Not instructions (2026-10-24).** How to work is in `docs/playbook/` (the only
> book that gives instructions). This file holds facts (build, layout, conventions, phone limits);
> they move into the Handbook in J6. Its old "how to work" sections are in
> `docs/archive/REFERENCE-old-instructions.md`.


A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**, building toward a
VRChat-style **3D hangout bar**: walkable, every object interactive, an avatar
visible in a mirror, and real music.

## Build and test
```sh
bash tools/setup.sh                  # installs Emscripten 6.0.12 if missing (CI uses the same)

source /opt/emsdk/emsdk_env.sh
emcmake cmake -B build -G Ninja      # once
cmake --build build                  # output: build/site/
NODE_PATH=$(npm root -g) node tools/smoke.mjs   # headless Chromium + software WebGPU
bash tools/native/run.sh             # car (old + new: drive_lab), race, JSON, math, physics tests (g++ and em++ + node)
node tools/lint_emasm.mjs            # quoted names in EM_ASM (Release renames the rest)
node tools/proto_test.mjs            # room protocol codec on proto/vectors.json (C++ twin in run.sh)
node tools/room_test.mjs             # Cloudflare room via its own wrangler dev (setup.sh installs it)
node tools/ledger_test.mjs           # the Ledger (money, items) via its own wrangler dev
NODE_PATH=$(npm root -g) node tools/mp_test.mjs     # two/three players + the room (wrangler dev)
NODE_PATH=$(npm root -g) node tools/voice_test.mjs  # voice chat between two players
NODE_PATH=$(npm root -g) node tools/meridian_test.mjs  # Meridian from a racing rig and back
NODE_PATH=$(npm root -g) node tools/race_mp_test.mjs   # two players race each other on the rigs
NODE_PATH=$(npm root -g) node tools/arcade_test.mjs    # the arcade: tap the platformer machine
NODE_PATH=$(npm root -g) node tools/rhythm_test.mjs    # 4K Mania: song select + a song on autoplay (needs ffmpeg)
NODE_PATH=$(npm root -g) node tools/chess_test.mjs     # the lounge's chessboard: a move and the computer's reply
NODE_PATH=$(npm root -g) node tools/gate_test.mjs      # the private bar's door: sign-in, invites, admins, file streaming
```
CI runs all of these before publishing (smoke is non-blocking until it has
passed once on GitHub's runners). "Looks the same" checks:
`OUT=build/before node tools/shots.mjs spots` (5 standard spots with `?still`,
before the change), same with `OUT=build/after`, then
`node tools/imgdiff.mjs build/before build/after`. After a deploy:
`EXPECT_SHA=<sha> NODE_PATH=$(npm root -g) node tools/check_live.mjs` (stays
out of the real room unless `NET=1`). Test helpers live in `tools/lib/`.
The smoke test prints the on-screen log and a `perf:` line (phase costs,
draw counts, start-up stages; never judge frame rate headless), and saves `build/smoke.png` and
`build/smoke-walk.png` (after walking forward and turning) and
`build/smoke-down.png` (looking down at your own body), which
can be viewed with the Read tool. To look at a specific spot, run with
`CAM="x,y,z,yaw,pitch"` (degrees) → `build/smoke-cam.png`; the page itself
accepts `?cam=x,y,z,yaw,pitch` (see `docs/DECISIONS.md` → Gotchas for
testing music and the song picker). `tools/shots.mjs` takes `VIEW=932x430`
(landscape), `HOLD=ArrowUp:9000` (hold a key, e.g. to drive) and
`REQUESTS=1` (print requests to outside hosts); `sit=30`
sits in a racing rig (`&pad` the skid pad, `&telemetry=1` the overlay). Car physics and whole races also run natively
(no browser): see `tools/native/*.cpp`. Software WebGPU (SwiftShader) works in this
container, so rendering can be checked visually. Performance can only be
judged on the user's phone.

## Layout
**Looped layout (2026-10-20, docs/LOOPED.md B):** `looped/` is the engine
(C++ namespace still `engine`; it never includes `bar/`), `bar/` the main
game, `bar/projects/<name>/` the bar's projects, each with a `project.json`
(`kind` web → copied to the site at its `site` path, the same addresses as
before; `kind` native → C++ built into the game): `meridian`, `platformer`,
`mania` (4K Mania), `chess`, `racing`. `bar/web/` is the page (the site's root).
- `looped/` is the reusable engine: GPU setup, math, meshes, camera, input
  (incl. taps), logging, files, text (own pixel font), procedural textures,
  audio bridge, skeletons and skinned meshes, physics, car physics
  (`vehicle.*`; the new driving physics in `drive/`, docs/DRIVING_REWORK.md), gamepads, smooth GPU uploads (`gpu_upload.*`), the background job queue
  (`jobs.*`), async pipelines (`pipeline_cache.*`), the room protocol codec
  (`netproto.*`), the frame
  profiler and performance overlay (`profiler.*`, `?perf=1`), and **the world
  core** (`looped/world/`: things, kinds, places, states, rules, loops on the
  shared clock; docs/ENGINE.md).
- `bar/` is the bar world itself. **The layout is a file:**
  `assets/world/places/bar.json` (every thing, where it sits, its parts) with
  `assets/world/kinds/bar.json` (what each kind is: model, shapes, seats, tap
  areas, loops); `bar_scene.cpp` reads them into the lists the game uses
  (`bar/world_files.*` finds the files: built into the game, handed to the
  room's physics). Reformat after editing: `python3 tools/place_format.py <file>`. `renderer.cpp` draws the scene,
  mirrors and avatar. `avatar.cpp` builds the body, skeleton and animation.
  `jukebox.cpp` is the playlist and its menu. `players.cpp` is the other
  players (multiplayer smoothing, their state slots), `shared_world.cpp`
  the shared glasses, jukebox and mirrors. `piano.cpp`, `darts.cpp`,
  `bartender.cpp` are the bar's games and jobs (4K Mania and chess are projects); `bar/projects/racing/` is the
  racing sim (`docs/RACING.md`: track, world + bots, controls, view, game).
  `main.cpp` is the entry point; `game.cpp` the frame's phases (a table:
  input → interaction → physics → net in → games → claims → net out →
  render), seats and things to use; `app_state.h` the shared App state, `hud.cpp` the on-screen text,
  `local_player.cpp` your camera, walking and held things. `world_bar.*`
  plays the bar's world (every thing's loop, `assets/world/kinds/bar.json`). `loader.*` is the loading screen's stages,
  `test_hooks.*` the test page options (`?cam`, `?still`, `?sit`...).
- `looped/shaders/` holds WGSL shaders, embedded into the binary at `/shaders/...`.
- `assets/` holds files streamed at runtime, copied to `site/assets/`. Every
  third-party asset needs a license and credit entry (see
  `assets/music/CREDITS.md`).
- `bar/web/index.html` is the single page: fullscreen canvas, status bar, log
  panel. Everything C++ prints goes to the log panel; errors open it
  automatically. `bar/web/audio.js` is the thin Web Audio + song-picker bridge, `bar/web/net.js`
  the multiplayer bridge, `bar/web/voice.js` WebRTC voice chat, `bar/web/menu.js`
  the HTML menus and settings, `bar/web/account.js` accounts and profiles.
  `bar/web/bridge.js` (with `looped/bridge.*`) is the per-frame mailbox: one
  pull at frame start, one flush at frame end; prefer it over new EM_ASM
  calls in the frame.
- `bar/projects/meridian/` is **Meridian**, the user's own driving game (three.js from
  jsDelivr: the one approved exception to the no-outside-engines rule, for this
  game only; credits in its CREDITS.md, Blender scripts in `incoming/meridian/`).
  Sitting in a racing rig opens a menu (`bar/web/menu.js` "sim"): the bot race or
  Meridian, which opens full screen in an iframe while the bar keeps its
  connection and stops drawing. Test: `tools/meridian_test.mjs`.
- `assets/midi/` holds the piano's MIDI songs (credits in its CREDITS.md).
- `docs/REMASTER.md` lists every crude first version to redo properly.
- `server/report.php` stores phone reports (Menu → Settings → Send report, or
  tap the build code 3 times, gives a code; read it with `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`).
- `docs/PROTOCOL.md` is the room protocol (Cloudflare room ↔ game); its two
  codecs are `cloud/proto.js` and `looped/netproto.*`, both tested on
  `proto/vectors.json`. `cloud/` is the Cloudflare Worker (`worker.js`:
  router, Origin check, `/health`) + room Durable Object (`room.js`), the
  multiplayer server, + the Ledger (`ledger.js`: money and owned things,
  `/bank`, test `tools/ledger_test.mjs`). The room checks every shared write (`rules.js`) and runs
  games itself (`cloud/games/`: darts, jukebox, glasses); the glasses use the
  bar's own C++ physics compiled for the Worker (`cloud/sim/`, built by
  `cloud/sim/build.sh` into `room_sim.wasm`, not in git: CI and `setup.sh` build
  it). It is deployed by CI to https://bar3d.joshuavelasco60.workers.dev,
  its address written into the site's `config.json`. The game finds it there
  (`bar/web/net.js`, which uses `proto.js`: `cloud/proto.js` copied in by the build).
  On localhost multiplayer is off unless `?cloud=ws://...` is given.
- `hub/` at joshuavelasco.com/hub: `hub.php` keeps the news posts, their
  screenshots, the Claude key and the loops' tuning (the board was taken out
  in J5.4; `/hub/` opens Dev); also World notes and the Dials; test
  `tools/hub_test.mjs`.
- `server/` is PHP on the user's IONOS hosting: `account.php` (logins,
  profiles) and `report.php`, uploaded by the deploy with a second copy of the
  game at joshuavelasco.com/bar (the user wants their domain kept: home page
  at `/`, the bar at `/bar/`). `relay.php` is the old multiplayer relay, no
  longer used by the game (Cloudflare replaced it, 2026-10-18).
- `tools/` holds dev scripts: `setup.sh` (toolchain), `smoke.mjs` (headless test),
  `check_live.mjs` (loads the deployed site headlessly), `menu_test.mjs`, `menu_games_test.mjs` (the menu after every game)
  (menus ↔ game through the mailbox), `mp_test.mjs`
  (two players through the room on local wrangler dev), `voice_test.mjs` (WebRTC voice
  between two players, handshake through the room), `shots.mjs` (screenshots with page options).
  `tools/assets/astc_pack.py` makes the phone's compressed textures at build time
  (`assets/tex/`, see DECISIONS → Compressed textures).
  `tools/assets/` builds the 3D models in headless Blender (see
  `docs/DECISIONS.md` → Asset pipeline); output goes to `assets/models/`.
- `.github/workflows/deploy.yml` builds on every push to `main` and publishes
  to the `gh-pages` branch (see `docs/DECISIONS.md` → Hosting).
- `PLAN.md` holds the plan; `docs/` explains (`DECISIONS.md`, the manual; old plans in `docs/archive/`).

## Conventions
- C++20, 2-space indent, `namespace engine` for engine code. Use the Dawn
  C++ API from `<webgpu/webgpu_cpp.h>` (emdawnwebgpu port).
- Asynchronous WebGPU setup uses callbacks (`CallbackMode::AllowSpontaneous`),
  not Asyncify.
- Log with `LOG_INFO` / `LOG_ERROR` from `looped/log.h`.
- Pixel ratio is capped at 2 (`looped/gpu.cpp`) for phone GPU budget.
- No outside engines or libraries. The engine is written from scratch.
  Exceptions must be discussed with the user first. (Approved so far: three.js,
  for Meridian, 2026-10-18, and for the user's platformer "Test 2" in the
  arcade, `bar/projects/platformer/`, added "as is", 2026-10-19.)

## iOS Safari constraints to remember
- WebGPU is available on iOS 26. There's no WebXR (AR would mean exporting USDZ for AR Quick Look).
- Audio can only start after a user tap. Motion sensors need a permission tap.
- GitHub Pages can't send COOP/COEP headers, so there's no SharedArrayBuffer
  or pthreads unless the coi-serviceworker workaround is added.
- Memory per tab is limited, and long sessions can be slowed by heat throttling.
- Don't commit copyrighted music to this repo (it's private since 2026-10; the rule holds anyway).
