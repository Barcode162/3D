# The data map

Every place J Production keeps data: who writes it, who reads it, how it gets
from one place to another, what wins when two disagree, and how you can tell
it's out of date. Part of the Handbook (facts, not instructions: what to do
is in `docs/playbook/`). Written in restructure J5.1; a store that isn't here
doesn't exist, and a new one is added here in the same commit that makes it.

On IONOS, the web root is `home/` (served as joshuavelasco.com) and the
stores sit next to it in `private/`, out of reach of the web. Every server
file below reads the store's folder from an environment variable in tests
(`DEV_DATA`, `STUDIO_DATA`, `UI_DATA`, …), so tests never touch the real data.

## The stores at a glance

| Store | Where | Holds | Written by | Read by |
|---|---|---|---|---|
| The repo | GitHub `Barcode162/3D`, `main` | all code, the built-in world, the plan, the books, the records | Claude, only through the tools (one commit per piece; `main` only through the ship) | CI (builds and deploys), every session |
| World store | `private/studio/` (`studio/api.php`) | the live world: numbered versions, each editor's draft, shared drafts, Claude's proposals | the user and admins (Studio → Publish), Claude (`node tools/studio.mjs push` / `propose`) | the game as it starts and every 15 s (`bar/web/live.js`), the room, the Studio, `studio.mjs pull` |
| UI store | `private/ui/` (`toolbelt/ui-editor/api.php`) | published themes and screens, numbered versions | the user and admins (UI Editor → Publish), Claude (`node tools/uikit.mjs push`) | `uikit.mjs pull` → `ui/system/` → the theme CSS the site and the bar use |
| Dev's store | `private/dev/` (`dev/api.php`) | the plan's data (`plan.json`, `rules.json`, `incidents.json`, `plan-meta.json`), the deploy reports (`ci-game.json`, `ci-site.json`), the user's notes, the front page's words and highlights | CI (plan.yml: the plan; deploy.yml and hub.yml: their reports), the user (notes from Dev → Plan and the Studio's Note for Claude, Publish), Claude (marks notes read) | Dev, the front page, `project.mjs status`, `ship.mjs` |
| The hub | `private/hub/` (`hub/hub.php`) | news posts and their screenshots (and the Studio notes' pictures), the Claude key (`key.json`), the loops' tuning (`tuning.json`, no reader since S4.2: KI-29); the retired board's files (`board.json`, `projects.json`) stay on the server, unused since J5.4 | Claude (news drafts: `tools/hub.mjs`), the user (publishes news, makes the key) | Dev → Publish, the news page (`news/`, published posts only; project names from `projects.json`), every server that checks the Claude key |
| Accounts | `home/bar/accounts/` (`server/account.php`; refused to the web by `.htaccess`) | users, names, login tokens (as SHA-256), avatars, admins and invited players | the bar's sign-up, profile and People screens | the bar's door (`gate.php`), the room (checks a token), Dev, the Studio, the UI Editor (who is an editor) |
| Phone reports | `private/bar/reports/` (`server/report.php`) | performance and crash reports, one file per 6-letter code, 30 days | the game (Send report) | Claude (`curl …report.php?code=XXXXXX`) |
| The room | Cloudflare Durable Objects (`cloud/room.js`, one per room name) | who is in the room right now, seats, games in play | the room server, from the phones' messages | the phones in that room |
| The ledger | Cloudflare Durable Object with SQLite (`cloud/ledger.js`) | credits (cents) and owned items, every change a row | only the server (the room's games) | a player's own wallet (`/bank` "me") |
| `gallery` branch | GitHub, branch `gallery` | each piece's before/after pictures and its line in the gallery | `tools/piece.mjs` (through `tools/lib/gallery.mjs`) | the user, the Records |
| `gh-pages` branch | GitHub Pages | a pointer page; its last commit "Site build <sha>" says which game build is deployed | `deploy.yml` | `deploy.yml` (what's live), `check_live.mjs` |
| The phone | the browser's `localStorage` | the login token (`bar3dToken`), name and profile copy, settings, the car setup, Studio draft backups | the bar's pages | the same pages, on that phone only |

## How each one flows

### The repo and the deploys
A piece is one commit on `job/J<n>`; the ship pushes `main` once per job.
`main` moving starts `deploy.yml` (the game: builds, uploads `home/bar/` and
the room, then tests) and `hub.yml` (the site, the hub, the Studio, Dev, the
UI Editor, the news page: tests first, uploads only if they pass). Each run
writes its state into Dev's store (`tools/ci_status.mjs`). Which folder each
workflow uploads: `docs/handbook/deploy-map.json` (J5.3).
**Stale shows as:** the `build xxxxxxx` label in the bar, `version.txt`, and
Dev's two deploy rows each name a build and how long ago.

### The world: two copies, one live
The **live world is the store's live version**. The repo's `assets/world/` is
the **built-in world**: the copy the game falls back to when the store has
nothing (version 0) or can't be reached, and Claude's working copy.
- The user publishes from the Studio → a new version; phones in the bar see
  it within 15 s.
- At the start of a job Claude pulls it (`node tools/studio.mjs pull`,
  committed as "World: from the studio vN"); `project.mjs status` says whether
  the repo matches the live version.
- A job that changed `assets/world/` publishes it once the deploy is live
  (`studio.mjs push`), as a new version by Claude.
**Conflict:** a publish names the version it started from; if someone
published since, it's refused (pull, merge, push again). The user's version
is never overwritten. **Stale shows as:** `studio.mjs status` ("repo matches
vN" or not); the Studio shows the live version and who published it.

### The UI: themes and screens
The repo's `ui/system/` is what the site and the bar are built from;
`ui/system/live-version.json` says which store version it holds. The user
publishes from the UI Editor; Claude pulls it (`uikit.mjs pull`, committed as
"UI: from the editor vN"), and `ui/themes.css` is regenerated from it
(`uikit.mjs css`, never by hand). **Conflict and staleness:** as the world.

### The plan
`docs/plan/plan.json` and `docs/charter/rules.json` are the source (written
only by `tools/plan.mjs`, which regenerates `PLAN.md`, the Charter's rules
block and CLAUDE.md's Never list). CI copies both into Dev's store; Dev reads
them there. **Stale shows as:** Dev's plan says which build it came from and
how long ago (J5.2).

### The user's channels (what each one is for)
Decided in J5.1 (KI-17; decision 0001):
- **The chat** is the one channel for wishes, questions, rules, "go", "broke",
  "undo". Everything the user wants starts here.
- **Dev → Plan notes** (and the Studio's ⋯ → Note for Claude, which lands in
  the same list with the thing, the camera view and a picture) are the chat
  for when the user is away from it: read at the start of every session (`project.mjs status` counts the unread ones) and
  handled exactly like a chat message (`docs/playbook/intake.md`).
- **The Studio** is the user's own hands on the world; **the UI Editor** the
  same for the look. When the user publishes, their version wins and Claude
  builds on it. Nothing to do when they haven't: `project.mjs status` shows
  "repo matches" and no pull is needed.
- **Dev → Publish** holds the user's switches: news, front-page highlights,
  the front page's words. Claude only writes drafts.

### The projects
One list: `toolbelt/tools.json` → `projects` (J5.5), in the repo. Dev →
Projects, the site's Projects page and front page (each project's `site`
card), and the news page (project names) all read it; a new project is one
entry there.

### Accounts, the room, the ledger
A login token is made by `account.php` and kept on the phone. The bar's door
(`gate.php`) lets in the owner, admins and invited players. The room checks
the token when a phone joins and never stores accounts. The ledger changes
only from the server; one change = one row with a unique reference, so a
repeated change does nothing. **Conflict:** the server's copy wins; a phone's
copy of its profile is refreshed from `account.php` (`me`).

### Phone reports
The game strips names and tokens, sends the report, shows a 6-letter code; the
user gives Claude the code. Files older than 30 days are deleted.
