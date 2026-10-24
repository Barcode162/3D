# The room server

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

The real-time server: one small server per room on Cloudflare, the authority for who is where, who holds what, and the shared world.

## Status

**Live.**

## What you can do with it

Be in the bar with others; the shared world (glasses, jukebox, mirrors, darts, chess, races) is the same for everyone.

## How it works

- `cloud/worker.js` (router, Origin check: only our own pages may connect, `/health`) and `cloud/room.js` (one Durable Object per room name): slots and resume tokens, HELLO → WELCOME, a guest first then a login check, STATE in, a 20 Hz TICK out only while something moved, claims, the shared world.
- `cloud/rules.js` checks every shared write; `cloud/games/` runs darts, glasses (with the bar's own C++ physics, `cloud/sim/`), the jukebox and races.
- AFK after 30 minutes.

## Data

Each room's storage (its world and `schemaVersion`).

## Commands

`node tools/room_test.mjs` (its own `wrangler dev`); `/health` for usage.

## Limits

Sizes and rates enforced (PROTOCOL §7).

## Checks

`proto`, `room`, `mp` (GitHub).

## Known traps

- Every room must save its world and `schemaVersion`, not only `main`.
- Test probes must send the `ping` keepalive, or the room (rightly) drops them.

## History

2026-10-18 (replaced the PHP relay).
