# The jukebox and music

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The jukebox: a song list played from its spot in the room, the same song at the same moment for everyone.

## Status

**Live.** Run by the room when online.

## What you can do with it

Tap the jukebox: pick a song, or load songs from your phone; the next one starts when a song ends.

## How it works

`bar/jukebox.*`; the room decides what plays next (`cloud/games/jukebox.js`); the song and its start time are a world entry, so late joiners hear the same part. The world's loops can lock to the song's beat (tempos in `assets/world/songs.json`).

## Data

`assets/music/*.m4a` (licensed, credited); the built-in list in `bar/jukebox.cpp` (kBuiltIn) and `cloud/games/jukebox.js` (same order, lengths).

## Commands

`tools/native/run.sh` checks the two lists match.

## Limits

No copyrighted music in the repo, ever.

## Checks

`native`, `room`.

## Known traps

Adding a song means both lists, the file, its length and its credit.

## History

2026-10-15; the room took it over on 2026-10-18.
