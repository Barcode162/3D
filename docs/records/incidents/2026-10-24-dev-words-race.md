# Incident 2026-10-24: J5's site deploy went red: Dev's test of saving the front page's words failed on GitHub

**Session:** [01PhA3LeFJ5iHgiMYbpW4zwG](https://claude.ai/code/session_01PhA3LeFJ5iHgiMYbpW4zwG)
**Errors counted:** 1
**Status:** closed

## What happened
After the ship of J5 (`0a1886f`), the site workflow's Dev test failed one check,
"the site's words are saved, and public for the front page"
([run](https://github.com/Barcode162/3D/actions/runs/40100167976)), so the
site wasn't uploaded. Dev → Publish → the front page's words loads the saved
words after the boxes appear; when that answer is slow, it overwrote what had
just been typed, and an empty headline was saved.

## Impact
J5's site and Dev changes were not live until the fix (the site stayed on the
build before; the bar, the room and the plan's own upload were not affected).
On a slow phone the same bug could wipe the user's typing in that box.

## Root cause
A bug in Dev that predates J5, which no check could see: the Dev test always
got the saved words back instantly on this machine, so the slow case was
never tried. J5's changes to Dev made the page do more at load and exposed it
on GitHub's slower machine.

## Fix
J5.7: the saved words fill only the boxes not typed in yet; the Dev test now
makes that answer slow on purpose (1.5 s) and checks the typed words survive.

## Proof
The new test fails on the old code (the same check as on GitHub) and passes
with the fix.
