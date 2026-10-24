# Rendering

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Drawing the scene: models, shapes, mirrors, avatars, text and pictures over heads, with culling, compressed textures and background loading.

## Status

**Live.**

## What you can do with it

- Settings → Graphics: Quality (Auto, High, Medium, Low), Mirror (Full, Simple), Lighting (New, Classic).
- Tap a mirror's glass to switch it: full → avatars only.

## How it works

- `looped/scene_renderer.*` draws a `RenderScene` (`looped/render_scene.h`: shapes with their look, placed `.glb` models, point lights, mirrors, rooms and doorways, the air, textures). The bar builds one in `bar/bar_scene.cpp → renderSceneOf`.
- Mirrors: each visible mirror gets its own pass from a camera reflected in the glass, only over the part of the screen it covers; sharpness drops with distance.
- Your avatar is whole in mirrors and headless in first person.
- Culling (`looped/culling.h`): what the camera can't see isn't drawn; rooms and doorways cut whole rooms away.
- Models (`looped/gltf.*`, `model.*`): glTF 2.0 binary, PBR metal/roughness, emissive; no animation or skins in files. Uploads (`gpu_upload.*`) go through the job queue: images decode off the main thread and go straight to the GPU, mipmaps on the GPU.
- Pipelines compile in the background (`pipeline_cache.*`): nothing stalls at start-up.
- Quality (`looped/quality.h`): High 2 device pixels per CSS pixel, Medium 1.6, Low 1.3; Auto steps down when the GPU is what holds the frame rate.
- Text (`text.*`), billboards (pictures over heads, `billboard.*`), soft shadows under avatars (`shadows.*`).

## Data

`looped/shaders/*.wgsl` (built in), `assets/models/*.glb` (gzipped by the build), compressed textures `assets/tex/` (made at build time by astcenc).

## Commands

- Page options: `?quality=auto|high|medium|low`, `?mirrorq=0|1`, `?cam=x,y,z,yaw,pitch`, `?still`, `?reflect=0`, `?perf=1`.
- `OUT=build/before node tools/shots.mjs spots` / `node tools/imgdiff.mjs build/before build/after`: looks-the-same checks; `tools/piece.mjs` takes before/after pictures.

## Limits

- Up to 32 lights in the light grid; moving boxes up to 320.
- Every draw path must go through `pipeline_cache` (no direct `CreateRenderPipeline`).

## Checks

- `wgsl` (the shaders compile in Chromium's WebGPU, quick), `smoke` (GitHub), the piece pictures, the phone (`?perf=1`).

## Known traps

- A screenshot of the running game takes very long on a machine without a graphics card (KI-5): tests take pictures through a helper that never fails on a slow one.
- The picture on this cloud machine is software drawing: right for checking looks, never for speed.

## History

Mirror sharpness by distance and culling (2026-10-19); compressed textures (2026-10-19); quality levels (2026-10-20).
