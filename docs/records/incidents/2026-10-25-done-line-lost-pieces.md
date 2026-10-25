# Incident 2026-10-25: J92's Done line named two pieces that never shipped

**Session:** [015cwLX4PmKxoTA3N6AscWbS](https://claude.ai/code/session_015cwLX4PmKxoTA3N6AscWbS)
**Errors counted:** 1
**Status:** closed

## What happened
Shipping J92 (build `c49cd10`) wrote the Done line "J85.1, J85.2, J92.1"
and the reply listed J85.1 (the loop library) and J85.2 (timelines) as
built. Neither is in the build: an earlier session recorded them in the
gallery (`pieces.jsonl`, commits `78c76a4` and `2aafaf3`, "built in
`4c9ea11`") but its branch never reached GitHub, so their code doesn't
exist anywhere. J85 stayed correctly open on the road.

## Impact
Nothing live broke. For a few minutes the plan's Done line and the
release's piece list claimed work that doesn't exist (Charter: never say
something is done without checking it).

## Root cause
`tools/step.mjs finish` took every piece recorded in the gallery since the
last finished step, without checking that each piece's commit is on the
branch being shipped.

## Fix
`step.mjs` keeps only pieces whose commit is in the branch (`git merge-base
--is-ancestor`), and names any it leaves out. The Done line was corrected
to "J92.1" with its real range (`f28d037^..f28d037`) and sealed with
`c49cd10`. J85.1 and J85.2 are to be built again when J85 comes up.

## Proof
The next ship prints a note for any recorded piece missing from the
branch and leaves it out of the Done line.
