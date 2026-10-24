# Intake: every message from the user

**When:** the user says anything.

**Before you start:** the session has started ([session.md](session.md)).

## Steps

1. **Classify the message** (one message can hold several; handle each):

| The user… | Kind | Goes to |
|---|---|---|
| says "go" | continue | [job.md](job.md): the job in Now (or the next one) |
| wants something added or changed ("make…", "I want…", "can you…") | wish | step 2 → 3 → 4 |
| asks how or why, or "status" | question | answer from the books and the live state; nothing is pushed |
| says "talk" | talk | conversation only, no tools that change anything |
| reports something broken, "broke J4.3" | problem | [revert.md](revert.md) → Broke |
| says "undo" | undo | [revert.md](revert.md) → Undo |
| says "park: …", or mentions an idea in passing | idea | Park, below |
| says "always…", "never…", "from now on…" | rule | A new rule, below |
| changes the order of the work | priority | ask nothing: record it (step 2), then `node tools/plan.mjs next "…"` |
| answers a question Claude asked | answer | record it (step 2), carry on where the question came from |
| says "stop", "pause", "wait" | stop | stop at once; save what's done ([session.md](session.md) → End); say where it stopped |
| sends a phone report code | report | [ways.md](ways.md) → Phone reports |

2. **Record the user's words exactly** in `docs/records/requests/<YYYY-MM>.md`
   with the next id (`<date> #<n>`) and one line "Understood: …" for what you
   took it to mean. Questions, talk and stops are not recorded (nothing is
   pushed for them). **(J4)** `node tools/project.mjs request`.
3. **Check it's possible and allowed: the verdict.** Before any work, give it
   one of six verdicts, with a reason tied to the Charter's limits (§4) or
   rules (§2–3):

| Verdict | Means | Then |
|---|---|---|
| **Doable now** | fits the rules, the limits, the freeze | a job (step 4) |
| **Doable after X** | needs another job first | a job with X before it |
| **Doable at a cost** | money, phone speed, a rule bent, a big time cost | one question to the user, with Claude's recommendation |
| **Needs an experiment** | nobody knows yet if it works here | a time-boxed experiment ([ways.md](ways.md)); nothing pushed until it works |
| **Not possible** | the limits forbid it | say why, and the closest thing that is possible |
| **Against a rule** | one of the user's rules says no | name the rule; the user may change it |

   **The capability check** (part of the verdict): if the work needs anything
   outside the repo (a new kind of push, a deletion, a publish, an outside
   service, a paid plan), confirm first that this environment allows it, by
   its documentation or by a dry run that leaves nothing behind if it fails.
   Known refusals: tag pushes and branch deletions on GitHub (Charter §4).

   **The permission check**: Charter §5. "Ask first" → one question with a
   recommendation; "never" → don't, and say which rule; not listed → ask.

4. **Make it a job.** A wish becomes a job (or joins the open job it belongs
   to, if it's the same work): the next free number `J<n>`, a spec from
   `docs/plan/jobs/TEMPLATE.md` saved as `docs/plan/jobs/J<n>.md`, linking the
   request id. Put it on the road with `node tools/plan.mjs add <chapter> J<n>
   "<what>" --serves N…`. While the freeze holds (until J8), a new wish is
   parked unless it's an urgent fix ([ways.md](ways.md)).
5. **Reply** ([reply.md](reply.md) → Understood): what you understood, the
   verdict, what happens next. Then do it ([job.md](job.md)), or wait for the
   answer if a question was asked.

## Park

`node tools/plan.mjs park <chapter> "<the idea in one line> (request <id>)"`.
No work, no debate. Chapters: world, site, engine, tools, backbone.

## A new rule

Only the user makes rules, and only in their own words:
1. Record the request (step 2): the rule's source.
2. `node tools/plan.mjs rule "**<short name>:** <the rule> (request <id>)"`
   (`--replaces "<old name>"` when it replaces one).
3. Add its "In practice" line to the Charter §3 table; `node tools/plan.mjs
   charter` copies the rule into the Charter.
4. If it changes how a procedure works, change that procedure in the same
   commit.

A rule is never written from a paraphrase without its request: see incident
`docs/records/incidents/2026-10-24-rule-never-said.md`.

## Unclear

If a message could mean two things: pick the likelier one, say so in one
line, and carry on. Ask only when the choice is the user's to make (Charter
§5): one question, Claude's recommendation first.

**Done when:** the message has a verdict and is answered, recorded, parked,
or on its way as a job.

**If something goes wrong:** anything this page doesn't cover →
[incident.md](incident.md) → The unexpected.

**Records:** the request (wishes, rules, problems, answers, priorities).
