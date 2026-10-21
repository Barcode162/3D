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

## Progress
- [x] Plan agreed in chat (2026-10-21). Next: E1 on "go".
