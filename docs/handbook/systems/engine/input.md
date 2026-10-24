# Input

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Touch, mouse, keyboard and game controllers turned into walking, looking and taps.

## Status

**Live.**

## What you can do with it

Left half of the screen: a joystick to walk. Right half: drag to look. A quick tap uses things. Computer: WASD or arrows, drag to look. Pads and wheels: in racing.

## How it works

- `looped/input.*` gives one `InputFrame` per frame; a tap is a touch that doesn't move.
- `looped/gamepad.*`: the browser's Gamepad API, read once per frame (Xbox and PlayStation pads work on iPhone).
- `bar/interaction.*`: what a tap lands on: things to pick up win, another player must be clearly nearer than a thing, a thing to use beats a seat on a tie.

## Data

Settings → look speed (`bar3dSettings`).

## Commands

`tools/native/interaction_test.cpp`.

## Limits

Touch listeners are passive and only queue touches; the game reads them in the frame.

## Checks

`native`, `touch` (GitHub, allowed to fail: KI-25).

## Known traps

- Never read layout (getBoundingClientRect) inside a touch handler.
- Finger ids can be negative (the mouse): "no finger" is a flag, never -1.

## History

Touch controls from the start; passive touches 2026-10-17.
