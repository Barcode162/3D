# The command-line tools

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

Claude's tools, all through one front door: `node tools/project.mjs <command>`.

## Status

**Live.**

## What you can do with it

Nothing for the user to run: Claude runs them. The list with usage: [commands](../../generated/commands.md).

## How it works

- `tools/project.mjs` reads the command list `tools/lib/commands.mjs` and runs each tool; `status` shows the whole truth on one screen.
- The main ones: `piece` (one piece: its checks, pictures, commit, push), `ship` (finish a job: checks, merge, push `main` once, verify live), `plan` (the plan keeper), `check` (the checks for what changed), `request` / `incident` / `decision` / `session` (the Records), `world` and `ui` (the stores), `notes`, `news`, `handbook`, `live`, `bench`.
- The locks: git hooks (`tools/githooks/`) and Claude Code's guard (`tools/lib/guard.mjs`): no hand push to `main`, no force push, no hand edits of generated files, and while the freeze holds no commit outside its allowed files (`tools/lib/freeze.mjs`, J7.1).

## Data

—

## Commands

`node tools/project.mjs help`.

## Limits

Runs on Claude's machine (Node 22); the stores need `HUB_KEY`.

## Checks

`playbook` (every command the Playbook names exists), `locks`, `workflows` (quick).

## Known traps

The tools stage everything (`git add -A`): don't leave unrelated files in the working tree during a piece.

## History

The front door, ship and records tools (J4, 2026-10-24).
