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
| Assets | Real modeled assets as GLB files in `assets/`: CC0 models/textures (Poly Haven, ambientCG), custom ones Claude makes with offline tools (Blender headless is a *tool*, like a compiler, never a runtime dependency), and the user's own GLBs. Every third-party file gets a license + credit entry. Procedural code stays for things that are better generated | Decided 2026-10-15: code-built boxes/cylinders look "basic AI 3D"; user wants highest quality |
| Async setup | Callbacks with `CallbackMode::AllowSpontaneous`, no Asyncify | Smaller and faster |
| Threads | None (yet) | Pages can't send COOP/COEP headers |
| Text | Own 5x7 pixel font in code (`engine/font.cpp`) → signed distance field atlas | No font files or libraries; SDF keeps it crisp at any size and gives outlines for free |
| Textures | Generated in code at startup (`engine/procedural.cpp`), 512px, texture arrays with mips; projected triplanar in object space | No image files in the repo; meshes need no UVs; object-space keeps textures stuck to objects once they can move |
| Audio | Browser Web Audio for output; C++ decides what plays and where (thin JS bridge `web/audio.js`). Music streams through an `<audio>` element → `PannerNode` (HRTF) | Like WebGPU for graphics: the platform API is the "driver". Streaming avoids decoding 3-minute songs into ~60 MB of memory each |
| Mirrors | Planar reflection: scene rendered with `viewProj * reflection` into a screen-sized texture, scissored to the mirror's screen rectangle; glass reads it 1:1 with `textureLoad` | Exact, simple, and cost scales with how much of the screen the mirror covers. Avatars-only mode skips the room (VRChat's cheap mirror) |
| Avatar | Procedural skinned mesh: rings (tubes) along the limbs and torso + ellipsoids for the head, each vertex weighted to 2 bones, blended near joints; per-vertex color/gloss/texture layer | No model files needed; one continuous mesh bends cleanly at elbows/knees. A GLB/VRM loader can feed the same skinning path later |
| Materials | `engine::Material` (look + physics + sound) in `engine/material.h`; the bar's library in `app/materials.cpp`, objects reference a `MaterialId` and may override color/glow/texture scale | One definition per kind of stuff feeds every engine (see World-simulation principles) |
| Shading | PBR metal/roughness: GGX + Smith + Schlick, Karis sphere-light widening (`PointLight::radius`), Karis env-BRDF approximation against a fake "room glow" (`roomGlow` in `scene.wgsl`) | Same model as glTF, so loaded assets look right. Diffuse leaves out 1/pi (keeps the old light tuning), so specular is scaled by pi |
| Model files | glTF 2.0 binary (.glb), own loader: `engine/json.*` (parser), `engine/gltf.*` (meshes, node tree flattened into model space, PBR materials, tangents computed if missing), `engine/model.*` (GPU upload, per-material bind group). PNG/JPEG decoded by the browser (`createImageBitmap`, `colorSpaceConversion: 'none'`) in `engine/fetch.cpp` | Standard format every tool exports; image decoding is a platform "driver" like AAC |
| Model placement | `BarScene::models` (`ModelPlacement`: file, position, rotation, scale, `MaterialId` for feel/sound). The file gives the look | Look from the asset, physics/sound from the material library |
| Physics | Own engine (`engine/physics.*`): moving bodies = compound spheres (cheap, stable contacts), world = oriented boxes; sequential impulses with Coulomb friction, restitution above 0.4 m/s, split position correction; fixed 1/60 s steps x 8 substeps, max 8 steps per frame; sleeping. Mass = material density x real solid volume | Stable on a phone without a full convex-collision system; materials drive weight, grip and bounce |
| Glass | glTF `alphaMode: BLEND` → see-through pipeline: premultiplied alpha, no depth write, sorted back to front, opacity rises at grazing angles (Fresnel) | Real glass look without refraction cost |
| Pixel ratio | Capped at 2 | Phone GPU budget, little visible loss |

## Big bar upgrade engine pieces (2026-10-16)
- **Loading screen** (`#loading` in `web/index.html`): covers the room until
  every model file is in. C++ (`updateLoadingScreen` in `app/main.cpp`)
  reports progress through `window['barLoading']` (downloads via
  `emscripten_async_wget2_data` progress, 85%; texture decoding the rest),
  then the page shows "Tap to enter" (that tap also unlocks sound) and
  calls `_app_enter`, which starts the intro text. `?skipintro` enters by
  itself; the test scripts use it and wait for the log line
  "Everything loaded".
- **Gotcha:** EM_ASM code goes through the JS minifier, which renames
  `window.foo.bar`. Always write `window['foo']['bar']` there.
