# Bar3D gallery

Every piece of work, newest first: what it is, its before/after pictures
(left before, right after) and its numbers. Made by `tools/piece.mjs`;
benchmarks by `tools/bench.mjs` (see `bench.jsonl`).

Latest benchmark (2026-10-22 18:44, 922ec64): gi_triangles_full 642040 tris, gi_triangles 99105 tris, gi_load_ms 167 ms, gi_simplify_ms 28 ms, gi_bvh_ms 239 ms, gi_patches 58292, gi_probes_usable 10128, gi_build_ms 6666 ms, gi_rays_per_s 0.59 M/s, gi_solve4_ms 12.6 ms

## R4.7 The old doors lead to Dev: /hub and /toolbelt open Dev (the old board kept as hub/board.html, unlinked; hub.php stays for news and Claude's key), /portal sends staff to Dev, the site's staff tab and sign-in say Dev, Dials, World notes and the UI Editor go back to Dev, the Studio gets a back key to Dev

`f247b0b` · 2026-10-24 06:39 · 15 files, +836 −896 · tests: quick, dev, toolbelt, hub, site

Phone check: open joshuavelasco.com/hub: Dev opens; joshuavelasco.com shows a Dev tab when you're signed in; in the Studio the back arrow (top left) goes to Dev

![R4.7 page-dev-index-demo-overview.jpg](R4.7/page-dev-index-demo-overview.jpg)

## R4.6 Dev's People tab: everyone with an account (owner and admins first, then who's waiting), Let in / Remove for players, Make admin / Remove admin for the owner only, invite by username (the bar's account.php, moved here from the gate page)

`81477b2` · 2026-10-24 06:31 · 4 files, +59 −1 · tests: quick, dev

Phone check: Dev → People: you at the top as Owner; anyone who signed up and waits has Let in

![R4.6 page-dev-index-demo-people.jpg](R4.6/page-dev-index-demo-people.jpg)

## R4.5 Dev's Publish tab: News (every post Claude writes after a step, its build, pictures and words; Publish puts it on joshuavelasco.com/news in one tap, Unpublish takes it off) and The site (the front page's headline, line, news highlight and its link, saved in Dev's store and public for R5)

`712a5ef` · 2026-10-24 06:27 · 5 files, +100 −1 · tests: quick, dev

Phone check: Dev → Publish: the R3 post is there as Draft; tap Publish, it turns Public and shows on joshuavelasco.com/news; The site: type a headline, Save the words

![R4.5 page-dev-index-demo-publish.jpg](R4.5/page-dev-index-demo-publish.jpg)

## R4.4 Dev's Plan tab: PLAN.md on the phone (Now with its pieces and checks, the road under the five chapters, Parked, Done, the North star, your rules; long lists fold), and Tell Claude: a note or an idea to park, kept in Dev's store; node tools/dev.mjs notes reads them at the start of every go (CLAUDE.md)

`c5be89e` · 2026-10-24 06:22 · 7 files, +130 −2 · tests: quick, dev, toolbelt

Phone check: Dev → Plan: R4 at the top with its pieces; the five chapters below; under Tell Claude pick 'Park an idea', write one, Send: it shows as Waiting

![R4.4 page-dev-index-demo-plan.jpg](R4.4/page-dev-index-demo-plan.jpg)

## R4.3 Dev's Overview, live: the bar's build (its config), the room server (its health: players, messages today), the last deploy (CI now writes its result to Dev's store, even when a test fails), news waiting to be published, the step in Now (how many pieces done, what's next, what comes after); Dev's store (dev/api.php: the plan, the deploy result, notes, the site's words; owner and admins); PLAN.md read in the browser; Projects drops its duplicate Open row

`340ed5a` · 2026-10-24 06:18 · 9 files, +283 −4 · tests: quick, dev

Phone check: Dev → Overview: The bar Up with its build code, Room server Up with people in the bar, Last deploy (after the next game deploy: Green or Red, tap opens the run), Now: R4 with 3 of 7

![R4.3 page-dev-index-demo-overview.jpg](R4.3/page-dev-index-demo-overview.jpg)

