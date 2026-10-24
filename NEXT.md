# NEXT: what "go" does next

The user says "go"; Claude does the whole step below, piece by piece (one
commit each, tagged), deploys once, then rewrites this file for the step after.
How: `CLAUDE.md`. The engine: `docs/ENGINE.md` (E1-E9, built). The Studio:
`docs/STUDIO.md` (S1-S10, built). **The plan now: `docs/PLAN.md`** (the
Alive plan, replanned 2026-10-22: tracks L M T R A W, wow and foundation
take turns, every visual piece has an off switch and before/after pictures,
every piece writes its page in `docs/manual/`).

## PAUSED (user, 2026-10-24): everything below waits for the rework
The user paused all engine work (L1.8, P1, P2 and every later step) until the
rework is done: one world site, the Dev area, a UI Editor in the engine's
Toolbelt, one house theme, a remade plan system. The plan is being agreed in
`docs/UI_PLAN.md` (steps R1-R6). **On "go": nothing from below.** Until the
user agrees the plan, answer the open questions there; after, R1 goes here.

## Where we are (2026-10-22)
- The engine plan E1-E9 is built and live (`22e9b28`).
- The user is phone-checking it (the list in the chat). Any "broke <piece>"
  is fixed first, before the step below (CLAUDE.md → Pinpointing breakage).
- **The Studio plan S1-S10 is built** (2026-10-22). Checks: per piece only its own test, the full check once per step (user). The user
  picked the **Faceplate** theme (the others stay switchable).
- Another Claude session built the website (homepage/: Home, Play, Engine, Vision, Sign in, the staff Portal); it owns homepage/.

## The next step: L1, real light (docs/PLAN.md §4.1)
Today: point lights with a hard reach, GGX highlights, a flat ambient colour,
one captured reflection cube, blob shadows under avatars only, tone mapping
inside the scene shader (LDR, so nothing can adapt), bloom and grade in post.
Light hits a surface once and stops. L1 makes light behave like real light.
**Rule (user, 2026-10-22): light only comes from light sources.** No fake
fill: ambient is zero in the new lighting. Everything is lit by bulbs,
flames, neon, screens, glowing things and the sky/street through the windows,
plus their bounces. Darkness is real; eyes adjusting lets you see in dim
places (with a limit: a dark corner stays dark, not grey). Classic keeps the
old look.
Each piece: an off flag, `tools/shots.mjs` before/after from the same spots
(bar top, fire, back corner, stage), its page in `docs/manual/light.md`.
Budget for the whole step on the phone: +4 ms at High, +1.5 ms at Low.
- [x] **L1.1** HDR and real eyes: the scene draws into rgba16float, tone mapping moves to post (softer filmic curve), auto exposure over a wide range (one candle can light a dark room, like your eyes) adapting over ~1 s, bloom from real brightness, ambient 0. Settings → Graphics: **Lighting: New / Classic** (also `?light=classic`). *Check:* walk from the fire to the dark back corner: the picture brightens slowly; flip New/Classic.
- [x] **L1.2** Real units, no hard reach: every light gets lumens and colour temperature (Kelvin) instead of guessed strengths; true inverse-square falloff; reach worked out from where the light stops mattering to the eye (no cut-off ring). The 22 bar lights moved to real values (a candle ~12 lm, a 40 W filament bulb ~400 lm). *Check:* a lamp's light fades smoothly across the whole room, no edge where it stops.
- [x] **L1.3** Lights with size: bulbs as glowing balls, windows and screens as glowing panels, neon as glowing tubes (area-light highlights and soft falloff); a size field per light in the Studio. *Check:* the neon sign's reflection on the bar top is a long streak, not a dot; the window lights the floor as a soft patch.
- [x] **L1.4** Real material brightness: every material checked against measured values (white plaster ~0.8, dark oak ~0.15, brick ~0.3), a table in `docs/manual/light.md`, a test that flags anything brighter or darker than real; fixes. *Check:* walls and ceiling read lighter, wood stays rich.
- [x] **L1.5** Radiosity, part 1 (no visible change yet): still surfaces split into patches (~20 cm), and "who sees whom" between patches worked out once per layout (a tool for the shipped world, a background worker in the Studio after a layout edit), saved with the world.
- [x] **L1.6** Radiosity, part 2 (the big one): lights hit patches, patches light each other, many bounces each frame until it settles; **you watch the light spread through the room** after loading; a lamp edit in the Studio re-spreads it in a moment; glowing surfaces (neon, jukebox, screens, fire) count as light sources. `?bounce=0`. *Check:* switch off every light but one lamp: the whole room is softly lit, brightest near it, light creeps round corners.
- [x] **L1.7** Moving things: a probe grid filled from the patches lights avatars, glasses and stools, so they match the room. *Check:* walk from a lamp into shadow: your avatar dims with the room.
- [ ] **L1.8** In the Studio: per light lumens, colour temperature, size and shape; Air gets bounce strength and exposure range; the moods (Evening, Party, Closing) retuned in real units; the budget meter counts patches. *Check:* in the Studio, turn the piano lamp to 2000 K and watch the room around it go amber.

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
- (this push) M0.2 truly matte (no plastic rim on rough things, cloth keeps little shine, coats not black, floor 0.75) → M0.2 commit
- `8722f75` Phone feedback: L1.8 no squares of light, M0 real roughness (matte coats, floor not a mirror), L1.9 softer highlights (bright light keeps its colour), L1.10 calmer light (darker, less glow); tools: place_check, stage_hunks → 5622548..HEAD
- `336e1d9` L1.7: the new light in the Studio (lumens, Kelvin, shape live; moods in real units) → 73f2d72
- `7322b92` L1.1-L1.6: eyes (HDR, adapting, AgX), real units (lumens, Kelvin), real material brightness, bounced light (core + in the game), lights with size; tools: piece runner, gallery, benchmarks, light tool → 69fb05c^..5d99353
- `96fbf26` S9.1-S9.3, S10.1-S10.4: live for everyone (a publish reaches an open bar within seconds, try your draft with a friend, undo a publish) and Claude in the loop (notes from the Studio, Claude's versions in History with what changed, deep links, proposals) → c00af5e..HEAD
- `c00af5e` S7.1-S7.3, S8.1-S8.4: light and look (materials as data, live lights, moods and the budget meter) and life (states and rules, the Loop Rack, drawn keys and presets, sounds) → 2c8b4da..c00af5e
- `2c8b4da` S5.1-S5.4, S6.1-S6.4: precision (magnifier, part mode, this one / every one, Inspect edits everything) and building (Library with pictures and prices, duplicate / row / rename / delete, boxes with push-pull and extrude, places) → f3a2dc4..2c8b4da
- `f3a2dc4` S4.1-S4.4: the hub as Needs you · Dials · Board · News, tuning baked into the world, the bar's menus, news and loading screen in Faceplate → 8797ac4..f3a2dc4 (with the room's grace fix and S5.1)
- `8797ac4` S2.1-S2.5, S3.1-S3.5: the Studio opens (login, cameras, picking, the Monitor, X-ray) and edits (Move/Turn/Size, slide, Drop, Put on, undo, the draft, Publish, History, Restore) → 5a04c79..8797ac4
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
