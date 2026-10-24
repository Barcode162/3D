# Incident 2026-10-24: a deploy waiting its turn was recorded as a hang

**Session:** [0167G39s9gUuuTU2uQ2wx1Wo](https://claude.ai/code/session_0167G39s9gUuuTU2uQ2wx1Wo)
**Errors counted:** 1
**Status:** closed

## What happened
KI-32 (committed 1f36cf1) said the game deploy of 31cb32d (run 214) hung in its Emscripten cache step for 25+ minutes; that step took 19 seconds. The run was running its hour of tests, and the next deploy (run 215, 666d414) was waiting its turn behind it (deploy.yml runs one deploy at a time and never cancels one).

## Impact
Nothing live. The note (KI-32) was on `job/J7` only, never on `main`. It told
the user a GitHub deploy was stuck and could only be cancelled by hand, which
was not true: the run ended by itself at 18:11 and `666d414` went live at
18:17.

## Root cause
The Playbook's "A deploy is red" covered a red run, not a run that looks
stuck. Nothing said that game deploys queue (`deploy.yml`: concurrency group
`pages`, `cancel-in-progress: false`) and that each one keeps its turn through
its hour of tests, so a new run sits waiting with no steps for up to an hour.
The waiting run was read as a hung step without checking the run before it or
the steps' start and end times.

## Fix
The Playbook's failures page gains "A deploy looks stuck" (look at the run
before it, and at each step's start and end times, before calling anything a
hang); the CI and deploys page names the queue under Known traps; KI-32 now
says what really happens (the queue), and its fix (the tests in their own job
after the deploy) goes on the road with J7.1. Same commit as this record.

## Proof
The J8.1 drills include "a deploy looks stuck": the answer key expects the
new procedure (look at the run before, the step times) and no cancel.
