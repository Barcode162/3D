# Driving

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Car physics for the racing sim: the original `looped/vehicle.*` and the new driving physics in `looped/drive/`, plus steering that feels like a real wheel for any input.

## Status

**Partial.** Racing uses it; the driving rework (`docs/plan/jobs/W20.md`) is in progress (its future job).

## What you can do with it

- Sit in a racing rig → race the bots or the other rig's player; SETUP on the race screen tunes the car; `?pad` opens the skid pad.

## How it works

- `looped/vehicle.*`: one rigid body, four raycast wheels with springs, dampers and anti-roll bars; Pacejka tires with combined slip; engine, clutch, gearbox, limited-slip differential.
- `looped/drive/` (new): `car_spec` (every number in SI units), `tire`, `vehicle_sim`; the race world ticks at 240 Hz and the drive sim steps twice per tick (480 Hz).
- `looped/steering.*`: the wheel in your hands follows your thumb and comes back to the middle by itself like a real car's caster; at speed the road wheels turn less.

## Data

Each car's `CarSpec` (`bar/projects/racing/cars.*`); car setup saved on the phone (`bar3dCarSetup`).

## Commands

`tools/native/drive_lab.cpp`, `vehicle_test.cpp`, `steering_test.cpp` (`CAR=n`), `race_test.cpp`: all in `native`.

## Limits

A race comes out the same only on the identical WebAssembly build (determinism).

## Checks

`native`, `race_mp` (GitHub).

## Known traps

Car space is x right, y up, -z forward (like the camera). The auto gearbox decides from wheel speed, not engine rpm.

## History

Racing sim 2026-10-16; steering feel 2026-10-19; driving rework MVP 2026-10-20.
