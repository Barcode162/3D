# Progress

**Live site:** https://barcode162.github.io/3D/
**Current focus:** Stage W, the world-simulation foundation (materials for every sense, PBR, GLB loader), then real assets one piece at a time

## Next up (start here in a new session)
New direction (2026-10-15): the goal is now a **world simulation** where
engines stack (see `docs/VISION.md` → North star). The user lets Claude
decide the details and only reviews/critiques. Work one step at a time,
push after each, tell the user what to look at.
1. **W2 GLB loader** (from scratch): meshes, UVs, normals, tangents, PBR
   textures; assets streamed from `assets/models/`.
2. **W3 First real asset: the bar counter**, replacing the blockout one.
   Set up the asset tool pipeline (try headless Blender in the container;
   CC0 textures from Poly Haven/ambientCG with credits).
3. **W4 Rigid-body physics** using the material's physics fields: pick up,
   throw, stack glasses (was "Physics" before).
4. **W5 Physical sound**: impact sounds synthesized from collisions and
   materials; room echo from room size and materials.
5. Then continue the art upgrade piece by piece (walls/floor → stools →
   back bar → tables/booth → jukebox → props), alternating with sim
   engines (shadows/bloom, heat/flames, liquids, smell field, day/night).
- Still open: ask the user how the first-person body looks on the iPhone
  (build after `fa20592`).

Last verified on iPhone: build `fa20592` (mirrors, avatar, walk animation, music), 60 fps even at the mirror, no bugs.

## The vision (full details in `docs/VISION.md`)
A cozy 3D hangout bar, viewed in first person on an iPhone:
- Walk around with touch joysticks.
- **Everything is interactive:** pick up glasses, flip light switches, use
  the jukebox, play darts or pool, sit on stools.
- **An avatar you can see in a mirror,** like VRChat.
- **Real background music** from a jukebox, with sound placed in 3D space.
- Top-quality lighting: warm lamps, neon signs, glow, reflections.
- Not now: multiplayer, voice chat (much later).

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
- [ ] More detail and props; better neon (real sign shapes)
- [x] Collision so you can't walk through walls or furniture (circle vs. floor rectangles)

### Stage 3: Interaction ("everything is interactable")
- [x] Tap-to-use (ray from the tap point, `Interactable` boxes in `bar_scene`), glow highlight + prompt when you stand at something
- [ ] Pick up / drop / throw objects (simple physics)
- [ ] Light switches, doors, taps, jukebox buttons
- [ ] Bar games: darts, pool table

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
- [ ] Later: load your own avatar model (GLB/VRM)

### Stage 6: Visual quality
- [ ] HDR + tone mapping, bloom for neon
- [ ] Shadows from lamps
- [ ] Reflections on bar top/floor, haze/light shafts

### Stage 7: Imports
- [ ] GLB model loader (moved up: it's W2 in Stage W)

### Stage W: World simulation (new north star, 2026-10-15)
- [x] W1 Material system (`engine/material.h`, `app/materials.*`: look + physics + sound in one record) + PBR shading (GGX, metal/roughness, sphere-light highlights, approximate room reflections)
- [ ] W2 GLB loader + asset pipeline
- [ ] W3 Real assets, one piece at a time (bar counter first)
- [ ] W4 Rigid-body physics driven by materials
- [ ] W5 Physical sound: synthesized impacts, room acoustics
- [ ] W6 Heat and fire (candle flames, steam, melting ice)
- [ ] W7 Liquids (pour and mix drinks)
- [ ] W8 Smell/air field (spreading haze), day/night and weather outside

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
