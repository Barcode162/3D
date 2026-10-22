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
| Exposure | `?exposure=-0.7` | -0.7 EV | the whole picture brighter (+) or darker (-) |
| Darkness kept | `?eyekeep=0.6` | 0.6 | 0 = every room looks equally bright, 1 = eyes never adapt |
| Reference | `?eyeref=-1.5` | -1.5 | the room brightness (log2) that shows as "normal" |
| Darkest / brightest adapted | `?eyemin=-9` `?eyemax=6` | | eyes don't adapt beyond these |
| Glow | `?glow=` `?glowth=` | the place's mood | glow strength and where it starts |
| Eyes off | `?eyes=0` | | a fixed exposure (for comparing) |

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
