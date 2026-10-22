# NEXT: what "go" does next

The user says "go"; Claude does the whole step below, piece by piece (one
commit each, tagged), deploys once, then rewrites this file for the step after.
How: `CLAUDE.md`. The engine: `docs/ENGINE.md` (E1-E9, built). The Studio:
`docs/STUDIO.md` (S1-S10, built). **The plan now: `docs/PLAN.md`** (the
Alive plan, replanned 2026-10-22: tracks L M T R A W, wow and foundation
take turns, every visual piece has an off switch and before/after pictures,
every piece writes its page in `docs/manual/`).

## Where we are (2026-10-22)
- The engine plan E1-E9 is built and live (`22e9b28`).
- The user is phone-checking it (the list in the chat). Any "broke <piece>"
  is fixed first, before the step below (CLAUDE.md → Pinpointing breakage).
- **The Studio plan S1-S10 is built** (2026-10-22). Checks: per piece only its own test, the full check once per step (user). The user
  picked the **Faceplate** theme (the others stay switchable).
- Another Claude session built the website (homepage/: Home, Play, Engine, Vision, Sign in, the staff Portal); it owns homepage/.

## The next step: L1, new light (docs/PLAN.md §4.1)
Today: point lights with GGX highlights, a flat ambient colour, one captured
reflection cube, blob shadows under avatars only, tone mapping inside the
scene shader (LDR), bloom and grade in post. L1 makes it a different bar.
Each piece: an off flag, `tools/shots.mjs` before/after from the same spots
(bar top, fire, back corner, stage), its manual page `docs/manual/light.md`.
Budget for the whole step on the phone: +4 ms at High, +1.5 ms at Low.
- [ ] **L1.1** HDR and eyes that adjust: the scene draws into rgba16float, tone mapping moves to post (a softer filmic curve), auto exposure from a tiny average of the picture (eyes adapt over ~1 s), bloom from real brightness. Settings → Graphics gets **Lighting: New / Classic** so the user can flip old and new on the phone (also `?light=classic`). *Check:* walk from the fire to the dark back corner: the picture brightens slowly as your eyes adjust; bulbs glow without white blobs.
- [ ] **L1.2** Real shadows from lamps: the strongest lights (a Studio "casts shadows" switch, 4 at most) get shadow cubes in one atlas, soft edges that sharpen near contact; still things are drawn into them once, redrawn only when something near the light moves. `?lampshadows=0`. *Check:* the stools and bottles throw shadows on the bar top; a glass you pick up casts one that moves.
- [ ] **L1.3** Bounced light the engine works out: a grid of light probes in each room; each one captures a small cube of the room (the E4.4 capture) and keeps it as 9 numbers per colour; two bounces; spread over frames so you **watch the room fill with light** for a few seconds after loading; re-done after a Studio light edit or a publish. Replaces the flat ambient. Probes stuck in walls move out or switch off. `?bounce=0`. *Check:* the ceiling glows warm above the lamps, corners go darker, the red neon tints the wall next to it.
- [ ] **L1.4** Contact shade: half-resolution ambient occlusion from the depth buffer, blurred, only where things meet (cheap at Low: off). `?ao=0`. *Check:* bottles sit on the shelf instead of floating; dark creases where the bar meets the floor.
- [ ] **L1.5** Light in the air: soft haze around bulbs and a beam from the stage lights (worked out per light along each view ray, no fog volume), haze strength in the Studio's Air. `?haze=0`. *Check:* a soft halo around every bulb; the stage spot shows a faint beam.
- [ ] **L1.6** In the Studio: per light "casts shadows" and "haze"; Air gets bounce strength and exposure; the moods (Evening, Party, Closing) retuned for the new light; the budget meter counts shadow lights. *Check:* in the Studio, turn on shadows for the piano lamp and watch them appear; switch Evening → Party.

## Waiting on the user: P2, the Studio phone check
The whole Studio is live; the user tries it on the phone, piece by piece
(the list in the chat). Every "broke <piece>" is fixed first, one commit each.
- [ ] **P2.1** Fix every "broke S…" the user reports.
- [ ] **P2.2** A `?perf=1` report from the Studio on the phone (a busy view, the Monitor open): if it drops under 50 fps, find what costs it.
- [ ] **P2.3** The multiplayer test's late joiner (Erin finds the fire out, E3.6b) fails in this container but passes on CI: make it pass both, or say why not.
- Ideas for after (not planned yet): AI inside the Studio (needs a paid API), importing your own models, mesh editing, several people editing at once (docs/STUDIO.md §9).

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
- (this push) S9.1-S9.3, S10.1-S10.4: live for everyone (a publish reaches an open bar within seconds, try your draft with a friend, undo a publish) and Claude in the loop (notes from the Studio, Claude's versions in History with what changed, deep links, proposals) → c00af5e..HEAD
- `c00af5e` S7.1-S7.3, S8.1-S8.4: light and look (materials as data, live lights, moods and the budget meter) and life (states and rules, the Loop Rack, drawn keys and presets, sounds) → 2c8b4da..c00af5e
- `2c8b4da` S5.1-S5.4, S6.1-S6.4: precision (magnifier, part mode, this one / every one, Inspect edits everything) and building (Library with pictures and prices, duplicate / row / rename / delete, boxes with push-pull and extrude, places) → f3a2dc4..2c8b4da
- `f3a2dc4` S4.1-S4.4: the hub as Needs you · Dials · Board · News, tuning baked into the world, the bar's menus, news and loading screen in Faceplate → 8797ac4..f3a2dc4 (with the room's grace fix and S5.1)
- `8797ac4` S2.1-S2.5, S3.1-S3.5: the Studio opens (login, cameras, picking, the Monitor, X-ray) and edits (Move/Turn/Size, slide, Drop, Put on, undo, the draft, Publish, History, Restore) → 5a04c79..8797ac4
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
