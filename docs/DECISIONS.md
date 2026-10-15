# Decisions and lessons learned

The record of what was chosen, why, and what was tried and failed, so no
future session re-litigates or repeats it.

## Tech choices
| Decision | Choice | Why |
|---|---|---|
| Language | C++20 | User asked for C++, not JavaScript |
| Graphics API | WebGPU (via Dawn's `webgpu_cpp.h`, emdawnwebgpu port) | Modern, compute shaders, iOS 26 Safari supports it |
| Compiler | Emscripten **6.0.12** (pinned in CI and CLAUDE.md) | C++ → WebAssembly |
| Build | CMake + Ninja | Standard, incremental |
| Shaders | WGSL files in `shaders/`, embedded into the binary | The only shader language WebGPU accepts |
| Engines/libraries | None. Everything from scratch | User's explicit wish; discuss any exception first |
| Assets | Generated in code where possible; third-party only with a license and credits | Public repo |
| Async setup | Callbacks with `CallbackMode::AllowSpontaneous`, no Asyncify | Smaller and faster |
| Threads | None (yet) | Pages can't send COOP/COEP headers |
| Pixel ratio | Capped at 2 | Phone GPU budget, little visible loss |

## Hosting and deploy (important, it took several tries)
- The site is served by **GitHub Pages: "Deploy from a branch" → `gh-pages` / root**.
- `.github/workflows/deploy.yml` builds on every push to `main` and
  **force-pushes the built site to `gh-pages`**, using `GITHUB_TOKEN` with
  `contents: write`.
- **Not used:** the "GitHub Actions" Pages source (`actions/deploy-pages`).
  It failed because Pages was off, and the user couldn't find that setting
  on the phone.
- Claude **cannot** change Pages settings from a session (the GitHub API path
  is blocked by the proxy). The user must do it at
  https://github.com/Barcode162/3D/settings/pages (that direct link works on
  the phone; the normal menus are hidden in mobile view).
- The repo name is **`Barcode162/3D`** (capitals). The site URL is
  **https://barcode162.github.io/3D/**.
- The repo is **public**.
- `index.html` gets the commit SHA stamped in (`__BUILD__`), which shows as
  "build xxxxxxx" and cache-busts `app.js`/`app.wasm`.
- To confirm a deploy from a session: `git fetch origin gh-pages` and check
  the commit message, or `curl` the site and grep `id="build"`.

## Workflow agreements with the user
- Push straight to `main` after every working step. No PRs; the user is on a phone.
- Keep explanations beginner-friendly and short. Say what they'll see on the phone.
- The user wants to be able to **start a fresh chat at any time** and resume
  from the repo alone. Keep `PROGRESS.md` current in every commit.

## Measured results
- Triangle (1.1): 60 fps on iPhone 15 Pro Max, Apple GPU, 860x1422 @2x.
- Bar blockout (184 objects, 8 lights, one draw call per object): **60 fps on
  iPhone**, 860x1550 @2x. Lots of headroom.
- iOS 26 Safari reports `iPhone OS 18_7` in the user agent (frozen UA), with
  `Version/26.3`. This is normal.

## Gotchas hit so far
- A global variable named `app` clashes with `namespace app`. The global is `g_app`.
- The on-screen log panel covers the bottom of the screen and takes touches.
  It's hidden by default and opens automatically on errors.
- Safari "Request Desktop Website" mode makes the user agent look like a Mac. Use mobile mode.
- Auto mode blocks `git push -f` from the session. CI does the force-push to `gh-pages` instead.
