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
- The Studio plan, step by step: S1 and S2 done (2026-10-22), S3 next. The user
  picked the **Faceplate** theme (the others stay switchable).

## The next step: S3, change things and publish (docs/STUDIO.md §8)
How (decided in S2): the Studio keeps the draft as a bundle (the files the
engine runs); an edit is an op (`studio/ops.mjs`: set / add / remove /
attach, each with its inverse) applied to the draft *and* sent to the engine
(`st.set` moves a thing at once; anything else rebuilds the scene from the
draft). Undo = the inverse op. The draft saves itself to `api.php` (save);
Publish → `api.php` (publish) → the bar loads it on its next start.
- [ ] **S3.1** Edit ops + undo + the plain-words diff (`studio/ops.mjs`, `studio/diff.mjs`; node tests on the real bar files). *Check:* nothing to see (tests).
- [ ] **S3.2** Move / Turn / Size: axis arrows on the selection + the dial strip (axis keys, world/local, value with ±, the fader with slide-up fine control, snapping with a tick). *Check:* move a table exactly 0.25 m with the number field.
- [ ] **S3.3** Slide on surfaces (drag the thing itself), Drop to the floor, Put on… (re-parent). *Check:* slide a glass along the counter; put it on a table; move the table, the glass follows.
- [ ] **S3.4** Undo/redo (keys + two/three-finger taps), the draft saving itself (save lamps), Changes in plain words in the Monitor. *Check:* three edits, two-finger tap twice, one left; reopen the Studio, it's still there.
- [ ] **S3.5** Publish (only when Problems are clear), History, Restore. *Check:* move a table, Publish, open the bar: it moved. Restore the version before: it's back.

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
- (this push) S2.1-S2.5: the Studio opens (/studio: login, home-screen app, cameras, picking, the Monitor, X-ray) → 5a04c79..HEAD
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
