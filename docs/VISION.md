# Vision: the full picture

This is the user's long-term dream for the project, collected from the
planning conversation. `PROGRESS.md` holds the current, concrete roadmap.
This file explains why it exists and where it's heading.

## Why this project exists
- It's the user's first project, built to **showcase 3D techniques** and to
  find **the limits of what Claude Code can build**. Boundary-pushing is the
  point.
- The engine is **from scratch in C++**: no Three.js, Unity or Godot. The user
  explicitly wants to build the groundwork themselves (with Claude) "so it
  really works."
- Everything is tested on the user's **iPhone 15 Pro Max (iOS 26, Safari)**,
  their only device.

## The main thing: a VRChat-style 3D hangout bar
"Like a real-life 3D hangout bar would look", built as a game plus engine
that keeps getting upgraded:
- **Realistic, cozy bar**: warm lamps, neon signs, polished wood, bottles,
  stools, booths, tables, a jukebox, a door.
- **Everything is interactive**: pick up and throw glasses, flip light
  switches, use the jukebox, play darts and pool, sit on stools, open doors.
- **A 3D avatar you can see in a mirror**, like VRChat's mirrors.
- **Real background music**: a smooth-jazz ambiance from the jukebox, heard
  from its spot in the room.
- **Top visual quality**: full audio and visuals, glow, reflections, shadows.

## Music (decided)
1. **CC BY smooth-jazz tracks** (Kevin MacLeod) as the bar's ambiance,
   already in `assets/music/`.
2. **The user's own songs**, loaded from the iPhone Files app into the
   jukebox. Local only, never uploaded or committed.
3. **Fun extra: a mini music studio (DAW) inside the bar**: synths, drum
   machine, sequencer, piano roll, mixer and effects, all C++ DSP. Songs made
   there can play on the jukebox. The DSP could later become a real **VST
   plugin** on desktop (iPlug2/JUCE).

## Mirrors and avatar (user's wishes, 2026-10-15)
- Two mirrors like VRChat's: one that reflects everything (a real
  mirror) and one that shows just the avatars / 3D models.
- An avatar with bones, body shapes and textures, then animations, so the
  bar feels real.
- The jukebox works like a real one: pick a song from its menu. Songs the
  user downloads can be loaded for testing.

## Later (explicitly NOT now)
- Importing 3D models (GLB/glTF loader written from scratch) and custom
  avatars (VRM).
- Multiplayer: friends join by link and see each other. This needs a server
  beyond GitHub Pages.
- Voice chat (WebRTC).
- Possibly C++ physics/simulation done in WebAssembly, AR export (USDZ for
  AR Quick Look), and a native iOS app (needs an Apple Developer account).

## Showcase ideas to work in along the way
GPU particles with compute shaders, raymarched effects, volumetric light
shafts or haze, real-time reflections, procedural textures, audio-reactive
visuals tied to the jukebox, and a day/night view through a window.