- **Light grid**: up to 32 lights (`Lighting` uniform, group 0 binding 4,
  written once). The floor is split into ≤ 16 x 16 cells over
  `BarScene::lightGridMin/Max`; each cell lists the ≤ 8 lights whose range
  reaches it, brightest first. The log says if any cell had more than 8
  (then the faintest are dropped there, which can show as a seam: shrink
  ranges). Model placement limit raised to 512.

- **Gzipped models**: the build (`site_assets` in CMakeLists) gzips every
  `.glb` (`gzip -9 -n`), the engine asks for `<file>.glb.gz` and unpacks it
  with the browser's `DecompressionStream` (`engine/fetch.cpp`, a `fetch()`
  with progress). Files starting with 1f 8b are unpacked; anything else is
  used as is, so a server that adds `Content-Encoding: gzip` (Apache on
  IONOS might) still works. Geometry-heavy models shrink to ~40-55%.
- **Glowing images** (screens, lit windows, signs): glTF `emissiveTexture`
  is read (`ModelMaterial::emissiveImage`, binding 7); `common.glow_image_material`.
  See-through glow (flames, `common.glow_cutout_material`, exported as PNG
  with `image_format="AUTO"`) is multiplied by alpha in `litModel`.
- **Floors**: `BarScene::floors` (raised rectangles, the stage and its
  step); the eye height follows `groundHeight` smoothly.
- **Mirror modes**: one mirror, tap the glass to step full → avatars only
  → off, or press a panel button (`Action::kMirrorMode`, buttons are
  separate models lit with `setModelBaseGlow`). Shared as `m<i>` =
  [on, avatarsOnly].
- **Zone models in world space**: each zone script builds its model
  where it stands in the room (model space = world space, placed at the
  origin), using `tools/assets/layout.py` for the shared floor plan
  (`app/bar_scene.cpp` repeats the numbers). The piano, sim rig and
  rhythm corner are built locally and placed/turned by the app.
- **Clutter**: `tools/assets/props_lib.py` builds bottles (11 shapes,
  16 glass colors and 16 labels through atlases), glassware with drinks,
  bar tools, garnish, books, frames, plants, candles. Set atlas UVs
  *before* moving a prop into place (`swatch_uvs` reads positions).

## Rhythm game (4K MANIA cabinet, 2026-10-16)
- Tap the cabinet → `app/rhythm.*` covers the screen (drawn with the text
  renderer: rects, text, and two new SDF shapes in the font atlas, a disc
  and a ring: `TextRenderer::addShape`). Circle skin at the user's request.
- Songs = the jukebox's list (built-in + songs loaded from the phone).
  Charts are made in C++ from the audio (`app/rhythm_chart.cpp`): own FFT,
  spectral flux in 4 bands, tempo by autocorrelation (preferring ~120
  BPM) and beat phase, notes snapped to beats / eighths / sixteenths for
  Easy / Normal / Hard, lanes from the spectral centroid (quartiles, so
  all lanes are used), no fast repeats on one key, chords on loud hits.
- The song is decoded fully (`engine::Track`, `trackLoad` in
  `web/audio.js`) and played as an AudioBufferSourceNode, so timing is
  sample-exact: song time = `ctx.currentTime - start - latency`. Touches
  come from `Input` capture mode with their time (`emscripten_get_now`);
  a press's song time = now's song time minus how long ago it happened.
  Windows: perfect 45 ms, great 90, good 135, miss 180. The jukebox is
  turned down (`setMusicGain`) while the game is open.
- Lanes are full-height screen columns (easy for thumbs); the X button is
  top-left. Keyboard: D F J K, Escape.
- Test: `NODE_PATH=$(npm root -g) node tools/rhythm_test.mjs [song] [diff]`
  (puts Ogg copies of the songs into build/site, opens
  `?skipintro&rhythm=song,diff&autoplay`, screenshots `build/rhythm-*.png`).

## Accounts (2026-10-16)
- User asked for sign up / log in, a profile picture, a bio and a
  "person's card", and seeing name + picture over people's heads.
- **Server:** `server/account.php` next to `relay.php` on IONOS (the
  deploy uploads both). No database: JSON files in `accounts/` (private,
  `.htaccess` → 403): `users/<id>.json` (bcrypt password hash, profile),
  `names.json` (username → id, flock'd), `tokens/<sha256>.json` (login
  sessions, newest 10 per user), `avatars/<id>.jpg` (served by
  `account.php?avatar=<id>&v=<rev>` with CORS, so github.io can use them).
  Failed logins wait 0.4 s (slows guessing).
- **Profile fields:** display name (20), bio (240), favorite drink and
  song (40 each), card color (8 choices), picture (256 px JPEG, cropped to
  a square in the browser before upload), join date.
