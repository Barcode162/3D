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
| Text | Own 5x7 pixel font in code (`engine/font.cpp`) → signed distance field atlas | No font files or libraries; SDF keeps it crisp at any size and gives outlines for free |
| Textures | Generated in code at startup (`engine/procedural.cpp`), 512px, texture arrays with mips; projected triplanar in object space | No image files in the repo; meshes need no UVs; object-space keeps textures stuck to objects once they can move |
| Audio | Browser Web Audio for output; C++ decides what plays and where (thin JS bridge `web/audio.js`). Music streams through an `<audio>` element → `PannerNode` (HRTF) | Like WebGPU for graphics: the platform API is the "driver". Streaming avoids decoding 3-minute songs into ~60 MB of memory each |
| Mirrors | Planar reflection: scene rendered with `viewProj * reflection` into a screen-sized texture, scissored to the mirror's screen rectangle; glass reads it 1:1 with `textureLoad` | Exact, simple, and cost scales with how much of the screen the mirror covers. Avatars-only mode skips the room (VRChat's cheap mirror) |
| Avatar | Procedural skinned mesh: rings (tubes) along the limbs and torso + ellipsoids for the head, each vertex weighted to 2 bones, blended near joints; per-vertex color/gloss/texture layer | No model files needed; one continuous mesh bends cleanly at elbows/knees. A GLB/VRM loader can feed the same skinning path later |
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
- Mirrors + skinned avatar + walk animation (`fa20592`): **60 fps on iPhone**,
  even standing at the full mirror (room drawn twice).
- Triangle (1.1): 60 fps on iPhone 15 Pro Max, Apple GPU, 860x1422 @2x.
- Bar blockout (184 objects, 8 lights, one draw call per object): **60 fps on
  iPhone**, 860x1550 @2x. Lots of headroom.
- iOS 26 Safari reports `iPhone OS 18_7` in the user agent (frozen UA), with
  `Version/26.3`. This is normal.

## Build ourselves vs. use the platform
The rule: we write the engine logic ourselves (rendering, math, physics,
animation, game logic, DSP for the music studio), but we use the
browser/phone APIs that are the only way to reach the hardware: WebGPU
(GPU), Web Audio output and its built-in AAC decoder (speakers), touch
events, file picker, network. Those are "drivers", not engines. Writing
our own AAC/MP3 decoder is possible but has no visible benefit. 
**User's answers (2026-10-15):**
- Song decoding: use the phone's built-in decoder. Test with songs the
  user downloads.
- Font: keep our own big pixel font (the user loves it).
- Own songs: the phone's file picker.
- Physics, avatar with mirror, and animations: yes, all ours.
- Order: jukebox menu → mirrors (full + avatars-only) → avatar (bones,
  shapes, textures) → animation → physics.
- Not answered yet: free third-party 3D models vs. everything from code;
  multiplayer server; voice chat.

## Gotchas hit so far
- Avatar rotation signs (avatar faces -Z, +X is its right side): rotating
  a limb by a positive angle about +X swings it forward; knees bend with a
  negative angle; a positive angle about +Z moves the right arm outward.
  The walk is phase-driven by distance moved (1.5 m per cycle), not time,
  so steps match speed and walls stop the legs.
- First-person body: the head and neck bones get a "collapse" skinning
  matrix (scale 0, moved to the base of the neck), so the same mesh draws
  headless with no extra geometry. The camera (your eyes) stays where it
  is; the body is placed so its eyes match it, and looking down pivots the
  eyes forward around the neck (`avatarTransform` takes the pitch).
  Without that shift, the chest hides your feet on a narrow portrait screen.
- Skinned clothes textures use the *rest-pose* position for triplanar
  mapping, so the pattern sticks to the body as limbs move. Skinned
  shading skips bump mapping (the bump would need per-vertex bone rotation).
- In WGSL, `textureSample` must not sit behind a per-pixel branch:
  `fs_skinned` always samples, then decides whether to use it.
- Shape builders (`engine/skinned_mesh.cpp`) flip each triangle to face
  its vertex normals, so winding mistakes can't happen there.
- Reflected views flip triangle winding: the mirror pass uses a second
  scene pipeline with `FrontFace::CW`. Things behind the glass (the wall,
  the frame) are dropped by a clip plane in `scene.wgsl` (`discard`).
- Each camera view (main + one per mirror) needs its own frame uniform
  buffer: `WriteBuffer` calls all land before the single submit.
- iOS opens the file picker only from a real tap on a real element. The
  jukebox menu draws its "Load song" button in WebGPU, and `web/audio.js`
  places an invisible `<label for=songPicker>` exactly over it while the
  menu is open (`setSongPickerRect`).
- Solid rectangles (menu panels) go through the text pipeline: the font
  has a hidden solid-block glyph (char 127, `font::kSolid`).
- In headless tests, `page.setInputFiles('#songPicker', files)` simulates
  picking songs.
- The Playwright test Chromium can't play AAC (`.m4a`). To test music
  headless, copy an Ogg/Opus version over the file in `build/site` only
  (ffmpeg is installed). Safari plays AAC fine.
- iOS only lets sound start inside a tap. `web/audio.js` unlocks on the
  first touchend (creates the AudioContext, plays a moment of silence in
  each music element); later `play()` calls from C++ then work.
- The error log panel opens on errors and covers the lower screen, so
  taps there go to the panel, not the game.
- Texture color maps store a *multiplier*, halved (0.5 = keep the object's
  color), so one wood texture serves dark and polished wood. Brick colors
  are absolute: give brick objects a mid-gray (0.5) color.
- A material enum named `kLeather` clashed with the color `kLeather`; the
  texture enum uses `kTex...` names.
- A fresh session can start on a detached HEAD with a stale local `main`.
  If `git push` is rejected, check `git status -sb`; if HEAD is ahead of
  `origin/main`, push with `git push origin HEAD:main`.
- Text vertices are placed in clip space on the CPU (screen text at depth 0),
  so one pipeline and one draw call cover both screen and world text. World
  text is depth-tested but doesn't write depth.
- A global variable named `app` clashes with `namespace app`. The global is `g_app`.
- The on-screen log panel covers the bottom of the screen and takes touches.
  It's hidden by default and opens automatically on errors.
- Safari "Request Desktop Website" mode makes the user agent look like a Mac. Use mobile mode.
- Auto mode blocks `git push -f` from the session. CI does the force-push to `gh-pages` instead.
