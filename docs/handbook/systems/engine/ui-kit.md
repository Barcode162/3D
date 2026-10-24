# The UI kit

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Looped UI: the themes, parts and screens every page wears (the site, Dev, the Studio, the Toolbelt, the bar's menus), and the engine's reader of the same themes.

## Status

**Live.**

## What you can do with it

- Change a theme or a screen in the [UI Editor](../tools/ui-editor.md); publish; Claude pulls it in.
- Pages switch look with `Looped.setLook(name)` (saved per device).

## How it works

- `ui/looped.js` (themes, save lamps, toasts, number rolls) and `ui/looped.css` (the parts).
- `ui/screen.js`: a screen file (`ui/system/screens/*.json`) is a tree of parts drawn with `Looped.screen()`; a part's `id` becomes a slot the page fills (Dev's tabs are screens).
- Themes (`ui/system/themes/*.json`) become `ui/themes.css` through `node tools/uikit.mjs css` (never by hand; the lock refuses hand edits).
- `looped/ui_theme.*`: the engine reads the same theme files, so screens it draws match.

## Data

`ui/system/` (the UI store's copy: themes, tokens, screens, parts); `ui/system/live-version.json` says which store version.

## Commands

`node tools/project.mjs ui status | pull | push --note … | css | check`.

## Limits

Gradients and shadows are kept as text in the engine (not drawn yet).

## Checks

`uikit` (quick), `ui`, `ui_editor`, `ui_api` (GitHub), `tools/native/ui_theme_test.cpp`.

## Known traps

The look kit ships twice: `/ui/` (the site's) and inside the bar (`bar/ui/`); both come from `ui/`.

## History

Themes and the Faceplate look (2026-10-22, `docs/archive/STUDIO.md` §4); the UI Editor (R1, 2026-10-23).
