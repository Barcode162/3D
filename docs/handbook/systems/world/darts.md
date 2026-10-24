# Darts

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The dartboard on the front wall: 301, three darts a turn, finish on exactly zero.

## Status

**Live.** Run by the room when online.

## What you can do with it

Stand at the oche, tap where you aim (your hand isn't perfect), watch it fly and stick; real scoring (20 sectors, doubles, triples, 25, bull 50); going below zero or to 1 is a bust.

## How it works

`bar/darts.*` draws; `cloud/games/darts.js` decides online (the phone only says where it aimed); offline the phone scores alone with the same rules.

## Data

The room's world key for the darts game.

## Commands

`?darts` starts the game.

## Limits

One game at a time at the board.

## Checks

`room` (the room's darts), the phone.

## Known traps

The board and rules exist twice (C++ and JavaScript): change both.

## History

2026-10-16; the room took it over on 2026-10-18.
