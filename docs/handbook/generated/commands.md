<!-- generated: node tools/handbook.mjs write (from tools/lib/commands.mjs); never edit by hand -->
# The commands

Every tool runs through one front door: `node tools/project.mjs <command>` (Claude runs them; the Playbook says when). `node tools/project.mjs help` prints the same list.

| Command | What it does | Usage | Runs |
|---|---|---|---|
| `help` | this list | `node tools/project.mjs help` | project.mjs itself |
| `status` | the whole truth on one screen, each part with its age (--brief: a few lines) | `node tools/project.mjs status [--brief]` | project.mjs itself |
| `plan` | the plan: now, check, tick, add, park, rule, start, next, chapter, seal, render | `node tools/project.mjs plan <command> …` | `tools/plan.mjs` |
| `piece` | one piece: its checks, pictures, commit, push (docs/playbook/job.md) | `node tools/project.mjs piece <ID> "<title>" --check "<phone check>" [--no-shots \| --pages <page>]` | `tools/piece.mjs` |
| `check` | the checks for what changed (or named systems, or all) | `node tools/project.mjs check [changed \| all \| <systems…>]` | `tools/check.mjs` |
| `ship` | finish a job: checks, merge, push main once, verify live, record, reply | `node tools/project.mjs ship "<what the job did>" [--news "<friendly lines>"]` | `tools/ship.mjs` |
| `request` | record what the user said, word for word | `node tools/project.mjs request "<the user's words>" --understood "<one line>"` | `tools/records.mjs request` |
| `incident` | start an incident record (docs/playbook/incident.md) | `node tools/project.mjs incident "<slug>" "<what happened>"` | `tools/records.mjs incident` |
| `decision` | start a decision record | `node tools/project.mjs decision "<slug>" "<the decision>"` | `tools/records.mjs decision` |
| `session` | the session line for the Records | `node tools/project.mjs session --asked "<request ids>" --done "<pieces>" --shipped "<builds>" [--errors "<incident files>"]` | `tools/records.mjs session` |
| `handbook` | the Handbook's generated pages: write them, or check them (current, every folder described, links) | `node tools/project.mjs handbook <write \| check>` | `tools/handbook.mjs` |
| `world` | the world store: status, pull, push, propose, log | `node tools/project.mjs world <status \| pull \| push --note … \| propose --note … \| log>` | `tools/studio.mjs` |
| `ui` | the UI store and the theme CSS: status, pull, push, css, check | `node tools/project.mjs ui <status \| pull \| push --note … \| css \| check>` | `tools/uikit.mjs` |
| `notes` | the user's notes from Dev → Plan: notes, read <ids> | `node tools/project.mjs notes <notes [--all] \| read <ids…>>` | `tools/dev.mjs` |
| `news` | news drafts: post, posts (the user publishes) | `node tools/project.mjs news <posts \| post --build <sha> --title … --text …>` | `tools/hub.mjs` |
| `live` | open the live bar headless and check the build (EXPECT_SHA=<sha>) | `node tools/project.mjs live` | `tools/check_live.mjs` |
| `bench` | the engine's benchmarks, against the last run | `node tools/project.mjs bench [native]` | `tools/bench.mjs` |
