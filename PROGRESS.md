# Progress

**Live site:** https://barcode162.github.io/3d/
**Current focus:** Stage 1, Engine foundation

## The vision
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
- [x] 1.1 Pipeline proof: C++ → WASM → WebGPU triangle, tap to change color, on-screen log, auto-deploy
- [ ] 1.2 Math library: vectors, matrices, quaternions
- [ ] 1.3 3D meshes + depth buffer: procedural boxes, cylinders, spheres
- [ ] 1.4 First-person camera + touch controls (left stick move, right drag look)
- [ ] 1.5 Lighting: multiple warm point lights, basic PBR materials
- [ ] 1.6 Text rendering in-engine (bitmap font) for labels/UI

### Stage 2: The bar room
- [ ] Room shell: floor, walls, ceiling, procedural wood/brick/tile textures
- [ ] Furniture: bar counter, stools, booths, tables, shelves of bottles
- [ ] Lighting set: pendant lamps, neon signs, candles
- [ ] Collision so you can't walk through walls or furniture

### Stage 3: Interaction ("everything is interactable")
- [ ] Tap-to-select (raycasting) and an interaction prompt
- [ ] Pick up / drop / throw objects (simple physics)
- [ ] Light switches, doors, taps, jukebox buttons
- [ ] Bar games: darts, pool table

### Stage 4: Audio
- [ ] C++ audio engine through Web Audio (starts on first tap)
- [ ] Jukebox plays the smooth-jazz playlist (`assets/music/`), spatialized in 3D
- [ ] Jukebox "load song" button: play the user's own songs from the iPhone
      Files app (local only, never uploaded)
- [ ] In-game credits for the CC BY tracks (see `assets/music/CREDITS.md`)
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
