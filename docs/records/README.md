# The Records

What happened, kept and never rewritten (a correction is a new entry). Not
instructions: how to work is in `docs/playbook/`. Every kind is written with
`node tools/project.mjs <kind>` (tools/records.mjs), so its shape never drifts.

| Folder or file | What | Written by |
|---|---|---|
| `requests/<YYYY-MM>.md` | the user's words, exactly as sent, with an id (`<date> #<n>`) and one line of what Claude understood | `project.mjs request` |
| `incidents/<date>-<slug>.md` | one error: what happened, impact, root cause, fix, proof | `project.mjs incident` |
| `decisions/<NNNN>-<slug>.md` | a choice that holds: context, decision, options not taken, consequences | `project.mjs decision` |
| `sessions/<YYYY-MM>.md` | one line per session that changed something: asked, done, shipped, errors | `project.mjs session` |
| `drills/<date>.md` | cold-start drills (J8.1): the requests, the answer key written first, each fresh session's result, the gaps found and what changed | by hand, one file per drill run |
| `known-issues.md` | open problems, each with the job that fixes it; closed ones below | by hand, one row each |

The plan's Done list (`PLAN.md` → 7) is the build log; the `gallery` branch
holds every piece's pictures and numbers.
