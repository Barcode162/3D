# The script language

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

J Production's own small, safe language for what things in the world do: a candle that flickers, a jukebox that knows the mood, a stool that remembers its regular. Short scripts that read almost like sentences, written by you, Claude or another AI; a mistake is caught before it runs, with its line and a fix hint. Not a replacement for C++ (the engine) or JavaScript (the pages).

## Status

**Planned.** Scoped in J7.3: [its spec](../../../plan/jobs/J43.md) and five jobs, J43 (the design on paper), J78 (the parser and checker), J79 (the safe runner), J80 (in the Studio), J81 (five behaviours as scripts). Formerly T1.

## What you can do with it

Nothing yet. Today a thing's life is data: states, rules and loops in its kind (see [the world model](world-model.md)).

## How it works

Not built. It will live in the engine (`looped/script/`, C++ → WebAssembly) on top of the world model's states, rules and loops: a script is a readable way to write a kind's data, plus the little logic data can't hold (counting, remembering, choosing). A script touches only its own thing and the messages it sends, within a step budget per frame and a memory limit.

## Data

—

## Commands

—

## Limits

Small on purpose: things, states, events, timers, loops, remembered values, messages. No files, network, accounts, credits or other players' data; no server-side scripts. Its jobs start in the order on the road (J43 first).

## Checks

—

## Known traps

—

## History

Named in the North star (N2); road item T1 until J7.1 renumbered it J43; its scope and first jobs set in J7.3 (2026-10-24).