## R4.2 Projects in Dev: every project with its own tools from toolbelt/tools.json (Bar3D: open the bar, the Studio, Dials, World notes, perf reports, the gallery, classic, world sync; Looped: UI Editor, Screens, Compare, Style page, engine flags, benchmarks, the light tool, the UI kit; the script language: planned; the workshop: Claude's tools); a new project is one line; ?demo on this computer for pictures

`85fa081` · 2026-10-24 06:09 · 8 files, +101 −29 · tests: quick, dev, toolbelt

Phone check: Dev → Projects: Bar3D with Open Bar3D and the Studio first; tap Studio, it opens; Looped lists the UI Editor

![R4.2 page-dev-index-demo-projects.jpg](R4.2/page-dev-index-demo-projects.jpg)

## R4.1 Dev opens at /dev: the bar's login (owner and admins only; a player is told so), Ember, five tabs (Overview, Projects, Plan, Publish, People), each tab a screen file (ui/system/screens/dev-*.json, editable in the UI Editor) whose slots (parts with an id) dev.js fills with live data; Looped.part() draws single parts; a throwaway-webspace helper for PHP page tests; its test

`03c3fe4` · 2026-10-24 06:04 · 16 files, +661 −128 · tests: quick, dev

Phone check: open joshuavelasco.com/dev and log in with your bar account: five tabs at the bottom (Overview, Projects, Plan, Publish, People); each opens its page (example rows for now)

![R4.1 page-dev.jpg](R4.1/page-dev.jpg)

## R3.3 The plan keeper (tools/plan.mjs): check the shape, now, tick, add (needs its chapter and North star line), park, rule, start (a road item becomes Now with its pieces), done; refuses anything that breaks the eight parts or five chapters; piece.mjs ticks Now's pieces, step.mjs writes Done through it; a test on a copy (quick check); in the Toolbelt

`a75881c` · 2026-10-24 05:32 · 9 files, +204 −11 · tests: quick

Phone check: none on the phone; open PLAN.md: R3.3 is ticked

## R3.2 Carry over: every open thing from NEXT.md, PROGRESS.md, the 68 hub cards, the Alive plan and the rework plan into PLAN.md's road (60 items, each in its chapter with the North star line it serves) and Parked; 14 old plan files moved to docs/archive/ (with a README); NEXT.md points to PLAN.md; CLAUDE.md, REFERENCE.md and README.md read the plan from PLAN.md

`9628809` · 2026-10-24 05:27 · 20 files, +128 −137 · tests: none

Phone check: open PLAN.md: part 5 lists everything still to do under the five chapters; docs/ has no other plan files

## R3.1 The World Plan (PLAN.md): one doc in eight fixed parts (North star N1-N5, your rules, the five chapters with where each stands, Now, the road, Parked, Done with the whole build log, where things are)

`882582d` · 2026-10-24 05:25 · 1 files, +104 −0 · tests: none

Phone check: open PLAN.md on GitHub: the top says what the project is; part 4 shows R3 with R3.1 ticked

## R2.6 Ember, the house theme (the user's mix): Warm's amber and type, Glass's frosted see-through panels, a three-colour background glow that slowly drifts, titles with a light passing through them and a soft glow; light and dark halves; the Toolbelt wears it (pinned), the UI Editor and Compare start on it; the other ten themes stay as options

`b4f2fea` · 2026-10-24 05:03 · 9 files, +249 −9 · tests: quick, toolbelt

Phone check: open the Toolbelt: amber glow behind frosted cards, the title 'Toolbelt' has a light passing through it every few seconds; the UI Editor opens on Ember; Compare themes shows 'House · Ember' first

![R2.6 page-toolbelt.jpg](R2.6/page-toolbelt.jpg)

![R2.6 page-ui-screen-s-front-door-look-ember.jpg](R2.6/page-ui-screen-s-front-door-look-ember.jpg)

![R2.6 page-ui-screen-s-dev-overview-look-ember-scheme-dark.jpg](R2.6/page-ui-screen-s-dev-overview-look-ember-scheme-dark.jpg)

