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
- Then the Studio plan, step by step.

## The next step: S1, one world and the new look begins (docs/STUDIO.md §8)
- [ ] **S1.1** The look, the faceplate: `ui/` (faceplate/smoke tokens, keys, dials, faders, the step light, the LED, Archivo + Martian Mono + OFL + credits, ~40 line icons) and the style page joshuavelasco.com/ui. *Check:* /ui shows every control on the smoke panel; phone in light mode → the faceplate.
- [ ] **S1.2** The front door in the new look: joshuavelasco.com with three doors, Bar · Studio · News. *Check:* the home page.
- [ ] **S1.3** The schema `assets/world/schema.json` (every part and field in words, units, ranges, controls; the choice lists move out of `bar/scene_names.cpp`) + the completeness test. *Check:* the bar unchanged, still all alive.
- [ ] **S1.4** The world store `studio/api.php` (live, bundle, draft, publish, history, restore; owner/admins + Claude key) + `tools/studio_api_test.mjs`; v1 = today's repo world. *Check:* /studio/api.php?live says version 1.
- [ ] **S1.5** The bar and the room load the live world (built-in fallback, format number, the room's version wins). *Check:* the log says "World: v1 (live)".
- [ ] **S1.6** Claude's side: `tools/studio.mjs pull/push/log`, the two lines in CLAUDE.md's routine, E8 tuning folded into real numbers (tuning.json retired). *Check:* the bar unchanged; the hub's World page still lists everything.

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
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
