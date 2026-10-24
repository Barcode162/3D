# 0003 · IONOS is home, Cloudflare is live, GitHub builds

**Date:** 2026-10-16 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user ("let's get Cloudflare going"; keep the domain)
**Source:** `docs/archive/DECISIONS.md` → Hosting split, Cloudflare room is the multiplayer server, One domain

## Context
The bar needs a real-time server, and the user wants their domain kept (joshuavelasco.com: the site at `/`, the bar at `/bar/`).

## Decision
IONOS web hosting holds the domain, the site, the bar's files, the stores and the accounts (PHP, small JSON files). Cloudflare Workers + Durable Objects run the rooms and the ledger. GitHub keeps the private repo and deploys through Actions.

## Options not taken
- Everything on Cloudflare now: accounts and the site would move off the user's domain hosting; kept for later (D1, R2, Pages: `docs/archive/DECISIONS.md` → Product direction).
- The PHP relay for multiplayer: retired on 2026-10-18.

## Consequences
Two places to deploy (`deploy.yml` uploads the bar and deploys the room); no threads in the browser (IONOS can't send COOP/COEP).