![R2.6 page-ui-screen-s-dev-overview-look-ember-scheme-light.jpg](R2.6/page-ui-screen-s-dev-overview-look-ember-scheme-light.jpg)

## R2.5 Motion as theme tokens: moving titles (a light passing through the letters, a breathing glow, both, or a gentle float; their paint, colour, glow and speed) and a background glow that slowly drifts; a Motion group in the UI Editor; off when the phone asks for less motion; every theme so far keeps still

`393d12e` · 2026-10-24 04:57 · 17 files, +231 −14 · tests: quick, toolbelt

Phone check: nothing moves yet on the bar, Studio or the hub; in the UI Editor, Theme → Motion → title-motion: float, the title starts to bob

![R2.5 page-ui-screen-s-front-door.jpg](R2.5/page-ui-screen-s-front-door.jpg)

## R2.4 Compare and mix: /ui/compare.html shows one screen in every theme side by side (light, dark or both; tap one for full size; in the Toolbelt); the UI Editor takes a whole group (colour, shape, type...) from another theme into this one, both halves, one undo; the editor and store tests use a free port and always stop their server

`8b9d289` · 2026-10-24 04:38 · 5 files, +135 −2 · tests: quick, toolbelt

Phone check: Toolbelt → Compare themes: six phones; switch Light / Dark / Both and the screen menu. In the UI Editor, Theme → Shape → 'Take Shape from…' Bold: the corners go square

![R2.4 page-ui-compare-s-front-door-scheme-dark.jpg](R2.4/page-ui-compare-s-front-door-scheme-dark.jpg)

![R2.4 page-ui-compare-s-dev-overview-scheme-light.jpg](R2.4/page-ui-compare-s-dev-overview-scheme-light.jpg)

## R2.3 Six candidate themes, each with a light and a dark half: Clean (calm, Apple-like), Warm (the bar at night), Bold (black frames, hard shadows, yellow), Glass (frosted panels over violet and cyan), Neon (the bar's signs), Soft (round, pastel); system fonts in the editor's font menu

`7542058` · 2026-10-24 04:30 · 10 files, +1197 −4 · tests: quick, toolbelt

Phone check: in the UI Editor pick each of Clean, Warm, Bold, Glass, Neon, Soft in the preview's theme menu and flip Dark / Light

![R2.3 page-ui-screen-s-dev-overview-look-clean-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-clean-scheme-light.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-warm-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-warm-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-bold-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-bold-scheme-light.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-glass-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-glass-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-neon-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-neon-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-soft-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-soft-scheme-light.jpg)

## R2.2 Light and dark in one theme: a theme file can carry its other half (only the tokens that differ), the page follows the phone's setting (or ?scheme= / Looped.setScheme), the editor's Dark / Light switch shows and edits either half (Light on a dark-only theme starts a plain light half); new type tokens (word font, key case, size, weight, spacing, width) so a theme can drop the capitals; the engine reads both halves

`3386773` · 2026-10-24 04:24 · 15 files, +244 −20 · tests: quick, toolbelt

Phone check: in the UI Editor tap Light above the preview: the screen turns light; tap Dark: back. The bar and Studio look the same as before.

![R2.2 page-toolbelt-ui-editor-index-screen-dev-overview.jpg](R2.2/page-toolbelt-ui-editor-index-screen-dev-overview.jpg)

## R2.1 Real screens to judge a theme on: the front door (a bar picture behind the words), Dev's Overview (status rows, what needs you, a tab bar) and the bar's jukebox menu; new parts: backdrop, list row, tab bar; pictures in ui/system/img/

`0bdee82` · 2026-10-24 04:19 · 10 files, +922 −45 · tests: quick, toolbelt

Phone check: in the UI Editor's screen menu (top), open Front door, Dev · Overview and Bar · Jukebox; each looks like a real page

![R2.1 page-ui-screen-s-front-door.jpg](R2.1/page-ui-screen-s-front-door.jpg)

![R2.1 page-ui-screen-s-dev-overview.jpg](R2.1/page-ui-screen-s-dev-overview.jpg)

![R2.1 page-ui-screen-s-bar-menu.jpg](R2.1/page-ui-screen-s-bar-menu.jpg)

