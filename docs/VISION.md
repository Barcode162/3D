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

## North star (updated by the user, 2026-10-15)
**Build a world simulation, not a set of props.** The bar stays the place
we build first, but the real goal is engines that simulate what the senses
perceive (sight, sound, feel, and more), so that real behavior *emerges*
from the systems instead of being faked one object at a time. Every engine
we add should make every other engine more powerful: "it stacks".
Highest quality is the bar for everything, so genuine world building
(real shapes, real materials, real physics) is required, not optional.

How that turns into engineering (Claude's plan, user agreed to let Claude
decide the details and only review/critique):
1. **One material = every sense.** Each surface type (oak, glass, brass,
   leather, brick, cloth...) is defined once with its look (PBR color,
   roughness, metalness, normal detail), its physics (density, friction,
   bounciness, how it breaks) and its sound (what it sounds like when hit,
   scraped, or walked on, and how much echo it soaks up). Add an object
   made of glass and it automatically looks, moves and sounds like glass.
2. **Sight:** physically based light (PBR), shadows, bloom, reflections,
   later global illumination.
3. **Feel / touch:** rigid-body physics (pick up, throw, stack, knock
   over), then soft things (cloth, cushions), then liquids (pouring
   drinks). Feel on screen = weight, inertia, footsteps that match the
   floor. (iPhone Safari has no vibration API, so no haptics.)
4. **Sound:** sounds *generated from the physics* (a glass clinking is
   synthesized from the collision and the material, not a recorded clip),
   room echo computed from the room's shape and materials, sound blocked by
   walls and doors.
5. **Beyond the senses' basics:** temperature and heat (candle flames, hot
   coffee, ice melting in a drink), smell as an invisible simulated field
   that spreads through the room (shown as subtle haze/particles), time of
   day and weather outside the window, wear and dirt that build up where
   things are used.
6. **Real assets:** every piece of the room gets replaced with a properly
   modeled one, one piece at a time (see "Art upgrade").

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

## Art upgrade (user, 2026-10-15; decided by Claude)
The blockout looks like "basic AI 3D". Rework the whole room into
real-looking assets, one piece at a time, big to small: bar counter →
walls/floor → stools → back bar and bottles → tables and booth → jukebox →
small props. Sources, mixed per piece:
- Free CC0 models and textures (Poly Haven, ambientCG) where they fit.
- Custom pieces modeled by Claude with scripted tools (e.g. Blender run
  headless) for the signature items (counter, jukebox, neon).
- The user's own GLB files (e.g. from a phone AI-3D app) whenever they like.
The user's role: look at screenshots / the live site and critique.

## Later (explicitly NOT now)
- Custom avatars (VRM). (The GLB loader itself moved up: it's needed now.)
- Multiplayer: friends join by link and see each other. This needs a server
  beyond GitHub Pages.
- Voice chat (WebRTC).
- Possibly C++ physics/simulation done in WebAssembly, AR export (USDZ for
  AR Quick Look), and a native iOS app (needs an Apple Developer account).

## Showcase ideas to work in along the way
GPU particles with compute shaders, raymarched effects, volumetric light
shafts or haze, real-time reflections, procedural textures, audio-reactive
visuals tied to the jukebox, and a day/night view through a window.


## Your own avatar (2026-10-16)
The user wants an avatar that really looks like them: realistic, Blender-quality modeling with a full bone rig, ideally built from a 3D scan of their face (e.g. an iPhone face-scan app exported to GLB/OBJ) so it's accurate. A rough procedural version based on their photo is in `app/avatar.cpp` for now. Note: everyone currently shares the same avatar body.

## Added 2026-10-16 (the "make everything work" request)
- Voice chat with WebRTC, as close to lossless as possible; voice settings:
  a threshold (noise gate), playback to test your mic.
- A settings menu with personal settings; an interactive menu for emotes,
  switching avatars and editing a profile "with lots of details".
- MIDI support for the piano; the top 10 classical songs preloaded; the
  keys and the strings must match what's being played.
- Sit on every chair (everyone can), an animation for every action.
- Darts must work; the racing game must work with the character visibly
  driving and others seeing the driver's screen; same for 4K Mania; one
  person per machine.
- A badge that lets people become bartenders and make drinks.
- Everything that looks usable should be usable. First everything in "very
  low format" (working, not pretty), then a remaster pass
  (`docs/REMASTER.md`).

## Racing sim (2026-10-16)
A genuine racing sim on the two rigs, alone or two players (left and right
rig), with bots you can bump into like Need for Speed, physics like
Assetto Corsa, amazing looks in the end. Start with the best possible shell
and structure (driving feel first), then polish. Touch controls must feel
buttery smooth with big steering travel; also keyboard, mouse and real sim
controls (wheels, gamepads). Sitting down dives you in: the whole screen
is the game (sideways); spectators see the race on the rig's three
monitors. Every mini game should become a really powerful game in its own
right. Also: a real realtime sync server (not phones connecting to each
other). Plan: `docs/RACING.md`.
