# Looped 2: the whole engine, rebuilt around loops (planned 2026-10-21)

**Top priority** (user, 2026-10-21, ahead of everything else, after Looped A-C).
The user: "full engine from top to bottom", "the logic is the main thing",
"if it exists, it has a full loop already live at all times", "cut it up into
loop pieces, like the position of mass in comparison to each other", "don't
create more work: make something that grows". Most of today's code may be
replaced. 3D models wait (Blender makes whatever the engine needs for now).

## The idea in plain words
Everything in the world is a **thing**. Every thing:
1. sits **relative to another thing** (a glass on the counter, a hand on an
   arm, a moth around a lamp). Move the parent, everything on it follows.
   That's the "position of mass in comparison to each other".
2. is always in a **state** (idle, in use, held, open, playing...).
3. and every state has a **loop**: a short animation made of **pieces**
   (move, turn, scale, glow, colour, sound...) that plays forever on one
   shared clock. Nothing is ever frozen: a stool creaks a little, a neon
   sign buzzes, the jukebox bounces to the beat, a glass catches the light.
4. **Rules** say what changes the state: "tapped → in use", "song ends →
   idle". When a state changes, the new loop blends in smoothly.

Because every loop is worked out from the clock, the whole world is known
from very little: **which state each thing is in and when it started**. So:
- multiplayer only sends state changes (tiny), every phone plays the rest
  itself, identically;
- the server runs the same rules (the engine's C++ already runs on the
  server as WebAssembly) and refuses cheats;
