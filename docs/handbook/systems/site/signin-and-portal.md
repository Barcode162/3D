# Sign in and the portal

*System page · chapter: site* · [all systems](../../generated/systems.md)

## What it is

Signing in and making an account on the site; the staff portal that leads to Dev.

## Status

**Live.**

## What you can do with it

- /signin/: sign in, or New account (asks for your birth date: 13 and up).
- /portal/: staff are sent to Dev; everyone else gets the 404 page.

## How it works

The forms talk to `bar/account.php` (see [accounts and the door](../backbone/accounts-and-door.md)); the login is the bar's (cookie `bar_t`).

## Data

Accounts on IONOS (`home/bar/accounts/`).

## Commands

—

## Limits

13+ only; the birth date stays private (your own replies carry only teen or adult).

## Checks

`site` (sign-in form, new account, portal for staff only).

## Known traps

The age rules are a Charter rule: never weaken them.

## History

Accounts 2026-10-16; the site's sign-in (R5, 2026-10-23).
