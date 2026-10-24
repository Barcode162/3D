# Seats and taps

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

Sitting and using things: which seat or thing a tap means, and who has it.

## Status

**Live.**

## What you can do with it

Tap a stool, sofa or bench to sit; tap a machine or thing to use it; a seat taken by someone else is theirs.

## How it works

Seats and tap areas come from the kinds (`seat`, `uses`). `bar/interaction.*` picks what a tap lands on; `bar/activity.*` keeps one activity at a time (Idle → Requesting → Active). Online, sitting sends a claim for `seat.<n>` and the room decides. Every seat and thing has a stable id (`bar/entities.*`, `bar/entity_ids.txt`).

## Data

Seats and uses in `assets/world/kinds/`; claims in the room.

## Commands

`?sit=N`; `tools/native/activity_test.cpp`, `entity_test.cpp`, `interaction_test.cpp`.

## Limits

One activity at a time.

## Checks

`native`, `mp`.

## Known traps

Small things are `direct`: they only count when the tap is right on them.

## History

Seats 2026-10-16; the room decides seats 2026-10-18.
