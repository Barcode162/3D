# "broke" and "undo"

## Broke

**When:** the user says "broke J4.3", or describes something broken.
1. Find the change: `git log --oneline --grep '^J4\.3 '` (the piece's
   commit); for a description, the last jobs in Done and their phone checks.
2. See it: the piece's own checks (`node tools/check.mjs <systems>`), its
   before/after pictures in the gallery, the live page; the browser test that
   covers it, if one does.
3. Fix it as a new piece (in the job in Now, or a small urgent job:
   [ways.md](ways.md) → Urgent fix), or revert the piece's commit
   (`git revert <sha>`) if the fix isn't clear. Ship.
4. Reply with the new build code and what to look at again.
5. An incident (a broken piece got through).

## Undo

**When:** the user says "undo" (or "put it back").
1. The last job's line in Done holds its commit range (`<first>^..<last>`).
2. On a new branch from `origin/main`: `git revert --no-edit <first>^..<last>`
   (newest first; resolve a conflict by keeping the older behaviour).
3. Run the checks covering the reverted files; ship it as its own job
   ("J<n> Undo J<m>") with a Done line; the plan gets the undone job back on
   the road unless the user says drop it.
4. Reply: the build code; the bar is as it was before that job.
