# 0001 · Each of the user's channels has one role

**Date:** 2026-10-24
**Status:** current
**Asked by:** Claude, within J5.1 (restructure spec, approved by the user's "go"; KI-17)

## Context
Three channels were pulled at every "go" without ever carrying anything from
the user (the world store: v1–v3 all published by Claude; the UI store: v0;
Dev notes: none), and no document said what each one is for (KI-17).

## Decision
- **The chat**: the one channel for wishes, questions, rules, "go", "broke", "undo".
- **Dev → Plan notes**: the chat for when the user is away from it; read at the
  start of every session and handled like a chat message (intake).
- **The Studio / the UI Editor**: the user's own hands on the world / the look.
  The user's publish wins; Claude pulls it at the start of a job and builds on it.
  When the user hasn't published, `project.mjs status` shows "repo matches" and
  nothing is pulled.
- **Dev → Publish**: the user's switches (news, highlights, the front page's
  words); Claude only writes drafts.

## Options not taken
- Retiring the Studio, the UI Editor or the notes: they are the user's tools
  (the North star wants the user able to shape the world by hand); unused so
  far is not unwanted.
- Pulling every store at every "go" by hand: replaced by one status line that
  says whether a pull is needed.

## Consequences
The full map is `docs/handbook/data-map.md`. KI-17 and KI-22 closed.