- **Client:** `web/account.js`, plain HTML forms over the game (the
  phone keyboard and photo picker need real form fields; the 3D world is
  still all ours). Token in localStorage; the loading screen shows who's
  signed in (or Log in / Sign up), the HUD has a name/"log in" button.
- **Relay:** a logged-in player sends `tok`; the relay checks it (then
  every 20 s), uses the account's display name and passes `acct` and `av`
  (picture revision) to the others, so names can't be faked.
- **In the world:** `engine/billboard.*` draws round pictures facing you
  (texture array, 8 slots, colored rim) over other players within 7 m,
  above their name. Tapping a player opens their card
  (`bar3dAccount.showCard`). Guests get a simple card.
- Gotcha: the game's key handler must ignore keys while a text field has
  focus, or W/A/S/D can't be typed (`onKey` checks document.activeElement).
- Test: `NODE_PATH=$(npm root -g) node tools/account_test.mjs` (two
  players, local PHP; screenshots `build/acct-*.png`).

## The "make everything work" pass (2026-10-16)
The user asked for voice chat, a settings menu, emotes, avatar switching,
detailed profiles, MIDI piano with 10 classical songs, usable seats for
everyone, darts, a racing game on the sims, 4K Mania visible to others,
one player per machine, a bartender badge and drinks, "everything
interactable", all crude but working, with a remaster list
(`docs/REMASTER.md`).
- **Menus are HTML** (`web/menu.js`), like the account screens: sliders,
  toggles and file pickers need real form controls on the phone. They
  queue commands for C++ (`bar3dUi.take`, read in `engine/ui.*`), and C++
  tells them the state as JSON (`bar3dUi.setInfo`). Settings live in
  localStorage (`bar3dSettings`); C++ re-reads them when `version()` changes.
- **Player state grew to 18 numbers** (`app/players.h` `slot::`): activity,
  its parameter (seat, machine), avatar look, talking level, emote +
  counter, 5 activity numbers (race position/steer, darts, rhythm screen),
  held drink, flags (bartender, on the mic). The relay's MAX_VALUES is 20.
  Append new slots at the end only.
- **Who uses what** is read from the others' states (no locks): a seat or
  machine is busy if another player's activity says so. If two take the
  same seat at once, the lower relay id keeps it.
- **Seats** (`BarScene::seats`): hip point, facing, how low; the body is
  placed by `seatedTransform`, the eyes by `seatedEye`. While seated the
  head turns at most ~110 degrees from the seat's facing.
- **Small interactables** have `direct = true`: they only count when the
  middle of the screen points at them (or you tap them), so walking past
  doesn't keep popping prompts.
- **Moving boxes** (`SceneRenderer::setBoxes`, up to 320, one draw each):
  piano key lights and strings, darts, drinks. **World quads**
  (`TextRenderer::addWorldQuad`) draw games onto screens in the room.
- **Sounds made in C++** (`engine/synth.*`): piano notes (additive,
  inharmonic partials, 3 detuned strings, made lazily per key at 22 kHz),
  Karplus-Strong guitar, bell, dart thunk, clink, pour, shaker, click,
  engine hum (loops), knock, cheer. Played through `engine::Sound`
  (`soundPlay` in `web/audio.js`), on buses (effects, piano, music, voice)
  whose volumes follow the settings.
- **MIDI**: own parser (`engine/midi.*`: formats 0/1, tempo map, sustain
  pedal, drums skipped). Songs from the Mutopia Project (public domain or
  CC BY-SA, `assets/midi/CREDITS.md`). Playback is scheduled ahead on the
  audio clock (lookahead grows when frames are slow, or notes would be
  skipped); a song is shared as world entry `pso` = [song, server start
  time] so everyone hears the same bar. Live notes are events `pk`
  [key, velocity, on]. Web MIDI keyboards: `navigator.requestMIDIAccess`
  (not on iPhone Safari).
- **Events** (`engine::net::sendEvent`): quick one-offs (piano keys, a bell
  ring) go over the WebRTC data channel when there is one, else through
  the relay's new message inbox (`"m"` in, `"inbox"` out).
- **Voice chat** (`web/voice.js`): one RTCPeerConnection per pair of
  players (the lower id calls), audio transceiver + data channel from the
  start, mic attached later with `replaceTrack` (no renegotiation). The
  handshake is "vanilla ICE": wait for candidate gathering (max 2.5 s),
  then send one offer/answer through the relay (`v:` messages). Opus asked
  for 510 kbps, FEC, no DTX (SDP fmtp + `maxBitrate`). Mic chain: source →
  gain (Mic boost) → analyser + gate gain → MediaStreamDestination. The gate
  opens above the threshold and holds for 15 checks. Remote voices: muted
  `<audio>` element (keeps the stream flowing) + MediaStreamSource →
  HRTF panner at their head → "voice" bus; "3D voices" off = plain element.
  Stage mic flag = refDistance 25 (heard everywhere).
