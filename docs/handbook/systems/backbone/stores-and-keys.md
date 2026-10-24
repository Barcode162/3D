# Stores and keys

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

Where the tools keep their data on IONOS (the world, the UI, Dev, the hub) and the one key Claude uses.

## Status

**Live.**

## What you can do with it

The user publishes from the Studio and the UI Editor; their version wins; Claude builds on it.

## How it works

- World store `studio/api.php` (`private/studio/`): numbered versions (a version never changes), drafts, shares, proposals; a publish names the version it started from and is refused if someone published since.
- UI store `toolbelt/ui-editor/api.php` (`private/ui/`): the same rules for themes and screens.
- Dev's store `dev/api.php` (`private/dev/`), the hub `hub/hub.php` (`private/hub/`).
- The Claude key: the owner makes it (shown once); its hash is `private/hub/key.json`; it travels as `X-Hub-Key`; in this environment it is `HUB_KEY`. Never in the repo, a log or a reply.

## Data

Every store and who writes it: the [data map](../../data-map.md).

## Commands

`node tools/project.mjs world …`, `ui …`, `notes …`, `news …`, `status`.

## Limits

Small JSON files, no database.

## Checks

`studio_api`, `ui_api`, `dev`, `hub` (GitHub).

## Known traps

Never edit a store's copy in the repo by hand (`assets/world/` comes from the store, `ui/system/` too).

## History

World store S3 (2026-10-22); UI store R1.7 (2026-10-23).
