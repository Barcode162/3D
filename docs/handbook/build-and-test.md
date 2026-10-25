# Build and test

Facts about building the game and checking it (when to do what: the
Playbook). The checks themselves: [the checks](systems/tools/checks.md).

## Building
```sh
bash tools/setup.sh                  # Emscripten 6.0.12, astcenc, the room's wrangler; runs at every session start
source /opt/emsdk/emsdk_env.sh
emcmake cmake -B build -G Ninja      # once
cmake --build build                  # → build/site/ (the bar's whole site)
```
`node tools/project.mjs check build` builds and lints `EM_ASM` (quoted names:
the release build renames the rest). The models are made in headless
Blender (`tools/assets/`, recipe 10); compressed textures at build time
(`tools/assets/astc_pack.py`).

## Checking
- `node tools/project.mjs check` — the checks covering everything changed since `main`; `check changed` only what isn't committed; `check all` everything; `check <systems…>` by name (the list: `tools/lib/checks.mjs`).
- Each check's log: `build/check/<name>.log`.
- Native (no browser): `bash tools/native/run.sh` — maths, JSON, physics, cars, races, the world, light, chess, the protocol.
- **On GitHub, the long browser tests run only when asked** (the user, request 2026-10-25 #15: they test on the phone): Actions → Build and deploy → Run workflow → tick "tests". Every deploy still builds, runs the quick checks and the room's tests, and goes live; Dev → Overview then shows "browser tests on request".
- Browser tests open the pages in headless Chromium with software WebGPU (SwiftShader): right for looks, never for speed. With `TEST_LIGHT=test` the bar opens with the tests' lighting.
- Pictures: the piece tool takes before/after pictures of the bar's spots (`tools/lib/spots.mjs`) into the gallery; `OUT=build/before node tools/shots.mjs spots` and `node tools/imgdiff.mjs build/before build/after` compare by hand.
- After a deploy: `EXPECT_SHA=<sha> node tools/check_live.mjs` (`node tools/project.mjs live`) opens the live bar headless (out of the real room unless `NET=1`).
- Speed is judged only on the phone (`?perf=1`, Send report).

## Page options (the bar's address)
Read once at start-up (`bar/test_hooks.h`); the page's own: `?skipintro`, `?server=`, `?cloud=`, `?room=`, `?name=`, `?testerror`.

| Option | What it does |
|---|---|
| `?cam=x,y,z,yaw,pitch` | start the camera there (degrees) |
| `?still` | no intro text, no idle motion (for picture diffs) |
| `?place=home` | another place |
| `?perf=1` | the performance overlay |
| `?quality=auto\|high\|medium\|low` | Settings → Graphics forced |
| `?mirrorq=0\|1` | mirrors full or simple |
| `?light=classic` / `?light=test` | the old lighting / the tests' lighting |
| `?reflect=0`, `?bounce=0`, `?exposure=`, `?eyekeep=` | light switches ([the manual](manual/light.md)) |
| `?sit=N`, `?emote=N`, `?pianosong=N`, `?rhythm=song,difficulty`, `?autoplay` | start in a seat, an emote, a piano song, 4K Mania |
| `?shift`, `?darts`, `?chess`, `?garage`, `?pad`, `?telemetry=1`, `?coin` | a bartending shift, darts, chess, a rig's garage, the skid pad, the race overlay, the coin |
| `?studio=1` | studio mode (the Studio frames the bar with it) |
| `?walkpose=`, `?walkcheck`, `?previewturn=` | walking and customizer checks |
| `?demo` (Dev) | Dev with sample data, no login |

## Conventions
C++20, 2-space indent, `namespace engine` in `looped/` (which never includes
`bar/`); the Dawn C++ API (`<webgpu/webgpu_cpp.h>`), asynchronous setup with
callbacks (not Asyncify); logs with `LOG_INFO` / `LOG_ERROR`; the pixel ratio
capped at 2; no outside engines or libraries without the user's yes
([technology](tech.md)).