## R1.7 Save and publish in the UI Editor: undo history, a draft kept on the phone (back after a reload), Publish keeps every version (toolbelt/ui-editor/api.php, the Studio's login, Claude's key), History, the editor starts from the newest published version; node tools/uikit.mjs pull/push/status bring it into the repo (ui/system/live-version.json), pulled at the start of every go; the engine reads theme files (looped/ui_theme.*, with a native test)

`b394ba3` · 2026-10-24 04:09 · 18 files, +951 −81 · tests: quick, toolbelt

Phone check: in the UI Editor change a colour, then tap Publish (top right): log in with your bar account, write a note, Publish; open Publish again: History shows v1 with your note; reload the page: your colour is still there

![R1.7 page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg](R1.7/page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg)

## R1.6 Screen editing in the UI Editor: every option of the selected part (words, numbers, on/off, choices, icons, lists), move up and down, duplicate, delete, add a part after it or inside it, a new screen, the screen's name; deep link to a part (&sel=1.0.1.0); fix: the theme notes no longer overlap, the top bar fits a phone

`0b6722f` · 2026-10-24 04:02 · 4 files, +148 −11 · tests: quick, toolbelt

Phone check: in the UI Editor tap Publish: change its words to 'Go live', the key changes; '+ Add after…' → Chip, then the up arrow moves it before the key; the bin deletes it; undo brings it back

![R1.6 page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg](R1.6/page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg)

## R1.5 Theme editing in the UI Editor: every token in 6 groups (colour, surfaces, shape, depth, type, lights) with colour pickers that keep see-through, sliders, font and choice menus and the exact text; name, about, light or dark, copy as a new theme, back to as loaded; undo and redo (one step per slider drag); deep links (?mode=theme&group=shape)

`f138572` · 2026-10-24 03:57 · 5 files, +511 −51 · tests: quick, toolbelt

Phone check: in the UI Editor tap Theme, then Shape: drag key-r and every key in the preview rounds; Colour: pick a new accent, the preview changes; the undo arrow takes it back

![R1.5 page-toolbelt-ui-editor-index-mode-theme-group-shape.jpg](R1.5/page-toolbelt-ui-editor-index-mode-theme-group-shape.jpg)

![R1.5 page-toolbelt-ui-editor-index-mode-theme-group-colour.jpg](R1.5/page-toolbelt-ui-editor-index-mode-theme-group-colour.jpg)

## R1.4 The UI Editor (toolbelt/ui-editor/): live preview on top (phone, computer, in game; any theme), the parts list and the inspector below, side by side on a computer; a tap in the preview or the list selects the part in both; its own test

`98da435` · 2026-10-24 03:52 · 9 files, +340 −6 · tests: quick, toolbelt

Phone check: open joshuavelasco.com/toolbelt/ui-editor: the sampler in the preview; tap Publish, it gets a dashed outline and 'Key' shows below with its options; tap a row in the list, the preview outlines that part

![R1.4 page-toolbelt-ui-editor.jpg](R1.4/page-toolbelt-ui-editor.jpg)

## R1.3 Screen files and the web renderer: a screen is a tree of parts (ui/system/parts.json lists the 20 kinds and their options), Looped.screen() draws one (ui/screen.js), /ui/screen.html?s=sampler shows every part; uikit checks every screen

`a6d79dd` · 2026-10-24 03:47 · 7 files, +309 −2 · tests: quick

Phone check: open joshuavelasco.com/ui/screen.html?s=sampler: keys, choices, knob, fader, meter, save lamps, icons and a see-through panel, all working; add &look=jukebox to see it in another theme

![R1.3 page-ui-screen-s-sampler.jpg](R1.3/page-ui-screen-s-sampler.jpg)

![R1.3 page-ui-screen-s-sampler-look-backbar.jpg](R1.3/page-ui-screen-s-sampler-look-backbar.jpg)

## R1.2 The theme files: every token described once (ui/system/tokens.json: group, kind, what it does), the 4 themes as files (ui/system/themes/), ui/themes.css and looped.js's theme list written from them by tools/uikit.mjs, a check that they agree (quick check and CI); every token's value is unchanged