- **Badges** (`server/account.php`): `badges` per user; owners (first
  account, or `accounts/admins.json`) give them in the profile. The client
  checks `bar3dAccount.hasBadge`.
- **Shared world keys added**: `pso` piano song, `lt` [lights dim, fire
  on], `dk0`..`dk7` drinks on the counter (type + 1, 0 = empty).
- Tests: `tools/voice_test.mjs` (two players through a local relay, fake
  mic: WebRTC connects, voice level arrives, data channel open, Bob seen
  sitting), `tools/shots.mjs "sit=0" "darts" ...` (screenshots with page
  options: `?sit=n`, `?emote=n`, `?pianosong=n`, `?darts`, `?shift` with
  `?badge=bartender` on localhost only).
- **Gotcha:** inside EM_ASM, `\.` and `\d` in regexes are eaten (the code
  becomes a C string). Write `[.]` and `[0-9]`.
- **Gotcha:** headless pages run at ~1-2 fps and background tabs slow
  their timers; anything timed in JS (the noise gate) must count ticks,
  not milliseconds.

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
- Asset pass (`964adfe`): showed **30 fps**, but that was **iOS Low Power
  Mode**, which caps Safari at 30 fps. Always ask whether Low Power Mode
  is on before chasing a 30 fps reading.
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
- Third-party models vs. code: answered 2026-10-15, real assets, mixed
  sources (see the Assets row and VISION.md → Art upgrade). The user lets
  Claude decide details and only reviews.
- Multiplayer: yes, now (2026-10-15). The user bought IONOS hosting to
  use as the server, for them and their brother. Voice chat: not yet.

## Multiplayer (2026-10-15)
- The user has **IONOS Web Hosting Plus** (paid for a year): Apache + PHP
  + SFTP, no long-running processes, so **no WebSocket server**. Chosen
  instead: a **PHP relay** (`server/relay.php`). Each phone POSTs its state
  (eye x,y,z, yaw, pitch) every 100 ms and gets back everyone else's latest.
  Rooms are JSON files in `rooms/` next to the script (flock'd, private via
  `.htaccess`); a player who stops sending is dropped after 5 s.
  Fine for 2-8 players. If it ever feels laggy: M3 = WebRTC data channels
  phone-to-phone, with the PHP relay only for the handshake.
- Requests are sent as `text/plain` so browsers skip the CORS preflight;
  the relay answers with `Access-Control-Allow-Origin: *`, so the GitHub
  Pages copy of the game can use the IONOS relay.
- Smoothing (`app/players.cpp`): others are played back 250 ms in the past,
  interpolated between received states, on the sender's clock (offset
  estimated from the fastest delivery). Their walk animation comes from how
  far they move, like your own.
- Rendering: `SceneRenderer::setOthers` draws up to 7 extra copies of the
  avatar mesh (own object slot + bone buffer each), in the room and in both
  mirrors. Name tags are world text over their heads.
- Deploy: `deploy.yml` → "Upload to IONOS" (only when the
  `IONOS_SFTP_HOST/USER/PASSWORD` secrets exist) mirrors `build/site` +
  the relay into `home/bar/` on the webspace with lftp, plus the landing
  page into `home/`. So the game is also at `https://joshuavelasco.com/bar/`,
  and there it uses `relay.php` next to itself.
  On github.io it uses `DEFAULT_SERVER` in `web/net.js`.
- IONOS account facts: SFTP host `access-5021658795.webspace-host.com`
  (port 22), user `su454332` (password only in GitHub secrets). System
  domain `s1114537466.onlinehome.us` answers on http only (no SSL), and the
  webspace said "No domain connected". IONOS offers a free
  `s1114537466.online`. **HTTPS is required**: Safari only allows WebGPU on
  https pages, and the https github.io page can't call an http relay.
  The container can't reach port 22 (proxy); only GitHub Actions uploads.
- **Domain: `joshuavelasco.org`** (IONOS, 2026-10-15). It points straight
  at the `bar/` webspace folder, so the game is at the domain root and the
  relay at `https://joshuavelasco.org/relay.php` (`DEFAULT_SERVER` in
  `web/net.js`, used by the github.io copy). Tested live over http: health
  check OK, two headless players through it OK, ~85 ms per request,
  `rooms/` gives 403. IONOS sends `x-ws-ratelimit-limit: 1000` (seems per
  second; fine). HTTPS was not on yet at first (TLS "internal error" = no
  certificate assigned to the domain in IONOS → Domains & SSL).
