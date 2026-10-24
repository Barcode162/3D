# The bar and its rooms

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

Bar3D's place: the main bar, the arcade behind doorway A, room 3, and a second place, home. Walk in from the browser at joshuavelasco.com/bar.

## Status

**Live.** Private: the owner, admins and invited players (see [accounts and the door](../backbone/accounts-and-door.md)).

## What you can do with it

- Walk, look, sit, pick things up, play the games, talk by voice.
- Menu → Places: go to another place (the bar, home); `?place=home`.

## How it works

- The layout is a file: `assets/world/places/bar.json` (every thing) with `assets/world/kinds/bar.json` (what each kind is). `bar/bar_scene.cpp` turns them into the renderer's, physics' and seats' lists; `bar/world_bar.*` plays every thing's loop on the shared clock.
- Rooms and doorways cut drawing: your room in full, another room only through its doorway on screen. Models used only in another room stream in when you come within 8 m, after 20 s, or when its doorway is in view.
- The arcade: an 8 × 8 m room (x 0.4..8.4, z -15.7..-7.7) with a cosmic carpet, neon and 7 lights; its chiptune is made in code and follows where you are.

## Data

`assets/world/places/*.json`, `kinds/*.json`; the live version in the world store ([data map](../../data-map.md)).

## Commands

`node tools/project.mjs world status | pull | push`; page options `?place=`, `?cam=`, `?still`.

## Limits

32 lights for the whole bar (the arcade uses 7).

## Checks

`smoke`, `live_world`, `travel` (between places; allowed to fail, KI-25), the phone.

## Known traps

A room's shell must load at start if part of it faces another room (the arcade's doorway sign was once missing at spawn).

## History

The bar from 2026-10-15; the arcade 2026-10-19; places and travel (E7) 2026-10-21; the layout as a file 2026-10-21.
