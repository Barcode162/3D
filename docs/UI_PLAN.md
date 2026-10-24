# The rework: one world, one Dev area, one UI tool (DRAFT v3, 2026-10-24)

Status: **being agreed with the user. Nothing built yet.** The user reviews
the plan as an artifact page; this is the repo copy. Once agreed, R1 goes into
`NEXT.md`.

## Decided (user, 2026-10-24)
- **All other work is paused** (L1.8, P1, P2, every engine step) until R6 is done.
- The bar's menus stay plain HTML over the 3D (no GPU cost) until Looped is understood better.
- The UI Editor is an **engine tool in the Toolbelt**, next to the others.
  Dense and compact is fine, with every setting there.
- The plan/progress system gets a **full remake**: list what exists, design again from zero.
- **One world, not pages:** joshuavelasco.com is the world. From it you
  enter the bar, look around (showcase), or go to Dev (staff). Plus a short
  humble part: we make the engine and our own script language.
- It should use studied techniques for retention, smoothness and wow, combined in a new way.
- The front door gets **a new scene built in the engine**, shown for now as a
  short looping render (a clip), not live 3D. Live 3D is parked for after R6.
- The maker's name is **J Production** (the humble part, across the site).
- **The World Plan:** one single doc of truth whose structure never changes,
  whatever the user asks for; it always points at the project's direction.

## What we have
- Pages: 14 in 4 looks (site.css; ui/themes.css with 4 themes; world.html
  purple; list.html dark). Public: Home, Play, Engine, Vision, News, Sign in.
  Back room: Portal, Hub (Needs you, Board, News desk), Everything,
  Dials, World, People (gate.php). Game and tools: the bar, Studio, /ui/, classic.
- Tracking in 9 places: plans (PLAN.md, ENGINE.md, STUDIO.md, VISION,
  LOOPED, LIGHTING, 5 old plans + archive), progress (NEXT.md, PROGRESS.md,
  ENGINE/STUDIO progress, the Hub board: 58 cards / 13 sections + places
  map, docs/hub/bar3d.md), proof and notes (gallery branch, news posts,
  DECISIONS.md, docs/manual, docs/audit).

## What it becomes
- **The world** (everyone), three depths: Outside = the front door (one
  line, Enter the bar, Look around, Dev for staff); Around = Showcase, News,
  Made here; Inside = the bar, Dev. Fixed positions, one continuous move
  between places, no page loads, Back goes up one level. Sign in is a sheet.
- **Dev** (staff): Overview (bar, room server, site, CI, live build, needs
  you, phone reports) · Plan (the remade system) · Publish (news drafts and
  posts; the public places' words and pictures) · People (accounts, invites,
  admins, ledger) · Toolbelt (Studio, UI Editor, Dials, Gallery, Tests, Perf,
  Engine flags; later Script, Light).
- Old → new: Home/Play/Engine/Vision → front door, Showcase, Made here;
  Portal + Needs you → Overview; board, Everything, map, NEXT/PROGRESS →
  Plan; news desk → Publish; gate people → People; Studio, Dials,
  world.html, /ui/, gallery, perf box → Toolbelt; 4 themes + site.css → one theme.

## Techniques (the sources)
Places not pages (zooming UIs: Raskin 2000, Pad++) · grabbable spring motion
(Apple "Designing Fluid Interfaces", WWDC 2018) · continuity (View
Transitions, Material container transform) · <0.4 s response (Doherty and
Thadani 1982; prefetch on touch-down, optimistic saves) · few choices
(Hick-Hyman, progressive disclosure) · thumb first (Fitts, Hoober 2013) ·
one peak and a good end (peak-end rule, Kahneman 1993) · a reason to return
(Zeigarnik, endowed progress: Nunes and Dreze 2006; "new since you were
here", live presence; no streaks) · aesthetic-usability (Kurosu and
Kashimura 1995). Only transform/opacity animation on the phone.
The combination: a world you move through + grabbable motion + live
presence, with the same map turning into a 3D scene later.

## Steps (one "go" each)
- **R1 Toolbelt + UI Editor:** R1.1 the Toolbelt (one list and page of the
  engine's tools) · R1.2 theme/screen file format + test, Faceplate as the
  first theme file · R1.3 web renderer, /ui/ drawn from files (same look) ·
  R1.4 editor shell (preview, parts, dense inspector, phone-first) · R1.5
  theme editing (every token, light/dark) · R1.6 screen editing (phone,
  computer, in-game previews) · R1.7 save, versions, undo, publish, pull tool;
  Looped C++ reads the theme file (no engine drawing yet).
- **R2 The house theme:** 3 candidates, the user picks or mixes, it gets a name.
- **R3 The World Plan, one doc:** R3.1 write it in its fixed shape · R3.2
  carry over the open work (58 Hub cards, NEXT.md, PROGRESS.md, old plans)
  into its areas, archive the old files · R3.3 the plan tool (adds a request
  into its area, ticks pieces, writes the build log, refuses anything that
  breaks the shape; piece.mjs and step.mjs use it).
- **R4 Dev.**
- **R5 The world front door:** the new scene built in the engine, rendered as a short looping clip; then the front door, Showcase, News, Made here, Sign in.
- **R6 The theme on everything;** old themes removed. Then engine work resumes.
- Parked: the front door scene live in the engine instead of a clip (after R6, same places).

## The World Plan's fixed shape (PLAN.md, proposed)
1. North star (only the user changes it) · 2. Your rules, dated (only the
user) · 3. The areas, fixed list, one status line each · 4. Now: the step
"go" does, pieces, phone checks · 5. The road: Next and Later, each item
under its area, naming the North star line it serves · 6. Parked, by area ·
7. Done: the build log · 8. Where things are: links to how-it-works docs.
Rules: every request lands in an area (no fit = a question, never a new
part); a road item that serves no North star line goes to Parked; Claude
edits only through the plan tool, which refuses shape breaks; Dev · Plan
shows the same doc, nothing else keeps a tick list.
Areas (draft): The world site, The bar, Characters, Games, Social, Homes,
Shops and money, Engine (Looped), Script language, Toolbelt, Dev, Server and
accounts, Speed and quality.

## Open questions, round 3 (recommendation first)
1. Front door scene: the bar from the street at night, slow drift to the lit door, ~8 s seamless loop (rec) / the whole block.
2. The 13 areas: confirm or change now (fixed after).
3. PLAN.md at the repo root replaces NEXT.md, PROGRESS.md, the Hub board and the progress parts of other docs (rec).
4. Assumed unless the user objects: carry over still-wanted work and archive the rest; Dev at /dev with /hub and /portal redirecting.
