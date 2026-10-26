# Incident 2026-10-26: the lounge's new things refused by the world (J70)

**Session:** [015cwLX4PmKxoTA3N6AscWbS](https://claude.ai/code/session_015cwLX4PmKxoTA3N6AscWbS)
**Errors counted:** 1
**Status:** closed

## What happened
J70 (the lounge taken apart) shipped in `0d40f55`. The user sent the live
log: "World: bookshelf: two things have this id" and 17 lines "kind ...: no
states (every thing needs a live loop)", one for each new lounge kind.

## Impact
Live for about half an hour. The new bookshelf thing took the id
`bookshelf`, already the id of the bookshelf's tap spot; the world keeps the
first thing with an id and drops the second, so "Bookshelf, tap: read a
book" was gone. The 17 kinds had no loop (they still drew; the world
reported each one as a problem).

## Root cause
New things and kinds were added to the place and kinds files without
running the bar's world test (`tools/native/bar_world_test.cpp`), which
checks exactly this: unique ids, and a live loop for every kind. Under the
rule "The user tests" no tests ran before saving or shipping, and the
live check (`tools/check_live.mjs`) only shows that the bar loads, not the
world's own list of problems.

## Fix
J71.1: the lounge bookshelf's thing is `lounge_bookshelf`; every new
lounge kind has a live loop (a faint shimmer; the fireplace's embers
breathe, the candlesticks' flames flicker, the floor lamp's bulb breathes).
The world test passes, and fails on the files as J70 shipped them.

## Proof
The user allowed it (request 2026-10-26 #9): the piece tool runs the
seconds-long world check (`tools/world_check.sh`) whenever a piece changes
`assets/world/`, even with `--test none`; a failure stops the piece.
