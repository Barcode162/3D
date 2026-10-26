# Recipes: how each common change is made

The mechanics of the changes asked for most, step by step, with the exact
files and commands. Part of the Handbook: it says **how a change is made**.
**Whether and when** it is made is the Playbook's: every recipe runs inside a
job (`docs/playbook/job.md`: the request recorded, the verdict, one piece =
one commit through `node tools/project.mjs piece`, the ship). During the
restructure, features are frozen (`docs/archive/restructure.md` §7): recipes
1-11 and 14-17 wait for the unfreeze unless they fix something broken.

Words the user might use are in *italics*. Every piece ends with its own
checks (the piece tool picks them) and a phone check in plain words.

## The world (things, kinds, places)

The world has two copies ([data map](data-map.md) → The world): the live
version in the world store and the repo's `assets/world/`. **Before any world
change:** `node tools/project.mjs world status`; if the user published since,
`node tools/project.mjs world pull` and commit it as "World: from the studio vN".
**After the job is live:** `node tools/project.mjs world push --note "<pieces>"`
(or `world propose --note …` when the user should approve it in the Studio
first). Field by field: [data-world.md](generated/data-world.md); every thing
today: [inventory.md](generated/inventory.md).

### 1. Move a thing
*"Move the jukebox closer to the stage", "turn the sofa".*
1. Find the thing's id in [inventory.md](generated/inventory.md) (or the Studio's World list).
2. In `assets/world/places/<place>.json`, change its `at` (metres, on its parent), `turn` (degrees about up) or `scale`.
3. `python3 tools/place_format.py assets/world/places/<place>.json`.
4. The piece (pictures from the spots near it: `--spots`); phone check: where to stand and what's moved.

### 2. Add a thing
*"Put a candle on every table".*
1. Pick its kind ([inventory.md](generated/inventory.md) → kinds; a new kind: recipe 10).
2. Add `{"id": "<kind>-<n>", "kind": "<kind>", "parent": "<thing it sits on>", "at": [x, y, z]}` to the place file. The id never changes once players have seen it.
3. Format; the entity check (`tools/native/entity_test.cpp`, in `native`) wants `bar/entity_ids.txt` updated when the thing is shared (a seat, a use): that's a deliberate layout change.
4. The piece; phone check.

### 3. Remove a thing
*"Take the chalkboard away".* Remove its entry from the place file (and things whose `parent` is it, or give them a new parent); update `bar/entity_ids.txt` if it was listed; format; the piece.

### 4. Change a light
*"Make the neon redder", "brighter lamps over the bar".* A lamp's `light` part (colour, lumens, Kelvin, size) is on its kind (every lamp of that kind) or on the thing (only that one). Colour by Kelvin for white light (1800 K candle, 2700 K warm bulb); lumens for brightness. Phone check at night positions; the new light is checked on the phone (KI-28). Fields: [data-world.md](generated/data-world.md) → Light.

### 5. Change or add a mood
*"A party mood".* In the place file: `"moods": {"Party": {"lights": {"<kind or thing>": {"color": [r, g, b], "intensity": 1}}}}` and `"mood": "Party"` for the one in use. The Studio's Look tab shows them.

### 6. Change or add a material
*"The counter should look like marble".* `assets/world/materials.json` (look, feel, sound); a new name also goes in the schema's `material` choice list (`assets/world/schema.json` → `choices.material`); `node tools/albedo_test.mjs` checks the brightness is physically sensible; the thing's kind names it (`parts.model.material`).

### 7. Change a sound
*"The fire should crackle louder".* The kind's `sound` (by itself: effect, how often, how loud, how far) and `touchSound` parts. Sounds are made in code (`looped/synth.*`); a new effect is engine work (`looped/synth.cpp` and the schema's `effect` list).

### 8. Change a thing's loop
*"The lamps should sway slower".* The kind's `loops` → the loop's `period` (seconds, or beats with `music`) and its pieces (channel, shape, amount). Field list: [data-world.md](generated/data-world.md) → Loop, Piece.

### 9. Change a thing's rule or state
*"Tapping the lamp should switch it off".* The kind's `states`: each with its loop and `on` (event → next state), `after` / `afterLoops` → `next`, `enter` (messages to send). The room runs the same rules. Native check: `tools/native/world_test.cpp`.

### 10. A new kind
*"A pool table".* A model (`tools/assets/<name>.py`, run in headless Blender: `tools/assets/get_blender.sh`; output `assets/models/<name>.glb`, with its credit in `assets/models/CREDITS.md` if anything is from outside), then a kind in `assets/world/kinds/<place>.json` (`"is"` an existing base kind when it fits; parts: model, solids, seats, uses...). Schema check: `node tools/schema_test.mjs`. Then recipe 2.

### 10b. A Poly Haven model (the remake's kit, J35)
*"A real bar stool".* Find it at api.polyhaven.com (`?t=models`; every model is CC0). `bash tools/assets/polyhaven_fetch.sh <id> <scratch dir> 1k` downloads it; `/opt/blender/blender -b --factory-startup --python tools/assets/polyhaven.py -- <scratch dir>/<id> assets/models/<name>.glb --tris <budget> [--tex 1024] [--height m] [--turn deg]` slims it, stands it on its footprint's middle and writes it. Give it a budget in `assets/models/budget.json` (checked by `python3 tools/assets/glb_report.py --budget`), a line in `assets/models/CREDITS.md` (name, authors, page), its materials in `albedo.json` (`python3 tools/albedo_audit.py`), and a preview (`tools/assets/preview.py`). Then recipe 10's kind.

