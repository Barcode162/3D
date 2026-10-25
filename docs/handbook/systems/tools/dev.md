# Dev

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The control room at joshuavelasco.com/dev: is everything up, what needs you, the plan, publishing, people. For the owner and the admins.

## Status

**Live.**

## What you can do with it

- Overview: the Live panel (tiles for the bar's live build, who's in the bar now with a heartbeat, both deploys with their tests, Claude's errors this week, Claude's latest job; it checks again every 15 s, and a changed tile glows and counts), tap a tile to tell Claude it looks good, it broke, or a note; what needs you (news to publish, world changes proposed, your calls); the job in Now with its ring.
- Projects: every project as a card with its tools as tiles (from `toolbelt/tools.json`), the perf report box; each project has its own page (its tools by group).
- Plan: the job in Now, Up next, the road as one card per chapter; each chapter, each job, Done, Parked and the North star has its own page (a job's page: its words, verdict, place in line, what it waits for, its spec); the box to tell Claude something (a note, or an idea to park).
- Publish: news drafts, highlights, the front page's words.
- People: who may come in, admins, Claude's key.
- Everywhere (J90): the trail (where you are, each step a way back), Back, Find (every page, tool and job in one field), Calm (all motion off); every page has its own address.

## How it works

`dev/index.html` draws each tab from a screen file (`ui/system/screens/dev-*.json`, editable in the UI Editor) and fills its slots (`overview.js`, `projects.js`, `plantab.js`, `publish.js`, `people.js`). `dev.js` is the router: the address after # is the tab and the page under it (#plan/J85); each tab's file adds its pages under it (`Dev.pages`), and the trail is drawn from them. `motion.js` is the motion (the slides with the View Transitions API, the stagger, the ripple and tilt, the counting, Calm); `find.js` is Find. The motion moves only transform and opacity (the phone's GPU does it without repainting); the background glow moves in steps under a pixel, 8 a second, so the frosted panels over it redraw 8 times a second, not 60. `dev/api.php` is its store (`private/dev/`): the plan's data (from `plan.yml` within about a minute of a push), the deploy reports (`tools/ci_status.mjs`), notes, the site's words, highlights. Login: the bar's (owner and admins), or Claude's key.

## Data

`private/dev/`: `plan.json`, `rules.json`, `specs.json`, `incidents.json`, `questions.json`, `plan-meta.json`, `ci-game.json`, `ci-site.json`, `notes.json`, `site.json`, `highlights.json` ([data map](../../data-map.md)).

## Commands

`node tools/project.mjs status` reads the same store; `node tools/project.mjs notes notes | read <ids>`; `?demo` opens Dev with sample data (no login).

## Limits

Players are kept out ("for the owner and admins").

## Checks

`dev` (GitHub: every tab, the plan and its nested pages, the trail and Back, the Live tiles and reactions, Find, Calm, notes, publishing, people); `ui_labels` (every control against its Handbook page).

## Known traps

Pictures in its test go through a helper that notes a slow one (J5.8). GitHub's software drawing prepares each new visual effect once, so the first picture or the first slide after a load can take 20 s there (not on a phone): the test's pictures hold the motion still and get 45 s, and a slide is dropped after 1.5 s (J90). Controls drawn from data (rows, chips, tool tiles, a key named after a project or a chapter) carry `.dev-row-btn` or `.dev-data`, so the labels test doesn't ask the Handbook to list them.

## History

The hub became Dev (R4, 2026-10-23); the plan as data (J4.1), the plan's own workflow and the truth on Overview (J5, 2026-10-24); each road item's verdict and job number on Plan (J7.1); the remake: nested pages with a trail, Back, Find, the Live panel and its reactions, constant motion with Calm (J90, 2026-10-25).
