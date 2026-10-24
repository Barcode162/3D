# Incidents: the zero-error loop

**An error** is anything that didn't go as expected because of us: a check we
turned red, a failed deploy, something broken on the live site, a tool or a
procedure step that didn't work as written, a wrong statement to the user,
lost work, a permission crossed, a rule written that the user never said. A
check catching a mistake before it's saved is the system working, not an
error. Problems we only discover (they were there before) are known issues
(`docs/records/known-issues.md`), not errors.

**When:** any error, however small; and anything this Playbook doesn't cover
(The unexpected, below).

## Steps

1. **Stop** what you were doing.
2. **Contain:** is something live, the user's data, or saved work at risk?
   Make it safe first: undo the change ([revert.md](revert.md)), stop a
   publish, keep the user's work.
3. **Record** it: `docs/records/incidents/<YYYY-MM-DD>-<slug>.md` with: what
   happened, the impact, the root cause, the fix, the proof. **(J4)** `node
   tools/project.mjs incident new`.
4. **The root cause:** not "what broke" but **why the way we work let it
   through**: which procedure, tool, check or doc was missing or wrong. Ask
   "why?" until the answer is something we can change.
5. **Fix the system:** change that procedure, tool, check or doc so the same
   mistake can't happen the same way again. Prefer a check or a tool over a
   sentence.
6. **Prove it:** the new check fails on the old mistake (try it), or say which
   later step (e.g. the J8 drills) will prove it.
7. **Count it** in the session line; tell the user ([reply.md](reply.md) →
   Something went wrong).

**Done when:** the incident file has all five parts and the fix is committed.

## The unexpected

**When:** something happens that no procedure covers.
1. Stop. 2. What's at risk (live, data, work)? Contain it. 3. Is there a
procedure after all (the [index](README.md))? Follow it. 4. If not: take the
safest option that keeps everything recoverable; if the choice is the user's
(Charter §5), ask one question with a recommendation. 5. Afterwards, write the
missing procedure ([change.md](change.md)) and record an incident if anything
went wrong.
