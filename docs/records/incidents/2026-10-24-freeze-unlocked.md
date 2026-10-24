# Incident 2026-10-24: a path rename reached frozen code

**Session:** [0167G39s9gUuuTU2uQ2wx1Wo](https://claude.ai/code/session_0167G39s9gUuuTU2uQ2wx1Wo)
**Errors counted:** 1
**Status:** closed

## What happened
While renaming the racing spec (J7.1), a search-and-replace of its path also changed comments in looped/, bar/projects/racing/ and tools/native/: files the freeze doesn't allow. Caught before anything was saved, by reading the list of changed files; nothing committed.

## Impact
Nothing live, nothing saved. The checks for the changed files started the
bar's browser tests on this machine (minutes of work for nothing), which is
how the stray files showed up.

## Root cause
The freeze (`docs/plan/restructure.md` §7) was a list in a document; no tool
refused a commit outside it. J6.6 changed comments in code under its own spec,
so "doc paths in code comments" looked routine, and the search-and-replace ran
over the whole repo instead of the allowed folders.

## Fix
J7.1: a lock. `tools/lib/freeze.mjs`, run by the new git hook
`tools/githooks/commit-msg`, refuses any commit that changes a file outside
the freeze's allowed list while the user's Freeze rule stands; the user's own
store changes ("World: …", "UI: …") and merges pass; an urgent fix passes
with `FREEZE_OK="urgent fix: …"` (Playbook → Urgent fix). The edits were
undone and the racing spec kept its file name (`docs/plan/jobs/W20.md`); the
plan finds a renumbered job's spec through its old tag.

## Proof
`node tools/locks_test.mjs`: the lock refuses `looped/drive/tire.h`,
`bar/projects/racing/cars.h` and a world file in a J7.1 commit, and lets the
allowed folders and the user's store changes through.
