# Hosting

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

Where everything runs: IONOS for the domain, the site, the bar's files and the stores; Cloudflare for the room; GitHub for the code and the deploys.

## Status

**Live.**

## What you can do with it

Visit joshuavelasco.com (the site), /bar/ (the private bar), /dev/, /studio/, /news/, /classic/.

## How it works

- IONOS web hosting (PHP 8.4, SFTP): the web root `home/` is joshuavelasco.com; the bar in `home/bar/`; the stores in `private/` beside it, out of reach of the web.
- Cloudflare Workers + Durable Objects: the room server and the ledger (bar3d.joshuavelasco60.workers.dev).
- GitHub: the private repo `Barcode162/3D`, Actions (the deploys), Pages (`gh-pages`: a pointer page that records the deployed build).

## Data

See the [data map](../../data-map.md) and the [deploy map](../../deploy-map.json).

## Commands

—

## Limits

IONOS can't send the headers for threads (SharedArrayBuffer).

## Checks

`gate`, `site`, `room` on GitHub; `node tools/project.mjs live`.

## Known traps

Uploads use `lftp mirror` without `--ignore-time` (it once skipped same-size edits); per-build files are sent again by name.

## History

joshuavelasco.com since 2026-10-15; Cloudflare room 2026-10-18; the bar private (2026-10-20).
