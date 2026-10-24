# 0007 · Simulate, don't script: one material for every sense

**Date:** 2026-10-15 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user (the North star: "don't fake the world, simulate it")
**Source:** `docs/archive/DECISIONS.md` → World-simulation principles; `docs/archive/ENGINE.md`

## Context
Special cases per object don't add up to a living world.

## Decision
Prefer behaviour that falls out of a system over scripting one object. A material is one record for every sense (look, feel, sound). Things live by states, rules and loops on one shared clock, so every phone and the room agree without talking.

## Options not taken
- Scripted animations per object: cheaper first, but nothing connects.

## Consequences
New senses add fields to materials and parts, not special cases; the world model ([world model](../../handbook/systems/engine/world-model.md)) is how life is added.
