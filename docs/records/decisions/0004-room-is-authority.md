# 0004 · The room decides the shared world

**Date:** 2026-10-18 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** Claude, within the user's direction ("rework every system; first the server side")
**Source:** `docs/archive/DECISIONS.md` → Cloudflare room is the multiplayer server

## Context
Phones deciding on their own made the shared bar disagree (who sits where, which glass is where, which song is on).

## Decision
The room (one Durable Object per room) is the authority: seats and machines by claims, every shared write checked by `cloud/rules.js`, darts, glasses (with the bar's own physics), the jukebox and races run by the room. Phones ask; the room answers; everyone ends up the same.

## Options not taken
- Each phone decides, others follow: the old way; disagreements and cheating.

## Consequences
New shared things need a rule in `cloud/rules.js` and a room test (recipe 27); offline, the phone plays alone with the same rules.
