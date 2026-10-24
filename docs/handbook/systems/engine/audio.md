# Audio

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Every sound in the bar: positional sound through Web Audio, synthesized instruments and effects, MIDI, the jukebox's songs.

## Status

**Live.**

## What you can do with it

- Menu → Settings: music and effects volumes.
- The piano, the jukebox, glasses clinking, darts, the bartender's steps, an engine hum while racing.

## How it works

- `looped/audio.*` decides what plays and where (3D); `bar/web/audio.js` is the thin Web Audio bridge.
- iPhone rule: sound may only start inside a tap. The first touch creates the audio context and blesses every music element with a moment of silence; after that the game can start music at any time.
- `looped/synth.*`: sounds made in code: a grand piano (inharmonic partials, three detuned strings, hammer knock), plucked string, bell, dart thunk, glass clink, pour, shaker, click, engine hum.
- `looped/midi.*`: reads MIDI files (formats 0 and 1, tempo changes, sustain).

## Data

`assets/music/` (jukebox and 4K Mania songs, with licences), `assets/midi/` (piano songs), `assets/world/songs.json` (tempos).

## Commands

`?pianosong=N` starts a piano song; `?autoplay` for 4K Mania.

## Limits

No sound before the first tap (the phone's rule). No copyrighted music, ever.

## Checks

`menu`, `rhythm` (GitHub), the phone.

## Known traps

On Safari, never call `setTargetAtTime` every frame without cancelling: it piles up and stutters.

## History

Synthesized sounds and MIDI (2026-10-16); piano samples synthesized once at start (step 1.2b).
