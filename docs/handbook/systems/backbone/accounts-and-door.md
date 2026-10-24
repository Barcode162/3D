# Accounts and the door

*System page · chapter: backbone* · [all systems](../../generated/systems.md)

## What it is

Who you are (accounts, profiles, badges) and who may come into the private bar.

## Status

**Live.** Accounts may move to Cloudflare later (`docs/DECISIONS.md` → Product direction).

## What you can do with it

- Sign up (13+), log in, your profile (picture, display name, bio, favourite drink and song, card colour, details, badge slots), change your password, a recovery code.
- Owners: badges (bartender...). Admins: People (invite, remove, make admin) in Dev → People.

## How it works

- `server/account.php`: plain PHP with small JSON files; passwords only as bcrypt hashes, tokens only as their SHA-256; the birth date stays private (your replies carry only teen or adult).
- `server/gate.php`: the door. `.htaccess` sends every request to it except a few files; without a login that may play, the page is the sign-in screen; with one, the file is streamed. Who may play: the owner and admins (`accounts/admins.json`; while it doesn't exist, the first account) and the invited (`accounts/players.json`). Claude's checks pass with the hub key.
- The game keeps the login token in `localStorage` (`bar3dToken`).

## Data

`home/bar/accounts/` (`users/`, `names.json`, `tokens/`, `avatars/`, `admins.json`, `players.json`).

## Commands

—

## Limits

Teens and adults are kept apart; the 18+ casino uses earned credits only (the Charter).

## Checks

`gate`, `hub` and `site` (sign-in, passwords, recovery codes); `account_test` is obsolete (KI-27).

## Known traps

Never weaken the age rules (the Charter's Never list).

## History

Accounts 2026-10-16; the private bar and its door 2026-10-20.
