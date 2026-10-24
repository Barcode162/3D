# Dev

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The control room at joshuavelasco.com/dev: is everything up, what needs you, the plan, publishing, people. For the owner and the admins.

## Status

**Live.**

## What you can do with it

- Overview: the bar's live build, the room server, both deploys (game and site) with their tests, Claude's errors this week, what needs you (news to publish, world changes proposed, your calls), the job in Now and its next piece.
- Projects: every project with its tools (from `toolbelt/tools.json`), the perf report box.
- Plan: the job in Now with its pieces (tap it for its spec), the road by chapter (tap a job for its spec), Parked, Done, the North star, your rules, and the box to tell Claude something (a note, or an idea to park).
- Publish: news drafts, highlights, the front page's words.
- People: who may come in, admins.

## How it works

`dev/index.html` draws each tab from a screen file (`ui/system/screens/dev-*.json`, editable in the UI Editor) and fills its slots (`overview.js`, `projects.js`, `plantab.js`, `publish.js`, `people.js`). `dev/api.php` is its store (`private/dev/`): the plan's data (from `plan.yml` within about a minute of a push), the deploy reports (`tools/ci_status.mjs`), notes, the site's words, highlights. Login: the bar's (owner and admins), or Claude's key.

## Data

`private/dev/`: `plan.json`, `rules.json`, `specs.json`, `incidents.json`, `questions.json`, `plan-meta.json`, `ci-game.json`, `ci-site.json`, `notes.json`, `site.json`, `highlights.json` ([data map](../../data-map.md)).

## Commands

`node tools/project.mjs status` reads the same store; `node tools/project.mjs notes notes | read <ids>`; `?demo` opens Dev with sample data (no login).

## Limits

Players are kept out ("for the owner and admins").

## Checks

`dev` (GitHub: every tab, the plan, the deploy rows, notes, publishing, people).

## Known traps

Pictures in its test go through a helper that notes a slow one (J5.8).

## History

The hub became Dev (R4, 2026-10-23); the plan as data (J4.1), the plan's own workflow and the truth on Overview (J5, 2026-10-24); each road item's verdict and job number on Plan (J7.1).
