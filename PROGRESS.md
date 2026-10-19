# Progress

**Live site:** https://barcode162.github.io/3D/
**Current focus:** The **racing sim** (`docs/RACING.md`): a real driving sim on the two rigs (1-2 players + bots, real physics, full-screen game, race on the rigs' monitors for everyone), then a realtime sync server

## Next up (start here in a new session)
**2026-10-19, NEW TOP PRIORITY (user): `docs/UPGRADE_PLAN.md`**, 15 items
(two-player racing with car choice + real steering feel, an arcade room with
the user's platformer "Test 2", signs/ladders, 4K Mania like osu!mania, mirror
quality, badge slots, clipping props, walk-through fixes, stuck seats, chess,
there. The server-run bot race (below) is replaced by its items 1-2.
Done so far: 12 (stuck seats), 2 (steering, `engine/steering.*`), 1 (two-player
racing with a garage), 3-5 (the arcade, its platformer machine and music).
6 (A-frame signs, barricades, ladder), 7 (4K Mania like osu!mania). 8 (mirror sharp up
close), 9 (badge slots), the Clash Royale style UI, 10 (pots, hanging plants). 11 (walk-through).
13 (chess). Next: 14 (avatar customization), 15 (animations).
there. The server-run bot race (below) is replaced by its items 1-2.

**2026-10-18, NEW TOP PRIORITY (user): the server runs the game.** Multiplayer now
goes through the Cloudflare room (build after `173f06d`; `docs/DECISIONS.md` →
"Cloudflare room is the multiplayer server"). The user's order: (1) the connection
and the server side first: move authority for every shared thing into the room
(done 2026-10-18: seats, glasses, darts + 4K Mania, drinks, value checks on every
shared write; see DECISIONS → "The room checks every shared write"), next: the room
*running* the game logic itself (darts, the jukebox and the glasses' physics done 2026-10-18, `cloud/games/`,
`cloud/sim/`; next: races with bots on the room (the same wasm route: engine/vehicle +
app/race in the Worker), 4K Mania score checks, piano/jukebox timing, races with bots, glass physics), and bartender badges checked by the room for the station too; (2) only then client-side work. Work
only on what the user names. Keep joshuavelasco.com (home) + /bar (the game).
Open: joshuavelasco.com doesn't load on the user's phone ("can't be reached",
2026-10-18) while barcode162.github.io/3D does and the domain answers from
everywhere we tested; suspect the domain's IPv6 (AAAA) record at IONOS.

**2026-10-16, NEW TOP PRIORITY: the restructure. The plan is
`docs/RESTRUCTURE.md`; work through its §4 steps in order, one step per
session, starting at 0.1.** Every system was audited (scorecard and
problems checklist in §3). Goals: 60 fps on the iPhone 15 Pro Max with no
start-up lag, zero multiplayer sync issues (the Cloudflare room becomes the
authority), far fewer error paths. Read "How to use this plan" in that file
first. Ask the user the open questions in its §5 when a step needs them
(Q1 is answered, 2026-10-17: the user made the Cloudflare account and added
the GitHub secrets `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID`; don't
ask again). The plan was reviewed against the code on 2026-10-17 (see its
last section). **Done: Phase 0, Phase 1 (0.1-1.8) 2.1a-c, 2.2, 2.3a, 2.3b, 2.4a, 2.4b (2026-10-17) and 2.5, 3.1 and 3.2a (2026-10-18). Next: 3.2b, 3.2c, 3.3 (rest of the room server, then its dark launch)**
(batch them; see Execution order in RESTRUCTURE §4). User's pace (2026-10-17): the full plan stays (no cuts); Claude batches
related steps (about 3 per message), automatic tests on every change, docs
once per batch. Racing: keep the racing steps for multiplayer/sync, but
skip anything that changes car/track models or visuals (the user will
redesign the racing game). Waiting on the user: 3 reports for the G4 baseline (after
"Tap to enter", at the big mirror, after 2 minutes walking), and the tap
stutter (yellow bar on every tap, seen 2026-10-17): read `recentHitches`
(`touches`, `phase`) in their reports.
The racing plan below waits, except small driving-feel tweaks the user asks for.

**2026-10-16, previous top priority: the racing sim. The plan and checklist
is `docs/RACING.md`; work through its "Steps" in order.** Steps 1-3 are
built: loading stutter fixed (`engine/gpu_upload.*`), real car physics
(`engine/vehicle.*`), a 2.45 km circuit, 7 bots, collisions, full-screen
3D race with touch / keyboard / mouse / gamepad / wheel controls (see
"What exists now" in `docs/RACING.md`). **Next:** the user tries it on
the iPhone (sit in a sim rig); tune the driving feel from their
feedback; then step 4 (the race on the rigs' three monitors), step 5 (two
players), step 6 (realtime server on Cloudflare: decided, waiting for the user's
account + GitHub secrets, see `docs/DECISIONS.md` → Hosting split). Everything below waits.

**2026-10-16, "make everything work" pass: built and deployed.** Voice
chat (WebRTC), menu (emotes, avatars, profile, voice, settings, piano,
bartender), seats for everyone, piano with 10 classical MIDI songs + MIDI
keyboards, darts with scoring, racing game on the sims, 4K Mania and races
shown on the machines' screens, bartender badge + drinks, many small
interactions. All crude on purpose; every crude part is on the remaster
list, `docs/REMASTER.md`. **Next:** the user tests it on the iPhone (with
their brother for voice); fix what breaks, then remaster items in the order
the user picks.

**2026-10-16, new top priority: the big bar upgrade.** The user asked for
a completely new bar (new shape, packed with real-bar clutter), 3 rooms
(the other two taped off as "under construction"), one giant 2-in-1
mirror, darts / racing sims / 4K rhythm machine / grand piano with real
strings, and a loading screen. **The full plan and checklist is
`docs/BAR_REDESIGN.md`.** First pass built and deployed the same day:
new room shell, curved bar, tall back bar, piano, big mirror, both
construction doorways, 2 sim rigs, rhythm cabinet, darts zone, lounge
with fireplace, decor pass, loading screen. User checked it: looks
great; rug fixed; the rhythm machine is now playable (circle skin).
Then accounts (sign up, log in, profile picture, bio, person cards,
pictures over heads) were added. Ideas the user mentioned for later: a
racing game for the sims.
**Next:** ask the user how it
looks and runs on the iPhone (fps with Low Power Mode off, load time,
anything that looks wrong), fix that first; then the open items in
`docs/BAR_REDESIGN.md` ("Still open"), then interactions (sit, play the
piano, darts scoring, the rhythm game). Everything below
this paragraph is the older plan and waits until the upgrade is done.

New direction (2026-10-15): the goal is now a **world simulation** where
engines stack (see `docs/VISION.md` → North star). The user lets Claude
decide the details and only reviews/critiques. Work one step at a time,
push after each, tell the user what to look at.
**Asset pass done and verified on iPhone (build `964adfe`):** every piece
looks as it should. The 30 fps the user saw was **iOS Low Power Mode**
(it caps Safari at 30 fps), not the engine. Next:
1. **Multiplayer M2 is built (2026-10-16), check it on two phones.** The
   relay at https://joshuavelasco.com/bar/relay.php works (live test with
   two headless players passed). M2 = shared world: glasses, the jukebox
   song (joins mid-song) and mirrors on/off are the same for everyone (see
   `docs/DECISIONS.md` → Shared world). Ask the user to try it with their
   brother. Then M3 = smoother movement with WebRTC, voice chat.
   **Status at session end (2026-10-16):** build `355fed6` is live on both
   sites; the user hasn't tested it on phones yet and will report failures
   later. Assume it works until they say otherwise. Then ask which comes
   next: the performance pass (2) or glass-clink sounds (W5, item 3).
2. **Performance pass (no longer urgent).** Re-measure with Low Power Mode
   off first. Goal: 60 fps on the iPhone 15 Pro Max. Room is ~170k triangles, ~200 draw calls, ~80
   textures (all RGBA, uncompressed), and the full mirror draws the whole
   scene a second time. Ideas, cheapest first:
   - Measure: add a debug toggle (`?perf=`) to switch off the mirror pass,
     models by group, or lights, to find what costs the most.
   - Mirror: draw the reflection at half resolution, skip small props in
     it, and only render it when the mirror is on screen.
   - Fewer triangles: bottles (37k in the back bar) and stools (5.6k each)
     can drop a lot (lathe segments, bevel segments); simplify far props.
   - Fewer draw calls: merge static models that share materials into one
     scene mesh at load time (stools, tables, coasters).
   - Textures: 512 for most surfaces, 1k only for floor/brick/counter top.
   - Shading: 8 point lights per pixel is the main per-pixel cost; light
     culling per object (only the nearest 3-4 lights) could halve it.
   - Pixel ratio: try 1.5 instead of 2 as a fallback.
3. **W5 Physical sound** (impact sounds synthesized from collisions and
   materials; room echo), then more sim engines (shadows/bloom,
   heat/flames, liquids, smell field, day/night).

### Asset workflow (for new or changed models)
1. `bash tools/assets/build_all.sh <scratch dir>` rebuilds every model in one
   go (installs Blender 4.5, fetches CC0 Poly Haven textures, runs all
   scripts) and renders Cycles previews to `<scratch>/previews/`.
   Or run one script: `/opt/blender/blender -b --factory-startup --python
   tools/assets/<script>.py -- <texdir> <out>`.
2. Helpers in `tools/assets/common.py` (boxes, lathe, torus, tube, PBR and
   image materials, glow, pixel text, atlas UVs); add a row to
   `assets/models/CREDITS.md`.
3. Place it in `app/bar_scene.cpp` (`b.model(...)` plus `b.solid(...)` for
   its physics shape).
4. Check: build, smoke test, `CAM="a;b;c"` close-ups (several in one run);
   after the deploy, `node tools/check_live.mjs`.
Budget: textures 1k for big surfaces, 512/256 for props; check with
`python3 tools/assets/glb_report.py` (download size, triangles, texture
memory).

- Asset follow-ups: bottles as physics bodies (knock them off shelves),
  darts game on the dartboard, jukebox buttons, door that opens.
- Physics follow-ups: knock bottles off shelves (make them bodies), glass
  breaking (material flag), stacking glasses, beer in the pint (W7).
- Still open: ask the user how the first-person body looks on the iPhone
  (build after `fa20592`).

Last verified on iPhone: build `964adfe` (all real models): everything looks right. 30 fps was Low Power Mode, not the engine.

## The vision (full details in `docs/VISION.md`)
A cozy 3D hangout bar, viewed in first person on an iPhone:
- Walk around with touch joysticks.
- **Everything is interactive:** pick up glasses, flip light switches, use
  the jukebox, play darts or pool, sit on stools.
- **An avatar you can see in a mirror,** like VRChat.
- **Real background music** from a jukebox, with sound placed in 3D space.
- Top-quality lighting: warm lamps, neon signs, glow, reflections.
- Multiplayer: the user and their brother in the same bar (M1 built 2026-10-15). Voice chat later.

## Roadmap
### Stage 1: Engine foundation
- [x] 1.1 Pipeline proof: C++ → WASM → WebGPU triangle, tap to change color, on-screen log, auto-deploy (verified on iPhone: 60 fps, Apple GPU)
- [x] 1.2 Math library: vectors, matrices, quaternions (`engine/math.h`)
- [x] 1.3 3D meshes + depth buffer: procedural boxes, cylinders, spheres (`engine/mesh.*`)
- [x] 1.4 First-person camera + touch controls (left stick move, right drag look; WASD + mouse on a computer)
- [x] 1.5 Lighting: 8 warm point lights + emissive + haze + tone mapping + PBR materials (W1)
- [x] 1.6 Text rendering in-engine (`engine/font.*`, `engine/text.*`, `shaders/text.wgsl`): hand-drawn 5x7 pixel font → SDF atlas with mipmaps, outlines, screen + 3D world text, one draw call

### Stage 2: The bar room
- [x] Rough blockout (`app/bar_scene.cpp`): room, bar counter, stools, back bar with bottles,
      pendant lamps, neon frame, tables, booth, jukebox, mirror frame, door
- [x] Procedural textures (`engine/procedural.*`, `engine/texture.*`): wood planks (floor), wood grain (bar, tables, shelves, panels, door), brick (walls), leather (stools, booth), plaster (ceiling). Color + gloss + bump, mipmaps, triplanar mapping (no UVs needed)
- [x] More detail and props; better neon (real sign shapes): all real models now (W3)
- [x] Collision so you can't walk through walls or furniture (circle vs. floor rectangles)

### Stage 3: Interaction ("everything is interactable")
- [x] Tap-to-use (ray from the tap point, `Interactable` boxes in `bar_scene`), glow highlight + prompt when you stand at something
- [x] Pick up / put down / throw objects (W4)
- [x] Light switch, door, taps, bell, guitar, mic, fireplace, books, chess, tip jar (crude, 2026-10-16)
- [x] Darts (301 with real scoring), racing sims, piano, bartending (crude, 2026-10-16)
- [x] Sit on every seat (everyone, one per seat)
- [ ] Pool table

### Stage 4: Audio
- [x] C++ audio through Web Audio (`engine/audio.*` + thin bridge `web/audio.js`; unlocks on first tap)
- [x] Jukebox plays the smooth-jazz playlist (`app/jukebox.*`), 3D-positioned (HRTF), tap = next song, after the last song tap = off
- [x] Jukebox menu (`JukeboxMenu` in `app/jukebox.*`): song list with codes A1, A2…, pages, Load song / Stop / Close
- [x] "Load song": the user's own songs from the iPhone Files app (local only, never uploaded; gone after reload)
- [x] In-game credits for the CC BY tracks ("Now playing" banner with the credit)
- [ ] Generated sound effects (glass clink, footsteps, switches)

### Stage 4b: Mini music studio (the fun extra, later)
A small DAW inside the bar, all DSP written in C++:
- [ ] Synth engine: oscillators, envelopes, filters (the "VST-style" core)
- [ ] Drum machine + step sequencer
- [ ] Piano roll / pattern editor, tempo, loop
- [ ] Mixer with effects (reverb, delay, EQ)
- [ ] Songs made in the studio can play on the jukebox
- [ ] Later: export the same DSP code as a real VST plugin (desktop)

### Stage 5: Avatar + mirror
- [x] Mirror 1: full reflection (reflected camera into a screen-sized texture, scissored to the mirror, clip plane at the glass)
- [x] Mirror 2: avatars-only mirror (VRChat style), studio-lit backdrop. Tap a mirror to turn it on/off
- [x] Skinned humanoid avatar (`engine/skeleton.*`, `engine/skinned_mesh.*`, `app/avatar.*`): 18 bones, one smooth mesh (~4k vertices) built from tubes/ellipsoids, 2-bone skinning in `vs_skinned`; face, hair, t-shirt (fabric texture), jeans (denim texture), belt, shoes; head follows where you look
- [x] Walk/idle animation (`advanceWalk` + `Avatar::pose`): stride driven by distance moved, legs/knees/feet, arm swing, hip/chest twist, bob; idle breathing and sway; head follows where you look
- [x] First-person body: the avatar is drawn in the main view with the head and neck folded away (`Avatar::hideHead`); looking down tilts the head around the neck, so the body shifts back and you see your feet
- [x] Avatar follows your movement; visible in the mirror
- [x] Avatar look modeled on the user's photo (2026-10-16): fair skin, dark curly hair, stubble, brows, athletic build, charcoal tee, dark jeans, white sneakers. Rough first pass (hair reads too afro-like, stubble a bit dark); user said it's good enough for now
- [ ] Later: realistic Blender-built avatar of the user with a full rig, ideally from a 3D face scan (see `docs/VISION.md`)
- [x] 8 avatar looks to switch between in the menu (2026-10-16, crude: color/hair variations)
- [x] Emotes (11) and activity animations (sit, drive, piano, darts, bartend, drink), crude key poses
- [ ] Later: load your own avatar model (GLB/VRM)

### Stage 6: Visual quality
- [ ] HDR + tone mapping, bloom for neon
- [ ] Shadows from lamps
- [ ] Reflections on bar top/floor, haze/light shafts

### Stage 7: Imports
- [x] GLB model loader (done as W2)

### Stage M: Multiplayer (2026-10-15)
- [x] M1 See each other: PHP relay on IONOS (`server/relay.php`), bridge `web/net.js` + `engine/net.*`, smoothing + walk animation `app/players.*`, up to 7 other avatars with name tags, in mirrors too. `?room=`, `?name=`. Test: `tools/mp_test.mjs`
- [x] IONOS upload + domain joshuavelasco.org (game + relay live over http)
- [x] Domain moved to joshuavelasco.com (HTTPS on), landing page + game at `/bar/`
- [x] M2 Shared world (`app/shared_world.*`): held/thrown/knocked glasses, jukebox song (late joiners hear the same part), mirrors on/off; world entries with revisions in the relay. Test: `tools/mp_test.mjs` (5 checks)
- [x] M3 (part) WebRTC phone-to-phone: voice chat + data channel for quick events (2026-10-16). Movement still goes through the relay

### Stage W: World simulation (new north star, 2026-10-15)
- [x] W1 Material system (`engine/material.h`, `app/materials.*`: look + physics + sound in one record) + PBR shading (GGX, metal/roughness, sphere-light highlights, approximate room reflections)
- [x] W2 GLB loader (`engine/json.*`, `engine/gltf.*`, `engine/model.*`, `engine/fetch.*`) + asset pipeline (`tools/assets/`, Blender headless)
- [x] W3 Real assets: room shell, bar counter, back bar with ~110 bottles, stools, tables, booth, jukebox, lamps, neon sign, candles, beer taps, coasters, napkin holders, menus, dartboard, chalkboard, posters, mirror frames (`tools/assets/`, `assets/models/`)
- [x] W4 Rigid-body physics (`engine/physics.*`, `app/props.*`): compound-sphere bodies, fixed boxes, friction/bounce/mass from materials, sleeping, impact events; pick up / put down / throw glasses
- [ ] W5 Physical sound: synthesized impacts, room acoustics
- [ ] W6 Heat and fire (candle flames, steam, melting ice)
- [ ] W7 Liquids (pour and mix drinks)
- [ ] W8 Smell/air field (spreading haze), day/night and weather outside

## Log of the "make everything work" pass
- **2026-10-16:** User asked for voice chat, settings, emotes, avatar
  switching, detailed profiles, MIDI piano with 10 classical songs, seats,
  darts, racing, 4K Mania for others, one player per machine, bartender
  badge and drinks, everything interactable, all crude, plus a remaster
  list. All built in one go (build `790cfbe` and after); details in
  `docs/DECISIONS.md` → "make everything work", crude parts in
  `docs/REMASTER.md`.

## Decisions
- **Music:** (1) CC BY smooth-jazz tracks by Kevin MacLeod as the bar's
  ambiance, (2) the user's own songs loaded from the phone, (3) a C++
  synth/mini DAW as a fun extra (Stage 4b).

## Log
- **2026-10-15:** Project set up. Toolchain is Emscripten 6.0.12 with the
  emdawnwebgpu port. First triangle renders, and the smoke test passes using
  SwiftShader WebGPU.
- **2026-10-15:** Music decided. Added 4 CC BY jazz tracks (AAC, about 15 MB),
  served at `/assets/music/`. Added Stage 4b, the mini music studio.
- **2026-10-15:** Site live via the `gh-pages` branch (Pages → Deploy from branch).
  The triangle ran on the iPhone at 60 fps. Built the math library, meshes,
  depth buffer, first-person camera, touch joysticks, lit shader and the first
  walkable bar blockout (184 objects, 8 lights).
- **2026-10-15:** Bar blockout verified on iPhone at 60 fps. Saved the full
  handoff: `docs/VISION.md`, `docs/DECISIONS.md`, a "Next up" list, and a
  SessionStart hook that installs Emscripten, so a brand-new chat can resume
  from the repo alone.
- **2026-10-15:** 1.6 in-engine text. Own 95-character pixel font, drawn
  as a distance-field texture (sharp at any size, with outlines). Shows a
  "BAR3D" title + controls hint on start (fades once you move; replaces the
  HTML hint), floating JUKEBOX / MIRROR name tags, and an on-screen prompt
  when you stand at one and look at it.
- **2026-10-15:** Procedural textures. Five materials generated in C++ at
  startup (about 1.3 s in the test container), with bump maps so mortar
  and plank gaps catch the light. Verified on iPhone: 60 fps, < 1 s load.
- **2026-10-15:** Jukebox music. Tap detection, ray picking, highlight
  glow, Web Audio bridge with iOS unlock, 3D-positioned music, "Now
  playing" banner with the CC BY credit. Tested headless with an Ogg
  stand-in (test Chromium can't decode AAC).
- **2026-10-15:** User answered the build-ourselves list (see
  `docs/DECISIONS.md`). Jukebox menu: tap the jukebox → song list with
  jukebox codes, paging, Load song (Files app), Stop, Close.
- **2026-10-15:** Mirrors. Full mirror (brass frame, z = -2.2) and avatar
  mirror (black frame, z = 1.1) on the right wall; tap to toggle. Placeholder
  avatar. Drawing code moved from `main.cpp` to `app/renderer.*`.
- **2026-10-15:** Skinned avatar with skeleton, body shapes, face and
  clothes textures; replaces the mannequin.
- **2026-10-15:** Walk and idle animation. Session wrapped up here; all
  work committed and deployed. Not yet verified on iPhone: mirrors,
  avatar, animation (builds `0f74167` onward).
- **2026-10-15:** iPhone check of `fa20592`: 60 fps even at the full
  mirror, music works, animations good, no bugs. Added the first-person
  body (look down to see yourself). Smoke test now also saves
  `build/smoke-down.png` (looking ~70° down).
- **2026-10-15:** New north star from the user: a world simulation where
  engines stack and cover every sense, highest quality, real assets.
  Claude decides details, user reviews. Roadmap reordered (Stage W).
- **2026-10-15:** W1 done. Every object is now made of a named material
  (oak floor, varnished wood, brick, leather, chrome, brass, bottle glass...)
  that also carries physics and sound values for later engines. The shader
  is physically based (GGX). Chrome now looks darker: it reflects the dark
  room; real reflections come with environment maps later.
- **2026-10-15:** W2 + W3. Own GLB loader (JSON parser, node tree, PBR
  textures decoded by the browser). Blender 4.5 runs headless in the
  container; `tools/assets/bar_counter.py` models the counter (paneled
  front, mahogany top with drink rail, brass foot rail on brackets) with
  CC0 Poly Haven wood. Test camera: `?cam=x,y,z,yaw,pitch`.
- **2026-10-15:** W4 physics. Own rigid-body engine: glasses are rings of
  small spheres, the world is boxes (every scene object + colliders for the
  counter model), fixed 60 Hz steps with 8 substeps. 7 glasses (pint and
  rocks, modeled in Blender, see-through glass with Fresnel edges): look at
  one and tap to pick up; tap a surface to put it down, tap elsewhere to
  throw. Impacts are recorded for W5 sounds.
- **2026-10-15:** Asset pass, all at once (the user asked for every piece
  in one go, then one test round). 23 Blender-built models replace every
  blockout shape: room shell (Poly Haven plank floor, red brick, plaster
  ceiling with beams, wainscot, baseboards, door with window), back bar
  (cabinet, smoked mirror, LED-lit shelves, ~110 bottles in 6 shapes with
  generated labels), bar stools and table stools (tufted leather), round
  and booth tables, channel-tufted booth, arched jukebox with glow tubes and
  a record under a glass dome, pendant lamps, neon "BAR" sign with martini
  glasses, candles, beer taps, coasters, napkin holders, menu cards,
  dartboard cabinet, chalkboard, two posters, mirror frames. Parts are
  merged per material (few draw calls), glow strength comes through
  `KHR_materials_emissive_strength`, and interactables can light up
  models. `tools/assets/build_all.sh` rebuilds everything.
- **2026-10-15:** Deploy got stuck: GitHub's Pages deployment for the
  asset build hung and blocked every later one. `deploy.yml` now cancels
  unfinished Pages deployments before publishing. iPhone check of
  `964adfe`: all assets look right, 30 fps. Session closed; next session
  starts with the performance pass.
- **2026-10-15:** The 30 fps was iOS Low Power Mode. The user wants
  multiplayer next, hosted on their new IONOS plan (waiting on which plan).
- **2026-10-15:** M1 multiplayer built and tested headless (two browsers,
  local PHP relay: each sees the other walk, with name tag). Deploy now
  uploads the game + relay to IONOS over SFTP once the secrets exist.
- **2026-10-15:** Domain joshuavelasco.org connected (points at `bar/`).
  Relay tested live with two players. Only the SSL certificate is missing.
- **2026-10-15:** User switched to **joshuavelasco.com** (points at webspace
  `home/`) to host several projects. Added the landing page
  (`homepage/index.php`): one "Projects" button, dropdown lists every
  subfolder with a `project.json`. The bar now deploys to `home/bar/`.
- **2026-10-16:** Live relay on joshuavelasco.com checked (two headless
  players OK). M2 shared world built: the relay keeps small "world
  entries" (glass poses, jukebox song + start time, mirrors) with revision
  numbers; whoever moves a glass sends it, others follow it smoothly (a
  "kinematic" body that still knocks their glasses). `mp_test.mjs` now
  checks glass, late join, mirror and jukebox sync.
- **2026-10-16:** Session closed with M2 deployed (`355fed6`), not yet
  checked on phones.
- **2026-10-16:** User wrapped up the mini games for now (racing for the
  sims is a later idea). Built accounts: sign up / log in, profile
  picture, bio, favorites, card color, person cards, pictures + names
  over heads (`docs/DECISIONS.md` → Accounts).
- **2026-10-16:** User: everything great, the rug clipped under the
  couch (fixed: one slab, ends before the sofa). Made the 4K MANIA cabinet
  playable with a circle skin (charts generated from any song).
- **2026-10-16:** The big bar upgrade (`docs/BAR_REDESIGN.md`). Loading
  screen with progress and "Tap to enter"; light grid (32 lights); glowing
  image textures; walkable stage; one giant 3-mode mirror with a button
  panel; models served gzipped. New 18 x 14 m room: curved counter with a
  full underbar and 8-tap tower, floor-to-soffit back bar (~300 bottles,
  onyx bay, library ladder), grand piano with 236 strings, two taped-off
  doorways to future rooms, two racing sims, a 4K rhythm cabinet, darts
  corner, fireplace lounge, string lights, neon, gallery and collector
  walls. Shared helpers: `tools/assets/layout.py` (floor plan),
  `tools/assets/props_lib.py` (bottles, glasses, tools, books, plants...).
- **2026-10-16:** User asked for a genuine racing sim (Assetto-like
  physics, NFS-style bots and contact, 1-2 players, smooth touch + keyboard
  / mouse / wheel controls, full-screen game, race on the rigs' monitors
  for spectators), a fix for the loading stutter, and a real realtime
  server. Plan written: `docs/RACING.md`. Loading stutter fixed: all
  texture work moved to the GPU and spread over frames.
- **2026-10-16:** Racing sim steps 2-3 built: car physics (Pacejka tires,
  suspension, drivetrain), Bar3D Ring track, bots, collisions, race rules,
  race renderer with sky/sun/PBR/haze, touch/keyboard/mouse/pad controls.
  Native tests in `tools/native/`.
- **2026-10-16:** Cloudflare realtime server written (`cloud/worker.js`,
  one Durable Object room per bar, WebSockets, asks IONOS to verify
  logins); `web/net.js` uses it when `cloud.json` gives its address, else
  the IONOS relay. Tested locally (`tools/cloud_test.mjs`, `CLOUD=` in
  `tools/mp_test.mjs`). **Not deployed yet:** next is the deploy step
  (wrangler with the `CLOUDFLARE_*` secrets, which the user has added, and
  writing `cloud.json`). User's plan: Cloudflare = live backend, IONOS =
  accounts and other sensitive/permanent data (move accounts from JSON
  files into IONOS MySQL later).
- **2026-10-17:** Restructure step 0.1 done. Closed the token leak:
  `?server=` / `?cloud=` overrides now only work for localhost or exact
  allowlisted hosts (`web/net.js`), and logins/passwords go only to
  same-origin or allowlisted hosts. `cloud/.wrangler` removed from git and
  ignored. IONOS upload list generated from the build, `mirror` no longer
  skips same-size edits. Racing docs corrected (frame-rate dependence,
  remote-car mass). `tools/shots.mjs` takes its own `server=` and
  `REQUESTS=1` (prints requests to outside hosts).
- **2026-10-17:** Restructure step 0.2 done. Frame profiler
  (`engine/profiler.*`): time per frame phase, draw/triangle counts per
  pass, loading jobs, heap, hitches, start-up stages. Performance overlay
  (Menu → Settings → Performance overlay, or `?perf=1`). Smoke test prints
  a `perf:` line from `window.bar3dPerf`. First finding: at the door the
  bar draws ~527 objects plus ~525 more for the mirror each frame (target
  ≤ 150 + 60, Phase 6).
- **2026-10-17:** Restructure step 0.3 done. Logs have levels and times
  and can't flood; errors show a red badge instead of opening the log;
  fatal errors show "Something broke - tap to rejoin" and send a report.
  Menu → Settings → Send report gives a 6-letter code (`server/report.php`).
  User's first overlay check (build da02dee, iPhone): 60 fps at the door,
  CPU ~1 ms, 531 + 529 draws; a yellow bar (20-34 ms frame) on every tap.
- **2026-10-17:** First phone report `VH3WMR` (build 13be3f2, at the bar):
  avg 16.7 ms, p95 18, game CPU 0.8 ms (worst 3 ms), 529 + 527 draws,
  ~1.3M triangles per frame, start-up: GPU ready 0.8 s, everything loaded
  2.7 s. Every hitch had CPU ≤ 2 ms, so stutters come from the GPU or
  Safari, not our code; the GPU is likely near its limit (Phase 6 draw
  cuts and 1.7 half-resolution mirror should fix it). Fixes: tap counts
  were lost (events arrive between frames), gaps > 1 s (app in background)
  no longer count as hitches, new `gpu` number (submit → done time) in
  the overlay and reports, race engine sound no longer reschedules Web
  Audio values every frame (the user saw lag only while pressing gas).
  Note: the race sim is timed under "interaction" (race.update runs there).
- **2026-10-17:** Restructure step 0.4 done. CI runs native tests (g++ and
  em++), `php -l`, an EM_ASM lint and the browser smoke before publishing,
  plus a Cloudflare check. New: `tools/imgdiff.mjs`, `?still`,
  `tools/shots.mjs spots`, `tools/lib/`, `EXPECT_SHA` for check_live. The
  build copies only changed files (models gzipped once).
- **2026-10-17:** Race report `ETCPZN` (build 018ee60): during the race CPU
  ~1 ms and GPU (submit → done) 3-9 ms, yet frames of 30-40 ms keep coming
  while a finger is down, so the time goes outside our frame. The engine
  sound fix didn't help. User: GPU number at the bar door is 14-15 ms (near
  the 16.7 ms budget: Phase 6 and 1.7 matter). Changes: touch handlers no
  longer measure the page per event (canvas size cached once per frame),
  the joystick moves once per frame (no layout thrash), and the profiler
  times input handlers (`events` phase; hitches show `moves`, `eventsMs`).
  If `eventsMs` stays small in the next report, pull the input mailbox of
  step 2.1a forward (JS collects touches, C++ reads them once per frame).
- **2026-10-17:** Report `7XCVUN` (user was tapping the gas over and
  over): one late frame (25-40 ms) per tap, with our touch handlers taking
  ~0 ms. Cause: emscripten's touch listeners can cancel the touch, so
  Safari waits for the page's code before drawing. Pulled the touch part of
  step 2.1a forward: passive listeners queue touches
  (`window.bar3dTouchQueue`), `Input::pollTouches()` handles them once per
  frame (with their arrival times); the phone's pretend mouse clicks after
  a tap are ignored. Profiler: touches now count for the frame they
  arrived in.
- **2026-10-17:** The tap stutter is still there after the passive touch
  change (user, build b059f86). Parked by the user ("not causing issues");
  look again later with phone reports (2.1a/2.2 may help).
- **2026-10-17:** Restructure step 1.1 done: only the 3 avatar textures are
  generated, models download while they're made, nothing is drawn behind
  the loading screen or 4K Mania (1 draw there instead of ~1,050), you join
  the room and voice when you enter. Headless: everything loaded 9.5 → 5.2
  s, heap 84 → 48 MB. mp_test now waits for both players to load.
- **2026-10-17:** Restructure step 1.2a done: piano notes are made about
  15x faster (middle C 24 → 1.7 ms), bass notes ring up to 6 s, and a
  finished piano song no longer makes people who join later prepare it.
- **2026-10-17:** Restructure step 1.3 done: shaders compile in the
  background, and "Tap to enter" waits until a warm-up frame has finished
  on the GPU (loading text shows "Warming up…").
- **2026-10-17:** Restructure 1.4 + 1.2b + 1.5 done (one batch): loading
  work in small jobs after each frame (longest ~7-12 ms headless), models
  parsed and uploaded in slices, downloads retried 3 times with a
  "parts failed - Retry" message, piano notes from 30 samples made in the
  background, each player's piano keys/pedal kept apart, MIDI pedal
  shared, memory starts at 80 MB (no growth while playing).
- **2026-10-17:** User tested build d4097e2 on a PC: 280 fps (1920x1080),
  CPU 1.2 ms, no input stutter, driving perfect. So the tap stutter is an
  iPhone Safari quirk; dropped (don't chase it unless phone reports show it
  getting worse).
- **2026-10-17:** Restructure 1.6 + 1.7 + 1.8 done, Phase 1 complete:
  identical textures shared between models (237 → 159, 132 MB), mirror at
  half resolution with an oblique near plane (no `discard` anywhere),
  named loading stages, test options in `app/test_hooks.*`, version check
  before loading the game, a security policy on the page.
- **2026-10-17, HANDOFF (chat ended mid-batch):** step 2.1a is mostly done
  and pushed: `engine/bridge.*` + `web/bridge.js` (one `pull()` at frame
  start: canvas size from a ResizeObserver, dpr, safe area, hidden/blur
  count, text-field focus, modal open, audio running + sound clock,
  settings version, your voice level, touches, other players' voice levels,
  menu commands JSON, user song count; one `flush()` at frame end: JSON ops
  `status`, `stick`, `log`, `voicePlace` (20 Hz batch), `listener`, `info`).
  Gpu size settles 120 ms before reconfiguring; Outdated/Lost surface →
  reconfigure. Input: touches from the bridge, focus flag instead of the
  per-key `activeElement` call, `releaseAll()` on blur. Logs go through
  the flush once the frame loop runs (`engine::batchLogs`; fatal goes at
  once). Page calls per game frame: ~24 → ~3 (pull, flush, net status at
  times). Bonus: the mirror pass is skipped when the glass isn't on screen
  (at the door: ~525 fewer draws).
  **Left to do for 2.1a:** (1) `web/index.html` `?crossings` counter wraps
  the same import table twice (it's under two names), so it counts every
  call double: skip a table already wrapped (keep a list). (2) Check
  rotation (`VIEW=932x430` then `430x932` → one resize each) and
  `HOLD=ArrowUp:3000` still walks, `tools/mp_test.mjs` and
  `tools/voice_test.mjs` (voice positions/levels moved to the bridge). (3)
  Tick the 2.1a boxes in RESTRUCTURE §3.2 and write "Done" under 2.1a.
  Measuring page calls by name needs a Debug build:
  `emcmake cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug &&
  cmake --build build-debug`, then
  `node tools/smoke.mjs build-debug/site build/smoke-dbg.png` (prints
  graphics / clock / page calls and which EM_ASM blocks; look a block up
  with `grep -o "<number>: [^;]\{0,140\}" build-debug/site/app.js`).
  User wishes recorded this chat: mirror keeps avatars and nearby things
  full quality, only far things lower (RESTRUCTURE 6.3, VISION). Pace: Claude
  batches ~3 related steps per message; racing steps stay but don't change
  car/track models or visuals.
- **2026-10-17:** Restructure 2.1a done: one mailbox call each way per
  frame (page calls ~24 → ~3), canvas size from a ResizeObserver (one
  resize per rotation, checked in smoke), input read once per frame as a
  read-only `InputFrame`, presses timed by the browser's event time.
- **2026-10-17:** Restructure 2.1b done: a JSON writer that escapes
  properly (a MIDI file named with `\` or quotes works now), safer JSON
  reading (limits, no crash on garbage: new native json_test), menu
  commands pushed into the mailbox, menu info only when it changes, one
  panel at a time (menu vs account card), no blur behind panels, no
  looking/walking while a panel is open. New test: `tools/menu_test.mjs`.
- **2026-10-17:** Restructure 2.1c done: who's in the room reaches the
  game through the mailbox only when it changes (no page call per player
  per frame), saved settings are checked against a schema (a broken one
  → defaults + one warning). Page calls per frame: 2.8.
- **2026-10-17, phone baseline (G4), build 4ae5f15:** iPhone report
  8RN8VS: avg frame 17.2 ms (≈58 fps), p95 21 ms, CPU ~1 ms/frame, GPU
  ~9.3 ms (worst 60), 529 draws / 657k triangles, ready 3.7 s, entered
  4.8 s, heap 80 MB, no errors; hitches (~76 after enter) are GPU/browser
  side (CPU ≤ 2 ms in each), some on taps, one 148 ms pause with no CPU
  work. User: "everything works well". PC report 6SH8QY: 3.6 ms frames,
  CPU 0.85 ms, GPU 7.4 ms; one 67 ms spike in the "interaction" phase
  (mouse move, no tap): look at it if it repeats (maybe a raycast or a
  first-use pipeline). So the GPU is the phone's limit now (Phase 6),
  not the CPU.
- **2026-10-17:** Restructure 2.2 done: one clock per frame (the
  browser's frame time), no jump when you come back from another app,
  sound pauses in the background and comes back ("Tap to resume sound" if
  iOS blocks it), others see "(away)" under your name while you're gone.
- **2026-10-17:** Restructure 2.3a done: one shared maths file (copies
  removed, new math_test); race and car results unchanged to the byte.
- **2026-10-17:** Restructure 2.3b done: glasses settle in half a second
  (the pint at the left end of the bar used to wobble forever), smooth
  drawing between physics steps, others' glasses glide instead of judder,
  a broken or escaped glass goes back where it was, physics 0.37 → 0.10
  ms per frame. New native physics_test.
- **2026-10-17:** Restructure 2.4a done: the race runs on a tick clock
  (same results at any frame rate, checked at 30/57.3/60/120 fps), cars
  and camera drawn between physics steps (no judder at speed).
- **2026-10-17:** Restructure 2.4b done: the race track is built in the
  background after you enter (no freeze when you sit), resets find a free
  spot and pass through cars for 2 s, quicker steering and brake bite,
  race buttons avoid the notch/home bar. Car physics unchanged.
- **2026-10-17, HANDOFF (user paused to regroup; "exciting news" for the
  project coming in a new chat):** step 2.5 is mostly built and pushed.
  Done: sound plays/stops/changes go as commands in the frame's mailbox
  flush (`sp`/`ss`/`sv`/`se` ops in `web/bridge.js`; C++ picks voice ids,
  times are absolute sound-clock times), 32-voice cap (oldest replaced),
  one shared HRTF emitter for the piano (`audio::createEmitter`, other
  placed sounds use equal-power panners), raw sound data dropped once the
  AudioBuffer exists, the 4K Mania track goes through the music bus (music
  slider), unlock listeners removed once sound runs, games hold
  `engine::Sound` values instead of pointers into `g_app.sfx`, smoke prints
  audio commands per frame and checks a piano song stays ≤ 32 voices
  (headless: max 6). **Left for 2.5:** revoke object URLs (MIDI blob URL
  after the piano fetched it; user songs when replaced), run
  `tools/rhythm_test.mjs` and `tools/voice_test.mjs` (not run since the
  audio change), tick the 2.5 boxes in RESTRUCTURE §3.2, write "Done" under
  2.5. Remaining after 2.5: about 54 steps (Phase 3: 10, Phase 4: 7,
  Phase 5: 14, Phase 6: 12, Phase 7: 6, Phase 8: 3); at ~3 per message,
  about 18 more "continue"s. Gates still open: G2 (IONOS MySQL secrets,
  for 5.1a), G3 (user approval of `astcenc`, for 6.8).
- **2026-10-18:** Restructure 2.5 done: last bits were freeing local file
  links (MIDI files after the piano reads them, the mic test recording,
  no duplicate link when the same song is picked twice). rhythm_test and
  voice_test pass with the new sound path.
- **2026-10-18:** Restructure 3.1 done: the language the phone and the
  new Cloudflare room will speak is written down (`docs/PROTOCOL.md`), with
  one translator on each side (`cloud/proto.js`, `engine/netproto.*`) that
  are tested to agree byte for byte on shared examples
  (`proto/vectors.json`), and to refuse broken messages. Nothing changes on
  the phone yet.
- **2026-10-18:** Meridian (the user's driving game) is in the bar: sit in a racing
  rig → pick "Meridian" (or the bot race). `web/meridian/`, test
  `tools/meridian_test.mjs`.
- **2026-10-18:** Restructure 3.2b, 3.2c, 3.3 and the cutover done in one go
  (user: "server first"): the room ticks, events, signals, shared world with
  echo, claims (seats decided by the room), ghosts, AFK, rate limits, saved
  world, `/health` usage; CI deploys it and writes `config.json`; `web/net.js`
  rewritten for it (no relay any more); mp_test (8 checks) and voice_test run
  against the room on wrangler dev. Meridian was added afterwards (see the entry above).
- **2026-10-18:** Restructure 3.2a done: the new Cloudflare room
  (`cloud/room.js`) hands out player slots, lets a dropped phone come back
  to the same slot for 30 s (resume token), refuses other websites, old
  versions (4002) and a 17th player (4001), and checks logins without
  making you wait. `tools/room_test.mjs` (replaces `cloud_test.mjs`, in CI)
  runs it locally with 35 checks. Still not deployed (3.3).

