# 0002 · One engine, written from scratch

**Date:** 2026-10-15 (recorded as a decision in force on 2026-10-24, J6.6)
**Status:** current
**Asked by:** the user (no outside engines or libraries; exceptions only with their yes)
**Source:** `docs/archive/DECISIONS.md` → Tech choices, Build ourselves vs. use the platform

## Context
J Production wants its own engine and, later, its own language, running on a phone in the browser.

## Decision
The engine (Looped) and the game are written from scratch in C++20, built to WebAssembly, drawing with WebGPU through the Dawn C++ API. No outside engines or libraries. Approved exceptions: three.js inside Meridian (2026-10-18) and inside the platformer "Test 2" (2026-10-19), for those games only.

## Options not taken
- A ready engine (Unity, Godot, Babylon, three.js for the bar): not ours, and the user asked for our own.
- WebGL: older, and WebGPU is on iOS 26.

## Consequences
Everything is built and tested here (`tools/native/`, the browser tests); every outside asset needs a licence and a credit; a new exception is a question to the user first.
