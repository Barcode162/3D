# Skeleton and animation

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Bodies that bend: skeletons, skinned meshes, walking, sitting and emotes.

## Status

**Live.**

## What you can do with it

Walk, sit, emote (Menu → Emotes); see your body in the mirrors and when you look down.

## How it works

- `looped/skeleton.*`: bones in a tree; a pose turns into one matrix per bone (forward kinematics).
- `looped/skinned_mesh.*`: one smooth mesh, each vertex following up to two bones; builders for tubes and ellipsoids.
- `bar/avatar.*`: your body, 18 bones; the walk is driven by how far you move; your head tilts as you look.

## Data

Avatar looks (Menu → Avatar), sent to others in your state (`slot::kPose`, 19 numbers).

## Commands

`?emote=N`, `?walkpose=<deg>`, `?walkcheck` (logs foot slide); `tools/native/walk_test.cpp`.

## Limits

Models from files have no animation yet (`looped/gltf.h`).

## Checks

`native` (walk), `avatar` (GitHub).

## Known traps

Feet slide if the walk speed and the animation disagree: `?walkcheck` measures it.

## History

Walking animation 2026-10-19; avatar customizer 2026-10-19.
