# 4K Mania

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The 4-key rhythm game on the "4K MANIA" cabinet, made to look and play like osu!mania.

## Status

**Live.**

## What you can do with it

Song select (a scrolling wheel, four difficulties with osu!mania star ratings, your best grade per song), then notes and hold notes falling to four keys, judgements, a timing bar, health and a results screen. TEST (fingers, frames, the sound clock; GUARD for the phone's gestures) and SYNC (tap along to clicks: your sound's delay). TRAINING: the pack of 30 hand-built maps, ten stages from 2★ to 7★ (cleared at 95 %).

## How it works

`bar/projects/mania/rhythm.*`; every song is analysed into a chart by listening to it (`rhythm_chart.*`: onsets in four bands by FFT, tempo and beat phase, notes snapped to the beat, lanes following pitch, ringing sounds become holds). Plays with exact timing (`engine::Track`): one smooth clock for the notes and the taps, the sync's offset taken off (J117). Stars: osu!mania's formula (`stars.*`). How it keeps time and how maps are built: [the manual](../../manual/4k-mania.md).

## Data

`assets/music/rhythm/` (its own songs, with credits; the Training pack in `pack/`) plus the jukebox's (and songs loaded from your phone); best grades on the phone.

## Commands

`?rhythm=song,difficulty`, `?autoplay`; `tools/rhythm_test.mjs` (needs ffmpeg).

## Limits

Charts are made by listening, not by hand.

## Checks

`rhythm` (GitHub).

## Known traps

The test uses a short copy of a song made with ffmpeg.

## History

2026-10-16; like osu!mania 2026-10-19; round 2 2026-10-19; J117 (smooth clock, test, sync, real stars, the Training pack) 2026-10-28.
