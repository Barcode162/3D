# Incident 2026-10-24: The fix's deploy stayed red: a picture the Dev test takes timed out on GitHub

**Session:** [01PhA3LeFJ5iHgiMYbpW4zwG](https://claude.ai/code/session_01PhA3LeFJ5iHgiMYbpW4zwG)
**Errors counted:** 1
**Status:** closed

## What happened
J5.6 added two pictures to the Dev test (`build/dev-overview.png`,
`build/dev-plan.png`) with a plain `page.screenshot`. On GitHub's machine, which
draws in software, the Plan picture took 60 s in the first J5 run and over 90 s
in the J5.7 run ([run](https://github.com/Barcode162/3D/actions/runs/40100866642)),
and a timed-out picture fails the whole test, so the site stayed un-uploaded.

## Impact
J5's site and Dev changes stayed off the live site for one more round; nothing
live broke.

## Root cause
The project already knew pictures are slow on GitHub (`tools/site_test.mjs`
takes them through a helper that notes a slow one instead of failing), but
that rule lived only in that one test; nothing told a new picture to use it,
and here, where pictures are quick, nothing could show the difference.

## Fix
J5.8: the Dev test takes its pictures through the same helper (20 s, a note
instead of a failure). The rule goes into the Handbook's page on checks (J6).

## Proof
The next site deploy's Dev step passes on GitHub.
