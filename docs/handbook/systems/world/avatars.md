# Avatars

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

Your body and look: presets or a custom look made in the customizer; others see it.

## Status

**Live.** A1-A3 (alive avatars, real people, your face) planned (`docs/manual/avatars.md`).

## What you can do with it

Menu → Avatar: presets, or customize (body, skin, hair, top, bottom, shoes, eyes, facial hair); while it's open the camera stands in front of you.

## How it works

`bar/avatar.*`: a custom look is 17 small numbers (`AvatarStyle`); every look shares one skeleton. Others get look 255 in your state and the numbers as world key `a<your slot>` (only that slot may write it, `cloud/rules.js`).

## Data

On the phone: `avCustom`, `av0`..`av16`; in the room: `a<slot>`.

## Commands

`?previewturn=<deg>`, `?walkpose=`, `?walkcheck`.

## Limits

One mesh per slot, rebuilt in place.

## Checks

`avatar` (GitHub), `native` (walk).

## Known traps

The walk cycle's length must match how far `advanceWalk` moves the phase, or the feet slide.

## History

Customizer and walking 2026-10-19.
