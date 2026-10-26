# Light

How light works in the bar: where it comes from, how it spreads, and every dial for it in the Studio.

Every switch is also a page option for comparing (add it to the bar's
address, e.g. `?bounce=0`).

## L1 Real light

**Settings → Graphics → Lighting: New / Classic.** Classic is the lighting
before L1, one tap away for comparing (page option `?light=classic`).

### Eyes (L1.1)
The bar is drawn as real light, with any brightness (a bulb's core is
thousands of times brighter than a dark corner), and then shown the way eyes
see it:
- **Metering:** the picture's average brightness is measured every frame,
  weighted to the middle of the screen (where you're looking).
- **Adapting:** the exposure follows it over about a second: quicker when it
  gets brighter (0.6 s), slower when it gets darker (1.4 s), like eyes.
- **A dim room stays dim:** eyes don't turn every room into daylight. Part
  of a room's darkness stays visible (`eyeKeep`, 0.6 = 60%).
- **The curve (AgX):** very bright colours fade toward white like on film
  (a red neon's core turns pink-white) instead of clipping to flat colour.
- **No glow:** nothing is painted around bright things (the bloom was taken out in J92, the user's word: "get rid of the artificial glow"); a lamp is bright itself and lights what's around it, nothing more.

| Dial | Page option | Default | What it does |
|---|---|---|---|
| Exposure | `?exposure=-1` | -1.63 EV in the bar (-1.3 the engine's default; a third of a stop darker since 2026-10-26) | the whole picture brighter (+) or darker (-) |
| Darkness kept | `?eyekeep=0.6` | 0.6 | 0 = every room looks equally bright, 1 = eyes never adapt |
| Reference | `?eyeref=-1.5` | -1.5 | the room brightness (log2) that shows as "normal" |
| Darkest / brightest adapted | `?eyemin=-9` `?eyemax=6` | | eyes don't adapt beyond these |
| Eyes off | `?eyes=0` | | a fixed exposure (for comparing) |

### Real units (L1.2)
Every lamp says how much light it gives the way a bulb's box does, in
**lumens**, and how warm it is in **Kelvin**:

| Light | Lumens | Kelvin |
|---|---|---|
| a candle | 12 | 1800 |
| an old filament bulb (the bar's pendants) | 300–400 | 2400 |
| a 60 W household bulb | 800 | 2700 |
| a 100 W bulb | 1600 | 2700 |
| daylight through a window | | 6500 |

- Light falls off the real way (four times dimmer at twice the distance)
  and **has no edge**: each lamp reaches as far as its light still matters
  (down to about moonlight, 0.1 lux), fading out smoothly.
- **Colour temperature** sets the colour of white bulbs; the Colour field
  then works as a filter (white = none). Neon, signs and the jukebox keep
  their own colour (no Kelvin).
- Surfaces come out in real brightness (cd/m²) and the eyes adapt to it.
- In the Studio: Light → Lumens, Colour temperature. Brightness and Reach
  are now only for Classic.
- Each pixel adds up the 8 strongest lamps near it (6 at Medium quality, 4 at Low).

| Tuning | Page option | Default |
|---|---|---|
| glow of self-lit surfaces (neon, screens) | `?emissive=8` | 8 cd/m² per unit |
| the room's fill light (until bounced light, L1.6) | `?ambient=14` | 14 |
| white balance: how much eyes correct warm light | `?white=0.55` `?whitek=2700` | 55% toward white for 2700 K |

### Real material brightness (L1.3)
How much light a surface reflects (its **albedo**) decides how far light
spreads: dark surfaces swallow it. Many of the bar's materials were made far
darker than anything real (the mahogany bar top reflected 0.6% of light;
real mahogany reflects 4-12%), which is why lamps used to look bright only at
their core.
- `python3 tools/albedo_audit.py` measures every material of every model
  (the average of its colour texture, in linear light) and writes
  `assets/models/albedo.json`.
- `assets/models/albedo_rules.json` lists real ranges by material name
  (white plaster 0.45-0.8, brick 0.15-0.35, mahogany 0.04-0.12, black paint
  0.025-0.06, ...). A material outside its range is moved to the nearest edge,
  keeping its colour: 88 of 422 materials, in the new lighting only.
- `node tools/albedo_test.mjs` (in the checks) makes sure every model is measured.
- After changing or adding a model: run the audit again.

### Bounced light: how it works (L1.4, the core)
Light hits a surface, part of it bounces off (as much as the surface's
albedo says), lights the next surface, and so on: that is why one bulb
lights a whole room. The bar works it out like this:
1. **The place's still surfaces** (every model that doesn't move; glasses
   and darts are left out) are gathered and simplified (corners within 5 cm
   merged: 642 k → 99 k triangles; detail smaller than a patch doesn't
   change bounced light).
2. **Patches:** every surface gets a point every 25 cm (about 58 000 in the
   bar), with its colour, how much light it reflects and how much it glows.
3. **What each patch sees:** 32 rays from each patch find the patches they
   meet (a "ray tree", `looped/bvh.h`, makes that fast).
4. **Which lamps each patch sees:** a shadow ray to every lamp.
5. **Probes:** points every 50 cm in the air (about 10 000 usable in the
   bar; ones inside walls or outside the building are switched off); 48 rays
   each find what they see, and which lamps they see.
6. **Bouncing:** each round, every patch's light = its own glow + what it
   reflects of the lamps + what it reflects of the patches it sees. Each
   round is one more bounce. A closed white box ends at 5× its direct
   light, exactly as physics says (`tools/native/radiosity_test.cpp`).

### Bounced light in the game (L1.5)
- After the bar loads, the bounced light is worked out on the phone's GPU
  (the CPU gathers the surfaces and patches in the background, then the GPU
  casts about 4 million rays in a few dozen slices, one per frame). The
  first time, you **watch it spread** over about a second.
- From then on, **every frame**: one more bounce between the patches, and
  the probes refilled. Lamps changing brightness (dimmers, flicker, the
  music) change the bounced light at once.
- The flat ambient fill is gone: every pixel reads the bounced light from
  the 8 probes around it (probes inside walls and behind the surface don't
  count, so light doesn't leak through walls).
- **No lamp light through walls:** each probe knows which lamps it can see;
  a pixel's lamp light is scaled by what its probes see (soft, about 50 cm
  wide shadows; sharp shadows come in L2).
- It's built again when more rooms stream in or the Studio moves things
  (1.5 s after the last move).

| Switch / view | Page option |
|---|---|
| bounced light off (the old flat fill) | `?bounce=0` |
| bounce strength | `?bouncek=1` |
| the bounced light alone | `?giview=1` |
| probes usable (green) or not (red) | `?giview=2` |
| how much of the nearest lamp is in view | `?giview=3` |
| fewer rays (the tests' software GPU) | `?gilite=1` |

### Lights with size (L1.6)
Real lamps aren't points. A lamp can be:
- a **ball** (a bulb): its glass's radius (Softness) makes highlights rounder and softer;
- a **tube** (neon): it lights like a line of light and reflects as a **streak**;
- a **panel** (the jukebox's front, a lit sign, a screen): light from an area,
  brightest straight in front, reflected as a soft rectangle.

`node tools/light.mjs fit bar tools/migrations/l1_6_light_fit.json` finds the
glowing object beside every lamp (grouping glowing triangles that touch) and
measures it; `node tools/world_migrate.mjs l1_6_light_shapes` writes the
confident ones into the world: the bar's pendant bulbs are 16 cm balls, two
neons are tubes (1.1 m and 1.7 m), the jukebox's front a 1.3 × 0.8 m panel,
the arcade's four neon frames 2.3 m squares. Glowing surfaces that have a lamp
don't also glow into the bounced light (their light would count twice).

In the Studio: Light → Shape (ball, tube, panel), Length, Width, Lies along,
Middle.

### In the Studio (L1.7)
- **Inspect → Light:** Lumens, Colour temperature (Kelvin), Shape, Length,
  Width, Lies along change the lamp at once (its bounced light follows the
  next frame).
- **Look → moods:** Evening, Party and Closing carry lumens too: Party is
  30% brighter, Closing a third of the light and 400 K warmer.

Tools: `node tools/light.mjs gi` (build it, timed), `node tools/light.mjs
map bar 1.2 build/map.png` (a top-down map of the bounced light at 1.2 m),
`node tools/bench.mjs native` (`gi_*` numbers).

### No squares of light (L1.8)
The bar is cut into squares (1.5 m) and each pixel only adds up the lamps
its square lists. Before, a square could list only 8, and with real units
every lamp reached 16 m, so every square dropped some lamps, each a
different set: edges of light in squares. Now a square holds 16, and every
lamp's light fades out smoothly at the same faint level (it starts at 0.1
lux and rises, all lamps together, until no square is over its limit), so
no square drops a lamp. High: 12 per pixel (lamps reach down to about 0.75
lux); Medium 8; Low 5 (they fade out a little sooner). The log says
`lamps reach down to 0.75 lux` and no "too full".

### Softer highlights (L1.9)
Two things made bright parts look white instead of lit: the curve (AgX)
turns any colour white as it gets bright, and it ran out of room early.
Now the curve has 2 EV more room around middle grey (`?hiroom=0` as
before) and bright light keeps 65% of its colour (`?hicolour=0` as before,
1 = all): a lamp-lit wall stays amber.

### Calmer light (L1.10)
Seen on the phone: too bright, too much glow. The eyes aim a third of a
stop darker (exposure -1.3, `?exposure=-1` as before) and the glow is two
thirds as strong (the glow itself was taken out in J92).

## L2 Shadows and shaped light

**Lamp shadows (J97, 2026-10-26):** the 16 brightest lamps each keep a cube map of the still things around them (256² a face, 16-bit depth, drawn 12 faces a frame after loading and again when more rooms load or the Studio moves things; a lamp's own model is left out). Their light and their highlights are blocked by stools, tables and the counter; they no longer use bounced light's probe test (its 0.2 m steps were the squares of light). The other lamps (small bulbs, candles) keep the probes' test. Settings → Graphics → Lamp shadows, `?shadows=0`. Moving things (glasses, darts, people) don't cast lamp shadows yet. Still to come: lamp patterns (IES), lampshades that glow, fire that moves its shadows (J42).

**Soft shade (J91, 2026-10-26):** ambient occlusion in screen space (`looped/ssao.*`, `shaders/ssao.wgsl`). Before the main pass, the solid still things are drawn at half size into a depth and surface-direction prepass; for each pixel 12 samples in a spiral within 0.5 m say how much of the room nearby surfaces close in (Alchemy-style), a 4×4 blur follows the depth, and the main pass multiplies only the ambient and bounced light by it (never a lamp's direct light). The result also holds each pixel's distance: a surface the prepass didn't see (people, glass, moving things) gets no shade. Off with classic light and the tests' light. Settings → Graphics → Soft shade, `?ao=0`.

## L3 Eyes and camera

_Not built yet._

## L4 Reflections and caustics

_Not built yet._

## L5 The air

_Not built yet._

## L6 Light show

_Not built yet._

## L7 Living light

_Not built yet._

## L8 Paint the light

_Not built yet._

## The tests' lighting (`?light=test`)
For the automatic tests only (GitHub's machines draw without a graphics card):
classic lighting plus a flat light from above instead of the lamps, and no
bounced light, reflections, glow and mood or full mirrors. The tests check
what happens, not how it looks. Never on the phone.
