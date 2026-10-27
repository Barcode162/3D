# Ways of working

Not every request is a normal job. Each way below says how it differs from
[job.md](job.md); everything not mentioned works as there.

## A normal job
[job.md](job.md): spec, pieces, checks for what changed, ship once, verify,
reply.

## Urgent fix
**When:** something live is broken and people are affected (allowed even
during the freeze).
- The smallest change that fixes it, or a revert ([revert.md](revert.md)).
- Its own job number, one piece, shipped at once (it may push `main` outside
  the one-push-per-job rhythm).
- During the freeze, a fix outside its allowed files is refused by the lock
  (`tools/lib/freeze.mjs`) unless the piece runs with `FREEZE_OK="urgent fix:
  <what broke>"`.
- Always an incident ([incident.md](incident.md)).

## Experiment (a try-out)
**When:** the verdict is "needs an experiment", or Claude must find out
whether something works before promising it.
- Time-boxed (say how long up front: one session at most without asking).
- Done on the machine only: **nothing is committed or pushed** (the user's
  rule). Scratch files go in the session's scratchpad, not the repo.
- The result is a sentence: it works / it doesn't, and why. If it works, a
  real job follows ([intake.md](intake.md) step 4).

## Content from the user
**When:** the user published in the Studio (the world) or the UI Editor
(themes and screens).
- `node tools/studio.mjs pull` / `node tools/uikit.mjs pull`; commit as
  "World: from the studio vN" / "UI: from the editor vN" on the job branch.
- The user's content is never edited by hand in the stores. While the Studio
  is parked (J114) the world is edited in `assets/world/` and ships with the
  game; nothing is pushed to the world store.

## Talk, questions, status
- Answer from the books and the live state; no tools that change anything;
  **nothing is committed or pushed**.
- "status": what's live (build), what's in Now (job, pieces done), what's
  next, anything red or waiting for the user. A few lines.

## Phone reports
**When:** the user sends a six-letter code from Menu → Settings → Send report.
- `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'`.
- Speed is judged only from these (and the user's eyes), never from the
  cloud machine.

## A long job (several sessions)
- Every piece is pushed to `job/J<n>` as it's done; the plan ticks it; a new
  session resumes from the branch ([failures.md](failures.md) → Resume).
- The job ships when all its pieces are done, not before.

## Two sessions at once
- Each works on its own job branch; only the one finishing first pushes
  `main` first; the other merges `origin/main` before its own finish.
- Never two sessions on the same job.

## An audit
**When:** the user asks whether something is right, or before unfreezing.
- Read-only: checks, docs against code, the live site. Findings go to known
  issues (problems) or incidents (errors); fixes become jobs.
