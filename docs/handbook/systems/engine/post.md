# Post (glow and mood)

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

What happens after the scene is drawn: the eyes' exposure, the glow of very bright things, the film curve and the mood (tint, saturation, contrast, vignette).

## Status

**Live.**

## What you can do with it

- The place's mood (Studio → Look); compare with `?post=0` (off) / `?post=1`.

## How it works

The scene draws into an offscreen picture (linear light, rgba16float). `looped/post.*` then meters it (64×64 → 8×8 → 1×1), eases the exposure, picks the bright parts and blurs them at 1/4, 1/8 and 1/16 of the screen (off at Low quality), and applies the AgX curve and the mood while drawing onto the screen, with text and name pictures on top. Classic lighting arrives already in display colours.

## Data

`looped/mood.h` (a mood's numbers; no GPU code, so native tests can build layouts with moods); each place's `moods`.

## Commands

Page options `?post=0|1`, `?exposure=`, `?eyekeep=`.

## Limits

Glow is off at Low quality; the tests' lighting turns post off.

## Checks

`wgsl`, the piece pictures, the phone.

## Known traps

Judging brightness on a desktop screen: the phone's screen is the reference.

## History

Glow and mood (E-steps, 2026-10); eyes added with L1.1 (2026-10-23).
