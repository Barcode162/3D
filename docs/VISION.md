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

- **Mirror quality (2026-10-17):** your character in the mirror always looks
  full quality, and so does everything near you; only far-away things in the
  reflection may be lower quality (planned in RESTRUCTURE 6.3).

## The business goal (user, 2026-10-18)
The user wants this to become a product people pay for: **digital goods
inside a social 3D space** (mostly 3D scenes, plus the social side). The
target is enough income to cover the family's bills (housing, car, gas,
food) with a little left to reinvest, so the user can become a full-time
developer. They didn't set out to build a VRChat/Rec Room-style hangout,
but avatars + mirrors + multiplayer + voice made it one, and they're open
to leaning into that. They have about $100 to spend on upgrades (more
Claude, tools, or servers) and are open to changing fundamental pieces.
Next: a frank review of what exists, then a business + tech roadmap
(still to be written together).
- **More facts (2026-10-18):** Claude plan is the $20/month one; the $100
  is one-time (could become monthly if it pays off). IONOS hosting ($1 for
  the first year) and the domain ($1 + $1 domain guard) are one-year promos.
  The web stack was chosen only because the user works from a phone, not
  as a goal. The user sees the current bar as a small sandbox, is fine with
  restarting or changing the tech stack, and now wants a serious, clear
  direction: something that makes money and that they're proud of.

## The upgrade list (user, 2026-10-19)
Full checklist: `docs/UPGRADE_PLAN.md`. The big ideas in it: the two racing
rigs are for racing each other like Mario Kart (pick and customize your car);
steering must feel like a real car's wheel (self-centering, a real turning
circle, not slippery) and will be reused by later games; the second room
becomes an arcade (carpets, lights, arcade music that grows as you walk in)
with a platformer machine running the user's game "Test 2"; 4K Mania should
feel like osu!mania with more songs; real avatar customization (gender,
clothes, hairstyles, hair highlights and gradients) and more realistic
animations; a working chessboard; and a pass over the whole bar so nothing
clips, floats or traps you.

## The product (user, 2026-10-19): a virtual life in the browser
The direction that answers "The business goal" above. Gameplay and
smoothness come first; money never gets in the way of play.
- **Core loop:** you start at your **home**, jump to one of a **few main hubs**
  (the bar is one; few on purpose, because repetition builds habit), play,
  watch or queue, earn currency, grow your home and your look, show off.
  Progress is the point: socialize *and* grow your account.
- **Ages:** 13+ only. Teens (13-17) and adults (18+) are kept apart in lobbies,
  voice and messages. Under 13 would be a separate kids' edition (parked).
- **Lobbies:** smart matchmaking by age band, game preferences and skill (so you
  don't always lose). Every machine screen shows the real game live, simplified
  and pixelated (combos, speed). A fair **queue**: when your turn comes you get
  20 s to reach the machine, else the next person goes. A "Happening now" list
  makes it easy to reach the social part. A **no-friends lobby** for people
  who want to be left alone.
- **Homes:** invite-only, customizable on a smooth grid, expand up to 3x, levels
  1-3 (everything nicer). You can buy things like arcade machines for it.
- **Currency:** a made-up name (with the world's new name), valued like the
  dollar so prices are intuitive. Earn 5-50 per game (by length/difficulty),
  daily caps.
- **The mall:** an in-game store like IKEA: walk in, pick items up, cart,
  checkout, delivered home, install on the grid. Furniture etc. comes from here.
- **Casino (18+ only):** high risk, high reward (a rare 1k win). Earned currency
  only: never bought with real money, never cashed out (gambling law). Lawyer
  review before launch.
- **VIP marketplace (real money):** its own special place that screams "big
  baller", 5-star treatment. Any first purchase unlocks its door forever;
  others see the closed door and the glow. Rarity has real value: numbered
  items, seasonal collections that never return, event-only items (yearly and
  hosted events). Not daily rotations. Real money buys items directly, never
  currency. Payer must be 18+ (parents for teens).
- **Trading:** only items made to be tradeable (a small special set, so they're
  worth more); trade window both confirm, checked by the server, cooldown.
- **Events:** anyone can create one: date, time, RSVP, a clean invite card.
- **Memory book:** in-game camera only (no real photo uploads); a picture book
  you're proud of; likes, no comments; per-photo privacy; you can hide yourself
  from others' photos.
- **Talking:** voice is the main way. Teens: quick phrases + a phrase builder
  (fixed words and patterns, no hidden messages). Adults: free text, filtered.
  Block, mute and report everywhere.
- **Later:** a street and a city outside; homes as your "real home" there.
  Characters from the user's own character builder (in progress).

## Driving rework (user, 2026-10-19)
"A full game rework" of the driving, not a polish: the best possible,
fully planned physics (tires, suspension, aerodynamics, everything), the
feel of Drive Zone Online. Cars: a C7 Corvette "at least", the Toyota
GR86, the RX-7 and the Miata. Circuit racing first, plus a drift area and
open free drive. "Not the least amount of work: the absolute best possible
detailed plan from A to Z." Plan: `docs/DRIVING_REWORK.md`.
