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
- **Glow:** only light brighter than white glows, softly.

| Dial | Page option | Default | What it does |
|---|---|---|---|
| Exposure | `?exposure=-1` | -1 EV | the whole picture brighter (+) or darker (-) |
| Darkness kept | `?eyekeep=0.6` | 0.6 | 0 = every room looks equally bright, 1 = eyes never adapt |
| Reference | `?eyeref=-1.5` | -1.5 | the room brightness (log2) that shows as "normal" |
| Darkest / brightest adapted | `?eyemin=-9` `?eyemax=6` | | eyes don't adapt beyond these |
| Glow | `?glow=` `?glowth=` | the place's mood | glow strength and where it starts |
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

## L2 Shadows and shaped light

_Not built yet._

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
