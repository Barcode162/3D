# Model credits

Models are built by scripts in `tools/assets/` (run in Blender, see
`docs/archive/DECISIONS.md` → Asset pipeline). All geometry is our own. Photo
textures inside them are CC0 sets from [Poly Haven](https://polyhaven.com/textures)
(public domain, tinted/resized by the scripts); everything else (labels,
posters, dartboard, menus, chalkboard) is drawn by the scripts themselves.

| Model | Script | Poly Haven textures |
|---|---|---|
| `bar_stool.glb`, `table_stool.glb` | `stools.py` | Leather Red 02, Fabric Leather 02 |
| `round_table.glb` | `furniture.py` | Wood Table 001, Dark Wood, Leather Red 02 |
| `jukebox.glb` | `jukebox.py` | Dark Wood |
| `pendant_lamp.glb`, `candle.glb` | `lighting.py` | none |
| `dartboard.glb`, `chalkboard.glb` | `decor.py` | Dark Wood, Wood Table 001 |
| `pint_glass.glb`, `rocks_glass.glb` | `glasses.py` | none |
| `hall.glb` (the new room shell) | `hall.py` | Herringbone Parquet, Red Brick 03, Brick Wall 006, Painted Plaster Wall, Plastered Wall 02, Dark Wood, Wood Table 001, Black Painted Planks, Concrete Floor Worn 001, Plywood, Rubber Tiles, Velour Velvet, Fabric Pattern 07 |
| `bar_counter_l.glb` (curved counter, underbar, bar-top clutter) | `bar_counter_l.py` + `props_lib.py` | Wood Table 001, Dark Wood, Leather Red 02 |
| `back_bar_tall.glb` | `back_bar_tall.py` + `props_lib.py` | Dark Wood, Marble 01, Wood Table 001 |
| `piano.glb` | `piano.py` | Wood Table 001, Fabric Leather 02 |
| `stage.glb` (mic, guitar, amp, lights) | `stage.py` | Fabric Pattern 07, Leather Red 02, Wood Table 001, Dark Wood |
| `big_mirror.glb`, `mirror_button.glb` | `big_mirror.py` | none |
| `construction.glb` | `construction.py` | Plywood, Fabric Pattern 07 |
| `sim_rig.glb` | `sim_rig.py` | Fabric Leather 02 |
| `rhythm.glb` | `rhythm.py` + `props_lib.py` | Fabric Pattern 07, Dark Wood |
| `darts_zone.glb` | `darts_zone.py` | Dark Wood |
| `lounge.glb`, `fire_flames.glb` | `lounge.py` + `props_lib.py` | Marble 01, Dark Wood, Brick Wall 006, Leather Red 02, Velour Velvet, Wood Table 001 |
| `decor_walls.glb` | `decor_walls.py` + `props_lib.py` | Fabric Pattern 07, Dark Wood, Wood Table 001 |

## Poly Haven models (the remake, J35)

Whole models from [Poly Haven](https://polyhaven.com/models) (CC0), slimmed by
`tools/assets/polyhaven.py` (fewer triangles, 1k textures).

| Model | Poly Haven model | Authors |
|---|---|---|
| `bar_stool.glb` | [Bar Chair Round 01](https://polyhaven.com/a/bar_chair_round_01) (scaled to a 0.76 m seat) | Dairon Sanchez |

Bottle labels (`tools/assets/labels.py`, J35.4): made-up brands set in DejaVu Serif and Sans (Bitstream Vera licence, public-domain changes) and Liberation Serif (SIL Open Font License); both allow text drawn into pictures.

CC0 needs no credit, but we list sources anyway.