### 11. A new place
*"A rooftop terrace".* One file `assets/world/places/<id>.json` (its `kinds`, `air`, `spawn`, `rooms`, `things`) with its kinds; Menu → Places lists it; `?place=<id>` opens it; it gets its own room. Check: `tools/travel_test.mjs`.

### 12. Publish the world to the store
After the job that changed `assets/world/` is live: `node tools/project.mjs world push --note "<pieces>"`. Refused if the user published since: pull, merge, push again (the user's version wins).

### 13. Take in what the user published
At the start of a job when `node tools/project.mjs status` says a store is newer: `node tools/project.mjs world pull` / `ui pull`, then commit as "World: from the studio vN" / "UI: from the editor vN" on the job's branch. Never edit `assets/world/` or `ui/system/` to undo what the user published.

## Pages and the look

### 14. A new site page
*"A page about the games".* A folder `homepage/<name>/index.php` with `require __DIR__ . '/../site/layout.php'; site_head('<id>', '<title>', '<description>'); … site_foot();`; a tab in `site_tabs` (`homepage/site/layout.php`) if it's a tab; its Handbook page in `docs/handbook/ui/` (named `site-<name>.md`) and an entry in `tools/lib/labels.mjs` → `PAGES`; `tools/site_test.mjs` gets it in its list. Uploaded by `hub.yml`.

### 15. Change a theme
*"Darker buttons".* Normally the user does it in the UI Editor (then recipe 13). Claude: edit `ui/system/themes/<id>.json` tokens (every token: [data-ui.md](generated/data-ui.md)), then `node tools/project.mjs ui css` (writes `ui/themes.css`; never by hand) and `ui check`; after the job is live, `ui push --note "<pieces>"`.

### 16. Change a screen (a Dev tab and the like)
A screen is `ui/system/screens/<id>.json` (a tree of parts: [data-ui.md](generated/data-ui.md)); a part's `id` is a slot the page fills. Its controls' Handbook page changes with it (`docs/handbook/ui/`). Same store rules as recipe 15.

## Games, tools and checks

### 17. A new game in the bar
A web game (like Meridian): `bar/projects/<name>/` with `project.json` (`"kind": "web"`, `site`, `entry`, `opens_from`, `credits`), opened from a machine through `bar/web/menu.js` (GAMES); an outside engine needs the user's yes first (the Charter). A native game: `"kind": "native"` with its `code` files (C++, like chess). Its system page (`docs/handbook/systems/world/`), its test (recipe 18), its tool entry in `toolbelt/tools.json` if it has a page.

### 18. A new test
`tools/<name>_test.mjs` (Node; a browser test opens pages with `tools/lib/browser.mjs`, takes pictures through a helper that only notes a slow one). Then: in `tools/lib/checks.mjs` → `CHECKS` (its system) and `RULES` (which files it covers); in exactly one workflow as a step with an `id`, `if: ${{ !cancelled() … }}` and `timeout-minutes`; `node tools/workflows_test.mjs` proves all of it.

### 19. A new tool (command)
`tools/<name>.mjs` with its usage in its first comment lines; one entry in `tools/lib/commands.mjs` (name, summary, usage, run); `node tools/project.mjs handbook write` (the command list); the Playbook names it where it's used (`tools/playbook_test.mjs` checks it exists).

### 20. A new check that runs before every piece
An entry in `CHECKS.quick` and the system it belongs to; if GitHub doesn't run it, `NOT_ON_CI` with the reason.

### 21. Read a phone report
The user gives a 6-letter code: `curl -s 'https://joshuavelasco.com/bar/report.php?code=XXXXXX'` (the log's last 300 lines, the profiler, the device). Never paste a report's private parts into a reply.

### 22. An urgent fix
`docs/playbook/ways.md` → Urgent fix: the smallest change (or a revert), its own piece, shipped at once, always an incident (`node tools/project.mjs incident <slug> "<what happened>"`).

### 23. Revert a piece, or undo a job
`docs/playbook/revert.md`: the piece's commit by `git log --oneline --grep '^J<n>.<k> '`, `git revert <sha>` on the job's branch, the checks covering the reverted files, the ship. Never rewrite `main`'s history (the locks refuse it).

## Content and records

### 24. Add music or an outside asset
Only with a licence that allows it, never copyrighted music: the file under `assets/music/` (or `models/`, `midi/`, `ui/fonts/`), its line in that folder's `CREDITS.md` (title, author, licence, link); a jukebox song also goes in `bar/jukebox.cpp` (kBuiltIn) and `cloud/games/jukebox.js` (same order, its length), checked by `tools/native/run.sh`.

### 25. A news post
At the end of a job that people can see: the ship's `--news "<two or three friendly lines>"` writes a draft (`node tools/project.mjs news post --build <sha> --title … --text …` by hand). The user publishes it from Dev → Publish; Claude never does.

### 26. Add a project
One entry in `toolbelt/tools.json` → `projects` (id, name, status, about; a `site` card to show it on the site: `featured` with a tag and a line, or `more` with a link). Dev, the site's Projects page and the news page show it.

### 27. A new shared key in the room
*(a thing everyone sees the same)* The key's rule in `cloud/rules.js` (what it may hold, who may write it), the game's writer, `docs/handbook/protocol.md` §4.5, and `tools/room_test.mjs`; every room saves its world and `schemaVersion`.
