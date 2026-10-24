# 0005 · The game goes live first, its tests run after

**Date:** 2026-10-21 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user
**Source:** `docs/archive/DECISIONS.md` → Live first, long tests after

## Context
The game's browser tests take about an hour on GitHub; waiting for them held every change back.

## Decision
`deploy.yml` builds, tests the room, uploads the bar (live within about 6 minutes), then runs every test, each after any failure and with a time limit; a red run is fixed after, without stopping the next job. The site (`hub.yml`) uploads only when its tests pass (they are quick).

## Options not taken
- Tests before upload for the game: an hour per change.

## Consequences
A broken change can be live for up to an hour: the phone checks and "broke <piece>" catch it; Dev shows each deploy's real state.
