# Physics

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Rigid bodies written from scratch: glasses that stand, tip, roll and clink; walls and furniture you can't walk through; impacts the sound engine hears.

## Status

**Live.** The room runs the same physics for the shared glasses.

## What you can do with it

Pick up a glass, put it down, throw it; everyone sees the same glass (the room moves it).

## How it works

- `looped/physics.*`: the world is fixed boxes (walls, floor, counter, tables); moving bodies are a few spheres glued together (a glass is rings of small spheres). Weight, friction and bounce come from each body's material.
- Fixed 60 Hz steps of 4 substeps: contact impulses with friction, overlaps pushed apart. Impacts are events (for sound).
- `bar/props.*` builds the physics world from the bar's layout.
- The room runs the same C++ (`cloud/sim/`, built to `room_sim.wasm`) for the glasses (`cloud/games/glasses.js`).

## Data

Solids and blocks in each kind (`assets/world/kinds/`).

## Commands

`tools/native/physics_test.cpp` (in `node tools/project.mjs check native`).

## Limits

Spheres only for moving bodies (cheap and stable on a phone).

## Checks

`native`, `room` (the room's glasses).

## Known traps

- Draw bodies at `Body::shownPosition` (smoothed), not the raw position.
- Send a glass's "came to rest" after the TICK that carries its last pose, or late joiners see it jump.

## History

2026-10-16 (glasses), the room took over the glasses on 2026-10-18.
