# Chess

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

The lounge's chessboard: real pieces on the coffee table; play the computer, or the person already playing white.

## Status

**Live.**

## What you can do with it

Tap the board: you play white against the computer (or black against whoever is white). The camera looks down from your side; tap a piece, then a highlighted square.

## How it works

`bar/projects/chess/chess.*` (the board, the pieces sliding and hopping); `chess_rules.*` (every legal move, castling, en passant, promotion, check, mate, stalemate, simple draws, a small computer player). Shared through the room (world key `ch`, claims `machine.chess.w` / `.b`).

## Data

World key `ch` in the room.

## Commands

`?chess`; `tools/native/chess_test.cpp` (perft counts).

## Limits

The computer player is small (not a strong engine).

## Checks

`native` (chess rules), `chess` (GitHub).

## Known traps

After chess the Menu button must come back on top (`menu_games` checks it; it failed on GitHub in run 211, KI-5).

## History

2026-10-19 (upgrade item 13).
