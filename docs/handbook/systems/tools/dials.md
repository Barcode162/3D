# The Dials

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The world's numbers as dials (joshuavelasco.com/hub/dials.html): every field of kinds (loops, lanes) and things (where, turn, size).

## Status

**Live.**

## What you can do with it

Turn a dial: it goes into the same draft the Studio edits; Publish is the Studio's publish; "Open in the Studio" links to the thing.

## How it works

`hub/dials.js` reads the live world and your draft from `studio/api.php` and draws every field the schema knows.

## Data

The world store's draft.

## Commands

—

## Limits

Numbers only; picking and building are the Studio's.

## Checks

`studio` (S4.1, in a browser).

## Known traps

The old live tuning is gone: speeds became real loop lengths (S4.2); `hub.php`'s tuning has no reader (KI-29).

## History

S4.1 (2026-10-22).
