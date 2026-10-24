# Highlights and the front page's words

*System page · chapter: site* · [all systems](../../generated/systems.md)

## What it is

What the front page says: its headline and line, and one highlight (a picture, a few friendly words, a link).

## Status

**Live.**

## What you can do with it

Dev → Publish → Highlights: make one (title, words, link, picture), Push it to the front page (only one is shown); The front page's words: type and Save.

## How it works

`dev/api.php`: `site` (the words; GET `?site` is public for the front page), `highlights`, `highlight`, `push` (GET `?highlight` gives the pushed one; `?hi=<id>` its picture). `homepage/index.php` reads them.

## Data

`private/dev/site.json`, `private/dev/highlights.json`, `private/dev/highlights/<id>.jpg`.

## Commands

None for Claude: these are the user's switches (Claude can't push a highlight; the store refuses it).

## Limits

One highlight on the front page at a time.

## Checks

`dev` (GitHub: saved, pushed, public, Claude refused).

## Known traps

The words box keeps what you've typed even if the saved words load late (fixed in J5.7).

## History

R5.2-R5.3 (2026-10-23).
