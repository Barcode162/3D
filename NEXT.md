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
- The Studio plan, step by step: S1, S2 and S3 done (2026-10-22), S4 next. The user
  picked the **Faceplate** theme (the others stay switchable).

## The next step: S4, the new look everywhere (docs/STUDIO.md §6, §8)
How: the hub page keeps its login and data (`hub/hub.php`); its screen is
rebuilt on `/ui` (Faceplate) with four tabs. Dials reads the same files as
the Studio (`studio/draft.mjs` is shared: the same draft, the same Publish).
Tuning (E8.2's speed/off) folds into real loop periods in the world.
- [ ] **S4.1** The hub as Needs you · Dials · Board · News; Dials = every field of the world from the schema (numbers with units and a fader, choices as chips, search), editing the Studio's draft, Publish; "Open in Studio" on every thing (`/studio/#thing=<id>` selects it). *Check:* change the pendant lamps' swing period in Dials, Publish, the lamps in the bar swing faster.
- [ ] **S4.2** Tuning retired: the old speed/off values become loop periods in the world (one version published by Claude); the game stops asking `hub.php?tuning`. *Check:* the lamps still swing as before.
- [ ] **S4.3** The bar's menus and HUD in the new look (line icons from `/ui/icons.svg`, Faceplate keys and panels; the old Clash-style font goes). *Check:* open Menu in the bar.
- [ ] **S4.4** News and the bar's loading screen in the new look. *Check:* joshuavelasco.com/news and the bar starting up.

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
- (this push) S2.1-S2.5, S3.1-S3.5: the Studio opens (login, cameras, picking, the Monitor, X-ray) and edits (Move/Turn/Size, slide, Drop, Put on, undo, the draft, Publish, History, Restore) → 5a04c79..HEAD
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
