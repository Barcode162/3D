# Incident 2026-10-24: The first ship of J4 refused itself: it read PLAN.md as LAN.md

**Session:** [01PhA3LeFJ5iHgiMYbpW4zwG](https://claude.ai/code/session_01PhA3LeFJ5iHgiMYbpW4zwG)
**Errors counted:** 1
**Status:** closed

## What happened
J4.6 let the ship accept the finish's own unsaved files (the next job in Now,
the session line). It read `git status --porcelain` through a helper that trims
the whole output, so the first line lost its leading space and its file name
lost a letter: `PLAN.md` became `LAN.md`, which isn't a plan file, and the
ship refused.

## Impact
Nothing live, nothing pushed to main; one ship attempt refused safely.

## Root cause
The new filter was only tried on a clean tree (where it has nothing to read)
before J4.6 was committed: the piece's checks don't run the ship, and nothing
fed it a changed file.

## Fix
`50d0ec7`: the ship reads the status untrimmed (each line keeps its two status
letters before the name).

## Proof
The ship of J4 right after the fix accepted `PLAN.md`, `docs/plan/plan.json`
and the session line and shipped.
