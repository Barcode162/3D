# Projects

*System page · chapter: site* · [all systems](../../generated/systems.md)

## What it is

J Production's projects in one list: Bar3D, Looped, the script language, the workshop.

## Status

**Live.**

## What you can do with it

See them on joshuavelasco.com/projects and in Dev → Projects (with their tools).

## How it works

`toolbelt/tools.json` → `projects` is the only list (J5.5). Each project's `site` card says how the site shows it (`featured`: a big card, with a picture or an icon; `more`: the small list); a project without one isn't on the site. `homepage/site/layout.php` `site_project_list()` reads it; Dev's Projects tab and the news page too.

## Data

`toolbelt/tools.json`.

## Commands

A new project: one entry in `projects` (the toolbelt test checks its site card).

## Limits

The "Where we're going" card is the vision page, written in the Projects page itself.

## Checks

`toolbelt`, `site` (Projects lists the list's projects).

## Known traps

On IONOS the list is `home/toolbelt/tools.json`; tests point to it with `SITE_TOOLS`.

## History

One list since J5.5 (2026-10-24; before: three lists, KI-11).
