# 0010 · The site's pictures and the UI store's are kept apart

**Date:** 2026-10-24 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** Claude, within J6.6 (KI-19)
**Source:** KI-19

## Context
Two pictures exist twice: `homepage/site/img/jukebox.jpg` = `ui/system/img/jukebox.jpg`, `homepage/site/img/counter.jpg` = `ui/system/img/door.jpg`.

## Decision
Keep both copies. The site's pictures can be picked for front-page highlights (a saved highlight may name `/site/img/counter.jpg`), and `ui/system/img/` belongs to the UI store (never edited by hand). Each copy is deployed by its own workflow.

## Options not taken
- Point the site at `/ui/system/img/`: a highlight the user already saved could lose its picture.
- Delete the UI store's copy: it's the store's, not the repo's, to change.

## Consequences
About 200 KB twice on the server; nothing else. KI-19 closed.
