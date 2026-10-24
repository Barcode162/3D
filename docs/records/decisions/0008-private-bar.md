# 0008 · The bar is private; classic stays public

**Date:** 2026-10-20 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user
**Source:** `docs/archive/LOOPED.md` A1-A4

## Context
The bar was public on GitHub Pages; the user wanted control over who comes in.

## Decision
joshuavelasco.com/bar lets in only the owner, the admins and invited players (`server/gate.php`); the last public build stays at /classic/ (the `classic` branch); GitHub Pages keeps only a pointer page.

## Options not taken
- Public with accounts: anyone could come in.

## Consequences
Every bar file goes through the door; Claude's checks pass with the hub key; People in Dev invites.
