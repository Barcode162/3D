# Incident 2026-10-25: a square of light on the floor after J88

**Session:** [015cwLX4PmKxoTA3N6AscWbS](https://claude.ai/code/session_015cwLX4PmKxoTA3N6AscWbS)
**Errors counted:** 1
**Status:** closed

## What happened
J88.3 (bounced light at most 10 times a second, only while lamps change)
shipped in `b3c6b97`. The user saw a square of light on the floor with no
lamp making it (request 2026-10-25 #17). The before/after look had shown a
difference on the floor; it was put down to the candles' flicker after
comparing only the average colour.

## Impact
Live for about an hour: a sharp orange square on the floor near the bar.

## Root cause
Found by switching one feature off at a time on the same view: the square
is gone with bounced light off, and gone with only J88.3 undone (the build
before J88, and J88.1 alone, have none). Why the pacing makes a square is
not known yet. The look before shipping was judged by an average, which
hides a local fault.

## Fix
J96: J88.3 undone (bounced light every frame again). J88.3 goes back on
the road only with its cause understood.

## Proof
A before/after look is judged region by region, not by an average: any
difference that isn't plainly the flicker of a lamp is found before saving.
