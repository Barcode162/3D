# The ledger

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

The money and the things people own: a Durable Object with its own database, where every change is one transaction.

## Status

**Live** (earning in games; the mall and VIP store later).

## What you can do with it

Earn credits in games (the coin pops out); see your wallet.

## How it works

`cloud/ledger.js`: money in whole cents; a daily earning cap per player; every change is a row with a unique reference, so the same change twice does nothing; only the server changes balances; a player may only read their own wallet (`/bank` "me").

## Data

The ledger's SQLite database (Cloudflare).

## Commands

`node tools/ledger_test.mjs`.

## Limits

Only the room (games) writes; trades and shops come later.

## Checks

`ledger` (GitHub).

## Known traps

Everything forwarded from the internet is marked public and may only ask "me".

## History

2026-10-20 (master list item 3).
