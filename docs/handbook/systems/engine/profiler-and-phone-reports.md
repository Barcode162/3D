# Profiler and phone reports

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

How long each part of a frame takes, and the reports a phone sends so Claude can see what happened there.

## Status

**Live.**

## What you can do with it

- Settings → Performance overlay (or `?perf=1`): frame time per phase, draw counts, hitches, start-up times.
- Settings → Send report, or tap the build code 3 times: a 6-letter code. Give Claude the code.

## How it works

- `looped/profiler.*`: `beginFrame()` → `PROFILE(Phase::kX)` scopes → `endFrame()`; `prof::draw()` counts draws per pass; readable as `window.bar3dPerf`.
- `server/report.php`: stores a report (at most 256 KB) as `<code>.json` in `private/bar/reports/`; names and tokens are stripped by the game first, and a report holding a login-token-like string is refused.

## Data

`private/bar/reports/` (30 days, at most 500 files).

## Commands

`curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`; Dev → Projects → the perf report box.

## Limits

Safari rounds `performance.now()` to about 1 ms: one phase's time is coarse; averages are right. 10 reports an hour per address.

## Checks

`gate` covers the door; the report itself is checked on the phone.

## Known traps

The profiler's `gpu` number is Submit → `OnSubmittedWorkDone`, not pure GPU time.

## History

Profiler (step 0.2) and reports (step 0.3), 2026-10-17.
