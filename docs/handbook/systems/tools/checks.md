# The checks

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

Every test and check, listed once, run by system: only the ones covering what changed before each piece, all of them on GitHub at deploy.

## Status

**Live.** Some browser tests fail on GitHub because pictures of the running game time out (KI-5).

## What you can do with it

Nothing to do: Claude runs them. A deploy's results show in Dev → Overview.

## How it works

- `tools/lib/checks.mjs`: `CHECKS` (by system: quick, plan, workflows, workshop, build, native, game, room, hub, ui, uikit, dev, toolbelt, site, studio, server, mania, chess, arcade, meridian, race, menu, avatar, voice), `RULES` (which systems a changed file needs; a changed test runs its own), `NOT_ON_CI` and `NOT_LISTED` with their reasons.
- `tools/check.mjs changed` picks what covers the uncommitted files; `check.mjs` alone covers everything since `main`; `check.mjs all` everything.
- GitHub runs each check in exactly one workflow (`tools/workflows_test.mjs` proves it), every test after any failure, each with a time limit; a test allowed to fail names its open known issue.
- Browser tests open the bar with the tests' lighting (`TEST_LIGHT=test`).

## Data

`build/check/<name>.log` (each check's log).

## Commands

`node tools/project.mjs check [changed | all | <systems…>]`.

## Limits

This cloud machine has no graphics card: a browser test of the game takes long here; the phone is the real visual test.

## Checks

`workflows` checks the checks.

## Known traps

Pictures in tests must go through a helper that notes a slow picture instead of failing (site and Dev tests do; the game tests don't yet: KI-5).

## History

One list (J2.2), test only what changed (J3/J4), the tests' lighting (J2.1, 2026-10-24).
