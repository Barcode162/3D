<!-- generated: node tools/handbook.mjs write (from docs/handbook/folders.json, docs/handbook/deploy-map.json and the files); never edit by hand -->
# The map: every folder

What each folder is, how many files it holds, its main languages, and which workflow uploads it where (`docs/handbook/deploy-map.json`; "not uploaded": it stays in the repo). How the data moves between the places: [data-map.md](../data-map.md).

| Folder | What it is | Files | Languages | Uploaded by → to |
|---|---|---|---|---|
| `(root)` | The repo's root: the build (CMakeLists.txt), the entry for AI sessions (CLAUDE.md), the plan's generated view (PLAN.md), GitHub's front page (README.md). | 5 | Markdown, other, text | deploy.yml → home/bar |
| `.claude/` | Claude Code's settings for this repo: the session-start status and the locks (tools/lib/guard.mjs). | 1 | JSON | not uploaded |
| `.github/` | GitHub's own folder. | 4 | YAML | not uploaded |
| `.github/workflows/` | The four workflows: deploy.yml (the game, the room), hub.yml (the site and the tools' pages), plan.yml (the plan to Dev), classic.yml (the old public bar). | 4 | YAML | not uploaded |
| `assets/` | Files the game loads while it runs (copied to the site's assets/). | 331 | 3D models, audio, JSON | deploy.yml → home/bar |
| `assets/midi/` | The piano's songs as MIDI files, with their credits. | 11 | MIDI, Markdown | deploy.yml → home/bar |
| `assets/models/` | 3D models (.glb) made by tools/assets/ in headless Blender, with their credits. | 172 | 3D models, JSON, Markdown | deploy.yml → home/bar |
| `assets/music/` | The jukebox's songs, with their licences and credits. | 17 | audio, Markdown | deploy.yml → home/bar |
| `assets/piano/` | The piano's recorded notes (Salamander Grand Piano, CC BY 3.0, J100; tools/piano_samples.sh), with their credit. | 121 | audio, Markdown | deploy.yml → home/bar |
| `assets/world/` | The built-in world: its schema, catalog, materials, songs list, and the kinds and places files. | 10 | JSON | deploy.yml → home/bar |
| `assets/world/kinds/` | What each kind of thing is (model, shapes, seats, tap areas, loops, states), one file per place. | 3 | JSON | deploy.yml → home/bar |
| `assets/world/places/` | Every thing in a place, where it sits and its parts, one file per place (bar, home). | 3 | JSON | deploy.yml → home/bar |
| `bar/` | Bar3D itself, in C++: the frame's phases, the world, the player, the bar's games and jobs, the HUD, the Studio mode. | 157 | C++, pictures, JavaScript | deploy.yml → home/bar |
| `bar/projects/` | The bar's projects, each with its project.json (web: copied to the site; native: built into the game). | 48 | C++, JavaScript, JSON | deploy.yml → home/bar |
| `bar/projects/chess/` | The lounge's chessboard and its computer player (C++). | 5 | C++, JSON | deploy.yml → home/bar |
| `bar/projects/mania/` | 4K Mania, the rhythm game (a web page inside the bar). | 5 | C++, JSON | deploy.yml → home/bar |
| `bar/projects/meridian/` | Meridian, the user's driving game (three.js, the approved exception), opened from a racing rig. | 13 | JavaScript, Markdown, HTML | deploy.yml → home/bar |
| `bar/projects/platformer/` | The arcade's platformer "Test 2" (three.js, added as is). | 8 | JavaScript, Markdown, text | deploy.yml → home/bar |
| `bar/projects/racing/` | The racing sim: track, cars, bots, controls, view, the race (C++). | 17 | C++, JSON | deploy.yml → home/bar |
| `bar/web/` | The bar's page: index.html and its JavaScript bridges (menus, audio, network, voice, accounts, the live world). | 52 | pictures, JavaScript, JSON | deploy.yml → home/bar |
| `bar/web/badges/` | The badge pictures on players' cards. | 8 | pictures | deploy.yml → home/bar |
| `bar/web/icons/` | The page's own icons. | 22 | pictures | deploy.yml → home/bar |
| `cloud/` | The room server on Cloudflare: the Worker (router), the room (one per room name), the ledger, the protocol codec, the rules. | 15 | JavaScript, JSON, shell | deploy.yml → Cloudflare (the room server and the ledger) |
| `cloud/games/` | Games the room runs itself (darts, glasses, jukebox, race). | 4 | JavaScript | deploy.yml → Cloudflare (the room server and the ledger) |
| `cloud/sim/` | The bar's own C++ physics built for the room (room_sim.wasm), for the glasses. | 3 | shell, C++, JavaScript | deploy.yml → Cloudflare (the room server and the ledger) |
| `dev/` | Dev, the control room (joshuavelasco.com/dev): its page, tabs and store API. | 12 | JavaScript, PHP, CSS | hub.yml → home/dev |
| `docs/` | The books: Charter, Playbook, Handbook, Plan, Records, and the old notes until J6.6 sorts them. | 205 | Markdown, JSON | plan.yml → private/dev |
| `docs/archive/` | History: old plans, instructions and notes (DECISIONS, STUDIO, ENGINE, REFERENCE, AUTOMATION, the audits...). Never instructions. | 29 | Markdown, JSON | not uploaded |
| `docs/charter/` | The Charter: what the project is, the user's rules (rules.json, word for word), the glossary. | 3 | Markdown, JSON | plan.yml → private/dev |
| `docs/handbook/` | The Handbook: how everything works (maps, systems, pages and controls, recipes); generated/ is written by tools/handbook.mjs. | 104 | Markdown, JSON | not uploaded |
| `docs/handbook/generated/` | Handbook pages written from the files by node tools/handbook.mjs; never edited by hand. | 8 | Markdown | not uploaded |
| `docs/handbook/manual/` | The manual: how light, materials, the world, avatars, recipes and the timeline work, with every dial (and the road ahead for each). | 7 | Markdown | not uploaded |
| `docs/handbook/systems/` | One page per system, by chapter (engine, world, site, tools, backbone), one template. | 51 | Markdown | not uploaded |
| `docs/handbook/ui/` | Every page and its controls, by exact label (checked against the live pages by tools/ui_labels_test.mjs). | 28 | Markdown | not uploaded |
| `docs/plan/` | The plan's data (plan.json, written only by tools/plan.mjs) and the jobs' specs. | 29 | Markdown, JSON | plan.yml → private/dev |
| `docs/plan/jobs/` | One spec per job after the restructure (J<n>.md), from TEMPLATE.md. | 28 | Markdown | plan.yml → private/dev |
| `docs/playbook/` | The Playbook: the only instructions, one procedure per situation. | 10 | Markdown | not uploaded |
| `docs/records/` | The Records: known issues, and the folders below. | 30 | Markdown | plan.yml → private/dev |
| `docs/records/decisions/` | Decisions in force, numbered (tools/records.mjs decision). | 10 | Markdown | plan.yml → private/dev |
| `docs/records/drills/` | Cold-start drills: requests given to fresh AI sessions, the answer key, the results (J8.1). | 1 | Markdown | plan.yml → private/dev |
| `docs/records/incidents/` | One file per error: what happened, the root cause, the fix, the proof. | 15 | Markdown | plan.yml → private/dev |
| `docs/records/requests/` | The user's words, exactly as sent, one file per month. | 1 | Markdown | plan.yml → private/dev |
| `docs/records/sessions/` | One line per session that changed something, one file per month. | 1 | Markdown | plan.yml → private/dev |
| `homepage/` | The site joshuavelasco.com (PHP): the front page and its sections. | 27 | pictures, PHP, other | hub.yml → home/ |
| `homepage/engine/` | The engine's page (/engine/). | 1 | PHP | hub.yml → home/ |
| `homepage/play/` | Bar3D's page (/play/). | 1 | PHP | hub.yml → home/ |
| `homepage/portal/` | The staff portal (/portal/: staff are sent to Dev, everyone else gets a 404). | 1 | PHP | hub.yml → home/ |
| `homepage/projects/` | The projects page (/projects/), from toolbelt/tools.json. | 1 | PHP | hub.yml → home/ |
| `homepage/signin/` | Sign in and new account (/signin/). | 1 | PHP | hub.yml → home/ |
| `homepage/site/` | The site's frame (layout.php: header, tabs, footer), its look (site.css) and pictures. | 19 | pictures, PHP, CSS | hub.yml → home/ |
| `homepage/site/img/` | The site's pictures. | 15 | pictures | hub.yml → home/ |
| `homepage/vision/` | Where it's all going (/vision/). | 1 | PHP | hub.yml → home/ |
| `hub/` | The hub (joshuavelasco.com/hub): hub.php (news, screenshots, the Claude key, tuning), World notes, the Dials. | 6 | HTML, other, JavaScript | hub.yml → home/hub |
| `incoming/` | Source material before it becomes assets. | 8 | Python, Markdown, shell | not uploaded |
| `incoming/meridian/` | Meridian's source material. | 8 | Python, Markdown, shell | not uploaded |
| `incoming/meridian/blender/` | Blender scripts that made Meridian's models. | 7 | Python, shell | not uploaded |
| `looped/` | Looped, the engine (C++, namespace engine): GPU, math, meshes, camera, input, text, audio, physics, skeletons, jobs, the profiler, the network codec. Never includes bar/. | 127 | C++, WGSL, HTML | deploy.yml → home/bar |
| `looped/apps/` | Programs built on the engine alone (J86): the place viewer (looped/apps/viewer, served at /bar/looped/). | 2 | HTML, C++ | deploy.yml → home/bar |
| `looped/apps/viewer/` | The place viewer: any place from the world files, drawn and alive, with an orbit camera; no bar code. | 2 | HTML, C++ | deploy.yml → home/bar |
| `looped/drive/` | The new driving physics (docs/plan/jobs/W20.md). | 6 | C++ | deploy.yml → home/bar |
| `looped/shaders/` | The WGSL shaders, built into the game. | 10 | WGSL | deploy.yml → home/bar |
| `looped/world/` | The world core: things, kinds, places, states, rules, loops on the shared clock. | 10 | C++ | deploy.yml → home/bar |
| `news/` | The public news page (joshuavelasco.com/news). | 1 | PHP | hub.yml → home/news |
| `proto/` | The room protocol's test vectors (both codecs must pass them). | 1 | JSON | not uploaded |
| `server/` | PHP next to the game on IONOS: accounts, the private bar's door, phone reports, the old relay. | 7 | PHP, other, HTML | deploy.yml → home/bar |
| `server/rooms/` | Leftover data of the old relay (not uploaded). | 1 | JSON | not uploaded |
| `studio/` | The Studio (joshuavelasco.com/studio): the world editor around the bar in studio mode, and the world store's API. | 24 | JavaScript, pictures, JSON | hub.yml → home/studio |
| `toolbelt/` | The Toolbelt (every tool and the one list of projects: tools.json) and its page. | 6 | HTML, JSON, PHP | hub.yml → home/toolbelt |
| `toolbelt/ui-editor/` | The UI Editor (themes and screens) and the UI store's API. | 4 | PHP, CSS, JavaScript | hub.yml → home/toolbelt |
| `tools/` | The work tools and the tests (Node and shell): the front door project.mjs, piece, ship, plan, checks, records, the stores' tools. | 174 | JavaScript, Python, C++ | not uploaded |
| `tools/assets/` | Scripts that make the 3D models (headless Blender) and the compressed textures. | 41 | Python, shell | not uploaded |
| `tools/githooks/` | The git locks (pre-commit: secrets and the plan; commit-msg: the freeze; pre-push: main). | 3 | other | not uploaded |
| `tools/lib/` | Shared pieces of the tools (checks list, browser, gallery, plan data, the guard, the front door's commands). | 19 | JavaScript | not uploaded |
| `tools/migrations/` | One-time conversions of the world files (kept as the record of what changed). | 3 | JavaScript, JSON | not uploaded |
| `tools/native/` | Tests and tools that run the engine without a browser (g++ and em++): physics, cars, races, the world, light. | 24 | C++, shell | not uploaded |
| `tools/preflight/` | The preflight checklist (J112): one small rule file per check in rules/<area>/ (copy TEMPLATE.mjs), run by node tools/preflight.mjs before any test, build or push; a broken rule names itself, the spot and the fix. | 14 | JavaScript | not uploaded |
| `ui/` | The look kit shared by the site, the tools and the bar's menus: looped.js and looped.css (the parts), screens, fonts, icons, the theme CSS. | 46 | JSON, fonts, text | hub.yml → home/ui |
| `ui/fonts/` | The fonts (with their licences in ui/CREDITS.md). | 10 | fonts, text | hub.yml → home/ui |
| `ui/system/` | The UI store's copy: themes, tokens, screens and parts, pulled from the UI Editor (never edited by hand). | 26 | JSON, pictures | hub.yml → home/ui |
| `ui/system/img/` | Pictures the screens use. | 2 | pictures | hub.yml → home/ui |
| `ui/system/screens/` | Screens as data (Dev's tabs among them), drawn by ui/looped.js. | 9 | JSON | hub.yml → home/ui |
| `ui/system/themes/` | The themes (colours, shapes, motion), turned into ui/themes.css by tools/uikit.mjs css. | 12 | JSON | hub.yml → home/ui |
