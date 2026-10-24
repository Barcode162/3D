# The UI Editor

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The editor for themes and screens (joshuavelasco.com/toolbelt/ui-editor/): the look every page and the bar's menus wear.

## Status

**Live.**

## What you can do with it

Pick a screen or a theme; change tokens (colours, corners, motion) with live preview on phone or computer width; add, move, duplicate, delete parts; light and dark halves; Undo / Redo; Publish a new version (with what changed).

## How it works

`toolbelt/ui-editor/editor.js` edits `ui/system/` files; the preview wears the edited theme. The UI store (`toolbelt/ui-editor/api.php`, `private/ui/`) keeps every version; Claude pulls it into the repo (`uikit.mjs pull`) and makes `ui/themes.css` (`uikit.mjs css`).

## Data

`private/ui/` (versions); the repo's `ui/system/` and `live-version.json`.

## Commands

`node tools/project.mjs ui status | pull | push --note … | css | check`.

## Limits

Owner and admins (Claude with the key).

## Checks

`ui_api`, `ui_editor`, `uikit` (quick).

## Known traps

Never edit `ui/system/` or `ui/themes.css` by hand (the lock refuses `themes.css`).

## History

R1 (2026-10-23).
