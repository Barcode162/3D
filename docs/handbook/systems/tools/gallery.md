# The gallery

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

Every piece's before/after pictures and stats, and the benchmarks' history, kept forever on the `gallery` branch.

## Status

**Live.**

## What you can do with it

Look at how the bar changed piece by piece (the branch's README is the page, newest first).

## How it works

`tools/lib/gallery.mjs`: an orphan branch (never merged, never deployed), checked out as a worktree in `build/gallery`; `tools/piece.mjs` adds each piece; `tools/bench.mjs` the benchmarks.

## Data

`pieces.jsonl`, `bench.jsonl`, `latest/<spot>.png` (the next before), `<TAG>/<spot>.jpg`, `README.md`.

## Commands

`node tools/project.mjs piece …` (pictures unless `--no-shots`); `--pages <page>` for a static page.

## Limits

Site pages served by PHP can't be pictured by the piece tool (it serves the repo as static files).

## Checks

—

## Known traps

Never delete `gallery` (the Charter).

## History

2026-10-22.
