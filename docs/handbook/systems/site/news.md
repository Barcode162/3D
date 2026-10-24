# News

*System page · chapter: site* · [all systems](../../generated/systems.md)

## What it is

The public news board: posts Claude drafts after a job and the owner publishes.

## Status

**Live.**

## What you can do with it

- Dev → Publish: read the drafts, Publish one (it goes to joshuavelasco.com/news), or delete it.
- The front page shows the newest news.

## How it works

Drafts and pictures live in the hub (`hub/hub.php`: `post`, `posts`, `shot`, `publish`, `unpost`); `news/index.php` shows only the published ones and only their pictures; project names come from the [one list of projects](projects.md).

## Data

`private/hub/posts-<project>.json`, `private/hub/shots/<project>/`.

## Commands

`node tools/project.mjs news posts | post --build <sha> --title … --text …`; the ship writes one with `--news`.

## Limits

Only the owner publishes; Claude only drafts (the Charter: publishing is the user's switch).

## Checks

`hub` (GitHub: a draft, the owner's publish, the public page, pictures only for published posts).

## Known traps

Nothing reaches the public page until the owner's Publish.

## History

Hub v3 news (2026-10-20); Dev → Publish (R4, 2026-10-23).
