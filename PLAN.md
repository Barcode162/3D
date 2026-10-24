# J Production · The World Plan

The one document of truth for the whole project: where it's going, what
holds always, every part of the world, what's being built now, what comes
next, what's parked and what's done. **Its eight parts never change** (shape
kept by `node tools/plan.mjs check`). Any request lands in one of them;
nothing ever adds another plan file. Details that *explain* how things work
live in the docs listed in part 8; they never hold plans or ticks.

## 1. North star
*Only the user changes this part.*
- **N1 A world you're inside.** joshuavelasco.com is one place, not a set of
  pages: the bar, the showcase and Dev are places in it, joined by movement.
- **N2 Made from scratch.** Our own engine, Looped, and our own script
  language: no outside engines. Made by J Production.
- **N3 Real, and alive.** Real light, shapes, materials, physics and sound,
  pushed livelier than real life: the world keeps moving on its own; joining
  in is optional and fun. Engines that stack: each one makes the others better.
- **N4 A life with other people.** A hangout for 13+: homes, games, events,
  credits, voice; safe by design.
- **N5 Made on the iPhone.** Everything is judged on the user's phone
  (Safari): smooth first, one look (the house theme Ember) everywhere.

## 2. Your rules
*Decisions that always hold, newest first. Only the user adds or changes them.*
- (2026-10-24) **News highlights are for people, not engineers:** a picture and a short, friendly line, no technical words; they go on the front page only when the user pushes them from Dev → Publish.
- (2026-10-24) **The website keeps what works:** the current site's boxes, layout and animations stay; it's dressed in Ember (amber, frosted boxes, the glow, moving titles) and rearranged for the company front page.
- (2026-10-24) **joshuavelasco.com is J Production's company site, not only Bar3D:** an intro, one eye-catching news item, Enter the bar; scroll down for the bar's screenshots and what there is to do; other projects in their own tab; Looped and the script language shown as works in progress that admins and devs use.
- (2026-10-24) **Dev holds everything except the main page and the games:** the Studio, every tool, the plan, publishing, people; organised by project (Bar3D, the Looped engine, the script language, every future project).
- (2026-10-24) **All engine work is paused until the rework (R3-R6) is done.**
- (2026-10-24) **This plan:** one doc, eight fixed parts, five chapters; every road item names the North star line it serves.
- (2026-10-24) **The house theme is Ember**; the other themes stay as options. One look for the site, Dev, the tools and the menus (R6).
- (2026-10-24) **The bar's menus stay plain HTML** over the 3D (no GPU cost) until Looped is understood better.
- (2026-10-24) **The UI Editor is an engine tool** in the Toolbelt: dense, every setting.
- (2026-10-24) **The site keeps few words**: the minimum text, a real 3D scene behind it later.
- (2026-10-23) **Speed over tests**: no browser tests on the cloud machine; the quick check + pictures gate a piece; CI runs the rest; the phone is the real test.
- (2026-10-22) **Light only comes from light sources**: no fake fill; darkness is real; eyes adjust.
- (2026-10-22) **Automation first**: anything done twice becomes a tool.
- (2026-10-22) **Wow and foundation take turns**; every visual piece has an off switch and before/after pictures.
- (2026-10-21) **"go" = one whole step**: one piece = one commit with its own phone check ("broke <piece>" finds it); one push to main per step.
- (2026-10-19) **13+ only**; teens (13-17) and adults kept apart. The casino is 18+ and earned credits only.
- (2026-10-15) **From scratch in C++**: no outside engines or libraries (three.js only for Meridian and the platformer). Every third-party asset licensed and credited; no copyrighted music.

