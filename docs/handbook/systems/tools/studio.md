# The Studio

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The world editor at joshuavelasco.com/studio: the bar's own engine in studio mode, with tools around it to pick, move, build, light, give life to and publish the world.

## Status

**Live.**

## What you can do with it

- Look around freely; tap a thing to pick it (exact picking with the magnifier).
- Move / Turn / Size with arrows or exact numbers (snapping 5 cm, 15°, 5 %).
- Build: add any kind from the Library onto the surface in front of you.
- Look: moods, each light's colour and brightness, the budget meter.
- Life: states and rules with Test buttons, the Loop Rack (lanes, shapes, keys you draw), sounds.
- Monitor: World (every thing), Inspect (in words), Problems.
- Changes and History in plain words; Undo; Publish (a new version, live for every phone within 15 s); Share a draft with a friend (a code); Proposals from Claude to Preview, Apply or Discard; ⋯ → Note for Claude.

## How it works

`studio/studio.js` frames `/bar/?studio=1` (`bar/studio_mode.*`: no room, no HUD, a free camera, a tap picks) and talks to it with `bar3dBridge.command('st.*', …)`. Edits are operations in JavaScript (`studio/ops.mjs`): each changes one spot and gives back its inverse, paths name things by id. The draft (`draft.mjs`) is saved in the world store (`studio/api.php`); `diff.mjs` says what changed in words; `schema.mjs` checks against `assets/world/schema.json`.

## Data

The [world store](../backbone/stores-and-keys.md) (`private/studio/`): versions, drafts, shares, proposals.

## Commands

`node tools/project.mjs world status | pull | push --note … | propose --note … | log`.

## Limits

Owner and admins only (Claude with the key). The 32-light limit and the phone budget (the meter).

## Checks

`studio_api`, `ops` (GitHub, fast); `studio` (the whole Studio in a browser: allowed to fail, KI-24, KI-30).

## Known traps

Adding things reloads the Studio's frame from the draft. Commands land one frame late. Each tool keeps its own axis (Turn is always about Y).

## History

S1-S10, 2026-10-22 (`docs/STUDIO.md`).
