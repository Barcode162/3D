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
- The Studio plan, step by step: S1-S4 done (2026-10-22), S5 next. The user
  picked the **Faceplate** theme (the others stay switchable).

## The next step: S5, precision (docs/STUDIO.md §5.3-5.4, §8)
- [ ] **S5.1** The magnifier: hold a finger still on the world, a lens shows what's above your finger, bigger, with its name; let go to select it. (🔒 lock is from S2.3.) *Check:* long-press a candle on a table: the lens shows it; let go, it's selected; lock; orbit; still selected.
- [ ] **S5.2** Part mode: Thing | Part; a selected thing lists its parts (shapes you bump into, tap areas, seats, light, sound); a part glows as a box and Move / Size edit it (shown at once; the bar has it after Publish). *Check:* select the jukebox's tap area, make it wider, publish, it's easier to tap in the bar.
- [ ] **S5.3** This one | Every <kind> (N): edits go to the thing or to its kind; dots on fields that differ from the kind; Reset to kind; New kind from this. *Check:* make every bar stool 5 cm taller in one change; make one of them different.
- [ ] **S5.4** Inspect becomes editable: every field from the schema with the right control (numbers with a fader, choices as chips, switches, colours). *Check:* change a seat's facing in Inspect.

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
- (this push) S4.1-S4.4: the hub as Needs you · Dials · Board · News, tuning baked into the world, the bar's menus, news and loading screen in Faceplate → 8797ac4..HEAD
- `8797ac4` S2.1-S2.5, S3.1-S3.5: the Studio opens (login, cameras, picking, the Monitor, X-ray) and edits (Move/Turn/Size, slide, Drop, Put on, undo, the draft, Publish, History, Restore) → 5a04c79..8797ac4
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
