# Materials

What surfaces are made of and how they catch light: textures, wear, dirt, layers, light passing through.

Every switch is also a page option for comparing (add it to the bar's
address, e.g. `?bounce=0`).

## M0 Real roughness
Many models were made glossier than real things: the parquet at 0.07 is a
mirror, the coats shone like plastic. `assets/models/rough_rules.json`
gives each kind of material (by name, first match wins) the least
roughness a real one has: coats, felt, rugs 1.0 (no shine at all); velvet
and other fabric 0.95; walls 0.75; leather 0.55; wood 0.45; the bar's floor
0.6 (varnish worn dull by feet); the bar top 0.28 (varnished, wiped all night); piano lacquer 0.16;
chrome 0.12; glass, liquids, screens and glowing parts as made.
`python3 tools/albedo_audit.py` writes it into `assets/models/albedo.json`
(`"rough"`), and the new lighting never draws a material smoother than
that. `?roughfix=0`: as the models were made.

## M1 Real surfaces

_Not built yet._

## M2 Layers

_Not built yet._

## M3 Light through things and real depth

_Not built yet._
