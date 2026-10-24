# The technology: every language and outside tool

What J Production is built with, where each piece is used, why, whether it's
ours, and whether it runs for players (runtime) or only to make and check
things (build time). Part of the Handbook (facts; how to work is in
`docs/playbook/`). The rule behind it (the Charter, the user's rules): **no
outside engines or libraries**: the engine, the game and the tools are
written from scratch; an exception needs the user's yes first. Every
third-party asset needs a licence that allows it and a credit entry.

## Languages (all ours)

| Language | Where | What it does |
|---|---|---|
| C++20 | `looped/`, `bar/`, `bar/projects/{chess,racing}`, `cloud/sim/`, `tools/native/` | the engine and the game, built to WebAssembly; the same code runs natively in the tests and in the room's physics. 2-space indent, `namespace engine` in `looped/`, logs with `LOG_INFO` / `LOG_ERROR`. |
| WGSL | `looped/shaders/` | the GPU's programs (drawing, light, post), built into the game. |
| JavaScript | `bar/web/`, `cloud/`, `ui/`, `dev/`, `studio/`, `toolbelt/`, `hub/`, `bar/projects/{mania,meridian,platformer}`, `tools/` | the page's bridges (menus, audio, network, voice), the room server, the look kit, the tools' pages, and every work tool and test (Node). |
| PHP | `server/`, `homepage/`, `hub/`, `dev/`, `studio/`, `toolbelt/ui-editor/`, `news/` | the site and every store's API on the IONOS hosting (small JSON files, no database). |
| HTML and CSS | the pages | the pages and their looks (`ui/looped.css`, `ui/themes.css` from the themes). |
| JSON | `assets/world/`, `ui/system/`, `docs/plan/`, `docs/charter/rules.json`, `toolbelt/tools.json`, `docs/handbook/*.json` | data: the world, the themes and screens, the plan, the rules, the lists. |
| Python | `tools/assets/` (Blender scripts, textures), `incoming/meridian/blender/` | makes the 3D models and textures; never runs for players. |
| shell | `tools/setup.sh`, `tools/native/run.sh`, `tools/githooks/`, `cloud/sim/build.sh`, the workflows | setting up, building, the locks. |
| The script language | (planned: road item T1) | our own small, safe language for things in the world. |

## Outside tools and services

| Tool | Version | Where | Why | Runtime or build | Ours? |
|---|---|---|---|---|---|
| Emscripten (with the emdawnwebgpu port: Dawn's C++ WebGPU API) | 6.0.12 (`tools/setup.sh`, `deploy.yml`) | builds C++ to WebAssembly | the only way C++ reaches the browser | build | no (open source) |
| CMake + Ninja | the machine's | the game's build (`CMakeLists.txt`) | builds and copies the site into `build/site/` | build | no |
| WebGPU (the browser's) | iOS 26 Safari, Chromium | drawing | the phone's GPU from the web | runtime | the browser |
| Web Audio, WebRTC, WebSocket | the browser's | sound, voice chat, the room connection | built into every browser | runtime | the browser |
| astcenc (ARM, Apache 2.0) | 5.2.0 | `tools/assets/astc_pack.py` | compressed textures for the phone | build | no |
| Blender (headless) | 4.5.14 (`tools/assets/get_blender.sh`) | `tools/assets/*.py` | makes the 3D models from scripts | build | no |
| Python PIL | the machine's | `tools/hub.mjs post` | shrinks news pictures | build (Claude's side) | no |
| ffmpeg | the machine's | `tools/rhythm_test.mjs` | a short copy of a song for the test | test | no |
| Playwright + Chromium | 1.56.1 on GitHub; pre-installed here | every browser test | opens the pages and the game headless | test | no |
| Node.js | 22 | every tool and test | runs the tools | build and test | no |
| wrangler (Cloudflare) | 4.42.0 (`cloud/package.json`) | `cloud/` | runs the room locally for tests and deploys it | build and test | no |
| Cloudflare Workers + Durable Objects | compatibility date 2025-10-01 | the room server and the ledger | one small server per room, real time | runtime (the service) | no |
| IONOS web hosting (PHP, SFTP) | the host's PHP | joshuavelasco.com | the user's domain: the bar, the site, the stores | runtime (the service) | no |
| lftp | Ubuntu's | the workflows | uploads to IONOS over SFTP | build | no |
| GitHub (repo, Actions, Pages) | `actions/checkout@v4`, `actions/cache@v4` | the repo, CI, `gh-pages` | keeps the code, builds, tests, deploys | build | no |
| three.js | 0.169.0 (Meridian), 0.161.0 (the platformer), from jsDelivr | `bar/projects/meridian/`, `bar/projects/platformer/` | **the approved exceptions** (user, 2026-10-18 and 2026-10-19): only inside those two games | runtime | no |
| Claude Code | | this workshop | the AI that builds everything (docs/playbook/) | build | no |

## Assets from outside (each with its licence and credit)

| What | Where | Credits |
|---|---|---|
| Music | `assets/music/` | `assets/music/CREDITS.md` |
| MIDI songs | `assets/midi/` | `assets/midi/CREDITS.md` |
| 3D models | `assets/models/` | `assets/models/CREDITS.md` |
| Fonts (Archivo, Martian Mono, Limelight, Righteous, Unbounded: SIL Open Font Licence) | `ui/fonts/` | `ui/CREDITS.md`, `ui/fonts/OFL-*.txt` |
| Meridian's and the platformer's own assets | `bar/projects/meridian/`, `bar/projects/platformer/` | their `CREDITS.md` |
