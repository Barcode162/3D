# Light

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

How light works in the bar: real units, eyes that adapt, bounced light, lights with size, moods. The new lighting is the default; Classic stays one tap away.

## Status

**Live** (L1 done); L2-L8 planned (`docs/manual/light.md`).

## What you can do with it

- Settings → Graphics → Lighting: New / Classic.
- The light switch and the lamps react to touch; the Studio's Look tab picks a place's mood and each light's colour and brightness.
- Compare with page options: `?light=classic`, `?bounce=0`, `?exposure=-1`, `?eyekeep=0.6` (every dial: `docs/manual/light.md`).

## How it works

- Real units (`looped/light_units.h`): lamps in lumens and Kelvin, true inverse-square falloff; the renderer's unit is the candela.
- Eyes (`looped/post.*`): the picture is metered and the exposure eases toward it in about a second; a dim room stays partly dim (`eyeKeep`); the AgX curve fades very bright colours to white; only brighter-than-white light glows.
- Bounced light: the CPU half (`looped/radiosity.*`, background jobs) splits the still surfaces into patches (~25 cm) and a grid of probes (~50 cm) and works out who sees whom once per layout; the GPU half (`looped/gi.*`, `shaders/gi.wgsl`) bounces light between the patches every frame, so dimmers and flicker follow at once.
- Lights with size (L1.6): lamps are small shapes, not points.
- The tests' own lighting `?light=test`: classic, one flat light instead of the lamps, no reflections, no glow, simple mirrors: quick in software (GitHub's tests use it, `TEST_LIGHT=test`).

## Data

Each kind's `light` part (colour, lumens, Kelvin, size); each place's `moods` and `mood`; `assets/world/materials.json` (how much light each surface reflects).

## Commands

- `tools/native/light_tool.cpp`, `bench_gi.cpp`, `radiosity_test.cpp`: light without a browser.
- `node tools/project.mjs bench` for the engine's benchmarks.

## Limits

- The new light is checked on GitHub only by the shaders' compile check; the phone and the piece pictures are the real check (KI-28: the user's call).

## Checks

- `wgsl`, `native` (light units, radiosity), the piece pictures (taken here with the new light), the phone.

## Known traps

- The tests' lighting turns reflections off: a test that expects reflections can't pass under it (KI-30, the Studio's S7.2).
- No squares of light: probes are blended (L1.8); a change to the probe grid shows as blotches first.

## History

L1 (2026-10-23): eyes, real units, real material brightness, bounced light, lights with size, the Studio's light, then L1.8-L1.10 polish. `?light=test` added 2026-10-24 (request #6).
