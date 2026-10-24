# The piano

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The grand piano: it plays itself, or you play it from the bench with the on-screen keyboard or a real MIDI keyboard.

## Status

**Live.**

## What you can do with it

- Sit at the bench and play; pick a song (Menu → Piano songs: ten classical songs, or your own `.mid` file).
- Everyone hears the notes from the piano's spot; keys light up, strings glow and shake.

## How it works

`bar/piano.*`; notes synthesized in `looped/synth.cpp` (30 samples made once at start, the keys between are pitched); MIDI files read by `looped/midi.*`; songs and live notes shared through the room.

## Data

`assets/midi/` (songs, with credits); the room's world key for the piano song.

## Commands

`?pianosong=N`.

## Limits

iPhone Safari has no Web MIDI: a real keyboard works on computers and Android only.

## Checks

`menu` (the song list), the phone.

## Known traps

A song file you load goes through the menu's list; `menu_test` checks it arrives unbroken.

## History

2026-10-16.