`a21442c` · 2026-10-24 03:43 · 14 files, +580 −73 · tests: quick

Phone check: nothing should change: the bar's menus, the Studio and joshuavelasco.com/ui look exactly as before in every theme

![R1.2 page-ui.jpg](R1.2/page-ui.jpg)

![R1.2 page-toolbelt.jpg](R1.2/page-toolbelt.jpg)

## R1.1 The Toolbelt: one list of the engine's tools (toolbelt/tools.json) and one page that opens them (/toolbelt/); the piece runner takes phone pictures of site pages (--pages) and pushes to any branch (--branch)

`a28d85c` · 2026-10-24 03:39 · 7 files, +230 −7 · tests: quick

Phone check: open joshuavelasco.com/toolbelt: three groups (Make, Tune and check, Run by Claude); tap Studio, Dials and Style page, each opens; the search box filters

![R1.1 page-toolbelt.jpg](R1.1/page-toolbelt.jpg)

## L1.7 The new light in the Studio: lumens, colour temperature, shape and size change live; moods in real units (Party brighter, Closing dimmer and warmer)

`73f2d72` · 2026-10-23 03:09 · 6 files, +45 −10 · tests: native

Phone check: In the Studio, select a ceiling lamp, set Colour temperature to 2000 and Lumens to 1500: the room around it turns amber and brighter at once. Look → Closing dims and warms the bar.

## L1.6 Lights with size: tubes and panels (streak and rectangle highlights, soft area light), lamps fitted to the glowing objects beside them (light tool fit + migration), lamp-backed glow not counted twice in the bounced light

`5d99353` · 2026-10-23 01:12 · 17 files, +504 −23 · tests: changed · headless frame 97.23 ms, 288 draws

Phone check: Look at the jukebox: its light comes from its whole front and reflects as a soft rectangle on the floor; in the arcade the neon frames light like big soft squares.

![L1.6 door.jpg](L1.6/door.jpg)

![L1.6 door-switch.jpg](L1.6/door-switch.jpg)

![L1.6 mirror.jpg](L1.6/mirror.jpg)

![L1.6 mirror-switch.jpg](L1.6/mirror-switch.jpg)

![L1.6 bartop.jpg](L1.6/bartop.jpg)

![L1.6 bartop-switch.jpg](L1.6/bartop-switch.jpg)

![L1.6 backbar.jpg](L1.6/backbar.jpg)

![L1.6 backbar-switch.jpg](L1.6/backbar-switch.jpg)

![L1.6 piano.jpg](L1.6/piano.jpg)

![L1.6 piano-switch.jpg](L1.6/piano-switch.jpg)

## L1.5 Bounced light in the game: the GPU casts the build rays (looped/gi.h, shaders/gi.wgsl), one more bounce every frame, every pixel reads its 8 probes; the flat fill replaced; no lamp light through walls; debug views; shader check tool

`e390b36` · 2026-10-22 20:09 · 20 files, +1269 −13 · tests: changed · headless frame 169.82 ms, 288 draws

Phone check: After entering, look around for a few seconds: walls and corners near lamps glow softly with bounced light; the arcade's purple spills onto the floor; nothing glows through walls. ?giview=1 shows the bounced light alone.

![L1.5 door.jpg](L1.5/door.jpg)

![L1.5 door-switch.jpg](L1.5/door-switch.jpg)

![L1.5 mirror.jpg](L1.5/mirror.jpg)

![L1.5 mirror-switch.jpg](L1.5/mirror-switch.jpg)

![L1.5 bartop.jpg](L1.5/bartop.jpg)

![L1.5 bartop-switch.jpg](L1.5/bartop-switch.jpg)

![L1.5 backbar.jpg](L1.5/backbar.jpg)

![L1.5 backbar-switch.jpg](L1.5/backbar-switch.jpg)

![L1.5 piano.jpg](L1.5/piano.jpg)

![L1.5 piano-switch.jpg](L1.5/piano-switch.jpg)

## L1.4 Bounced light, the core: still surfaces gathered and simplified, a ray tree, patches, probes, lamp links, bounces (CPU reference, physics test: a white box ends at 5x its direct light); tools/light.mjs, gi benchmark

