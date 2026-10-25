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

**Who can talk (J84):** the room passes a handshake only between two signed-in people of the same age group (teen with teen, adult with adult; Charter: 13+, teens and adults kept apart). It learns each login's group (`band`: teen, adult, or none for an account made before birth dates were asked) with its login check, keeps it with the player, and never sends it to anyone. A refused handshake comes back to its sender as `{refused}`: `guest` (you have no login), `birth` (your account has no birth date: Menu → Voice chat → Add my birth date, once) or `apart` (anything about the other person, so a refusal never reveals their age group).

## Data

Settings on the phone (`micGate`, `micGain`, `pushToTalk`, `voice3d`...).

## Commands

`tools/voice_test.mjs` (two players, a fake microphone).

## Limits

Behind strict networks a direct connection can fail (no relay server for media). Phone to phone, each side can see the other's internet address (a relay that hides it is J63, parked: a cost). Text messages and lobbies by age group are J20.

## Checks

`voice` (GitHub, allowed to fail: KI-25; its checks passed in run 211, then a picture timed out).

## Known traps

`voice.js`'s header still says the relay carries the handshake; the room does since 2026-10-18 (sorted in J6.6). In headless Chromium the remote level sometimes reads 0.

## History

2026-10-16.
