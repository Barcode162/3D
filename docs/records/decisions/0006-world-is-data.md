# 0006 · The world is data, and the store's version is the live one

**Date:** 2026-10-21 / 2026-10-22 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user (the Studio), Claude within it
**Source:** `docs/archive/DECISIONS.md` → The bar's layout is a file; `docs/archive/STUDIO.md` §7

## Context
The bar's layout and life were code; the user wanted to shape the world by hand.

## Decision
The world is files (`assets/world/`: places, kinds, catalog, materials, songs, the schema). The world store's live version is the live world (the game and the room load it); the repo's copy is the built-in fallback and Claude's working copy. A publish names the version it started from; the user's publish wins.

## Options not taken
- The repo as the only world: the user couldn't change it without Claude.
- The store as the only world: no fallback when it's down, no review of Claude's changes.

## Consequences
Claude pulls at the start of a job and publishes after it's live (recipes 12, 13); the store copies are never edited by hand.
