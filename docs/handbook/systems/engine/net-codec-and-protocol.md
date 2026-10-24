# Network codec and protocol

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

How the phones and the room talk: one WebSocket, binary messages for movement, text messages for everything else, the same codec on both sides.

## Status

**Live.** Protocol v1 (`docs/handbook/protocol.md`).

## What you can do with it

Be in the bar with others: see them move, sit, play, hear the same song.

## How it works

- `looped/netproto.*` (the game's side) and `cloud/proto.js` (the room's) both pass the golden vectors `proto/vectors.json`.
- Binary STATE (your 16-byte avatar block plus activity data), PROP (an object's pose), TICK (everyone's states, 20 a second while something moves). Text messages: a type letter + JSON, checked field by field.
- `bar/web/net.js` is the bridge: it sends your state up to 20 times a second only when it changed; the shared world goes through the room, which numbers each write and sends it to everyone.
- Decoders never crash on garbage: they return false and say why.

## Data

`docs/handbook/protocol.md` (the spec), `proto/vectors.json`.

## Commands

`node tools/proto_test.mjs`; `tools/native/proto_test.cpp`.

## Limits

The room enforces sizes and rates (PROTOCOL §7); the room accepts the protocol version and the one before.

## Checks

`proto` (GitHub), `native`, `room`, `mp`.

## Known traps

`looped/net.h`'s header still speaks of the old relay; the room replaced it on 2026-10-18 (sorted in J6.6).

## History

Relay (PHP) 2026-10-15; the Cloudflare room and protocol v1 on 2026-10-18.
