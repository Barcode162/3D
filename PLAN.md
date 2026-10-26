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
- (2026-10-26) **The user tests:** Claude runs no tests before saving or shipping work; the user tests everything on the phone ("we need to be able to work without waiting for tests from you I can test everything"; "no more test … let me test it instead"). One exception, on the user's word (request 2026-10-26 #9, "Yes, run it"): the seconds-long world check (tools/native/bar_world_test.cpp) runs whenever a piece changes the world's files (assets/world/). GitHub's long browser tests run only when the user asks ("Only when I ask", request 2026-10-25 #15); the build's quick checks still run with every deploy.
- (2026-10-25) **Research, then build:** before any of a job's code changes, Claude researches what the job touches and writes the findings into the job's plan; the user sees them first ("We need to research to see what we are doing before we continue … 1 mistake is worse than making sure you are doing the right thing", request 2026-10-25 #12).
- (2026-10-24) **Only what the user wants gets pushed:** talking, questions, checks and try-outs are never committed or pushed to GitHub; only changes the user asked to add or edit are, through a job.
- (2026-10-24) **Never assume the user knows anything:** the user only says what they want, in any words; Claude plans, organises, builds, checks, ships and documents everything else, and explains in plain words.
- (2026-10-24) **Zero errors per session:** any error, however small, gets its root cause found and recorded, and the procedure, tool or check that let it through is changed so it can't happen the same way again.
- (2026-10-24) **Nothing outside the guidelines:** every request is checked against what's possible and allowed before any work; anything the Charter doesn't allow, or doesn't mention, doesn't happen without asking the user first.
- (2026-10-24) **The restructure (J1–J8) governs until it's done:** its spec is `docs/archive/restructure.md`; where an older instruction disagrees, the spec wins. It replaces "one doc, eight fixed parts" with five books: Charter, Playbook, Handbook, Plan, Records.
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
- (2026-10-22) **Light only comes from light sources**: no fake fill; darkness is real; eyes adjust.
- (2026-10-22) **Automation first**: anything done twice becomes a tool.
- (2026-10-22) **Wow and foundation take turns**; every visual piece has an off switch and before/after pictures.
- (2026-10-21) **"go" = one whole step**: one piece = one commit with its own phone check ("broke <piece>" finds it); one push to main per step.
- (2026-10-19) **13+ only**; teens (13-17) and adults kept apart. The casino is 18+ and earned credits only.
- (2026-10-15) **From scratch in C++**: no outside engines or libraries (three.js only for Meridian and the platformer). Every third-party asset licensed and credited; no copyrighted music.

## 3. The chapters
*The fixed list. Every road and parked item sits under one of these.*
- **The world:** what players see and do: the bar, avatars, games, people. *Now:* the bar is built and alive (three rooms, darts, chess, piano, 4K Mania, racing, bartending, jukebox, voice, profiles); the new light is live; the freeze lifted (J8.3), now remade item by item: the world remake J35, J70-J77 (every thing its own model, placed, lit and heard).
- **The site:** the front door, the showcase, the news. *Now:* J Production's front page (Home, Projects, News) in Ember, with highlights pushed from Dev (R5); open again since J8.3.
- **The engine:** Looped, the script language, speed. *Now:* Looped: E1-E9 built (things with live loops, physics, sound, net), real light L1.1-L1.7 live, the tests' own lighting (J2.4); the script language is scoped (J43, J78-J81: small, safe, errors with a line and a fix hint); open again since J8.3.
- **The tools:** the Toolbelt and Dev. *Now:* Dev (R4), the Studio, Dials, the UI Editor, Ember everywhere (R6); the five books and their checks are done (J1-J8); next: Dev remade (J90) and the Studio's Loop Maker (J57).
- **The backbone:** server, accounts, money. *Now:* accounts with ages, the ledger (credits), the multiplayer room, voice; accounts still on IONOS; both deploys report their real state to Dev (J2).

## 4. Now
**J110 · Smoother graphics** (the engine). Smoother graphics: the GPU check and reports measure late frames, lighter lighting with the same picture, Ultra and Auto ease sharpness only while frames run late (request 2026-10-26 #14) · N2 N3
- [x] **J110.1** The GPU check and reports measure what you feel: each step's late frames and slowest 5%, beside the wait *Check:* Settings → Help Claude fix lag → Check the GPU, then send a report
- [x] **J110.2** Lighter lighting, the same picture: bounced light keeps fewer numbers per pixel; lamps too faint to see here skipped, shadow looks included *Check:* The bar looks the same; fewer stutters
- [x] **J110.3** Ultra and Auto keep it smooth: they step by late frames; Ultra eases sharpness 3x to 2.5x to 2x only while frames run late, back to 3x when steady; Auto climbs back up *Check:* On Ultra walk around: smooth

## 5. The road
*Next and later, by chapter. Every item: its job number, what it is, the North star line it serves, and its verdict (docs/playbook/intake.md step 3), judged as if unfrozen: nothing outside the freeze's list starts before J8. Old tags stay valid as "Formerly".*
**Up next, in order:** J109 → J28 → J106 → J103 → J104 → J105 → J75 → J76 → J77 → J85 → J57 → J65 → J20 → J68 → J37 → J39 → J43 → J78 → J79 → J80 → J81 → J24 → J25 → J13 → J9 (the user's notes of 2026-10-26 #13 first: quality levels and speed, the piano, the books, chess bots; then the shells and the new rooms)

### The world
- [ ] **J9** Alive avatars: breathing, blinking, looking at whoever talks, shifting weight, feet planted on stairs; one body with sliders. · N3
  *Formerly* A1 · *Verdict:* Doable now — Procedural motion on the skeleton the engine already has; Claude writes it, your phone judges how alive it feels
- [ ] **J10** Real people: skin light passes into, hair strands, wet see-through eyes, clothes that swing, lips that follow voice. · N3
  *Formerly* A2 · *Verdict:* Doable at a cost — Phone speed: skin light, hair strands and swinging cloth are among the most expensive things to draw on a phone; each ships only if the frame budget allows (J37's report)
- [ ] **J11** Your face: an avatar face from a few selfies. · N3 N4
  *Formerly* A3 · *Verdict:* Needs an experiment — Building a face from photos with no outside libraries (rule: from scratch) is research; a time-boxed try shows whether it looks like you
- [ ] **J12** Memory: the floor wears where people walk, the dartboard gets holes, your usual stool gets your nameplate. · N3
  *Formerly* W1 · *Verdict:* Doable after J62 — Wear and holes that everyone sees and that last need the room to decide and keep shared things
- [ ] **J13** Mood and chance: one cozy↔wild dial for the whole bar; "maybe" events (30% rain, a surprise band). · N3
  *Formerly* W2 · *Verdict:* Doable now — The world model's states, rules and loops already cover moods and chance; a dial and events are data plus a little engine
- [ ] **J14** Time and era: scrub time back and see ghosts replay the night; a "what if" copy; the era dial (1920s ↔ neon future). · N3
  *Formerly* W3 · *Verdict:* Doable after J62 — Replaying the night needs the room to record every shared move; the room's storage is limited, so a replay keeps one night
- [ ] **J15** Scale and layers: shrink to mouse size; blend two places. · N3
  *Formerly* W4 · *Verdict:* Needs an experiment — Mouse size strains the engine's precision, light and physics; a try shows whether it holds on the phone
- [ ] **J16** Home screen with "Happening now": start at your home, one tap to a busy hub, a live event or a friend. · N4
  *Formerly* W5 · *Verdict:* Doable after J18, J19 — It shows homes and events, so those come first
- [ ] **J17** Lobbies and matchmaking: a few main hubs (the bar first), instances, matching by age band, preferences and skill; the no-friends lobby. · N4
  *Formerly* W6 · *Verdict:* Doable after J65 — Matching by age band needs ages enforced on the server (Charter: teens and adults apart)
- [ ] **J18** Your home: invite-only, placing things on a smooth grid, expand up to 3x, levels 1-3. · N4
  *Formerly* W7 · *Verdict:* Doable after J62 — Homes others visit need the room to keep their state; invite-only rooms already exist (the room "looped")
- [ ] **J19** Events and the memory book: date, time, RSVP, a clean invite card; the in-game camera, a photo book with likes, no comments, per-photo privacy. · N4
  *Formerly* W8 · *Verdict:* Doable after J20, J65 — Shared photos of teens need the safety tools and server-side age bands first (Charter: law and safety)
- [ ] **J20** Safety in the world: teens and adults kept apart (lobbies, voice, messages); block, mute, report (keeps the last seconds of voice); the teen phrase builder, filtered text for adults. · N4
  *Formerly* W9 · *Verdict:* Doable after J65 — Age bands and reports enforced on the server first; keeping seconds of voice for reports needs a privacy notice: your call
- [ ] **J21** The mall, IKEA-style: walk in, pick things up, cart, checkout, delivery and install at home. · N4
  *Formerly* W10 · *Verdict:* Doable after J18 — Delivery and install go to your home; buying uses earned credits through the ledger, which exists
- [ ] **J22** The casino: 18+, earned credits only, never bought or cashed out. · N4
  *Formerly* W11 · *Verdict:* Doable after J20, J65 — 18+ enforced on the server and the safety tools in place first (Charter: the 18+ casino, earned credits only)
- [ ] **J24** Small interactions done properly: bell, guitar, books, TV, door, taps. · N3
  *Formerly* W13 · *Verdict:* Doable after J70, J71 — Books, the TV and the taps become their own things in the world remake
- [ ] **J25** Physics props: knock bottles off, glasses break and stack, pour drinks. · N3
  *Formerly* W14 · *Verdict:* Doable after J72 — Bottles and glasses become real things first; the physics exists
- [ ] **J26** The pool table. · N4
  *Formerly* W15 · *Verdict:* Doable now — A new thing with physics (balls, a cue) the engine already has; its model grown by script
- [ ] **J27** Room 3, taped off today (its question is still open). · N3
  *Formerly* W16 · *Verdict:* Doable now — Waits only on your answer to its open question: what room 3 becomes (a music studio, a quiet library lounge, or something else)
- [ ] **J28** Chess: bots with a chosen Elo, a menu to find a 1v1 or pick a bot. · N4
  *Formerly* W17 · *Verdict:* Doable now — A chess engine with a chosen strength, from scratch in C++, is well within reach; finding a 1v1 uses the room
- [ ] **J29** Live low-res machine screens and a fair queue (20 s to claim your turn). · N4
  *Formerly* W18 · *Verdict:* Doable at a cost — Phone speed: every live screen draws a second picture each frame; the fair queue itself is room logic and cheap
- [ ] **J30** More arcade games; bots in two-player races. · N4
  *Formerly* W19 · *Verdict:* Doable now — Our own game code, like the arcade today; race bots reuse the race's AI
- [ ] **J31** Racing, your plan (docs/plan/jobs/W20.md): the driving rework's stages on our own made-up makes (Vance Ridgeback GS, Kaito Sora 24, Hoshi R13 Twin, Hoshi Kaze 2.0: shapes close to the real cars, real specs, no real names or badges; your decision 2026-10-19); the rigs' screens show the driver's race so anyone in the bar can watch; and the race's visual upgrade (a real track instead of the glitchy terrain, light through the leaves, soft sun reflections, water) done together with the track rebuild in stage 14 (your answer: combine them so nothing is done twice). · N3
  *Formerly* W20 · *Verdict:* Doable now — Your plan is complete and fits the limits: made-up makes keep it legal (a lawyer checks before anything is sold), the physics is ours; the light through leaves and water cost phone speed, so they ship within the frame budget (J37's report)
- [ ] **J32** Bodies that move right: stepping when turning on the spot, sit-down and stand-up motions, hands that really hold things, a face that moves when you talk. · N3
  *Formerly* W21 · *Verdict:* Doable after J9 — Builds on the alive avatars' motion
- [ ] **J33** Your own character builder (face, details); outfits and gadgets to show off (shop items). · N4
  *Formerly* W22 · *Verdict:* Doable after J9 — The body with sliders first; outfits are sold for earned credits through the ledger
- [ ] **J34** The game screens remade in Ember after R6: HUD, 4K Mania and race screens, emote faces, the loading screen. · N5
  *Formerly* W23 · *Verdict:* Doable now — Plain HTML over the 3D in the house theme (rule: the menus stay HTML), no cost on the phone
- [ ] **J75** The world remake, step 7, placement: every thing sits on something real (a floor, a shelf, a table, a wall), snaps there in the Studio, and the place check refuses things floating, sunk or overlapping. · N3
  *Verdict:* Doable after J70, J71, J72, J73, J74 — The place check (tools/place_check.py) and the inventory's Sits on column exist; this makes them strict
- [ ] **J76** The world remake, step 8, light with the things: every lamp, bulb, sconce, neon sign and the fire is a thing that carries its own light at its real spot and bulb colour, each room balanced again (no light without a source). · N3
  *Verdict:* Doable after J75 — The light rule (light only from light sources) and the light engine exist; the lights move from helper spots onto their things
- [ ] **J77** The world remake, step 9, sound with the things: every thing that makes a sound carries it (the fire crackles from the fireplace, music from the speakers, glasses clink where they are), from its material's sound. · N3
  *Verdict:* Doable after J75 — The sound engine and the materials' sounds exist; sources move onto their things; your phone judges how it sounds
- [ ] **J103** The cozy lounge grows: a bigger fireside room that breaks normal architecture (expanded beyond the old wall, its own nooks and levels) (request 2026-10-26 #8) · N3
  *Verdict:* Doable after J74 — It changes the room shell, so it comes with or after the shells' remake
- [ ] **J104** A sunken seating nook by the big mirror: a few steps down, seats, a pair of speakers, its own chill music with its own player to pick and control tracks (request 2026-10-26 #8) · N3 N4
  *Verdict:* Doable after J74 — A change to the floor and walls (with the shells); music only with a licence that allows it, like the jukebox's CC-BY tracks
- [ ] **J105** A holographic race broadcast in a wall recess above the sim rigs: the whole track as a spinning hologram, one camera on both drivers that splits smoothly into one per driver when they separate (one camera in solo play), the placing at the top, a minimap of the track with both cars in two colours at the bottom (request 2026-10-26 #8) · N3 N4
  *Verdict:* Doable after J73 — J73
- [ ] **J106** The world remake, step 6, the room shells remade: the hall, the lounge's and the arcade's walls, floors and ceilings at real size with clean edges and real materials; doors and windows become things that open · N3
  *Verdict:* Doable now — The props are out (J70-J73), so the shells are only walls, floors and ceilings; formerly J74's job, moved for the user's notes (2026-10-26 #12)
- [ ] **J109** Books with turning pages: swipe from the left or right edge and the page curls over, its text bending with it (request 2026-10-26 #13) · N3
  *Verdict:* Doable now — The reader (J99) lays out pages; a page curl is drawn by the page itself (CSS 3D or canvas)

### The site

### The engine
- [ ] **J36** Light leftovers: Air gets bounce strength and exposure range; the budget meter counts patches (lights in real units shipped in L1.7). · N3
  *Formerly* L1.8 · *Verdict:* Doable now — Small additions to the light that's live
- [ ] **J37** The engine phone check: fix every "broke" from the E1-E9 list; a ?perf=1 report by the fire with the mirror in view (against 63H3FJ, 16.7 ms); if reflections or shadows cost over 1 ms, make them cheaper. · N5
  *Formerly* P1 · *Verdict:* Doable now — Needs your phone: a ?perf=1 report by the fire; Claude can't measure the phone (Charter: Claude)
- [ ] **J38** Bug: the menu stops answering after some games. · N5
  *Formerly* EN1 · *Verdict:* Needs an experiment — GitHub's "the menu still answers after every game" passed in run 214; a try on your phone shows whether it still happens
- [ ] **J39** Speed: three phone reports (after entering, at the mirror, after 2 min walking); faster drawing part 2 (render core, layout from Blender, quality settings). · N5
  *Formerly* EN2 · *Verdict:* Doable now — Three ?perf=1 reports from your phone, then the drawing work they point to
- [ ] **J40** Clean core leftovers: 4.1c, 4.3b, the darts / bartending / race adapters, entity ids on the wire. · N2
  *Formerly* EN3 · *Verdict:* Doable now — Clean-up inside our own engine code
- [ ] **J41** Real surfaces: textures that never visibly repeat, rounded edges, automatic wear, dirt and smudges. · N3
  *Formerly* M1 · *Verdict:* Doable at a cost — Phone speed and memory: textures that never repeat, and wear, cost texture memory in a browser tab; built within the budget
- [ ] **J42** Shadows and shaped light: every light casts shadows, real lamp patterns (IES), lampshades that glow, fire that moves its shadows, contact shade. · N3
  *Formerly* L2 · *Verdict:* Doable at a cost — Phone speed: a shadow from every light costs a lot on a phone; done light by light against the frame budget (J37's report)
- [ ] **J43** The script language, step 1, its design on paper: our own small, safe language for what things do (states, events, timers, loops, remembered values, messages between things), readable by you and by AIs, every error with its line and a fix hint; the grammar on one page, 20 scripts for things that exist today, every error's words; nothing built. Its scope and jobs: docs/plan/jobs/J43.md. · N2 N3
  *Formerly* T1 · *Verdict:* Doable now — A small language inside our own engine, from scratch in C++ (rule: no outside engines); the scope is kept small on purpose, and this step is paper only
- [ ] **J44** Eyes and camera: smooth edges (TAA), white balance like the eye, seeing in the dark, film grain in the dark, focus (no glare or starbursts: only real light, 2026-10-25 #10). · N3
  *Formerly* L3 · *Verdict:* Doable at a cost — Phone speed: smooth edges, glare and grain are extra full-screen passes; each gets an off switch
- [ ] **J45** Layers: varnish over wood, velvet and fabric sheen, brushed metal, wet surfaces. · N3
  *Formerly* M2 · *Verdict:* Doable after J41 — Layers sit on top of the real surfaces
- [ ] **J46** Reflections and caustics: several reflection captures, screen reflections, light bounced off shiny things, caustics. · N3
  *Formerly* L4 · *Verdict:* Doable at a cost — Phone speed and memory: more reflection captures and screen reflections cost frame time
- [ ] **J47** The air, the rest: dust in the light, smoke near the ceiling, heat shimmer (the haze and beams come in J91). · N3
  *Formerly* L5 · *Verdict:* Doable at a cost — Phone speed: haze and beams through the air are expensive; a cheap version first
- [ ] **J48** Recipes: stool, chair, table, bottle, glass grown from recipes, every copy slightly different, dials in the Studio. · N2 N3
  *Formerly* RC1 · *Verdict:* Doable now — Models grown from recipes by script is how Claude makes 3D (Charter: Claude); the Studio's dials show them
- [ ] **J49** Light through things and real depth: subsurface (wax, skin, lampshades), glass with thickness, parallax depth; every model at real size. · N3
  *Formerly* M3 · *Verdict:* Doable at a cost — Phone speed: light through wax and skin, and thick glass, are expensive; every model at real size costs nothing
- [ ] **J50** Light show: theatre-style cues and light that is the music, as scripts; real bulb colours; a dying bulb. · N3
  *Formerly* L6 · *Verdict:* Doable after J81 — Cues and light that follows the music are written as scripts
- [ ] **J51** Living light: a real sky through the windows (your time and weather), players carry light, neon afterglow. · N3
  *Formerly* L7 · *Verdict:* Doable at a cost — Your local time is free; real weather needs an outside weather service (Charter: ask first), and a sky through the windows costs phone speed
- [ ] **J52** Scan it: bring real things in with the iPhone camera. · N3 N5
  *Formerly* RC3 · *Verdict:* Needs an experiment — Safari gives the camera but not the depth sensor; a model from photos, from scratch, is research: a time-boxed try
- [ ] **J53** Paint the light: brush warmth and shadow in the Studio; the engine places and tunes lamps to match. · N3
  *Formerly* L8 · *Verdict:* Needs an experiment — Working back from painted warmth to real lamps is research; a try in the Studio shows whether it's usable
- [ ] **J78** The script language, step 2, the parser and checker (C++, looped/script/): every error with its line, column, plain words and a fix hint; native tests on the 20 examples and on broken copies of them. · N2
  *Verdict:* Doable after J43 — Needs the grammar settled first; plain C++ with native tests, no graphics card needed
- [ ] **J79** The script language, step 3, the tiny safe runner: events, states, timers, loops, remembered values, messages; a step budget per frame and a memory limit, a script touching only its own thing; a kind's states, rules and loops turned into a script and back without losing anything. · N2 N3
  *Verdict:* Doable after J78 — Runs inside the world model each frame; the budget keeps the phone smooth, and scripts sleep until their events
- [ ] **J80** The script language, step 4, scripts in the Studio: a script for a kind, errors underlined with their hint as you type, the change live on the phone. · N2 N5
  *Verdict:* Doable after J79 — The Studio's live editing exists; your phone judges how it is to write
- [ ] **J81** The script language, step 5, five behaviours rewritten as scripts (the light switch, the mirror, the polished things' knock, the candle, the jukebox), behaving exactly as before, the tests proving it. · N2 N3
  *Verdict:* Doable after J80 — Proves the language on real things without changing what you see
- [ ] **J85** The Loop Maker, step 1, the engine: a library of loops (patterns: sway, breathe, pulse…) used by name anywhere, and a place's timeline like a DAW's arrangement (tracks over things, clips of patterns at a beat, faded and layered on what each thing does), on the shared clock and the music's beat, the same on every phone and in the room; the showroom's stage plays a 16-beat show. Its plan: docs/plan/jobs/J85.md. · N2 N3
  *Verdict:* Doable now — Our own engine code: patterns and timelines on the shared clock the world model already has

### The tools
- [ ] **J55** The Studio phone check: fix every "broke S…"; a ?perf=1 report from the Studio with the Monitor open; the multiplayer test's late joiner (E3.6b) passing here too. · N5
  *Formerly* P2 · *Verdict:* Doable now — Needs your phone: every "broke S…" you report, and a ?perf=1 report with the Monitor open
- [ ] **J56** Engine flags in the Toolbelt: switch features (?light=classic, ?bounce=0 ...) without typing addresses. · N5
  *Formerly* TL1 · *Verdict:* Doable now — A list of switches in the Toolbelt; no engine change
- [ ] **J57** The Loop Maker, step 2, in the Studio, like a DAW or a video editor: the timeline strip (bars and beats, playhead, play, pause, scrub, loop region, zoom), a track per thing or kind, patterns dragged on as clips (moved, stretched, faded, copied), a clip opens its pattern in the Loop Maker (lanes per channel, keys drawn with a finger, presets, saved to the library); every edit plays live and publishes for everyone. Its plan: docs/plan/jobs/J57.md. · N2 N3
  *Formerly* T2 · *Verdict:* Doable after J85 — The engine plays patterns and timelines first (J85); clips need no script language, only events on the timeline wait for J79
- [ ] **J58** Launch: the check (rules, overlaps, blocked doors), bot players walk the draft, a report, then Launch with one-tap undo. · N3
  *Formerly* T3 · *Verdict:* Doable after J57 — Launch checks the timeline's draft; bot players walk it headless here (slow without a graphics card, fine for a check)
- [ ] **J59** Claude in the Studio, live: Claude's edits stream into an open Studio one by one; you comment and press Launch. · N2
  *Formerly* RC2 · *Verdict:* Doable now — Claude already writes to the Studio's store with its key; the open Studio shows each edit as it lands
- [ ] **J67** The sign-up and profile browser test rewritten for the room server (tools/account_test.mjs, KI-27). · N4
  *Verdict:* Doable now — Our own test; sign-up and profiles have no browser test on GitHub today
- [ ] **J68** The hub's unused loop tuning (?tuning, tune) and hub/world.html's unused api() taken out (KI-29). · N2
  *Verdict:* Doable now — Nothing reads them since S4.2; the hub test covers what stays

### The backbone
- [ ] **J61** The room checks chess moves and 4K Mania scores; then more games pay credits. · N4
  *Formerly* B2 · *Verdict:* Doable now — Touches credits, so it starts only on your yes (Charter §5); the room already runs the games' shared state
- [ ] **J62** The room decides every shared thing (Phase 5). · N4
  *Formerly* B3 · *Verdict:* Doable now — The room (Cloudflare Durable Objects) already holds shared state; moving every shared thing there is big but within its limits
- [ ] **J65** Safety on the server: age bands enforced, reports kept, blocks remembered. · N4
  *Formerly* B6 · *Verdict:* Doable now — Touches accounts and ages, so it starts only on your yes (Charter §5); the room and the ledger can enforce it

## 6. Parked
*Not on the road yet, by chapter. The user promotes them.*

### The world
- The street and the city outside; homes there as your "real home".
  *Verdict:* Doable after J77 — The world remake first; a city is many places, a big time cost after that
- A kids edition: a separate, simpler game in the same universe (under 13).
  *Verdict:* Against a rule — The 13+ rule (2026-10-19): the platform is 13+ only, and an under-13 game brings children's privacy laws
- A new name for the world and its currency (Bar3D is a working name).
  *Verdict:* Doable now — Claude can propose names and check they're free to use; the choice is yours
- Mini music studio (a DAW in the bar).
  *Verdict:* Doable at a cost — A big time cost: a music studio in the browser, from scratch; the sound engine exists, and room 3 could hold it (J27)
- Avatar from a face scan (the user's own character builder).
  *Verdict:* Needs an experiment — The same research as J11 (a face from photos)
- The street front: the bar seen from the street at night (front wall, lit door, people moving behind the windows, a street lamp), a new place in the world (was R5.5)
  *Verdict:* Doable after J77 — A new place, after the world remake
- J23 · VIP marketplace (real money, its own place, unlocked forever by a first purchase; numbered, seasonal and event items) and trading (request 2026-10-24 #7: no paid services for now)
  *Verdict:* Doable after J64 — Needs payments (J64, parked)

### The site
- The front door scene running live in Looped instead of a clip (after R6, same places).
  *Verdict:* Doable at a cost — Phone speed and loading: the front page would load the whole engine before showing anything; the clip below is the light way
- The front page's looping clip: the camera drifting to the bar's lit door, ~8 s, rendered from the engine, behind the intro (was R5.6)
  *Verdict:* Needs an experiment — Drawing ~8 s of the bar here without a graphics card is very slow in the new light; a try shows whether a session can do it

### The engine
- World-simulation engines: sound made from the physics, heat, smell as a field, day and night.
  *Verdict:* Needs an experiment — Sound from the physics, heat, and day and night are within reach; smell can only show through what you see and hear (a phone makes no smells)
- The engine checks itself: it notices when something isn't right (a thing out of place, a missing model, a broken loop) and helps say what and where (request 2026-10-24 #5)
  *Verdict:* Doable now — Checks on the world's data and in the engine (a thing out of place, a missing model, a broken loop); the Handbook's inventory already flags some
- Smooth edges (4x MSAA, or a cheaper smoothing pass): out of J88 on the user's word, for later when we optimize; MSAA smooths jagged edges but costs phone speed and ~75 MB (request 2026-10-25 #13; research in docs/plan/jobs/J88.md)
  *Verdict:* Doable at a cost — Phone speed and memory: Filament rates 4x MSAA high cost on mobile
- Lamps sorted in 3D cells: left out after research (no gain in the bar: every lamp reaches every cell; the per-pixel cap shortens lamps, not the cells; docs/plan/jobs/J88.md)
- Bounced light at its own pace (J88.3, undone by J96): redo once the cause of its square of light is found (incident 2026-10-25-square-of-light)
  *Verdict:* Needs an experiment — Why pacing the bounce makes a square isn't known yet

### The tools
- AI dock: a workspace on the site where you talk to Claude and it works on the project there.
  *Verdict:* Doable at a cost — Talking to Claude from the site needs a paid API key kept on a server (Charter: money, secrets): your decision
- AI inside the Studio (needs a paid API), importing your own models, mesh editing, several people editing at once.
  *Verdict:* Doable at a cost — The AI part needs a paid API (your decision); each imported model needs a licence; mesh editing and editing together are big time costs
- A design for the Studio more like a 3D modelling room (the hub card "Design idea").
  *Verdict:* Doable now — A new look for the Studio's frame in Ember; your phone judges it
- J69 · The tests on a GitHub machine with a graphics card, so the new light is tested too (KI-28; request 2026-10-24 #7: no paid services for now)
  *Verdict:* Doable at a cost — GitHub's machines with a graphics card are paid; the shader check, the pictures here and your phone cover the new light meanwhile
- Claude gets better from how the user works with it: what worked, what the user liked or corrected, kept as lessons in the books and read at every session start, so the engine, the Studio and modelling get easier each time (2026-10-25 #2)
  *Verdict:* Doable now — Claude can't be retrained, but the books can learn: lessons written from each session and read at the next (the Playbook and the records already do part of this)
- Claude gets better from how the user works with it: what worked, what the user liked or corrected, kept as lessons in the books and read at every session start, so the engine, the Studio and modelling get easier each time (2026-10-25 #2)
  *Verdict:* Doable now — Claude can't be retrained, but the books can learn: lessons written from each session and read at the next (the Playbook and the records already do part of this)

### The backbone
- The site on Cloudflare Pages (unlocks threads in the game).
  *Verdict:* Doable at a cost — A hosting change is your decision (Charter §5: hosting); Cloudflare Pages is free and can send the headers threads need, IONOS can't
- J63 · Voice relays: TURN for strict networks, then a voice server (SFU) for big lobbies (request 2026-10-24 #7: no paid services for now)
  *Verdict:* Doable at a cost — Relays cost money per use, and there's no budget for paid services now; voice works without them on most networks
- J64 · Payments (Stripe) for the VIP store (request 2026-10-24 #7: no paid services for now)
  *Verdict:* Doable at a cost — A payment provider, tax handling and terms; a provider like Stripe takes a cut of each sale rather than a monthly fee, so it can come back once there is something to sell
- J89 (accounts hardened) is built inside J60: the new logins on Cloudflare get its expiry, guessing limit and one-at-a-time saves (request 2026-10-25 #7)
  *Verdict:* Doable now — Folded into J60
- J60 · Everything off IONOS (accounts, every store, the site and the bar's files to Cloudflare, then the address switch): put off, IONOS works for now (request 2026-10-25 #9); its plan stays in docs/plan/jobs/J60.md
  *Verdict:* Doable at a cost — On the free plan the password hashes are as strong as 10 ms per login allows; the switch needs your two changes

## 7. Done
*Newest first: build code, the pieces, the commits. "undo" reverts one line.*
- `f0074b3` J110.1, J110.2, J110.3: Smoother graphics: reports measure late frames, lighter lighting with the same picture, Ultra and Auto ease sharpness only while frames run late → fb7283c^..ee94e60
- `5d680c1` J108.1, J108.2, J108.3, J108.3 (fix): The piano redone: its glow and sound back at the piano, a bigger keyboard with loudness by touch, and sparks rising from the strings → 61c77a5^..87a2348
- `cf58fb5` J107.1, J107.2: J107: quality in four levels (Ultra, High, Medium, Low) that set every feature together, and a 20-second GPU check for the phone report → 71d8b24^..be3b0ed
- `b188a36` J74.1: J74: your notes from the phone: less haze, a softer highlight, the string lights, mirror bulbs and spotlights give their own light, and a crosshair with its editor in Settings → Pointer → b04cdd4^..b04cdd4
- `a489b01` J73.1, J73.2: J73: the stage, the darts corner, the taped-off room and the arcade taken apart into their own things; the DARTS sign reads the right way; J72's bottles fixed so they deploy → dd40f9c^..5c841ab
- `981a2a1` J72.1, J72.2: J72: every bottle on the back bar (275) and every glass on the tables its own thing; the wine rack shows its bottles → f618a04^..9cb13e3
- `080c01f` J100.1, J100.2: J100: a real piano: the recorded Salamander grand (credited), loaded when someone plays → b24a760^..a70619d
- `6d4f1af` J101.1: J101: chess full screen like the chess sites, in six calm themes with matching pieces and backgrounds → 64e0cd7^..64e0cd7
- `ae02b1c` J99.1: J99: the lounge bookshelf: real books to read (Dostoyevsky, Einstein, Melville, Fitzgerald) in a reader with pages to turn → f13aaed^..f13aaed
- `4887cd3` J102.1: J102: a real fire in the fireplace: flames the shader draws (tongues of different heights, red to yellow), not moving pictures → 83fb722^..83fb722
- `e4a1905` J71.1, J71.2: J71: the lounge fix (read a book again, no World problems) and the walls' props taken apart: every sign, photo, plant, speaker, sconce and crate its own thing; the clock over the bar is round → f971219^..c76ce13
- `0d40f55` J70.1, J70.2: J70: the lounge taken apart: every object by the fire its own thing (Poly Haven sofa, armchairs, mantel clock, painting, vase); floating things on the mantel and the lamp shade fixed → 49bb635^..3a43f37
- `35e68bb` J35.1, J35.2, J35.3, J35.4, J35.5: J35: the remake's first corner: real wooden bar stools, the counter rebuilt (round end, unbroken rail, clean steel), bottles of real glass with drinks and sharp labels, industrial hanging lamps; the kit for Poly Haven models → 6be0a2a^..4bd8356
- `b4821f4` J91.1, J91.2, J91.3: J91: soft shade where things meet (ambient occlusion) and light beams (lamp light in a thin haze, cut by what blocks it); Settings → Graphics → Soft shade, Light beams → 3455757^..bb6755a
- `fa1b9fe` J97.1: J97: lamp shadows: stools, tables and the counter block the lamps' light and their shine; no more squares of light → 1cd9231^..1cd9231
- `11033e0` J98.2: J98: tables softer, the floor a little shinier, steel calmer, the bar a third of a stop darker → 1be7df8^..1be7df8
- `83dd678` J96.1: J96: the floor's square of light and the reflections that followed you, fixed → a3c9f6f^..a3c9f6f
- `b3c6b97` J88.1, J88.3: J88: up to 255 lamps (bounced light and walls for 127), bounced light at its own pace → 38f092c^..db6eac2
- `7521b64` J95.1: J95: GitHub's long browser tests only on request; puts J87 (copies drawn together) live → 6f66cb1^..6f66cb1
- `1889278` J87.1: J87: copies drawn together: every copy of a model in one draw, up to 4096 models → eb42d32^..eb42d32
- `018725f` J94.1: J94: the place viewer draws → 6a7dd81^..6a7dd81
- `13e33e6` J93.1: J93: urgent fix: the models' textures load again on the phone → 28767be^..28767be
- `673494d` J86.1, J86.2: J86: Looped is its own engine: its own library, one host object for the page, its own place loader and player, limits given at start, and a second program (the place viewer at /bar/looped/) built from it alone → d7b2ce2^..550e9d1
- `c49cd10` J92.1: J92: no artificial glow: the bloom painted around every bright thing is gone, only real light is left; and the user tests from now on → f28d037^..f28d037
- `85d8742` J90.1, J90.2, J90.3: Dev remade: every page nests (a project, a chapter, a job, Done, Parked) with its own address, a trail and Back; Find over everything; the Live panel on Overview with reactions for Claude; constant motion (a drifting glow, slides, ripples, tilt, counting) with Calm to stop it → 98ae472^..c831282
- `4c06da4` J84.1, J84.2: Voice chat only between signed-in people of the same age group (teens with teens, adults with adults), enforced by the room; a guest is told voice is for signed-in players, and an older account adds its birth date once → 41f3e66^..05e186d
- `109fe1c` J54.1, J54.2, J54.3: GitHub's browser tests run to their end again: the game keeps at most 2 frames waiting on the GPU and draws nothing while hidden, the tests use their own light and a frame cap, a slow picture is noted instead of stopping a test, every test has a time limit, and the two-player tests take turns drawing on GitHub's small machine → 2ae86dd^..593d6ce
- `2eb56b4` J83.1: J83 Claude's key, made from Dev → People → 19dbbcd^..19dbbcd
- `4944bcc` J82.1: J82 the showroom: a new place (Menu → Places) with five stations that show what Looped does → 3f56ddb^..3f56ddb
- `1c830fd` J66.1, J66.2: J66: deploys never wait for, or get stopped by, the tests: the game's tests run in their own job and queue after the deploy, every step has a time limit, and the room test's grace check is steady → c3980dd^..6715116
- `a29204e` J8.1, J8.2, J8.3: J8: proof, then unfreeze: 22 cold-start drills with fresh AI sessions all passed and the five gaps they found are fixed; the final audit checked every data flow live; the restructure's spec is history; your Freeze rule is out and the first real job (J66) is in Now → 903e814^..e3e310e
- `450fb52` J7.1, J7.2, J7.3, J7.4: J7: the plan rebuilt with a reality check: every road and parked item has a job number and a verdict tied to the Charter's limits, shown in Dev; the world remake laid out as nine jobs; the script language's realistic scope and its first five jobs; the order you approved; the paid items parked; the freeze is a lock → 2334b6b^..fc947b6
- `666d414` J6.7: J6.7: the fix for J6's red site deploy → 88258dc^..88258dc
- `31cb32d` J6.1, J6.2, J6.3, J6.4, J6.5, J6.6: J6: the Handbook: the maps of folders and technology, a page per system, the data reference and world inventory written from the files, every page and control by label (checked against the live pages), recipes, the old notes sorted → 694f753^..820a1f2
- `f6d3335` J5.8: J5.8: the second fix for J5's red site deploy → 544f17d^..544f17d
- `902cfba` J5.7: J5.7: the fix for J5's red site deploy → c4b3fca^..c4b3fca
- `0a1886f` J5.1, J5.2, J5.3, J5.4, J5.5, J5.6: J5: data flows and Dev: the data map, the plan on Dev within a minute, one deployer per folder, the retired board out, one list of projects, Dev shows the truth → 7258356^..5cc1f57
- `c215278` J4.1, J4.2, J4.3, J4.4, J4.5, J4.6: J4: tools and locks: one front door (project.mjs: status, ship, records), the plan as data, checks for only what changed, the locks on main and on generated files, the Playbook tested against the tools → a867cf9^..4a3e765
- `d00bd4c` J3.1, J3.2, J3.3, J3.4, J3.5, J3.6, J3.7, J2.4: The Playbook: a procedure for every situation (session, intake, a job, replies, incidents, failures, broke and undo, ways of working, changing the structure), one voice (CLAUDE.md the short entry), test only what changed and push only what the user wants; and the tests' own lighting (J2.4) → 00d6601^..959ba86
- `8793265` J2.1, J2.2, J2.3: Honest checks: GitHub's browser tests use the classic light, every check is listed once and runs in exactly one workflow, every test runs after any failure with a time limit, allowed-to-fail only with a known issue, both deploys report their real state to Dev → a8e1dd9^..1c9c7ef
- `546c7d4` J1.1, J1.2, J1.3: Freeze and the Charter: R6 closed, the freeze and the user's new rules, the restructure's spec, known issues, the Charter (the user's words, what's realistic, the permission list) and the glossary → b8bcf57^..34ad18c
- `87bd110` R6.1, R6.2, R6.3, R6.4, R6.5, R6.6: Ember on everything that's left: the bar's menus and loading screen, the door page, the Studio, Dials, World notes, News, the UI Editor, Compare, Screens, the style page; the old hub boards removed → 14936f8^..197d923
- `ebca03a` R5.1, R5.2, R5.3, R5.4: J Production's front page: the site in Ember, highlights, the front page rearranged, Projects → 29cf0ac^..c2ffbfe
- `ad87523` R4.1, R4.2, R4.3, R4.4, R4.5, R4.6, R4.7: Dev, the control room: Overview, Projects, Plan, Publish, People; the old hub, portal and Toolbelt lead there → 03c3fe4^..f247b0b
- `58d93ec` R3.1, R3.2, R3.3: The World Plan: one doc (PLAN.md), the old plans archived, the plan keeper → 882582d^..a75881c
- `2eabe5f` R2.5-R2.6: Ember, the house theme (Warm + frosted glass + a drifting glow + moving titles); motion tokens → 393d12e^..b4f2fea
- `45292a6` R2.1-R2.4: real screens, light and dark in one theme, six candidates (Clean, Warm, Bold, Glass, Neon, Soft), compare and mix → 0bdee82^..8b9d289
- `aa0d3ae` R1.1-R1.7: the Toolbelt and the UI Editor (theme and screen files, the web renderer, theme and screen editing, publish with every version kept, the engine reads themes); the plan rework (docs/archive/UI_PLAN.md), engine work paused → a28d85c^..b394ba3
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
- `CLAUDE.md`: how a "go" runs. `docs/archive/REFERENCE.md`: the long guide (layout, every tool, iOS limits).
- `docs/archive/AUTOMATION.md`: the tools (piece, step, plan, uikit, studio, bench, gallery).
- `docs/archive/ENGINE.md`: how Looped is built (things, loops, layers). `docs/archive/STUDIO.md`: how the Studio works.
- `docs/handbook/manual/`: the manual (light, materials, recipes, timeline, world, avatars).
- `docs/handbook/protocol.md`: the multiplayer wire format. `docs/plan/jobs/W20.md`: the driving spec.
- `docs/archive/DECISIONS.md`: gotchas and why things are the way they are.
- `docs/archive/`: every old plan and progress file, kept to read, never to plan from.
