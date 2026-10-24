# The arcade

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The arcade room and its machine: the user's platformer "Test 2", added as is.

## Status

**Live.**

## What you can do with it

Walk into the arcade (behind doorway A), tap the platformer machine: it opens full screen; leave to come back to the bar.

## How it works

The room and the cabinet are made by `tools/assets/arcade.py` and `arcade_cabinet.py`. Tap → `bar/web/menu.js` GAMES.platformer, a full-screen iframe like Meridian (the bar stops drawing, its sounds go quiet). The game is `bar/projects/platformer/` unchanged (three.js 0.161.0).

## Data

None stored by the bar.

## Commands

`tools/arcade_test.mjs`.

## Limits

One machine so far.

## Checks

`arcade` (GitHub; stopped at a picture in run 211, KI-5).

## Known traps

In the asset scripts, `draw_text`'s row counts from the bottom of the image.

## History

2026-10-19 (upgrade items 3-5).
