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
**R3 · The World Plan** (the tools). The plan system remade as this one doc.
- [x] **R3.1** This file in its fixed shape: the North star, the rules, the chapters, Now, Done, where things are. *Check:* read PLAN.md on GitHub: it says what the project is in one screen.
- [ ] **R3.2** Carry over: every open thing from NEXT.md, PROGRESS.md, the 68 hub cards and the old plans into its chapter (road or parked); the old plan files move to `docs/archive/`. *Check:* the road has every open thing once.
- [ ] **R3.3** The plan tool (`tools/plan.mjs`): add a request into its chapter, tick pieces, write Done, refuse anything that breaks the shape; the piece and step tools use it. *Check:* none on the phone (Claude's tool).

## 5. The road
*Next and later, by chapter. Every item: a tag, what it is, the North star line it serves.*
**Up next, in order:** R4 → R5 → R6, then engine work resumes.

### The world
### The site
- [ ] **R5** The world front door: a new scene built in Looped (the bar from the street at night, the camera drifting to the lit door, ~8 s seamless loop) shown as a clip; the front door, Showcase, News, Made here, Sign in as one place you move through (places not pages, grabbable spring motion, continuity, answers under 0.4 s, thumb first). · N1 N5
### The engine
### The tools
- [ ] **R4** Dev, the control room: Overview (is everything up, what needs you, phone reports), Plan (this doc), Publish (news drafts, the site's words and pictures), People (accounts, invites, admins), Toolbelt; built from screen files in Ember; /hub and /portal send you there. · N1 N5
- [ ] **R6** Ember on everything: the Studio, the bar's menus, the loading screen; the old themes retired from use. · N5
### The backbone

## 6. Parked
*Not on the road yet, by chapter. The user promotes them.*
### The world
### The site
- The front door scene running live in Looped instead of a clip (after R6, same places).
### The engine
### The tools
### The backbone

## 7. Done
*Newest first: build code, the pieces, the commits. "undo" reverts one line.*
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
