# The world model

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Looped's world core: every thing in a place, each a kind, each in a state, each state playing its loop on the shared clock. The bar, the Studio, the Dials and the room all speak it.

## Status

**Live.** The bar's layout and its life come from it (`assets/world/`).

## What you can do with it

- Touch a living thing (a lamp, a table, the fire, the light switch): it follows its kind's tap rule.
- Change any thing, kind, loop, state or rule in the [Studio](../tools/studio.md) or the [Dials](../tools/dials.md); publish, and every phone in the bar sees it within 15 s.

## How it works

- A **kind** (`assets/world/kinds/<place>.json`) says what a thing is: its parts (model, solids, seats, tap areas, light, sound, price...), its loops, its states (each with a loop and rules), the smaller things it's made of. A kind can build on another (`"is": "furniture"`).
- A **place** (`assets/world/places/<place>.json`) lists every thing: its kind, where it sits (on what), its own parts, the place's moods.
- A **loop** (`looped/world/loop.*`) lasts `period` seconds (or beats, locked to the music) and is made of pieces: one channel (`move.y`, `turn.y`, `glow`...) following one shape (wave, keys, noise...). Pure maths from the clock, so every phone and the room get the same answer.
- **Everything follows from the clock plus one fact per thing**: which state it's in and since when. Timed rules ("after one loop, back to idle") are worked out from the clock. Only outside events (a tap) make a Change the network carries; the room runs the same rules and has the last word.
- `World::build(library, place)`, then each frame `update(clock)`; `send(i, "tap", t)`; `takeChanges()` → the room; `apply(change)` from the room. The bar plays it in `bar/world_bar.*`; `bar/bar_scene.cpp` turns the place file into the renderer's and physics' lists.
- The catalog (`assets/world/catalog.json`, `looped/world/catalog.*`): every kind you could own, its name, category, price in credits, tradeable or not.

## Data

- `assets/world/schema.json` (every field described), `kinds/`, `places/`, `catalog.json`, `materials.json`, `songs.json` (tempos for beat-locked loops).
- The live version is the [world store](../backbone/stores-and-keys.md)'s; the repo's copy is the built-in fallback ([data map](../../data-map.md) → The world).

## Commands

- `node tools/project.mjs world status | pull | push --note … | propose --note …`
- `python3 tools/place_format.py <file>` reformats a place file after editing.
- `node tools/schema_test.mjs` checks every world file against the schema.

## Limits

- Two places today: `bar` and `home` (`?place=home`).
- A field the schema doesn't describe is refused by the schema check (and the Studio).

## Checks

- `schema` (quick), `native` (`tools/native/world_test.cpp`, `bar_world_test.cpp`, `entity_test.cpp`), `live_world` and `studio` on GitHub.

## Known traps

- Thing indexes are the engine's order; files speak ids. Never store an index.
- Every thing's stable id is `bar/entity_ids.txt`; a changed id there is a deliberate layout change (`entity_test`).
- A Studio command lands one frame late: read results a frame after sending.

## History

Looped 2 (2026-10-21, `docs/archive/ENGINE.md`): the whole engine rebuilt around loops; the bar's layout became a file the same day.
