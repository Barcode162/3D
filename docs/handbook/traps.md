# Known traps from before the restructure

Mistakes already made once, gathered in the old notes (now history in
`docs/archive/DECISIONS.md`). The traps that still hold are on each system's
page under **Known traps** ([the systems](generated/systems.md)); these
sections keep the full list, with their dates, for when a system page isn't
enough. Something here contradicting the code or a system page: the code
wins, and the system page gets fixed.

- [Gotchas hit so far](../archive/DECISIONS.md#gotchas-hit-so-far): `EM_ASM` and the minifier, headless frame rates, Safari audio, touch handlers, IONOS uploads, loading stutter, the room locally.
- [Looped gotchas](../archive/DECISIONS.md#looped-gotchas-2026-10-21): the engine split.
- [World gotchas](../archive/DECISIONS.md#world-gotchas-2026-10-21): the world core.
- [Layout gotchas](../archive/DECISIONS.md#layout-gotchas-2026-10-21): the layout as a file.
- [The E3-E9 batch: gotchas](../archive/DECISIONS.md#the-e3-e9-batch-gotchas-2026-10-21): things over the room, places, life.
- [The Studio, steps S1-S10](../archive/DECISIONS.md#the-studio-step-s1-2026-10-22): the Studio's own traps (commands a frame late, finger ids, the magnifier, reloads from the draft).

The ones that bit most often:
- **Pictures in tests on GitHub** time out on a machine that draws in software: take them through a helper that notes a slow one (KI-5).
- **`EM_ASM` code** is minified: quote property names; `\.` and `\d` in regexes are eaten.
- **Never judge speed headless**: browsers there run at 1-2 frames a second; only the phone counts.
- **Never read layout in a touch handler**; touches are passive and only queued.
- **IONOS uploads**: `lftp mirror` without `--ignore-time`, and per-build files sent again by name.
- **Every room saves its world and `schemaVersion`**, not only `main`.
