# Materials

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

One record per kind of stuff (oak, glass, brass, leather...) for every sense: the look for the renderer, the feel for physics, the sound for audio.

## Status

**Live** (M0 real roughness); M1-M3 planned (`docs/handbook/manual/materials.md`).

## What you can do with it

- In the Studio, change what a thing is made of; materials are data, read before every build of the place.

## How it works

- `looped/material.h`: look (PBR colour, roughness, metal), feel (density, friction, bounciness), sound. New engines add fields here instead of special cases.
- `bar/materials.*` defines the bar's materials; `assets/world/materials.json` holds them as data (Studio-editable).
- `looped/procedural.*`: surface detail made in code from noise (tiling colour and bump images), so the repo needs no texture files for them.

## Data

`assets/world/materials.json`; a thing's material by name in its kind (`bar/scene_names.cpp` maps the words).

## Commands

`node tools/albedo_test.mjs` checks every material's brightness is physically sensible.

## Limits

Materials don't yet layer or let light through (M2, M3 planned).

## Checks

`albedo` (quick), `schema`.

## Known traps

A new material name must exist in both `materials.json` and the schema's choice list.

## History

Material records 2026-10-15 (one material, all senses); real roughness (M0) and materials as data (S7, 2026-10-22).
