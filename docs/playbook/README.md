# The Playbook

**The only book that gives instructions.** Every situation has a procedure
here; every procedure has the same shape: **When** · **Before you start** ·
**Steps** (numbered, each naming its exact command or file) · **Done when** ·
**If something goes wrong** · **Records**.

Order of authority: Charter (`docs/charter/`) > Playbook > Handbook > Plan >
Records > Archive. The Playbook never contradicts the Charter; if it seems to,
the Charter wins and this book is fixed in the same session (an incident).
Until J8, `docs/plan/restructure.md` sits just under the Charter.

Every command runs through the front door, `node tools/project.mjs <command>`
(`help` lists them); `tools/playbook_test.mjs` checks that every command this
book names exists.

## Every session

1. Start: [session.md](session.md) → Start.
2. Every message from the user: [intake.md](intake.md).
3. End: [session.md](session.md) → End.

## Situation → procedure

| Situation | Procedure |
|---|---|
| A session starts | [session.md](session.md) → Start |
| The user says anything | [intake.md](intake.md) |
| "go" | [job.md](job.md) (the job in Now) |
| A wish, a change, a new feature | [intake.md](intake.md) → feasibility → [job.md](job.md) |
| A question, "status", "talk" | [intake.md](intake.md) → answer only; nothing is pushed |
| "park: …", a new idea in chat | [intake.md](intake.md) → Park |
| A new rule ("always…", "never…") | [intake.md](intake.md) → A new rule |
| Something impossible, against a rule, unclear | [intake.md](intake.md) → Verdicts |
| "broke J4.3", something broken on the phone | [revert.md](revert.md) → Broke |
| "undo" | [revert.md](revert.md) → Undo |
| The user published in the Studio or the UI Editor | [ways.md](ways.md) → Content from the user |
| A phone report code | [ways.md](ways.md) → Phone reports |
| Building a piece | [job.md](job.md) → A piece |
| Finishing a job | [job.md](job.md) → Ship |
| Replying to the user | [reply.md](reply.md) |
| A check fails here | [failures.md](failures.md) → A check fails |
| GitHub's run is red | [failures.md](failures.md) → A deploy is red |
| A GitHub run looks stuck (in progress or queued for long) | [failures.md](failures.md) → A deploy looks stuck |
| The live site or the room is down | [failures.md](failures.md) → Live is broken |
| A conflict (merge, store) | [failures.md](failures.md) → Conflicts |
| A tool crashes, a key is missing, the network or disk refuses | [failures.md](failures.md) → The machine |
| The session was cut off mid-job | [failures.md](failures.md) → Resume |
| Docs and reality disagree | [failures.md](failures.md) → Drift |
| A secret leaked, data at risk | [failures.md](failures.md) → Security |
| Any error at all | [incident.md](incident.md) |
| **Anything not on this list** | [incident.md](incident.md) → The unexpected |
| An urgent fix, an experiment, a long job, an audit | [ways.md](ways.md) |
| Changing the books, the tools, the structure | [change.md](change.md) |

## Three rules every procedure keeps

1. **Nothing outside the Charter.** Charter §5: allowed / ask first / never;
   anything not listed: ask first.
2. **Test only what the change touches**; **push only what the user wants**
   (talk, questions, checks and try-outs are never pushed).
3. **Never say it's done or live without checking it.**
