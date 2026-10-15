# Bar3D: guide for Claude sessions

A from-scratch 3D engine in **C++ → WebAssembly → WebGPU**, building toward a
VRChat-style **3D hangout bar**: walkable, every object interactive, an avatar
visible in a mirror, and real music. Read `PROGRESS.md` first: it says what
is done and what is next.

## Working with the user
- The user works **only from an iPhone 15 Pro Max (iOS 26, Safari)** through the
  Claude app. They can't run anything locally or open devtools.
- They're a beginner in graphics and C++. Explain in plain language, keep
  replies short, and say what they'll see on the phone after each change.
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
  changes.
- Before pushing: the build passes and the smoke test passes. Look at the
  screenshot.

## Build and test
```sh
# One-time per container: install the compiler (same version CI uses).
git clone --depth 1 https://github.com/emscripten-core/emsdk.git /opt/emsdk
/opt/emsdk/emsdk install 6.0.12 && /opt/emsdk/emsdk activate 6.0.12

source /opt/emsdk/emsdk_env.sh
emcmake cmake -B build -G Ninja      # once
cmake --build build                  # output: build/site/
NODE_PATH=$(npm root -g) node tools/smoke.mjs   # headless Chromium + software WebGPU
```
The smoke test prints the on-screen log and saves `build/smoke.png`, which
can be viewed with the Read tool. Software WebGPU (SwiftShader) works in this
container, so rendering can be checked visually. Performance can only be
judged on the user's phone.

## Layout
- `engine/` is the reusable engine: GPU setup, logging, files. Later: math,
  meshes, camera, input, audio.
- `app/` is the bar world itself (game code).
- `shaders/` holds WGSL shaders, embedded into the binary at `/shaders/...`.
- `assets/` holds files streamed at runtime, copied to `site/assets/`. Every
  third-party asset needs a license and credit entry (see
  `assets/music/CREDITS.md`).
- `web/index.html` is the single page: fullscreen canvas, status bar, log
  panel. Everything C++ prints goes to the log panel; errors open it
  automatically.
- `tools/` holds dev scripts. `.github/workflows/deploy.yml` builds and
  deploys to Pages.

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
