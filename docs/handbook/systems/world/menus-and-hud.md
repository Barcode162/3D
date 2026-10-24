# Menus and HUD

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The Menu button and its panels (HTML over the game), and the text the game draws on screen.

## Status

**Live.**

## What you can do with it

Menu: Emotes, Avatar, Profile, Voice chat, Settings, Piano songs, Places, and the drink list for bartenders. Only one panel covers the game at a time.

## How it works

`bar/web/menu.js`; C++ reads settings (`bar3dSettings`) and gets commands through the mailbox (`bar3dBridge.command`); it tells the menus what's going on (`bar3dUi.setInfo`). `bar/hud.*`: frame rate, prompts, name tags, pictures over heads, machine screens, banners, toasts.

## Data

Settings in `localStorage` (`bar3dSettings`, checked against the menu's own list on load).

## Commands

`tools/menu_test.mjs`, `tools/menu_games_test.mjs`.

## Limits

Panels are HTML (the phone's keyboard and photo picker only work with real form fields).

## Checks

`menu`, `menu_games` (GitHub; both failed in run 211: the Menu button hidden, KI-5).

## Known traps

A short tap whose click doesn't arrive: `menu.js onTap` guards it.

## History

HTML menus 2026-10-16; Clash Royale style 2026-10-19; one panel at a time (modal manager).
