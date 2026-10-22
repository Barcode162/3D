# NEXT: what "go" does next

The user says "go"; Claude does the whole step below, piece by piece (one
commit each, tagged), deploys once, then rewrites this file for the step after.
How: `CLAUDE.md`. The engine: `docs/ENGINE.md` (E1-E9, built). **The plan
now: `docs/STUDIO.md`** (the Studio, the hub as the Dials, the new look;
steps S1-S10).

## Where we are (2026-10-21)
- The engine plan E1-E9 is built and live (`22e9b28`).
- The user is phone-checking it (the list in the chat). Any "broke <piece>"
  is fixed first, before the step below (CLAUDE.md → Pinpointing breakage).
- The Studio plan, step by step: S1 done (2026-10-22), S2 next. The user
  picked the **Faceplate** theme (the others stay switchable).

## The next step: S2, the Studio opens (docs/STUDIO.md §8)
How it's built (decided in S1): the Studio page (`studio/index.html`) wraps
the bar's own page in an iframe, `/bar/?studio=1` (same origin: the page
calls into it directly); the bar in studio mode has no room, no HUD, no
body, a free camera, and taps pick things instead of using them. The C++ side
is `bar/studio/` (studio mode, cameras, picking, overlays); the Studio's
panels are `studio/studio.js` + `/ui/`. Messages: page → game through
`bar3dBridge.command('studio', ...)`, game → page through a `studio` op.
- [ ] **S2.1** joshuavelasco.com/studio: login (owner/admins), home-screen icon (manifest), the bar's engine in studio mode in a full-screen frame, the top and bottom bars. *Check:* open it, Add to Home Screen, open from the icon: your bar, full screen, nobody in it.
- [ ] **S2.2** Cameras: one finger orbits, two fingers pan and pinch, double-tap frames a thing; Fly, Top, Walk; saved views. *Check:* fly over the counter, switch to Top, back.
- [ ] **S2.3** Picking: tap → the thing glows in the selection colour + breadcrumb (Bar › parent › thing); the overlap list when several are under your finger. *Check:* tap a glass on the counter → "Bar › … › pint_glass-4".
- [ ] **S2.4** The Monitor drawer: World list (by parent, search), Inspect (read-only, from the schema), Problems (schema + life checks). *Check:* search "lamp", tap it, the camera flies to it.
- [ ] **S2.5** X-ray layers: tap areas, shapes you bump into, seats, lights' reach, where people appear. *Check:* turn on "tap areas": boxes show around the jukebox, the darts, the mirror.

## Waiting on the user: P1, the engine phone check
- [ ] **P1.1** Fix every "broke" the user reports (one commit each, tagged with the piece it fixes).
- [ ] **P1.2** A `?perf=1` report in the busy part of the bar (by the fire, the mirror in view): compare with E2's 63H3FJ (16.7 ms avg); if reflections or shadows cost more than 1 ms, make them cheaper.

## Done before (details in git and docs/ENGINE.md → Progress)
E1-E2 (core, the bar as things), E3.1-E3.7 (touch, seats, jukebox, darts,
late joiners, mirror), E4.1-E4.4 (precise touch, shadows, smoke,
reflections), E5.1-E5.2 (idle life, smooth poses), E6.1-E6.2 (sounds),
E7.1-E7.4 (boxes, the test home, travel, catalog), E8.1-E8.2 (the hub's World
page, tuning), E9.1-E9.2 (projects as a part, old keys gone).

## Build log (newest first: build code → what → commits; "undo" reverts one row)
- (this push) S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..HEAD
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
