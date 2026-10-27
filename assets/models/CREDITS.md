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
| `candle.glb` | `lighting.py` | none |
| `dartboard.glb`, `chalkboard.glb` | `decor.py` | Dark Wood, Wood Table 001 |
| `pint_glass.glb`, `rocks_glass.glb` | `glasses.py` | none |
| `hall.glb` (the new room shell) | `hall.py` | Herringbone Parquet, Red Brick 03, Brick Wall 006, Painted Plaster Wall, Plastered Wall 02, Dark Wood, Wood Table 001, Black Painted Planks, Concrete Floor Worn 001, Plywood, Rubber Tiles, Velour Velvet, Fabric Pattern 07 |
| `bar_counter_l.glb` (curved counter, underbar, bar-top clutter) | `bar_counter_l.py` + `props_lib.py` | Wood Table 001, Dark Wood, Leather Red 02 |
| `back_bar_tall.glb` | `back_bar_tall.py` + `props_lib.py` | Dark Wood, Marble 01, Wood Table 001 |
| `drink_*.glb` (the glasses on the tables, one model per drink since J72) | `drinks.py` + `props_lib.py` | none |
| `bottle_*.glb` (the back bar's bottles, one model per recipe since J72) | `bottles.py` + `props_lib.py` (labels: `labels.py`) | none |
| `piano.glb` | `piano.py` | Wood Table 001, Fabric Leather 02 |
| `stage.glb` (mic, guitar, amp, lights) | `stage.py` | Fabric Pattern 07, Leather Red 02, Wood Table 001, Dark Wood |
| `big_mirror.glb`, `mirror_button.glb` | `big_mirror.py` | none |
| The taped-off room's pieces (`caution_tape`, `plastic_sheet`, `sawhorse_*`, `cone_*`, `easel_sign`, `work_light`, `step_ladder`, `paint_bucket_*`, `drop_cloth`, `lumber`, `drywall`, `toolbox`; one model each since J73) | `construction.py` | Plywood, Fabric Pattern 07 |
| `sim_rig.glb` | `sim_rig.py` | Fabric Leather 02 |
| `rhythm.glb` | `rhythm.py` + `props_lib.py` | Fabric Pattern 07, Dark Wood |
| The darts corner's pieces (`board_darts`, `dart_surround`, `dart_holder`, `oche`, `dart_spotlights`, `neon_darts`; since J73) | `darts_zone.py` | Dark Wood |
| `fireplace.glb`, `candlestick.glb`, `mantel_photo_a.glb`, `mantel_photo_b.glb`, `mantel_books.glb`, `fire_tools.glb`, `log_basket.glb`, `rug.glb`, `coffee_table.glb`, `bookshelf.glb`, `floor_lamp.glb`, `side_table.glb`, `fire_flames.glb` (the lounge, one model each since J70) | `lounge.py` + `props_lib.py` | Marble 01, Dark Wood, Brick Wall 006, Wood Table 001 |
| The hall's decor, one model each since J71: `string_lights.glb`, `neon_*.glb`, `gallery_photo_*.glb`, `tin_sign_*.glb`, `license_plate_*.glb`, `record_*.glb`, `wall_clock.glb`, `wall_tv.glb`, `chalk_a_frame.glb`, `floor_plant_*.glb`, `hanging_plant_*.glb`, `corner_speaker.glb`, `wall_sconce.glb`, `fire_extinguisher.glb`, `smoke_detector.glb`, `wall_switch.glb`, `coat_rack.glb`, `umbrella_stand.glb`, `trash_can.glb`, `beer_crate.glb`, `keg.glb`, `bay_table.glb` | `decor_walls.py` + `props_lib.py` | Fabric Pattern 07, Dark Wood, Wood Table 001 |

## Poly Haven models (the remake, J35)

Whole models from [Poly Haven](https://polyhaven.com/models) (CC0), slimmed by
`tools/assets/polyhaven.py` (fewer triangles, 1k textures).

| Model | Poly Haven model | Authors |
|---|---|---|
| `pendant_lamp.glb` | [Hanging Industrial Lamp](https://polyhaven.com/a/hanging_industrial_lamp) (80% size, its glass made see-through) | Kuutti Siitonen |
| `chesterfield_sofa.glb` | [Sofa 02](https://polyhaven.com/a/sofa_02) (1.1x) | Kirill Sannikov |
| `armchair.glb` | [Green Chair 01](https://polyhaven.com/a/GreenChair_01) | Kirill Sannikov |
| `mantel_clock.glb` | [Mantel Clock 01](https://polyhaven.com/a/mantel_clock_01) | Yann Kervran, Rico Cilliers |
| `painting.glb` | [Fancy Picture Frame 01](https://polyhaven.com/a/fancy_picture_frame_01) (0.75 m high) | Rob Tuytel, Rico Cilliers |
| `vase.glb` | [Ceramic Vase 02](https://polyhaven.com/a/ceramic_vase_02) (0.27 m high) | James Ray Cock |
| `bar_stool.glb` | [Bar Chair Round 01](https://polyhaven.com/a/bar_chair_round_01) (scaled to a 0.76 m seat) | Dairon Sanchez |

Bottle labels (`tools/assets/labels.py`, J35.4): made-up brands set in DejaVu Serif and Sans (Bitstream Vera licence, public-domain changes) and Liberation Serif (SIL Open Font License); both allow text drawn into pictures.

CC0 needs no credit, but we list sources anyway.
| `arcade.glb`, `arcade_bench.glb`, the arcade's neon (`arcade_neon_*`), its signs (`arcade_sign_*`) | `arcade.py` | none (the carpet and signs are drawn by the script) |
| `arcade_cabinet.glb` | `arcade_cabinet.py` | none (the screen, marquee and side art are drawn by the script) |
| The chess pieces (`chess_*`, one model each) | `chess.py` | none |
| `credit_coin.glb` | `credit_coin.py` | none |
| The stage props as their own things since J73 (`stage_mic`, `stage_guitar`, `stage_amp`, `stage_lights`, `stage_stool`) | `stage.py` | Fabric Pattern 07, Leather Red 02, Wood Table 001, Dark Wood |
