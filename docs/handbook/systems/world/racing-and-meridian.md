# Racing and Meridian

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

Two racing rigs: sit in one and the whole screen becomes a race (the racing sim), or open Meridian, the user's own driving game.

## Status

**Live** (racing); **live** (Meridian, the approved three.js exception). The driving rework continues after the freeze.

## What you can do with it

- Sit in a rig → a menu: race (garage first: pick a car, paint and rims), or Meridian.
- Race the other rig's player when both are ready, else bots; SETUP tunes the car; `?pad` the skid pad; `?telemetry=1` the overlay.

## How it works

- `bar/projects/racing/`: the track (a spline sampled every metre: surface, curbs, barriers, scenery), the race world (240 ticks a second, never the wall clock), controls for touch, keys, mouse, pads and wheels, the view, the HUD, the engine sound.
- Two players: the room runs the lobby and the start (`cloud/games/race.js`, world key `rc`); each phone drives its own car.
- Meridian (`bar/projects/meridian/`, three.js 0.169.0) opens full screen in an iframe; the bar keeps its connection and stops drawing.

## Data

World key `rc` (16 numbers); car setup on the phone.

## Commands

`?sit=N&garage`, `?pad`, `?telemetry=1`; native race and car tests (`tools/native/`).

## Limits

Two rigs, so two players race each other; bots fill the rest.

## Checks

`native`, `race_mp`, `meridian` (GitHub).

## Known traps

After a racing rig the Menu button must come back on top (`menu_games`, failed on GitHub in run 211, KI-5).

## History

Racing sim 2026-10-16; Meridian 2026-10-18; two-player racing 2026-10-19; driving rework MVP 2026-10-20.
