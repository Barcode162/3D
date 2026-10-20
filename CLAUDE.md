# Bar3D: guide for Claude sessions

A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**, building toward a
VRChat-style **3D hangout bar**: walkable, every object interactive, an avatar
visible in a mirror, and real music.

## Start of every session (the user often starts fresh chats to save tokens)
1. Read **`PROGRESS.md`** (the master list and Parked), **`docs/VISION.md`**
   (the user's dream; "The product" is the current direction) and
   **`docs/DECISIONS.md`** (what was chosen, why, and what failed).
2. The SessionStart hook (`.claude/settings.json` → `tools/setup.sh`)
   installs Emscripten. If it didn't run, run `bash tools/setup.sh`.
3. Don't build or run headless tests until the user says "go" (tokens).

## The routine (user, 2026-10-19: fewer, deeper passes; save tokens)
- **"start"**: read the docs, show a short menu: the master list + Parked, one
  line each. No builds, no tests. The user picks one.
- **"plan"**: a short plan for the picked item: what changes, what they'll see
  on the phone, open questions. No code yet. The user says "go" or edits it.
- **"go"**: build it in steps; test and deploy **once at the end** (not after
  each small step), then the "How to check it worked" list.
- **"park: <idea>"**: add one line to Parked in `PROGRESS.md`. No work, no debate.
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
- Testing loop: push to `main` → GitHub Actions builds and force-pushes the site
  to the `gh-pages` branch (Pages serves it) → user opens
  **https://barcode162.github.io/3D/** in Safari. The `build xxxxxxx` label in
  the top-right shows which commit is live, so the user can confirm they have
  the newest version.

## Never lose work
- The container is temporary. **Commit and push to `main` after every working
  step**, without waiting to be asked. Pushing straight to `main` is the agreed
  workflow (no PRs).
- Update `PROGRESS.md` in the same commit whenever a step is finished or a plan
  changes. Record new decisions or gotchas in `docs/DECISIONS.md`, and new
  ideas from the user in `docs/VISION.md`. A fresh session only knows what's
  written in the repo.
- After pushing, confirm the deploy: wait for `gh-pages` to get a
  "Site build <sha>" commit (or poll `version.txt` on the site), then run
  `NODE_PATH=$(npm root -g) node tools/check_live.mjs` to load the live
  site headlessly, and tell the user the build code to look for.
- Before pushing code: the build passes and the smoke test passes. Look at the
  screenshot. Docs-only commits need no build or tests.

## Build and test
```sh
bash tools/setup.sh                  # installs Emscripten 6.0.12 if missing (CI uses the same)

source /opt/emsdk/emsdk_env.sh
emcmake cmake -B build -G Ninja      # once
cmake --build build                  # output: build/site/
NODE_PATH=$(npm root -g) node tools/smoke.mjs   # headless Chromium + software WebGPU
bash tools/native/run.sh             # car, race, JSON, math, physics tests (g++ and em++ + node)
node tools/lint_emasm.mjs            # quoted names in EM_ASM (Release renames the rest)
node tools/proto_test.mjs            # room protocol codec on proto/vectors.json (C++ twin in run.sh)
node tools/room_test.mjs             # Cloudflare room via its own wrangler dev (setup.sh installs it)
NODE_PATH=$(npm root -g) node tools/mp_test.mjs     # two/three players + the room (wrangler dev)
NODE_PATH=$(npm root -g) node tools/voice_test.mjs  # voice chat between two players
NODE_PATH=$(npm root -g) node tools/meridian_test.mjs  # Meridian from a racing rig and back
NODE_PATH=$(npm root -g) node tools/race_mp_test.mjs   # two players race each other on the rigs
NODE_PATH=$(npm root -g) node tools/arcade_test.mjs    # the arcade: tap the platformer machine
NODE_PATH=$(npm root -g) node tools/rhythm_test.mjs    # 4K Mania: song select + a song on autoplay (needs ffmpeg)
NODE_PATH=$(npm root -g) node tools/chess_test.mjs     # the lounge's chessboard: a move and the computer's reply
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
sits in a racing rig. Car physics and whole races also run natively
(no browser): see `tools/native/*.cpp`. Software WebGPU (SwiftShader) works in this
container, so rendering can be checked visually. Performance can only be
judged on the user's phone.

## Layout
- `engine/` is the reusable engine: GPU setup, math, meshes, camera, input
  (incl. taps), logging, files, text (own pixel font), procedural textures,
  audio bridge, skeletons and skinned meshes, physics, car physics
  (`vehicle.*`), gamepads, smooth GPU uploads (`gpu_upload.*`), the background job queue
  (`jobs.*`), async pipelines (`pipeline_cache.*`), the room protocol codec
  (`netproto.*`), the frame
  profiler and performance overlay (`profiler.*`, `?perf=1`).
- `app/` is the bar world itself. `bar_scene.cpp` holds the layout, lights,
  colliders, interactables and mirrors. `renderer.cpp` draws the scene,
  mirrors and avatar. `avatar.cpp` builds the body, skeleton and animation.
  `jukebox.cpp` is the playlist and its menu. `players.cpp` is the other
  players (multiplayer smoothing, their state slots), `shared_world.cpp`
  the shared glasses, jukebox and mirrors. `piano.cpp`, `darts.cpp`,
  `rhythm.cpp`, `bartender.cpp` are the games and jobs; `app/race/` is the
  racing sim (`docs/RACING.md`: track, world + bots, controls, view, game).
  `main.cpp` is the entry point; `game.cpp` the frame's phases (a table:
  input → interaction → physics → net in → games → claims → net out →
  render), seats and things to use; `app_state.h` the shared App state, `hud.cpp` the on-screen text,
  `local_player.cpp` your camera, walking and held things. `loader.*` is the loading screen's stages,
  `test_hooks.*` the test page options (`?cam`, `?still`, `?sit`...).
- `shaders/` holds WGSL shaders, embedded into the binary at `/shaders/...`.
- `assets/` holds files streamed at runtime, copied to `site/assets/`. Every
  third-party asset needs a license and credit entry (see
  `assets/music/CREDITS.md`).
- `web/index.html` is the single page: fullscreen canvas, status bar, log
  panel. Everything C++ prints goes to the log panel; errors open it
  automatically. `web/audio.js` is the thin Web Audio + song-picker bridge, `web/net.js`
  the multiplayer bridge, `web/voice.js` WebRTC voice chat, `web/menu.js`
  the HTML menus and settings, `web/account.js` accounts and profiles.
  `web/bridge.js` (with `engine/bridge.*`) is the per-frame mailbox: one
  pull at frame start, one flush at frame end; prefer it over new EM_ASM
  calls in the frame.
- `web/meridian/` is **Meridian**, the user's own driving game (three.js from
  jsDelivr: the one approved exception to the no-outside-engines rule, for this
  game only; credits in its CREDITS.md, Blender scripts in `incoming/meridian/`).
  Sitting in a racing rig opens a menu (`web/menu.js` "sim"): the bot race or
  Meridian, which opens full screen in an iframe while the bar keeps its
  connection and stops drawing. Test: `tools/meridian_test.mjs`.
- `assets/midi/` holds the piano's MIDI songs (credits in its CREDITS.md).
- `docs/REMASTER.md` lists every crude first version to redo properly.
- `server/report.php` stores phone reports (Menu → Settings → Send report, or
  tap the build code 3 times, gives a code; read it with `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`).
- `docs/PROTOCOL.md` is the room protocol (Cloudflare room ↔ game); its two
  codecs are `cloud/proto.js` and `engine/netproto.*`, both tested on
  `proto/vectors.json`. `cloud/` is the Cloudflare Worker (`worker.js`:
  router, Origin check, `/health`) + room Durable Object (`room.js`), the
  multiplayer server. The room checks every shared write (`rules.js`) and runs
  games itself (`cloud/games/`: darts, jukebox, glasses); the glasses use the
  bar's own C++ physics compiled for the Worker (`cloud/sim/`, built by
  `cloud/sim/build.sh` into `room_sim.wasm`, not in git: CI and `setup.sh` build
  it). It is deployed by CI to https://bar3d.joshuavelasco60.workers.dev,
  its address written into the site's `config.json`. The game finds it there
  (`web/net.js`, which uses `proto.js`: `cloud/proto.js` copied in by the build).
  On localhost multiplayer is off unless `?cloud=ws://...` is given.
- `server/` is PHP on the user's IONOS hosting: `account.php` (logins,
  profiles) and `report.php`, uploaded by the deploy with a second copy of the
  game at joshuavelasco.com/bar (the user wants their domain kept: home page
  at `/`, the bar at `/bar/`). `relay.php` is the old multiplayer relay, no
  longer used by the game (Cloudflare replaced it, 2026-10-18).
- `tools/` holds dev scripts: `setup.sh` (toolchain), `smoke.mjs` (headless test),
  `check_live.mjs` (loads the deployed site headlessly), `menu_test.mjs`
  (menus ↔ game through the mailbox), `mp_test.mjs`
  (two players through the room on local wrangler dev), `voice_test.mjs` (WebRTC voice
  between two players, handshake through the room), `shots.mjs` (screenshots with page options).
  `tools/assets/astc_pack.py` makes the phone's compressed textures at build time
  (`assets/tex/`, see DECISIONS → Compressed textures).
  `tools/assets/` builds the 3D models in headless Blender (see
  `docs/DECISIONS.md` → Asset pipeline); output goes to `assets/models/`.
- `.github/workflows/deploy.yml` builds on every push to `main` and publishes
  to the `gh-pages` branch (see `docs/DECISIONS.md` → Hosting).
- `docs/` holds the long-term context: `VISION.md` and `DECISIONS.md`.

## Conventions
- C++20, 2-space indent, `namespace engine` for engine code. Use the Dawn
  C++ API from `<webgpu/webgpu_cpp.h>` (emdawnwebgpu port).
- Asynchronous WebGPU setup uses callbacks (`CallbackMode::AllowSpontaneous`),
  not Asyncify.
- Log with `LOG_INFO` / `LOG_ERROR` from `engine/log.h`.
- Pixel ratio is capped at 2 (`engine/gpu.cpp`) for phone GPU budget.
- No outside engines or libraries. The engine is written from scratch.
  Exceptions must be discussed with the user first. (Approved so far: three.js,
  for Meridian, 2026-10-18, and for the user's platformer "Test 2" in the
  arcade, `web/arcade/platformer/`, added "as is", 2026-10-19.)

## iOS Safari constraints to remember
- WebGPU is available on iOS 26. There's no WebXR (AR would mean exporting USDZ for AR Quick Look).
- Audio can only start after a user tap. Motion sensors need a permission tap.
- GitHub Pages can't send COOP/COEP headers, so there's no SharedArrayBuffer
  or pthreads unless the coi-serviceworker workaround is added.
- Memory per tab is limited, and long sessions can be slowed by heat throttling.
- Don't commit copyrighted music to this public repo.
