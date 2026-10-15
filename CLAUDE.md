# Bar3D: guide for Claude sessions

A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**, building toward a
VRChat-style **3D hangout bar**: walkable, every object interactive, an avatar
visible in a mirror, and real music.

## Start of every session (the user often starts fresh chats to save tokens)
1. Read **`PROGRESS.md`** (roadmap, what's done, and the "Next up" list),
   **`docs/VISION.md`** (the user's full dream for the project) and
   **`docs/DECISIONS.md`** (what was chosen, why, and what failed).
2. The SessionStart hook (`.claude/settings.json` → `tools/setup.sh`)
   installs Emscripten. If it didn't run, run `bash tools/setup.sh`.
3. Build and run the smoke test (below) to confirm everything works.
4. When the user says "continue", pick the first unchecked item in "Next up"
   and go. Don't re-ask questions already answered in these files.

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
- Before pushing: the build passes and the smoke test passes. Look at the
  screenshot.

## Build and test
```sh
bash tools/setup.sh                  # installs Emscripten 6.0.12 if missing (CI uses the same)

source /opt/emsdk/emsdk_env.sh
emcmake cmake -B build -G Ninja      # once
cmake --build build                  # output: build/site/
NODE_PATH=$(npm root -g) node tools/smoke.mjs   # headless Chromium + software WebGPU
```
The smoke test prints the on-screen log and saves `build/smoke.png` and
`build/smoke-walk.png` (after walking forward and turning) and
`build/smoke-down.png` (looking down at your own body), which
can be viewed with the Read tool. To look at a specific spot, run with
`CAM="x,y,z,yaw,pitch"` (degrees) → `build/smoke-cam.png`; the page itself
accepts `?cam=x,y,z,yaw,pitch` (see `docs/DECISIONS.md` → Gotchas for
testing music and the song picker). Software WebGPU (SwiftShader) works in this
container, so rendering can be checked visually. Performance can only be
judged on the user's phone.

## Layout
- `engine/` is the reusable engine: GPU setup, math, meshes, camera, input
  (incl. taps), logging, files, text (own pixel font), procedural textures,
  audio bridge, skeletons and skinned meshes. Later: physics.
- `app/` is the bar world itself. `bar_scene.cpp` holds the layout, lights,
  colliders, interactables and mirrors. `renderer.cpp` draws the scene,
  mirrors and avatar. `avatar.cpp` builds the body, skeleton and animation.
  `jukebox.cpp` is the playlist and its menu. `players.cpp` is the other
  players (multiplayer smoothing). `main.cpp` is the frame loop,
  input → interaction, and on-screen text.
- `shaders/` holds WGSL shaders, embedded into the binary at `/shaders/...`.
- `assets/` holds files streamed at runtime, copied to `site/assets/`. Every
  third-party asset needs a license and credit entry (see
  `assets/music/CREDITS.md`).
- `web/index.html` is the single page: fullscreen canvas, status bar, log
  panel. Everything C++ prints goes to the log panel; errors open it
  automatically. `web/audio.js` is the thin Web Audio + song-picker bridge, `web/net.js`
  the multiplayer bridge.
- `server/` is the multiplayer relay (PHP), uploaded to the user's IONOS
  web hosting by the deploy (see `docs/DECISIONS.md` → Multiplayer).
- `tools/` holds dev scripts: `setup.sh` (toolchain), `smoke.mjs` (headless test),
  `check_live.mjs` (loads the deployed site headlessly), `mp_test.mjs`
  (two players through a local PHP relay).
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
  Exceptions must be discussed with the user first.

## iOS Safari constraints to remember
- WebGPU is available on iOS 26. There's no WebXR (AR would mean exporting USDZ for AR Quick Look).
- Audio can only start after a user tap. Motion sensors need a permission tap.
- GitHub Pages can't send COOP/COEP headers, so there's no SharedArrayBuffer
  or pthreads unless the coi-serviceworker workaround is added.
- Memory per tab is limited, and long sessions can be slowed by heat throttling.
- Don't commit copyrighted music to this public repo.
