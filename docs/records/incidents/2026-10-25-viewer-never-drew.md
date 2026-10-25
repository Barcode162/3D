# Incident 2026-10-25: the place viewer shipped without drawing

**Session:** [015cwLX4PmKxoTA3N6AscWbS](https://claude.ai/code/session_015cwLX4PmKxoTA3N6AscWbS)
**Errors counted:** 1
**Status:** closed

## What happened
J86.2 shipped the place viewer (`/bar/looped/`, build `673494d`) without
opening it once. Opened afterwards, it stopped at "Cannot read properties
of undefined (reading 'createView')": the renderer makes its surface
textures as one array, and with none to make (the viewer has no avatars)
the array was never created.

## Impact
The new viewer page showed nothing until J94. The bar wasn't affected (it
always has its avatars' textures).

## Root cause
A new program was shipped on the user's rule "The user tests" without
the one look that tells whether it starts at all; the engine assumed every
program has surface textures.

## Fix
J94: the renderer gives itself one plain layer when a program makes no
textures; the viewer's scene starts in its own task, as the bar's does.
Opened here with the project's browser settings: the showroom draws, lit,
with its sparks.

## Proof
A new page or program is opened once headless (its first log lines, one
picture) before it's shipped, even when no tests are run: that is checking
it's live, not a test.
