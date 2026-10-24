# Incident 2026-10-24: a rule written down as the user's that the user never said

**Session:** https://claude.ai/code/session_01PhA3LeFJ5iHgiMYbpW4zwG (start of J3)
**Errors counted:** 1 (the rule dates from an earlier session; J1.2's Charter repeated it and made it stricter)
**Status:** fixed

## What happened
`PLAN.md` → Your rules held "(2026-10-23) **Speed over tests**: no browser tests on
the cloud machine…", and `CLAUDE.md` called it "(user, 2026-10-23)". J1.2's Charter
copied it and turned it into a "never" (no browser tests here unless the user
asks). The user said (request 2026-10-24 #5): "I never said that". What the user
wants: test only what a change touches.

## Impact
Sessions were barred from running the browser test that covers a change (e.g.
Dev's test for a Dev change), so mistakes could only be found on GitHub after
a push. No live harm found.

## Root cause
Rules were written in Claude's own paraphrase and labelled "(user)", with no
record of the user's actual words to check them against. The paraphrase drifted
from what the user meant, and every later copy (CLAUDE.md, the Charter) trusted
the label.

## Fix
- The rule is replaced by the user's own ("Test only what the change touches"),
  through `node tools/plan.mjs rule … --replaces "Speed over tests"`; the Charter
  and `CLAUDE.md` follow it.
- Since J1, the user's words are kept exactly in `docs/records/requests/`, and a
  rule's "in practice" line is Claude's, kept apart from the user's words
  (Charter §3).
- J3.2 (intake) makes it a step: a new rule is written only with a link to the
  request that holds the user's words.

## Proof
Every rule added from J1 on names its request; the cold-start drills (J8.1)
include "a new rule" and check it's recorded with the user's words.
