# Frame and core

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

The heart of Looped: the WebGPU connection, the one clock per frame, the frame's phases, logging, maths, files and the page mailbox. Everything else runs inside a frame this sets up.

## Status

**Live.** Every frame of the bar goes through it.

## What you can do with it

- Nothing to tap: it's what makes the bar run at all. The build code at the top of the screen (`build xxxxxxx`) is the live commit.
- Log panel: errors open a red badge; Menu → Settings → Send report sends the last 300 log lines (see [profiler and phone reports](profiler-and-phone-reports.md)).

## How it works

- `looped/gpu.*` owns the WebGPU instance, adapter, device and the canvas; setup is asynchronous with callbacks (`CallbackMode::AllowSpontaneous`), never Asyncify. The pixel ratio is capped at 2.
- `looped/frame_time.*`: the browser gives each frame a start time; `frameTime()` is the one "now" for the whole frame; `clockMs()` is only for measuring.
- `bar/game.cpp` runs the frame as a table of phases: input → interaction → physics → net in → games → claims → net out → render.
- `looped/bridge.*` with `bar/web/bridge.js` is the mailbox: one `pull()` at the start of the frame (canvas size, safe area, touches, menu commands, voice levels), one `flush()` at the end (every page change the game queued). Calls between WebAssembly and JavaScript are slow on phones, so they're batched.
- **The engine on its own (J86):** Looped builds as its own library (CMake target `looped`; the bar's `app` links it) and talks to its page only through `window.loopedHost` (`looped/host.*`): `net`, `audio`, `voice`, `bridge`, `log`, `account`, `settings` and `storagePrefix`. `bar/web/index.html` fills it from the bar's own objects (`bar3dNet`, `bar3dAudio`…); a part a page doesn't give does nothing and is named once in the console. The engine's numbers are `window.loopedPerf` (the bar's page also shows them as `bar3dPerf`). `tools/lint_engine.mjs` (in the quick check) refuses a `bar/` include or a `bar3d…` name inside `looped/`.
- `looped/log.*`: `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` / `LOG_FATAL`; a ring of 300 lines in the page; a leading "Word:" is the line's category; fatal shows "Something broke - tap to rejoin" and sends a report.
- `looped/math.h`: vectors, 4×4 matrices (column-major, like WGSL), quaternions; right-handed, +Y up, the camera looks down -Z. `looped/json.*`: a reader with limits (never crashes on garbage) and a writer that escapes properly. `looped/file.*`: the files built into the game (`/shaders/...`).

## Data

No stored data. The game's own files are built in (CMake `--embed-file`); the page's settings live in the phone's `localStorage` (`bar3dSettings`).

## Commands

- Build: `source /opt/emsdk/emsdk_env.sh; emcmake cmake -B build -G Ninja` (once), then `cmake --build build` → `build/site/`.
- `node tools/project.mjs check build` builds and lints (`tools/lint_emasm.mjs`).

## Limits

- One thread: GitHub Pages and the hosting can't send the headers SharedArrayBuffer needs, so no threads in the browser.
- Phones: memory per tab is limited; long sessions can heat up and slow down. 60 fps on an iPhone 15 Pro Max is the hard budget.

## Checks

- `build`, `lint_emasm` (quick), `native` (`tools/native/run.sh`: maths, JSON and more without a browser), `smoke` on GitHub.

## Known traps

- Code inside `EM_ASM` goes through the minifier: quote property names (`obj['name']`), and `\.` / `\d` in regexes are eaten. `tools/lint_emasm.mjs` checks the names.
- Prefer the mailbox (`bridge`) to new `EM_ASM` calls inside the frame.
- Headless browsers run at 1-2 frames a second: never judge speed there; only on the phone (`?perf=1`).

## History

Built from scratch from 2026-10 (one engine, C++ → WebAssembly → WebGPU). Renamed Looped and split from the game on 2026-10-20 (`looped/` never includes `bar/`).
