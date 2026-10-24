# 0009 · The phone is the judge: 60 fps on an iPhone

**Date:** 2026-10-15 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user
**Source:** `docs/archive/DECISIONS.md` → World-simulation principles (phone budget), Measured results

## Context
The bar is played on phones; this cloud machine has no graphics card.

## Decision
60 fps on an iPhone 15 Pro Max is the hard budget. Speed is judged only on the phone (`?perf=1`, Send report); pictures and headless tests judge looks, never speed. The phone check is the real visual test of every piece.

## Options not taken
- Judging speed headless: 1-2 frames a second, meaningless.

## Consequences
Every piece has a phone check; quality levels step down by themselves (Auto); the light is checked on the phone (KI-28).