- **Domain now `joshuavelasco.com`** (2026-10-15; `.org` dropped). It
  points at the webspace folder **`home/`** (the old `bar/` folder is no
  longer served). HTTPS works. See "One domain, many projects" below.
- Page options: `?room=name` (separate bars), `?name=Josh` (remembered),
  `?server=URL` or `?server=off`.
- Testing: `NODE_PATH=$(npm root -g) node tools/mp_test.mjs` (needs `php`)
  opens two players and saves `build/mp-bob.png`. Gotcha: don't pause
  sending on `document.hidden`; headless background tabs count as hidden
  (and iOS pauses timers in the background anyway).

## Shared world (M2, 2026-10-16)
- The relay keeps a **world** per room next to the players: entries with a
  short key and up to 16 numbers, each with the writer's id and a
  revision number. A client sends its changes (`w`) and the newest
  revision it has (`since`); the reply carries only entries changed since
  then by others, plus the server clock (`now`). Last write wins. The world
  is forgotten after the room has been empty for 30 minutes.
- Keys: `g<body>` = glass [x,y,z, quat, velocity, state 0 moving / 1 held /
  2 resting]; `jb` = [song index or -1, server time it started]; `m<i>` =
  mirror on/off.
- Glasses (`app/shared_world.cpp`): the player moving one (holding it, or it
  is flying/sliding after their throw or knock) owns it and sends ~10x/s;
  on rest the final pose is sent once and nobody owns it. Others drive it
  as a **kinematic** body (`Body::kinematic`: no gravity, moved along its
  velocity, immovable to contacts), so it still knocks their own glasses.
  If two players claim the same glass: holding beats flying, else the
  lower player id wins. A glass only gets claimed after it has come to rest
  once since page load (so the start-up settle doesn't overwrite the
  shared positions) and not within 1.5 s of the network placing it.
- Jukebox: only built-in songs are shared (your own files stay on your
  phone). A late joiner seeks to `server now - start` (set on
  `loadedmetadata` in `web/audio.js`). A remote song waits for the first
  tap (sound unlock). When a song ends, everyone advances and writes the
  next song; starts within 2 s count as the same, so nobody restarts.

## One domain, many projects (2026-10-15)
- joshuavelasco.com → IONOS webspace `home/`. Each project is a subfolder
  (`home/bar/` = https://joshuavelasco.com/bar/), deployed by **that
  project's own repo** (its GitHub Action uploads over SFTP with the same
  three `IONOS_SFTP_*` secrets, which must be added to each repo; GitHub
  personal accounts have no account-wide secrets).
- The landing page `homepage/index.php` (in this repo for now, uploaded to
  `home/` by `deploy.yml`) shows one "Projects" button. Its dropdown lists
  every subfolder that has a **`project.json`**
  (`{"name", "description", "order"}`), read on each visit, so a new project
  appears when its first deploy lands; no landing-page change needed.
  Bar3D's is `web/project.json`.
- Deploys only add/replace files (lftp `mirror -R` without `--delete`, `put`),
  so one project's deploy never touches another's folder.
- Could move to its own repo later; then remove the two `put` lines here.

## World-simulation principles (2026-10-15)
- **One material, all senses:** a material record holds look (PBR), physics
  (density, friction, restitution) and sound (impact/scrape synthesis
  parameters, absorption). Objects reference a material; engines read the
  part they need. New engines extend the record instead of adding per-object
  special cases.
- **Simulate, don't script:** prefer a behavior that falls out of a system
  (a glass sounds like glass because it hit something) over a hand-placed
  effect.
- **Phone budget rules:** 60 fps on the iPhone 15 Pro Max is the hard limit.
  A sim that can't fit runs at lower rate, lower resolution, or only near
  the player.

## Asset pipeline (W3)
- Blender 4.5 LTS runs headless in the container: `bash tools/assets/get_blender.sh`
  (installs to `/opt/blender`, ~350 MB, not in setup.sh).
- Textures: `bash tools/assets/fetch_textures.sh <dir> [ids]` (CC0 Poly Haven 1k, via
  the Poly Haven files API, since some sets call the color map "albedo"/"coll1").
- Build a model: `/opt/blender/blender -b --factory-startup --python tools/assets/<name>.py -- <texdir> assets/models/<name>.glb`.
  Shared helpers in `tools/assets/common.py`: beveled boxes, cylinders,
  spheres, wood materials (tint + roughness baked into the images), box UVs
  with the grain along each piece (`grain_axis`), harden-normal bevels.
- Geometry is built y-up in model space, so export uses `export_yup=False`.
- Ngons (cylinder caps) must be triangulated or Blender can't export tangents.
- Rebuild everything in one go: `bash tools/assets/build_all.sh <scratch dir>`
  (installs Blender, fetches textures, runs every script, renders Cycles
  previews of each model to `<scratch>/previews/` for a quick look).
