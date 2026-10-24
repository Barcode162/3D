# When something fails

Each case: what to do, in order. Anything we caused is also an incident
([incident.md](incident.md)).

## A check fails

**When:** `node tools/piece.mjs` or `node tools/check.mjs` prints FAIL.
1. Read the failing log: `build/check/<name>.log` (the runner prints its first
   error lines).
2. Our change broke it → fix the change and run again. Never weaken, skip or
   delete the check to get green (Charter: never).
3. It was already broken before the change (try `git stash`, run the same
   check, `git stash pop`) → a known issue with the job that will fix it; the
   piece goes on only if the failure has nothing to do with it.

## A deploy is red

**When:** a GitHub run failed (Dev's deploy rows, or the GitHub tools).
1. Which step: the run's failed steps (Dev lists them by name).
2. Its log (the GitHub tools: the job's logs, the end of the failing step).
3. Ours (the job just shipped touched it) → fix it now, as a piece of that job
   (or the next job's first piece), and push; it's an incident.
4. Not ours (fails on the build before too, or a known issue) → the known
   issue gets the run's link; nothing else.
5. A one-off on GitHub's side (a download failed before any test ran) → note
   it; the next run shows whether it comes back. Re-running is only for that
   case.
Never make a test "allowed to fail" without an open known issue (the check
`tools/workflows_test.mjs` refuses it).

## Live is broken

**When:** the user reports it, Dev shows the bar or the room down, or a check
of the live site fails.
1. What changed last (Done, the last deploy) → if it's ours: [revert.md](revert.md)
   → Undo that job (or that piece), ship at once: the urgent fix
   ([ways.md](ways.md)).
2. Not ours (hosting, Cloudflare): check their status pages if reachable; tell
   the user what's down and what's not; nothing to push.
3. An incident either way.

## Conflicts

- **Merge conflict** when finishing a job: merge `origin/main` into the job
  branch, keep both sides' intent, run the checks for the files involved,
  commit the merge. If both sides changed the same thing differently and it's
  the user's call: ask.
- **The world store moved** (`studio.mjs push` refused: the user published
  since): `node tools/studio.mjs pull`, redo our change on top of theirs, push
  again. The user's version always wins a direct clash; tell them.
- **The UI store moved**: the same with `node tools/uikit.mjs`.

## The machine

- **A tool crashes**: read its message; a bug in our tool → fix the tool (a
  piece), an incident.
- **A key is missing** (`HUB_KEY` …): say what can't be done without it; the
  user adds it in the environment's settings; nothing is faked.
- **The network refuses** (a 403 from the proxy, a blocked host): don't work
  around it; report it; it's a limit (Charter §4) if it's permanent.
- **The disk is full**: delete build output (`build/`, caches) and try again.
- **GitHub refuses a push**: branch pushes work here; tag pushes and branch
  deletions don't. Never force-push `main`.

## Resume

**When:** a session ended mid-job (the container is temporary).
1. `node tools/plan.mjs now`: the job and its ticked pieces.
2. `git fetch origin job/J<n>`; check it out; its last commit is the last
   saved piece.
3. Anything not on the branch is gone: redo it from the spec. Carry on with
   the next open piece.

## Drift

**When:** a doc says one thing and the code, data or live site another.
1. The truth is what's live and in the code; check it.
2. Fix the doc (or the code, if the doc is the decision) in the same session,
   by the book that owns it (Charter > Playbook > Handbook > Plan > Records).
3. If a check could catch it next time, add the check.

## Security

**When:** a secret shows up somewhere public, or user data looks exposed.
1. Never paste the secret anywhere, not even in the incident.
2. Tell the user at once what leaked and where; the key must be replaced by
   the user (it's their account): say exactly where.
3. Remove it from the repo's current files; history rewriting is the user's
   call (it's `main`).
4. An incident; a check that would have caught it.
