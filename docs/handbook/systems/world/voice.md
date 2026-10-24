# Voice chat

*System page · chapter: world* · [all systems](../../generated/systems.md)

## What it is

Talking to other players, phone to phone (WebRTC), positional if you want.

## Status

**Live.**

## What you can do with it

Menu → Voice chat: mic on or off, push to talk, the noise gate (only sends while you're louder than it), mic boost, echo cancel, noise suppression, 3D voice, quality.

## How it works

`bar/web/voice.js`: one connection per pair of players; the handshake goes through the room; then audio goes straight between phones (Google's public STUN server finds a way through routers). Opus at up to 510 kbps.

## Data

Settings on the phone (`micGate`, `micGain`, `pushToTalk`, `voice3d`...).

## Commands

`tools/voice_test.mjs` (two players, a fake microphone).

## Limits

Behind strict networks a direct connection can fail (no relay server for media).

## Checks

`voice` (GitHub, allowed to fail: KI-25; its checks passed in run 211, then a picture timed out).

## Known traps

`voice.js`'s header still says the relay carries the handshake; the room does since 2026-10-18 (sorted in J6.6). In headless Chromium the remote level sometimes reads 0.

## History

2026-10-16.