- `finish_and_export` joins all parts that share a material into one mesh,
  so each model is a handful of draw calls (the counter went from 53 to 4).
- Many small things in one model (the ~110 bottles) share one material by
  pointing their UVs at a spot in a small color atlas (`swatch_uvs`);
  labels share one label atlas. One draw call per material, not per bottle.
- Glow: `plain_material(..., emission=rgb, strength=s)`; strengths above 1
  export as `KHR_materials_emissive_strength`, which `engine/gltf.cpp`
  reads. The tone mapper turns strong glow white: neon looks colored at
  strength ~2, bulbs and flames want 8-20.
- Painted text in textures (menus, chalkboard, posters, title cards) uses
  the engine's own 5x7 pixel font, parsed from `engine/font.cpp`
  (`common.draw_text`). Image arrays have row 0 at the bottom (v = 0).
- Box UVs follow `grain_axis`: walls running along z need `grain_axis="z"`,
  or the bricks come out turned 90 degrees.
- Interactables can light up models (`firstModel`/`modelCount`), not just
  scene objects: the renderer writes the glow into the model's slot.
- `CAM="a;b;c" node tools/smoke.mjs` saves several close-ups in one run
  (`build/smoke-cam-1.png`, ...).
- Check a model in the engine with the test camera:
  `CAM="x,y,z,yaw,pitch" node tools/smoke.mjs` → `build/smoke-cam.png`
  (yaw 0 looks toward -z, degrees; positive pitch looks up).

## Hosting split: Cloudflare + IONOS (2026-10-16, user said "let's get Cloudflare going")
- **Cloudflare = everything live:** Workers + Durable Objects (one
  realtime room per bar over WebSockets, server clock, owns seats /
  machines / shared world, later runs races and bots with our own C++
  compiled to WebAssembly), D1 (SQL database: accounts, profiles,
  friends, lap times, leaderboards), R2 (files: profile pictures,
  uploads, big assets), maybe Pages for the game itself later (it can send
  COOP/COEP headers, which unlocks threads / SharedArrayBuffer).
  Start on the free `*.workers.dev` address (no DNS work); later a
  subdomain like `bar.joshuavelasco.com`.
- **IONOS = the "home" side:** domains (joshuavelasco.com / .org), email,
  the landing page / personal site, **accounts (moving from JSON files to
  IONOS MySQL, restructure 5.1)**, and the old PHP relay until it's retired
  (restructure 3.5).
- **Updated 2026-10-16 (restructure):** accounts stay on IONOS (MySQL), not
  D1. D1 holds game data only (lap times, leaderboards, reports). The
  sentence above about "D1 (accounts, profiles...)" is superseded.
- The user needs to make: a free Cloudflare account, its Account ID and a
  custom API token (Workers Scripts, D1, Workers R2 Storage, Workers KV
  Storage, Cloudflare Pages: Edit), saved as GitHub secrets
  `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID`; the deploy then
  publishes the server with wrangler.

## Restructure (2026-10-16)
The top priority is `docs/RESTRUCTURE.md` (audit of every system plus a
step-by-step plan, one step per session). Its sync invariants (§1.3):
one writer per fact; the server orders every shared change and echoes it
to the writer too; contested things (seats, machines, glasses, drinks) are
claimed from the server; one clock (server time); momentary things are
events, lasting things are state; one backend per deployment (version
mismatch → "New version, tap to reload"); identity = server slot (+
account id); the server detects ghosts; join = full snapshot + deltas;
validate every incoming value. Q1 (Cloudflare account + GitHub secrets)
was answered by the user on 2026-10-17.
- **Server overrides are allowlisted (step 0.1):** `?server=` works only
  for localhost or exact hosts in `ALLOWED_HOSTS` in `web/net.js`
  (`joshuavelasco.com`); `?cloud=` only for localhost until 3.3 adds the
  room's workers.dev host to `ALLOWED_CLOUD_HOSTS`. Never a suffix match.
  Tokens and passwords go only to same-origin or allowlisted hosts.
- **Race determinism** holds only for the identical wasm build: a native
  build can differ from wasm, and the track surface hash uses `sin`
  (`app/race/track.cpp`). Two players must run the same build (steps
  2.4a/2.4b/7.3 add em++ tests, an integer hash and a build hash check).
- **IONOS upload:** `mirror` runs without `--ignore-time` (it skipped edits
  that kept the size), so after a fresh CI build it re-sends everything
  (about 45 MB of assets); the per-build `mput` list comes from
  `build/site/*.js *.json`.

