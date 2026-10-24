# The site's pages

*System page · chapter: site* · [all systems](../../generated/systems.md)

## What it is

joshuavelasco.com: the front page and its sections (Play, Engine, Vision, Projects, News, Sign in), in one frame and one look.

## Status

**Live.**

## What you can do with it

- Read about J Production, enter the bar, sign in.
- The front page's top words and its highlight are set by the owner in Dev → Publish.

## How it works

PHP (`homepage/`): `site/layout.php` is the frame (header, tabs, footer, who is signed in) with `site_head()` / `site_foot()`; `site/site.css` the look (with `/ui/`); each section is a folder with its `index.php` (`play/`, `engine/`, `vision/`, `projects/`, `signin/`, `portal/`). Tabs depend on who you are (guests: Home, Projects, News; staff also Dev).

## Data

Pictures in `homepage/site/img/`; the front page's words and highlight in Dev's store (`private/dev/site.json`, `highlights.json`).

## Commands

`node tools/project.mjs check site` (PHP syntax + `tools/site_test.mjs`).

## Limits

IONOS PHP 8.4; no database; pages render on the server.

## Checks

`site_php`, `site` (GitHub: every page loads, fits a phone, one headline, the right tabs).

## Known traps

Pictures are slow on GitHub: tests take them through a helper that only notes a slow one.

## History

The site in one look (R5, 2026-10-23); `homepage/` is uploaded only by `hub.yml` since J5.3.
