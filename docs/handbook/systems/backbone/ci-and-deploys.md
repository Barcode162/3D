# CI and deploys

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

How a change goes live: one push to `main` per job, then four workflows, each for its own folders.

## Status

**Live.** The game's browser tests fail on GitHub at their pictures (KI-5).

## What you can do with it

See each deploy's state in Dev → Overview (game and site rows with their tests).

## How it works

- `deploy.yml` (the game and the room), two jobs since J66: **build-and-publish** (build → quick checks and the room's tests → room deploy → upload `home/bar/`, live in about 6 minutes → `gh-pages`), queued one at a time (`pages`); then **tests** on its own machine and in its own queue (`game-tests`): the same commit built again, then every test, each with a time limit. The next deploy waits only for the previous deploy, never its tests. Every step has a time limit (`tools/workflows_test.mjs`).
- `hub.yml` (the site and the tools' pages): tests first, upload only if all pass.
- `plan.yml` (the plan to Dev, about a minute).
- `classic.yml` (the old public bar, by hand).
- Exactly one workflow per folder: `docs/handbook/deploy-map.json`, checked by `tools/workflows_test.mjs`. Each run writes its state to Dev (`tools/ci_status.mjs`).
- `main` moves only through the ship (`node tools/project.mjs ship`); the locks refuse anything else.

## Data

`private/dev/ci-game.json`, `ci-site.json`; `gh-pages`'s last commit "Site build <sha>".

## Commands

`node tools/project.mjs ship "<what>"`; `status` shows both deploys.

## Limits

A game deploy takes about an hour with its tests; the bar is live within about 6 minutes.

## Checks

`workflows` (quick).

## Known traps

A cancelled run (a newer push took over) is not a failure. Tag pushes and branch deletions are refused in this environment. Game deploys queue one at a time, and the tests queue apart (J66): a deploy waiting with no steps is behind the deploy before it (minutes), not hung; a tests job waiting is behind the previous build's tests (KI-32; [the Playbook](../../../playbook/failures.md#a-deploy-looks-stuck)). A tests job's end report for a build that's no longer live isn't sent to Dev (it would overwrite the newer build's row).

## History

Live first, tests after (2026-10-21); honest checks (J2); one deployer per folder and the plan's own workflow (J5).
