# Session: start and end

## Start

**When:** the first thing in every session, before answering anything but a
greeting.

**Before you start:** nothing; this procedure finds out the state.

**Steps:**
1. Read the state: `node tools/project.mjs status` (the work here, the job in
   Now, the live build, both deploys, the user's notes, open problems, whether
   the plan's files agree; each part with its age). Its short form is printed
   by itself when the session opens.
2. The deploys: the status's "Deploys" line (Dev's reports), or the GitHub
   runs (the GitHub tools) for the details. A red or unfinished run from the last session →
   [failures.md](failures.md) → A deploy is red, after answering the user.
3. What the user published: `node tools/studio.mjs status`, `node tools/uikit.mjs
   status`. If either is newer than the repo: `pull`, and commit it on the
   job's branch as "World: from the studio vN" / "UI: from the editor vN"
   (it's the user's own change, so it is pushed with the job).
4. The user's notes: `node tools/dev.mjs notes`. Each one goes through
   [intake.md](intake.md); mark it read with `node tools/dev.mjs read <id>` once
   it's handled.
5. Open problems that may matter today: `docs/records/known-issues.md`.
6. A job half done (Now has open pieces, its `job/J<n>` branch has commits not
   on `main`): continue it from its branch ([failures.md](failures.md) → Resume).

**Done when:** you can say in one line what's live, what's in Now, and
whether anything is red or waiting.

**If something goes wrong:** a command fails or a key is missing →
[failures.md](failures.md) → The machine. Never guess the state.

**Records:** none yet.

## End

**When:** before the last reply of a session that changed anything; before a
long wait; when the user says stop.

**Steps:**
1. Nothing half-saved: every piece committed and pushed to its job branch
   (`git status -sb` clean, not ahead). Talk, checks and try-outs are not
   committed (Charter: only what the user wants is pushed).
2. The session line in `docs/records/sessions/<YYYY-MM>.md`: date, session
   link, what was asked (request ids), pieces done, builds shipped, errors
   (the count, each linked to its incident). `node tools/project.mjs
   session --asked "<ids>" --done "<pieces>" --shipped "<builds>" [--errors "<incident files>"]`. Committed on the job's branch with the next piece or the
   job's finish; a session that only talked writes no line.
3. Every error of the session has its incident ([incident.md](incident.md)).
4. The reply ([reply.md](reply.md)): what was done, what to look at, what's
   still running, what needs the user.

**Done when:** a new session could carry on from the repo alone.

**Records:** the session line.
