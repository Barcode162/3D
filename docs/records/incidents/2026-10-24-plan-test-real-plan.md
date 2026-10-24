# Incident 2026-10-24: J6's site deploy went red: the plan test broke on the new Up next line

**Session:** [01PhA3LeFJ5iHgiMYbpW4zwG](https://claude.ai/code/session_01PhA3LeFJ5iHgiMYbpW4zwG)
**Errors counted:** 1
**Status:** closed

## What happened
The ship of J6 (`31cb32d`) moved J7 into Now; the plan's Up next line reads
"J8 (the restructure; then the road as J7 orders it)". `tools/plan_test.mjs`
works on a copy of the real plan with Now replaced by a test job, so in the
copy "J7" named nothing, the plan keeper refused every change, and 15 of its
checks failed on GitHub; the site wasn't uploaded.

## Impact
J6's site-side changes (the labels check) and the plan's data waited one more
round; the bar and the plan's own workflow were not affected.

## Root cause
Two gaps. The test still depended on the real plan's words (the same kind of
gap as the J4 incident `2026-10-24-rule-never-said`, where its `rule --replaces`
case was made self-contained, but not the rest). And the ship's `--skip-check`
skipped every check, including the plan's, although the next job and the
session line are written after the last piece's checks.

## Fix
J6.7: the test's copy sets its own Up next; the ship always runs the plan,
workflows and workshop checks on the final state, whatever is skipped.

## Proof
`node tools/plan_test.mjs` passes on the plan with J7 in Now (it failed
before); the next ship runs the plan checks first.