- **Frame profiler (step 0.2, `engine/profiler.*`):** fixed phase enum,
  `PROFILE(Phase::kX)` scopes in `frame()`, `prof::draw()` next to every
  draw call, start-up `stage()` marks. Overlay: Settings → Performance
  overlay or `?perf=1` (bars = time between frames, blue = CPU part, lines
  at 16.7 and 33.3 ms). `window.bar3dPerf` is a getter that builds the JSON
  only when read (a per-second refresh cost ~5 ms headless). Safari rounds
  `performance.now()` to ~1 ms without cross-origin isolation, so single
  phase times are coarse; trust averages. A hitch (> 20 ms between frames)
  is blamed on the previous frame's longest phase.

- **Logs and reports (step 0.3):** `LOG_INFO/WARN/ERROR/FATAL` (engine/log.h)
  call `bar3dLog.add` in `web/index.html` directly (no printf). Ring of 300
  lines, drawn once per frame. Errors show a red badge (tap = log); only
  fatal opens the panel, behind "Something broke - tap to rejoin" (reload,
  keeps the URL options), and sends a report by itself. Audio problems are
  warnings. Reports: Menu → Settings → Send report → POST to
  `report.php` next to the relay → a 6-letter code; read it with
  `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`. Files go
  to `<SFTP root>/private/bar/reports/` (web root is `<SFTP root>/home`),
  10 per hour per IP, 500 max, 30 days. The page removes your name, other
  players' names and long hex strings; the server refuses any 48-hex string.
  `?testerror` logs a test error. Test tools read log lines from the
  panel's child elements (lines no longer start with `[info]`).
- **Profiler `gpu` number:** time from Submit to `OnSubmittedWorkDone`
  (one in flight). It's an upper bound (includes waiting in the queue), but
  the only GPU timing that works without the timestamp-query feature.
- **Web Audio on Safari:** never call `setTargetAtTime` every frame without
  clearing old events; `soundSet` (web/audio.js) sends only 1% changes and
  uses `cancelAndHoldAtTime` first.
- **CI tests (step 0.4):** native tests (g++ + em++), `php -l`, EM_ASM lint
  and the browser smoke run before publishing; a failing step stops the
  deploy. SwiftShader WebGPU works on GitHub's ubuntu runners with
  Playwright 1.56.1 (`npx playwright install --with-deps chromium`), so the
  smoke is blocking (first green run 39007465628). Cloudflare check
  (2026-10-17): secrets set, wrangler accepts the token, workers.dev
  subdomain is `joshuavelasco60` → the room will be
  `bar3d.joshuavelasco60.workers.dev` (step 3.3 adds it to
  `ALLOWED_CLOUD_HOSTS`). Don't print `wrangler whoami` output in CI: it
  shows the account email and the logs are public.
- **Touch events:** never read layout (getBoundingClientRect, element
  size) or change page styles inside a touch handler; touchmove can fire
  120+ times a second on iPhone. The canvas size is measured once per frame
  (`Gpu::cssWidth`) and the joystick drawn once per frame
  (`Input::flushStick`).
- **Touches are passive (2026-10-17):** listeners only queue touches
  (`{passive: true}`, `touch-action: none` on the page stops scrolling and
  zoom); C++ reads the queue once per frame. Non-passive listeners (what
  `emscripten_set_touch*_callback` uses) cost a late frame per tap on
  iPhone. Because nothing cancels the touch, Safari also sends mouse events
  after a tap: mouse input within 1 s of a touch is ignored. Headless tests:
  CDP touch events arrive slowly at 2 fps (a tap looks like a long press),
  so send TouchEvents from inside the page.
- **Pipelines are async (step 1.3):** create them only through
  `engine::pipelines::create` (CreateRenderPipelineAsync); the slot stays
  empty until compiled, so draw code must check. Never use
  `GetBindGroupLayout` on a pipeline (it may not exist yet): give the
  pipeline an explicit layout (`pipelines::pipelineLayout`).
- **IONOS facts (`report.php?env=1`, 2026-10-17):** PHP 8.4.23, sodium yes
  (Ed25519 tickets possible, restructure 5.1c), pdo_mysql yes, pdo_sqlite
  yes, private folder writable and outside the web root, no open_basedir.

## Racing sim (2026-10-16)
- **Own physics, fixed 240 Hz**, substepped from the frame (the code's cap
  is 30 steps, but the frame dt cap of 0.1 s means really 24; slower phones
  slow the race instead of exploding, so results depend on frame rate
  until restructure step 2.4a). Tires are
  one Magic Formula curve over normalized combined slip with relaxation
  lengths; wheel spin is solved implicitly against the tire's pull (an
  explicit step was borderline unstable at 240 Hz). Clutch: lock test
  each step (can it hold the torque that syncs engine and wheels?).
