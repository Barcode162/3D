# Bar3D gallery

Every piece of work, newest first: what it is, its before/after pictures
(left before, right after) and its numbers. Made by `tools/piece.mjs`;
benchmarks by `tools/bench.mjs` (see `bench.jsonl`).

Latest benchmark (2026-10-22 18:44, 922ec64): gi_triangles_full 642040 tris, gi_triangles 99105 tris, gi_load_ms 167 ms, gi_simplify_ms 28 ms, gi_bvh_ms 239 ms, gi_patches 58292, gi_probes_usable 10128, gi_build_ms 6666 ms, gi_rays_per_s 0.59 M/s, gi_solve4_ms 12.6 ms

## L1.3 Real material brightness: every material measured (tools/albedo_audit.py → assets/models/albedo.json), the ones darker or brighter than anything real moved into real ranges (new lighting); models keep their shapes for rays

`922ec64` · 2026-10-22 18:40 · 17 files, +5003 −7 · tests: changed · headless frame 69.78 ms, 288 draws

Phone check: Look at the bar top and the stage: wood grain, the mahogany's red and the green wall show clearly instead of near-black.

![L1.3 door.jpg](L1.3/door.jpg)

![L1.3 door-switch.jpg](L1.3/door-switch.jpg)

![L1.3 mirror.jpg](L1.3/mirror.jpg)

![L1.3 mirror-switch.jpg](L1.3/mirror-switch.jpg)

![L1.3 bartop.jpg](L1.3/bartop.jpg)

![L1.3 bartop-switch.jpg](L1.3/bartop-switch.jpg)

![L1.3 backbar.jpg](L1.3/backbar.jpg)

![L1.3 backbar-switch.jpg](L1.3/backbar-switch.jpg)

![L1.3 piano.jpg](L1.3/piano.jpg)

![L1.3 piano-switch.jpg](L1.3/piano-switch.jpg)

## L1.2 Real units: every lamp in lumens and Kelvin, true inverse-square light with no edge, white balance like eyes; world lights migrated (tools/world_migrate.mjs)

`1975523` · 2026-10-22 18:25 · 23 files, +440 −93 · tests: changed · headless frame 76.03 ms, 288 draws

Phone check: Stand at the door: lamp light now spreads across the whole room in soft pools, brightest under each lamp, with no edge where it stops. Bulbs look warm, not orange.

![L1.2 door.jpg](L1.2/door.jpg)

![L1.2 door-switch.jpg](L1.2/door-switch.jpg)

![L1.2 mirror.jpg](L1.2/mirror.jpg)

![L1.2 mirror-switch.jpg](L1.2/mirror-switch.jpg)

![L1.2 bartop.jpg](L1.2/bartop.jpg)

![L1.2 bartop-switch.jpg](L1.2/bartop-switch.jpg)

![L1.2 backbar.jpg](L1.2/backbar.jpg)

![L1.2 backbar-switch.jpg](L1.2/backbar-switch.jpg)

![L1.2 piano.jpg](L1.2/piano.jpg)

![L1.2 piano-switch.jpg](L1.2/piano-switch.jpg)

## L1.1 Eyes: the scene drawn as real light (HDR), metered and adapting like eyes, the AgX film curve; Settings → Lighting New/Classic

`31c8f7b` · 2026-10-22 17:55 · 15 files, +437 −86 · tests: game · headless frame 54.47 ms, 288 draws

Phone check: Walk from the bright bar counter to the dark stage corner: the picture slowly brightens as your eyes adjust. Settings → Graphics → Lighting: flip New and Classic, highlights look softer in New.

![L1.1 door.jpg](L1.1/door.jpg)

![L1.1 door-switch.jpg](L1.1/door-switch.jpg)

![L1.1 mirror.jpg](L1.1/mirror.jpg)

![L1.1 mirror-switch.jpg](L1.1/mirror-switch.jpg)

![L1.1 bartop.jpg](L1.1/bartop.jpg)

![L1.1 bartop-switch.jpg](L1.1/bartop-switch.jpg)

![L1.1 backbar.jpg](L1.1/backbar.jpg)

![L1.1 backbar-switch.jpg](L1.1/backbar-switch.jpg)

![L1.1 piano.jpg](L1.1/piano.jpg)

![L1.1 piano-switch.jpg](L1.1/piano-switch.jpg)

## L0 Automation: piece runner, step finisher, benchmarks, gallery branch, manual pages, feature switches

`69fb05c` · 2026-10-22 17:28 · 18 files, +688 −0 · tests: none

Phone check: nothing to see: tools only

