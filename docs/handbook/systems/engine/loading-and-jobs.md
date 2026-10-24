# Loading and jobs

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Start-up without freezes: background jobs over frames, asynchronous downloads, the loading screen's stages up to "Tap to enter".

## Status

**Live.**

## What you can do with it

Watch the loading screen name each stage; tap to enter when ready.

## How it works

- `looped/jobs.*`: small jobs run after the frame is submitted, until a deadline; lanes (critical, normal, background); the queue learns how long each kind takes; jobs can be cancelled.
- `looped/fetch.*`: downloads and image decoding by the browser, with progress.
- `bar/loader.*`: the stages. "Ready" = every file in, background jobs done, every pipeline compiled, the warm-up frame finished, and frames steady (30 in a row under 20 ms, or 2 s).
- `bar/world_live.*`: the live world is fetched from the store before the scene is built; anything wrong → the built-in world.

## Data

`bar/web/loading.jpg` (the loading picture).

## Commands

`?skipintro`; the profiler's start-up times (`?perf=1`).

## Limits

At most 4 images decode at once.

## Checks

`smoke` (GitHub), `live_world`.

## Known traps

A job that takes longer than its kind usually does pushes the next frames: split big work into slices.

## History

Jobs and async pipelines (steps 1.3-1.4, 2026-10-17); the loading screen (2026-10-20).