## 3. The chapters
*The fixed list. Every road and parked item sits under one of these.*
- **The world:** what players see and do: the bar, avatars, games, people. *Now:* the bar is built and alive (three rooms, darts, chess, piano, 4K Mania, racing, bartending, jukebox, voice, profiles); the new light is live.
- **The site:** the front door, the showcase, the news. *Now:* six long pages in their own look (Home, Play, Engine, Vision, News, Sign in); R5 replaces them with the one-world front door.
- **The engine:** Looped, the script language, speed. *Now:* E1-E9 built (things with live loops, physics, sound, net); real light L1.1-L1.7 live; paused for the rework.
- **The tools:** the Toolbelt and Dev. *Now:* the Studio (S1-S10), the Dials, the Toolbelt, the UI Editor and Ember (R1-R2); Dev (R4) next, replacing the hub and the portal.
- **The backbone:** server, accounts, money. *Now:* accounts with ages, the ledger (credits), the multiplayer room, voice; accounts still on IONOS.

## 4. Now
**R6 · Ember on everything that's left** (the tools). Ember on everything that's left: the game's loading screen, its menus and the private bar's door page; the arcade's two web games' menus; the Studio, Dials, World notes, the UI Editor, Compare, Screens and the style page; the public News page; Ember becomes the default theme (the other ten stay as options); the retired hub board pages are removed. Not the 3D world, not the screens drawn inside it (later, W23), not Bar3D classic. · N5
- [x] **R6.1** Ember is the default theme: every page that wears the Looped look switches (the bar's menus, the Studio, Dials, News...); a theme picked on purpose is still remembered; pages over the 3D keep Ember's dark half; the other ten themes stay options *Check:* open the bar's menu: amber keys, words as typed, not capitals
- [x] **R6.2** The bar's menus and loading screen in Ember: panels you can read over the 3D (no extra blur, so no extra GPU work), keys, tiles, toggles, the player card, the top keys and the toasts on the house theme's tokens; the loading screen with the glow and the moving title *Check:* enter the bar: the loading screen glows amber; open Menu: warm frosted-looking panels, amber keys
- [ ] **R6.3** The private bar's door page (sign in, waiting to be let in) in Ember *Check:* open joshuavelasco.com/bar signed out: the door page is amber, like the site
- [ ] **R6.4** The Studio in Ember: its top bar, panels and keys *Check:* open the Studio: amber top bar and keys
- [ ] **R6.5** Dials, World notes and the public News page in Ember (World notes loses its purple) *Check:* open Dials, World notes and /news: all amber
- [ ] **R6.6** The last tools: the UI Editor's own buttons and panels, Compare, Screens and the style page in Ember; the retired hub board pages deleted (their data stays, Dev shows it) *Check:* open the UI Editor: its bars are amber, the preview still shows any theme

## 5. The road
*Next and later, by chapter. Every item: a tag, what it is, the North star line it serves.*
**Up next, in order:** engine work resumes: EN4 → L1.8 → P1 → P2 → EN1 → M1 → L2 → T1 → … (the Alive order).

### The world
- [ ] **A1** Alive avatars: breathing, blinking, looking at whoever talks, shifting weight, feet planted on stairs; one body with sliders. · N3
- [ ] **A2** Real people: skin light passes into, hair strands, wet see-through eyes, clothes that swing, lips that follow voice. · N3
- [ ] **A3** Your face: an avatar face from a few selfies. · N3 N4
- [ ] **W1** Memory: the floor wears where people walk, the dartboard gets holes, your usual stool gets your nameplate. · N3
- [ ] **W2** Mood and chance: one cozy↔wild dial for the whole bar; "maybe" events (30% rain, a surprise band). · N3
- [ ] **W3** Time and era: scrub time back and see ghosts replay the night; a "what if" copy; the era dial (1920s ↔ neon future). · N3
- [ ] **W4** Scale and layers: shrink to mouse size; blend two places. · N3
- [ ] **W5** Home screen with "Happening now": start at your home, one tap to a busy hub, a live event or a friend. · N4
- [ ] **W6** Lobbies and matchmaking: a few main hubs (the bar first), instances, matching by age band, preferences and skill; the no-friends lobby. · N4
- [ ] **W7** Your home: invite-only, placing things on a smooth grid, expand up to 3x, levels 1-3. · N4
- [ ] **W8** Events and the memory book: date, time, RSVP, a clean invite card; the in-game camera, a photo book with likes, no comments, per-photo privacy. · N4
- [ ] **W9** Safety in the world: teens and adults kept apart (lobbies, voice, messages); block, mute, report (keeps the last seconds of voice); the teen phrase builder, filtered text for adults. · N4
- [ ] **W10** The mall, IKEA-style: walk in, pick things up, cart, checkout, delivery and install at home. · N4
- [ ] **W11** The casino: 18+, earned credits only, never bought or cashed out. · N4
- [ ] **W12** VIP marketplace (real money, its own place, unlocked forever by a first purchase; numbered, seasonal and event items) and trading (tradeable items only, a server-checked window and cooldown). · N4
- [ ] **W13** Small interactions done properly: bell, guitar, books, TV, door, taps. · N3
- [ ] **W14** Physics props: knock bottles off, glasses break and stack, pour drinks. · N3
- [ ] **W15** The pool table. · N4
- [ ] **W16** Room 3, taped off today (its question is still open). · N3
- [ ] **W17** Chess: bots with a chosen Elo, a menu to find a 1v1 or pick a bot. · N4
- [ ] **W18** Live low-res machine screens and a fair queue (20 s to claim your turn). · N4
- [ ] **W19** More arcade games; bots in two-player races. · N4
- [ ] **W20** Racing: the rigs' visual upgrade on the rebuilt track (waiting on your answer), then the driving rework stages (docs/DRIVING_REWORK.md). · N3
- [ ] **W21** Bodies that move right: stepping when turning on the spot, sit-down and stand-up motions, hands that really hold things, a face that moves when you talk. · N3
- [ ] **W22** Your own character builder (face, details); outfits and gadgets to show off (shop items). · N4
- [ ] **W23** The game screens remade in Ember after R6: HUD, 4K Mania and race screens, emote faces, the loading screen. · N5
- [ ] **W24** Remaster the crude first versions of props and rooms. · N3

### The site

### The engine
- [ ] **L1.8** Light leftovers: Air gets bounce strength and exposure range; the budget meter counts patches (lights in real units shipped in L1.7). · N3
- [ ] **P1** The engine phone check: fix every "broke" from the E1-E9 list; a ?perf=1 report by the fire with the mirror in view (against 63H3FJ, 16.7 ms); if reflections or shadows cost over 1 ms, make them cheaper. · N5
- [ ] **EN1** Bug: the menu stops answering after some games. · N5
- [ ] **EN2** Speed: three phone reports (after entering, at the mirror, after 2 min walking); faster drawing part 2 (render core, layout from Blender, quality settings). · N5
- [ ] **EN3** Clean core leftovers: 4.1c, 4.3b, the darts / bartending / race adapters, entity ids on the wire. · N2
- [ ] **M1** Real surfaces: textures that never visibly repeat, rounded edges, automatic wear, dirt and smudges. · N3
- [ ] **L2** Shadows and shaped light: every light casts shadows, real lamp patterns (IES), lampshades that glow, fire that moves its shadows, contact shade. · N3
- [ ] **T1** The bar script language: parser, a tiny safe runner, errors with line numbers in the Studio; five behaviours rewritten as scripts. · N2 N3
- [ ] **L3** Eyes and camera: smooth edges (TAA), white balance like the eye, seeing in the dark, glare and starbursts, film grain in the dark, focus. · N3
- [ ] **M2** Layers: varnish over wood, velvet and fabric sheen, brushed metal, wet surfaces. · N3
- [ ] **L4** Reflections and caustics: several reflection captures, screen reflections, light bounced off shiny things, caustics. · N3
- [ ] **L5** The air: haze and beams, dust in the light, smoke near the ceiling, heat shimmer. · N3
- [ ] **RC1** Recipes: stool, chair, table, bottle, glass grown from recipes, every copy slightly different, dials in the Studio. · N2 N3
- [ ] **M3** Light through things and real depth: subsurface (wax, skin, lampshades), glass with thickness, parallax depth; every model at real size. · N3
- [ ] **L6** Light show: theatre-style cues and light that is the music, as scripts; real bulb colours; a dying bulb. · N3
- [ ] **L7** Living light: a real sky through the windows (your time and weather), players carry light, neon afterglow. · N3
- [ ] **RC3** Scan it: bring real things in with the iPhone camera. · N3 N5
- [ ] **L8** Paint the light: brush warmth and shadow in the Studio; the engine places and tunes lamps to match. · N3
- [ ] **EN4** CI's browser tests: since the new light (L1) the smoke test times out on GitHub's software GPU, so every test after it is skipped (the deploy itself still goes out); give CI a light-weight drawing mode and get every test running again · N5

### The tools
- [ ] **P2** The Studio phone check: fix every "broke S…"; a ?perf=1 report from the Studio with the Monitor open; the multiplayer test's late joiner (E3.6b) passing here too. · N5
- [ ] **TL1** Engine flags in the Toolbelt: switch features (?light=classic, ?bounce=0 ...) without typing addresses. · N5
- [ ] **T2** The timeline: the Studio's centre strip (bar time, a track per thing, drag events on, loop them, scrub to preview), playing for everyone. · N3
- [ ] **T3** Launch: the check (rules, overlaps, blocked doors), bot players walk the draft, a report, then Launch with one-tap undo. · N3
- [ ] **RC2** Claude in the Studio, live: Claude's edits stream into an open Studio one by one; you comment and press Launch. · N2

### The backbone
- [ ] **B1** Accounts off IONOS: logins move to the ledger's side, stronger password hashes, moved on first login. · N4
- [ ] **B2** The room checks chess moves and 4K Mania scores; then more games pay credits. · N4
- [ ] **B3** The room decides every shared thing (Phase 5). · N4
- [ ] **B4** Voice relays: TURN for strict networks, then a voice server (SFU) for big lobbies. · N4
- [ ] **B5** Payments (Stripe) for the VIP store. · N4
- [ ] **B6** Safety on the server: age bands enforced, reports kept, blocks remembered. · N4

## 6. Parked
*Not on the road yet, by chapter. The user promotes them.*

### The world
- The street and the city outside; homes there as your "real home".
- A kids edition: a separate, simpler game in the same universe (under 13).
- A new name for the world and its currency (Bar3D is a working name).
- Mini music studio (a DAW in the bar).
- Avatar from a face scan (the user's own character builder).
- The street front: the bar seen from the street at night (front wall, lit door, people moving behind the windows, a street lamp), a new place in the world (was R5.5)

### The site
- The front door scene running live in Looped instead of a clip (after R6, same places).
- The front page's looping clip: the camera drifting to the bar's lit door, ~8 s, rendered from the engine, behind the intro (was R5.6)

### The engine
- World-simulation engines: sound made from the physics, heat, smell as a field, day and night.

### The tools
- AI dock: a workspace on the site where you talk to Claude and it works on the project there.
- AI inside the Studio (needs a paid API), importing your own models, mesh editing, several people editing at once.
- A design for the Studio more like a 3D modelling room (the hub card "Design idea").

### The backbone
- The site on Cloudflare Pages (unlocks threads in the game).

## 7. Done
*Newest first: build code, the pieces, the commits. "undo" reverts one line.*
- (this push) R5.1, R5.2, R5.3, R5.4: J Production's front page: the site in Ember, highlights, the front page rearranged, Projects → 29cf0ac^..c2ffbfe
- `ad87523` R4.1, R4.2, R4.3, R4.4, R4.5, R4.6, R4.7: Dev, the control room: Overview, Projects, Plan, Publish, People; the old hub, portal and Toolbelt lead there → 03c3fe4^..f247b0b
- `58d93ec` R3.1, R3.2, R3.3: The World Plan: one doc (PLAN.md), the old plans archived, the plan keeper → 882582d^..a75881c
- `2eabe5f` R2.5-R2.6: Ember, the house theme (Warm + frosted glass + a drifting glow + moving titles); motion tokens → 393d12e^..b4f2fea
- `45292a6` R2.1-R2.4: real screens, light and dark in one theme, six candidates (Clean, Warm, Bold, Glass, Neon, Soft), compare and mix → 0bdee82^..8b9d289
- `aa0d3ae` R1.1-R1.7: the Toolbelt and the UI Editor (theme and screen files, the web renderer, theme and screen editing, publish with every version kept, the engine reads themes); the plan rework (docs/UI_PLAN.md), engine work paused → a28d85c^..b394ba3
- `fde3f14` M0.2 truly matte (no plastic rim on rough things, cloth keeps little shine, coats not black, floor 0.75) → M0.2 commit
- `8722f75` Phone feedback: L1.8 no squares of light, M0 real roughness (matte coats, floor not a mirror), L1.9 softer highlights (bright light keeps its colour), L1.10 calmer light (darker, less glow); tools: place_check, stage_hunks → 5622548..HEAD
- `336e1d9` L1.7: the new light in the Studio (lumens, Kelvin, shape live; moods in real units) → 73f2d72
- `7322b92` L1.1-L1.6: eyes (HDR, adapting, AgX), real units (lumens, Kelvin), real material brightness, bounced light (core + in the game), lights with size; tools: piece runner, gallery, benchmarks, light tool → 69fb05c^..5d99353
- `96fbf26` S9.1-S9.3, S10.1-S10.4: live for everyone (a publish reaches an open bar within seconds, try your draft with a friend, undo a publish) and Claude in the loop (notes from the Studio, Claude's versions in History with what changed, deep links, proposals) → c00af5e..HEAD
- `c00af5e` S7.1-S7.3, S8.1-S8.4: light and look (materials as data, live lights, moods and the budget meter) and life (states and rules, the Loop Rack, drawn keys and presets, sounds) → 2c8b4da..c00af5e
- `2c8b4da` S5.1-S5.4, S6.1-S6.4: precision (magnifier, part mode, this one / every one, Inspect edits everything) and building (Library with pictures and prices, duplicate / row / rename / delete, boxes with push-pull and extrude, places) → f3a2dc4..2c8b4da
- `f3a2dc4` S4.1-S4.4: the hub as Needs you · Dials · Board · News, tuning baked into the world, the bar's menus, news and loading screen in Faceplate → 8797ac4..f3a2dc4 (with the room's grace fix and S5.1)
- `8797ac4` S2.1-S2.5, S3.1-S3.5: the Studio opens (login, cameras, picking, the Monitor, X-ray) and edits (Move/Turn/Size, slide, Drop, Put on, undo, the draft, Publish, History, Restore) → 5a04c79..8797ac4
- `5a04c79` S1.1-S1.6: the theme system (/ui), the front door, the schema, the world store, the bar and the room load the live world, studio.mjs → 988d71f..5a04c79 (world v1 published after)
- `22e9b28` E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..22e9b28 (66526ef was the same game; 22e9b28 only fixed a test)
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).

## 8. Where things are
*Docs that explain; they never hold plans or ticks.*
- `CLAUDE.md`: how a "go" runs. `docs/REFERENCE.md`: the long guide (layout, every tool, iOS limits).
- `docs/AUTOMATION.md`: the tools (piece, step, plan, uikit, studio, bench, gallery).
- `docs/ENGINE.md`: how Looped is built (things, loops, layers). `docs/STUDIO.md`: how the Studio works.
- `docs/manual/`: the manual (light, materials, recipes, timeline, world, avatars).
- `docs/PROTOCOL.md`: the multiplayer wire format. `docs/DRIVING_REWORK.md`: the driving spec.
- `docs/DECISIONS.md`: gotchas and why things are the way they are.
- `docs/archive/`: every old plan and progress file, kept to read, never to plan from.
