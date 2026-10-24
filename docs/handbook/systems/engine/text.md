# Text

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

The engine's own font and text: on the screen (prompts, HUD) and in the world (labels, signs), all in one draw.

## Status

**Live.**

## What you can do with it

Read prompts, name tags, the machines' screens, banners and toasts.

## How it works

`looped/font.*`: a hand-drawn 5×7 pixel font (with descenders) turned into a signed distance field, sharp at any size, outlines for free. `looped/text.*`: `begin()` → `addScreen()` / `addWorld()` → `upload()` → `draw()`; one draw call a frame. `bar/hud.*` places the bar's text.

## Data

None stored: the font is built in code.

## Commands

None.

## Limits

One font, one weight; the HTML menus use the UI kit's fonts instead.

## Checks

`smoke`, the pictures.

## Known traps

World text is depth-tested: a sign behind a wall is hidden.

## History

From the start (2026-10).
