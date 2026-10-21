# Looped lighting: baked light, probes, reflections (D1b-D1c, planned 2026-10-21)

Done already: glow and mood (D1a, `looped/post.*`). Next, in shippable steps.
Facts checked: Blender isn't in the container until `bash tools/assets/get_blender.sh`
(4.5 LTS, ~350 MB); CI never runs Blender (models are built in a session and
committed), so baking happens in a session and its images are committed; CI
only ASTC-encodes them once (cached). Room-sized models sit at the origin
(hall, bar_counter_l, back_bar_tall, decor_walls, lounge, stage, darts_zone,
arcade, construction) and can be lightmapped; moved copies (stools, tables,
glasses, chess) get probes instead. Lights that loops drive stay live.

1. **Scene dump + "baked" flag on lights** (no visible change): `light(..., bake)`
   in bar_scene.cpp (false for loop-driven lights), `PointLight.baked`,
   `tools/native/scene_dump.cpp` → `assets/bake/scene.json`.
2. **Bake in Blender** (data only): a "Lightmap" UV in common.py
   (lightmap_pack, ~4 texel margin); `tools/assets/bake_lightmaps.py` (room glbs
   + furniture as shadow casters, Cycles point lights for baked lights only,
   DIFFUSE direct+indirect, colour off, 256 samples + denoise, 8 px margin);
   `lightmap_inject.py` (TEXCOORD_1 + image, `extras.bar3d_lightmap {image, scale}`);
   astc_pack.py "lightmap" use: RGBM, always 4x4. Hall 2048², others 1024² →
   ~16 MB GPU, ~14 MB download. Bake ~30 min session CPU. Calibrate units with
   one plane (live falloff `window²/(d²+0.25)` vs Cycles watts → one global scale).
3. **Loader + shader** (first visible): gltf.cpp reads TEXCOORD_1 + extras,
   material lightmap fields, a uv1 stream (+8 B/vertex), model.cpp binding 8
   (1×1 black when none). In `shade()`: with a bake, diffuse = baked irradiance
   (replaces ambient); baked lights add **specular only**; live lights add both.
   Phone: soft shadows under counter/stools/tables, warm bounce on the ceiling,
   darker corners. `?bake=0` = today's look.
4. **Irradiance probes** for moving things: 0.5 × 0.75 m grid (~6.4k probes, L1 SH)
   → `assets/bake/probes.bin` (3 RGBA16F 3D textures, ~150 KB), sampled in
   `shade()` when there's no bake (avatars, glasses, stools). `?probes=0`.
5. **Reflection capture (D1c)**, can ship first: after "Stage entered", render a
   256² rgba16f cube from the room centre, prefilter 6 GGX mips, replace
   `roomGlow` with a box-projected cube sample; re-capture when the last model
   arrives. ~4.2 MB, zero per-frame cost. Phone: brass, piano, bar top and
   glasses reflect the real neon and bulbs. `?envcube=0`.

Test each with `tools/shots.mjs` before/after (expected differences, judged by
eye), the off flag matching `build/before`, and smoke.