- **Auto gearbox decides from wheel speed**, not engine rpm, and blips
  the engine to the new gear's speed mid-shift. (Without that, a car
  started at speed with the engine at idle downshifted to 1st at 150 km/h
  and locked the rear wheels.)
- **Grid hold** is `CarInput::hold` (clutch out + brakes), not "brake
  pedal": brake while stopped means "reverse" to the auto gearbox.
- **Car space is x right, y up, -z forward** like the camera. With +z
  forward, +x is your LEFT in this right-handed world (caught before it
  shipped: "steer right" turned left). Heading = atan2(-dir.x, -dir.z).
- Barrier hits: one impulse per step from the deepest corner, applied at
  the car's middle height (low contact points flipped cars), hard hits
  soak up spin, a car through the barrier is put back, flipped cars reset
  after 1.5 s. Safety caps on spin (10 rad/s) and speed (110 m/s).
- Racing renders with its own pipeline (`shaders/race.wgsl`, cull none,
  the shader flips back-face normals). While racing the bar isn't drawn.
- Native tests: `g++` builds `engine/vehicle.cpp` and `app/race/*.cpp`
  (except view/game/controls/car_model) without Emscripten; see the
  commands at the top of `tools/native/*.cpp`.

## Gotchas hit so far
- **Loading stutter (fixed 2026-10-16):** model textures used to be
  decoded to pixels on the main thread (getImageData), converted to float
  with `pow()` per channel and mipmapped on the CPU, all in the frame the
  model arrived (the hall has 48 textures: seconds of freeze). Now
  `engine/gpu_upload.*`: createImageBitmap (off the main thread) →
  `copyExternalImageToTexture` → mipmaps drawn on the GPU, and every GPU
  setup step is a job run with a per-frame budget (`runJobs`, 8 ms on the
  loading screen, 3 ms after). Same pixels as before (diffed).
- JS inside `EM_JS` is closure-compiled: write WebGPU / DOM property names
  quoted (`dev['queue']['copyExternalImageToTexture']`, `{'source': b}`),
  or they get renamed and silently do nothing. `WebGPU.getJsObject(ptr)`
  turns a C++ handle into the JS object (needs `EM_JS_DEPS(x, "$WebGPU")`).
- Never capture `std::vector<bool>` elements with `[x = v[i]]` in a lambda:
  that copies a proxy that points into the vector (dangling later). Use
  `static_cast<bool>(v[i])`.
- Files in `web/` are copied by the `site_assets` target on every build
  (before, only when app.js relinked, so a JS-only change could test stale).
- Physics in the headless test runs at ~2 fps, so the world moves in slow
  motion there (max 8 fixed steps per frame). Judge timing on the phone.
- A model's object slot must get its full matrix before the initial
  `objectData` upload, or it renders at the origin until it moves.
- GitHub's own "pages build and deployment" run can hang in its "Deploy to
  GitHub Pages" step and time out after 10 minutes (seen on `3e3dcd9`, the
  first ~8 MB model upload). Our build is fine then; the site just stays
  on the old version, and every later deploy fails at once with "in progress
  deployment, please cancel <sha> first" until GitHub clears the stuck
  one. Fix: `deploy.yml` cancels recent unfinished Pages deployments
  (`POST /repos/.../pages/deployments/<sha>/cancel`, needs `pages: write`)
  before it pushes `gh-pages`. A Claude session can't do this itself (403).
- Pages sends `cache-control: max-age=600`, so Safari can show a build up
  to 10 minutes old. The deploy writes `version.txt`; `index.html` fetches
  it with `cache: 'no-store'` and reloads with `?b=<sha>` if it's newer.
- PBR metals (chrome, brass) have no diffuse color, so without real
  reflections they only show what `roomGlow` and the light highlights give
  them. Chrome looks darker than the old Phong version; environment maps
  (reflection probes) are the real fix, planned with Stage 6.
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
- `mp_test.mjs` runs at ~2 fps per page, and background tabs barely tick,
  so it waits on log lines/relay state rather than fixed times, and closes
  Alice before Carol joins.
  Against the live relay (`RELAY=...`) single checks can flake from the
  slowness; locally all 5 pass. Rerun before suspecting a bug.
- Blender gotchas (2026-10-16): `common.cylinder` skips the bevel below
  2.5 cm radius (bevels on pins/keys/caps tripled their triangles). A
  quad's front face is `right × up` (`common.panel`): for a sign on a wall
  facing -x use right = +z. The voussoir trick: build a box with local
  axes (radial, tangent, wall normal) via `Matrix((d, t, n)).transposed()`.
- A solid block in the shell hides anything built "inside" it (the
  fireplace needed an opening cut into the chimney breast).
- Auto mode blocks `git push -f` from the session. CI does the force-push to `gh-pages` instead.
