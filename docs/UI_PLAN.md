# UI plan: one Control panel, one UI tool, one house theme (DRAFT, 2026-10-24)

Status: **being agreed with the user. Nothing built yet.** The plan page the
user reviews is an artifact; this is the repo copy. Once agreed, step U1 goes
into `NEXT.md`.

## What we have
- Public site (homepage/, site.css): Home, Play, Engine, Vision, News, Sign in.
- Back room: Portal (homepage/portal), Hub (hub/index.html: Needs you, Board
  map, News desk), hub/list.html (old board, own colours), hub/dials.html,
  hub/world.html (own purple colours), People (bar gate.php?people).
- Game and tools: the bar (menus are HTML over the canvas, bar/web), the
  Studio, the UI style page (ui/: 4 Looped themes), Bar3D classic.
- Problems: no shared menu between back-room pages; Portal and Hub do the
  same job; two design systems (site.css and ui/themes.css) plus two one-offs;
  public pages are long and cramped on a phone.

## What it becomes
- **Public site:** Home, Play, News, About (Engine + Vision), Sign in. Few
  words; the text is edited from Control.
- **Control** (`/control`, staff only; /hub and /portal redirect), one tab bar:
  - Today: live build, CI, what needs the user, phone perf reports.
  - Progress: current step and pieces, build log, parked ideas.
  - Publish: News (Claude's build posts land as drafts; publish/unpublish)
    and Site (headline, pictures, banner of the public site; push).
  - People: accounts, invites, admins, roles.
  - Tools: Studio, Dials (with world.html's notes), UI Editor, live bar, classic.
- Data stays where it is (hub data, accounts); only the pages change.

## The UI Editor (engine tool)
- Two file kinds in `ui/system/`: a **theme** (colours, fonts, corners,
  spacing, depth, motion; light and dark) and **screens** (layouts of parts:
  stack, row, grid, text, button, card, list, tab bar, field, toggle, slider).
- Readers: the web renderer (looped.js: site, Control, bar menus) and an
  engine renderer (Looped C++: screens inside the 3D world).
- Editor: phone-first page (proposed `/ui/edit/`), Studio login and versions.

## Steps (one "go" each)
- **U1 Build the UI Editor:** U1.1 file format + test, Faceplate as the first
  theme file · U1.2 web renderer, /ui/ drawn from files (looks the same) ·
  U1.3 editor shell · U1.4 theme editing · U1.5 screen editing · U1.6 save,
  versions, undo, publish, pull tool · U1.7 engine reads the theme, one
  in-world screen drawn from a screen file.
- **U2 Pick the house theme:** 3 candidates, the user picks or mixes, it gets
  a name and becomes the only theme.
- **U3 Build Control** in the new theme; old pages redirect.
- **U4 Rebuild the public site.**
- **U5 Theme on the Studio, the bar's menus, the loading screen;** old themes removed.
- Parked: a 3D scene as the website background (PROGRESS.md → Parked).

## Open questions (recommendation first)
1. Engine draws in-world screens only, menus stay HTML (rec) / engine draws all UI.
2. Editor at /ui/edit/ (rec) / a mode inside the Studio.
3. Progress shows steps + build log + parked, card board retired (rec) / keep the board as a second view.
4. /control (rec) / keep /hub.
5. Control before the public site (rec) / site first.
6. Pause L1.8, U1 next (rec) / finish L1.8 first.
