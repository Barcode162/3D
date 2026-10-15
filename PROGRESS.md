# Progress

**Live site:** https://barcode162.github.io/3D/
**Current focus:** Stage 3 interaction + Stage 4 audio (jukebox works; next: more things to use)

## Next up (start here in a new session)
1. **Wait for the user's answers** to the "what do we build ourselves" list
   (given in chat on 2026-10-15; summary in `docs/DECISIONS.md` → "Build
   ourselves vs. use the platform"). Their answers may reorder this list.
2. **Load your own songs into the jukebox** from the iPhone Files app
   (an `<input type=file>` bridge; local only, never uploaded).
3. **More things to use**: light switches (dim/brighten the lamps), the
   door, glasses on the bar. Each is an `Interactable` in `bar_scene.cpp`
   plus an `Action`.
4. Small polish noticed on iPhone: the candle spheres on the tables look
   like white balls (make them real little candles with a flame glow), and
   the stools look plain.

Last verified on iPhone: build `c1cc070` (textures), 60 fps, loads in under 1 s.

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
- [ ] 1.5 Lighting: 8 warm point lights + emissive + haze + tone mapping done; still to do: real PBR materials
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
- [ ] Jukebox "load song" button: play the user's own songs from the iPhone
      Files app (local only, never uploaded)
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
- [ ] Mirror: render the scene from a reflected camera
- [ ] Procedural humanoid avatar with skeleton + walk/idle animation
- [ ] Avatar follows your movement; visible in the mirror
- [ ] Later: load your own avatar model (GLB/VRM)

### Stage 6: Visual quality
- [ ] HDR + tone mapping, bloom for neon
- [ ] Shadows from lamps
- [ ] Reflections on bar top/floor, haze/light shafts

### Stage 7: Imports
- [ ] GLB model loader, so you can bring your own 3D models into the bar

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