`1f6b899` · 2026-10-22 18:51 · 11 files, +1050 −20 · tests: changed

Phone check: nothing to see yet (the engine side of bounced light; it shows in L1.5)

## L1.3 Real material brightness: every material measured (tools/albedo_audit.py → assets/models/albedo.json), the ones darker or brighter than anything real moved into real ranges (new lighting); models keep their shapes for rays

`922ec64` · 2026-10-22 18:40 · 17 files, +5003 −7 · tests: changed · headless frame 69.78 ms, 288 draws

Phone check: Look at the bar top and the stage: wood grain, the mahogany's red and the green wall show clearly instead of near-black.

![L1.3 door.jpg](L1.3/door.jpg)

![L1.3 door-switch.jpg](L1.3/door-switch.jpg)

![L1.3 mirror.jpg](L1.3/mirror.jpg)

![L1.3 mirror-switch.jpg](L1.3/mirror-switch.jpg)

![L1.3 bartop.jpg](L1.3/bartop.jpg)

![L1.3 bartop-switch.jpg](L1.3/bartop-switch.jpg)

![L1.3 backbar.jpg](L1.3/backbar.jpg)

![L1.3 backbar-switch.jpg](L1.3/backbar-switch.jpg)

![L1.3 piano.jpg](L1.3/piano.jpg)

![L1.3 piano-switch.jpg](L1.3/piano-switch.jpg)

## L1.2 Real units: every lamp in lumens and Kelvin, true inverse-square light with no edge, white balance like eyes; world lights migrated (tools/world_migrate.mjs)

`1975523` · 2026-10-22 18:25 · 23 files, +440 −93 · tests: changed · headless frame 76.03 ms, 288 draws

Phone check: Stand at the door: lamp light now spreads across the whole room in soft pools, brightest under each lamp, with no edge where it stops. Bulbs look warm, not orange.

![L1.2 door.jpg](L1.2/door.jpg)

![L1.2 door-switch.jpg](L1.2/door-switch.jpg)

![L1.2 mirror.jpg](L1.2/mirror.jpg)

![L1.2 mirror-switch.jpg](L1.2/mirror-switch.jpg)

![L1.2 bartop.jpg](L1.2/bartop.jpg)

![L1.2 bartop-switch.jpg](L1.2/bartop-switch.jpg)

![L1.2 backbar.jpg](L1.2/backbar.jpg)

![L1.2 backbar-switch.jpg](L1.2/backbar-switch.jpg)

![L1.2 piano.jpg](L1.2/piano.jpg)

![L1.2 piano-switch.jpg](L1.2/piano-switch.jpg)

## L1.1 Eyes: the scene drawn as real light (HDR), metered and adapting like eyes, the AgX film curve; Settings → Lighting New/Classic

`31c8f7b` · 2026-10-22 17:55 · 15 files, +437 −86 · tests: game · headless frame 54.47 ms, 288 draws

Phone check: Walk from the bright bar counter to the dark stage corner: the picture slowly brightens as your eyes adjust. Settings → Graphics → Lighting: flip New and Classic, highlights look softer in New.

![L1.1 door.jpg](L1.1/door.jpg)

![L1.1 door-switch.jpg](L1.1/door-switch.jpg)

![L1.1 mirror.jpg](L1.1/mirror.jpg)

![L1.1 mirror-switch.jpg](L1.1/mirror-switch.jpg)

![L1.1 bartop.jpg](L1.1/bartop.jpg)

![L1.1 bartop-switch.jpg](L1.1/bartop-switch.jpg)

![L1.1 backbar.jpg](L1.1/backbar.jpg)

![L1.1 backbar-switch.jpg](L1.1/backbar-switch.jpg)

![L1.1 piano.jpg](L1.1/piano.jpg)

![L1.1 piano-switch.jpg](L1.1/piano-switch.jpg)

## L0 Automation: piece runner, step finisher, benchmarks, gallery branch, manual pages, feature switches

`69fb05c` · 2026-10-22 17:28 · 18 files, +688 −0 · tests: none

Phone check: nothing to see: tools only