- the hub can show and change any thing live (its loop's speed, colour...);
- a new object is a **file**, not code. The engine grows by adding new
  kinds of pieces and parts, never by special cases.

## The vocabulary (the whole engine is these words)
| Word | What it is | Example |
|---|---|---|
| **Thing** | anything in the world, with a stable id | `stool@-2.5,-1.3` |
| **Parent / anchor** | the thing it's placed on, with an offset | glass → counter |
| **Kind** | a file that says what a thing is: parts, states, loops, rules | `kinds/stool.json` |
| **Part** | an ability a thing has; each engine system reads its own part | model, collider, seat, tap, light, sound, shared, price |
| **State** | what it's doing now | `idle`, `spinning`, `held` |
| **Loop** | the animation of a state, repeating on the clock | idle sway, 4 beats |
| **Piece** | one channel of a loop: a curve over the loop's time | `rotate.y: wave 3°`, `glow: flicker` |
| **Rule** | when → then | `on tap: state spinning for 2 beats` |
| **Clock** | the room's time + the music's beat | shared by everyone |
| **Place** | a file listing things and where they sit | `places/bar.json`, later a home, the mall |
| **Catalog** | every kind you can own: price, tradeable, category | the mall, homes, the VIP shop read it |

Pieces come in a few shapes, all pure maths on the clock: **wave** (sine,
smooth), **keys** (keyframes), **noise** (seeded, never repeats visibly),
**orbit** (around the anchor: moths, planets), **beat** (thump on the music),
**follow** (track another thing: eyes look at you). Lengths that don't line
up (3, 7, 11 s) plus a seed per thing, so two identical stools never move
in step. Today's loops (swarm, pulse, flicker) become pieces.

Free motion (a thrown glass, a car, a walking player) is physics or input,
not a loop: while free, the thing's position comes from the simulation; when
it comes to rest it **settles** back into its idle loop on its new anchor.
Games with their own deep logic (chess rules, car physics, 4K Mania) keep it
inside them; on the outside they're things with states and loops like
everything else (a chessboard is `idle`/`playing`, its pieces are things).

## The layers (top to bottom, all of them rebuilt on things)
1. **Core**: clock, frame phases (moved from bar/game.cpp), jobs, memory
   budgets, logging, files.
2. **World**: things, parents, kinds, parts, places (files), ids that never
   change. Replaces bar_scene.cpp's hand-written layout and bar/entities.
3. **Logic**: states, rules, events. The same code runs on the phone and
   the room (WebAssembly). Replaces the per-feature code in activity,
   interaction, shared_world.
4. **Loops**: pieces, blending between states, beat lock. Grows out of
   looped/loops.*. **Rule enforced by a test: every thing has a live loop.**
5. **Multiplayer**: a thing with the `shared` part syncs by itself: state
   changes + free motion, checked by the room's copy of the rules. Lobbies,
   instances and travel between places come on top. Replaces the glasses,
   jukebox and darts sync code.
6. **Rendering**: draws things. Baked light, probes and reflections
   (`docs/LIGHTING.md`), shadows for moving things, particles (a particle
   is a tiny thing with a loop), detail levels, quality tiers.
7. **Animation**: the avatar is things too (each bone a thing with its
   parent), so walk, sit, sip, dance are loops with blending; IK for hands
   and feet.
8. **Physics**: colliders from parts, triggers (zones as things), free and
   settle, placement checks (homes' grid).
9. **Sound**: sounds are parts of things, in 3D (louder close, muffled
   through walls); voice fades by distance; the music drives the beat clock.
10. **Assets**: the catalog; models streamed by place and distance; Blender
    makes and exports what's needed (models and, later, places).
11. **UI**: the HUD and shared screens (inventory, shop, profile, queue)
    read things and the catalog.
12. **Tools**: the hub's live view of things (list, state, loop speed,
    colour, mood), tests, screenshots, profiler.

## Build order (each step ends with the bar working; batched deploys per "go")
- **E1 The core** (no visible change): thing tree, kinds, parts, places
  files, states, rules, loops with pieces, the clock. Pure logic with native
  tests, runs in the browser and in the room.
- **E2 The bar as things**: bar_scene.cpp → `places/bar.json` + `kinds/`;
  every thing gets its idle loop (first visible change: the whole bar
  alive); today's loops → pieces; the test "every thing has a loop".
- **E3 Logic and sync**: taps, seats, the jukebox, darts, glasses through
  rules; the room runs them; the old sync code removed.
- **E4 Drawing things**: renderer reads things; lighting (bake, probes,
  reflections), particles, shadows.
- **E5 The avatar as things**: bones, loops for walk/sit/sip/dance, blending,
  IK; other players the same.
- **E6 Sound and physics on things**: 3D sound, triggers, free/settle.
- **E7 Places, catalog, travel**: the catalog, a second place (a test home)
  to prove "a new place is a file", lobbies and instances.
- **E8 The hub as the control room**: live list of things, loops and states,
  change them from the phone.
- **E9 The projects re-plugged** (racing, chess, mania, Meridian, platformer)
  and every old path deleted.

## How we know it's right
- A new object = one kind file (+ a model). A new place = one place file.
- No feature code in the engine knows the bar exists (CI grep, as today).
- Every thing in every place has a live loop (a test).
- Two phones (headless) show identical loops and states (mp_test).
- The bar keeps working after every step: the full test list in CLAUDE.md.

## Where things are (2026-10-21)
- `looped/world/loop.*`: the clock, channels, pieces (shapes: hold, wave, keys,
  steps, noise, flicker, orbit, beat, spin), loops.
- `looped/world/kind.*`: kinds (parts, loops, states, rules, things inside,
  pivot, "is" to build on another kind, "still" with a reason), places (rows).
- `looped/world/world.*`: the running world: things on things, states, rules
  (`send`, `sendAll`, `setState`), timed states worked out from the clock (a
  late joiner skips whole rounds), `Change` for the network, `apply`,
  `attach` (re-parent, no jump), `check()` (every thing alive).
- `assets/world/kinds/bar.json`: every kind in the bar; `assets/world/songs.json`:
  the built-in songs' tempo and first beat.
- `bar/bar_place.*`: the bridge from today's layout (bar_scene.cpp) to things;
  `bar/world_bar.*`: plays the world each frame and hands it to the renderer
  (`setModelPose`, `setModelLook`, `setLightScale`, glowing boxes).
- Tests: `tools/native/world_test.cpp` (the core), `bar_world_test.cpp` (the
  real bar: every thing alive, the bar's events).

## How to add life (for Claude)
- A new loop for an object: edit its kind in `assets/world/kinds/bar.json`
  (a model's kind is its file name). Units: metres, degrees, seconds or beats.
- Something new that lives on its own (moths, sparks): a kind with a
  "glowbox" part, placed in `bar/bar_place.cpp` → kExtras (until places are files).
- A reaction: a state with a rule (`"on": {"music": "playing"}`); the bar sends
  "music" / "quiet" and "fire-on" / "fire-off" to every thing.
- Run `tools/native/bar_world_test` after: it fails if anything isn't alive.

## Progress
- [x] Plan agreed in chat (2026-10-21).
- [x] **E1 the core** (2026-10-21): everything above in `looped/world/`, 70 checks;
  4,000 things update in under 1 ms (native and wasm).
- [x] **E2 first pass** (2026-10-21): the bar as 146 things (106 models, 29
  lights, 5 moths, 6 sparks), 42 kinds; 138 alive, 8 still with a reason (room
  shells, the mirror). Stools drift on their swivels, tables rock a hair, lamps
  sway with their moths, candles and the fire flicker (out = embers), the
  jukebox and 4K Mania cabinet thump on the beat, back-bar lights breathe,
  glass and polish catch the light, lights breathe/buzz/groove. The old loop
  system is gone. Renderer: model moves batched into one upload per frame.
- Phone check of E2 (report 63H3FJ, build 81eeb58, 8 min): avg 16.7 ms (60
  fps), p95 17, CPU 1.1 ms, GPU 8.8 ms, 144 draws / 160k triangles, heap 80 MB,
  no errors. The world costs ≈ 0.25 ms a frame (the "scene" phase; 0.03 before).
  One 35 ms CPU spike in "interaction" (no touch): watch if it repeats.
  User: "perfect everywhere, I love it".
- [ ] **E2 second pass, with E3**: the layout itself into `places/bar.json`
  (seats, colliders, things to use become parts), bar_scene.cpp retired.
- [ ] E3 logic and sync: taps and seats through rules; `Change` over the room
  protocol; the room runs the world (cloud/sim) and has the last word.
