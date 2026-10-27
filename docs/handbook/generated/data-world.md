<!-- generated: node tools/handbook.mjs write (from assets/world/schema.json); never edit by hand -->
# Data: the world's files

Every field the world's files can hold, in plain words (docs/archive/STUDIO.md 7.3). The Studio's Inspect panel, the hub's Dials and the checks are all drawn from this. Types: number, int, bool, text, id, choice (from 'choices'), vec3 (x y z), color (r g b, 0..1), rect (x0 z0 x1 z1 on the floor), box (two corners), quad (four corners), range (a number or [min, max]), list (of 'item'), map (named entries of 'item'), states (state names). 'live': the Studio can change it without rebuilding the scene. tools/schema_test.mjs fails if a world file uses a field missing here.

"Live": the Studio changes it without rebuilding the scene. The files: `assets/world/places/*.json`, `kinds/*.json`, `catalog.json`, `materials.json`, `songs.json` ([the world model](../systems/engine/world-model.md)).

## Place (`place`)

A whole place: the bar, a home. Its things, where people appear, its air.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `moods` | Moods | map |  |  |  | Named sets of lights (S7.3): by kind or by thing, their colour and brightness. |
| `mood` | Mood now | text |  |  |  | Which of the moods the place uses. |
| `id` | Name | id |  |  |  | The place's name in the files and the menu. |
| `about` | About | text |  |  |  |  |
| `kinds` | Kind files | list |  |  |  | Which kind files this place uses. |
| `spawn` | Where people appear | object |  |  |  |  |
| `air` | Air | object |  |  |  | The room's ambient light and haze. |
| `lightGrid` | Light grid | object |  |  |  | The box the baked light covers. |
| `rooms` | Rooms | list |  |  |  | Boxes that stream their models in when you're near. |
| `doorways` | Doorways | list |  |  |  |  |
| `areas` | Areas | map |  |  |  | Named boxes the game asks about (is someone in the arcade?). |
| `points` | Points | map |  |  |  | Named spots (where a door is). |
| `things` | Things | list |  |  |  |  |

## Thing (`thing`)

One thing in a place: what kind it is and where it sits.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `id` | Name | id |  |  |  | Never changes once players have seen it (the network uses it). |
| `kind` | Kind | kindRef |  |  |  |  |
| `parent` | Sits on | thingRef |  |  |  | Move that one and this one follows. |
| `at` | Where | vec3 | m |  | yes | On its parent (or in the place). |
| `turn` | Turn | number | ° | -180 … 180 (step 15) | yes | Around the up axis. |
| `scale` | Size | range | × | 0.05 … 10 (step 0.05) | yes |  |
| `phase` | Loop offset | number |  | 0 … 1 (step 0.05) | yes | Starts its loops this far in, so copies don't move in step. |
| `seed` | Seed | int |  | 0 … 99999 | yes | Its own randomness (noise, flicker). |
| `state` | Starts in | stateRef |  |  |  |  |
| `row` | Row | object |  |  |  | Copies in a line. |
| `loops` | Its own loops | map |  |  |  |  |
| `parts` | Its own parts | parts |  |  |  | Overrides the kind's parts. |

## Kind (`kind`)

What a thing is: its parts, states, loops and rules. Every thing of a kind shares them.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `id` | Name | id |  |  |  |  |
| `about` | About | text |  |  |  | What it does, in plain words (shown in the hub). |
| `is` | Builds on | kindRef |  |  |  | Starts from another kind and changes some of it. |
| `still` | Stands still because | text |  |  |  | Only if it has no loop: why. |
| `pivot` | Pivot | vec3 | m |  | yes | The point its loops turn and scale around (a lamp's hook). |
| `start` | Starts in | stateRef |  |  |  |  |
| `parts` | Parts | parts |  |  |  |  |
| `loops` | Loops | map |  |  |  |  |
| `states` | States | map |  |  |  |  |
| `on` | Rules in every state | map |  |  |  |  |
| `things` | Things inside it | list |  |  |  |  |

## State (`state`)

What a thing is doing now, the loop it plays and its rules.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `loop` | Plays | loopRef |  |  |  |  |
| `on` | Rules | map |  |  |  | When this event comes → go to that state. |
| `after` | After seconds | number | s | 0 … 600 (step 0.1) |  |  |
| `afterLoops` | After loops | int |  | 1 … 100 |  |  |
| `next` | Then | stateRef |  |  |  |  |
| `free` | Moves freely | bool |  |  |  | Physics or a player moves it (a held glass). |
| `enter` | On entering, tell | list |  |  |  |  |

## Loop (`loop`)

A short movement that repeats forever on the shared clock.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `period` | Length | number | s | 0.05 … 120 (step 0.05) | yes | Seconds per loop (beats when locked to the music). |
| `beat` | Locked to the music | bool |  |  | yes |  |
| `restart` | Starts over | bool |  |  | yes | Starts from the beginning when its state begins (actions). |
| `desync` | Own timing | bool |  |  | yes | Each copy starts at its own point. |
| `blend` | Blend in | number | s | 0 … 3 (step 0.05) | yes |  |
| `pieces` | Lanes | list |  |  |  |  |

## Lane (`piece`)

One channel of a loop: what moves, and how.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `ch` | What moves | choice: channel |  |  | yes |  |
| `shape` | Shape | choice: shape |  |  | yes |  |
| `amp` | Amount | range |  | -10 … 10 (step 0.01) | yes | Metres, degrees or a fraction, by channel. |
| `value` | Value | range |  | -10 … 10 (step 0.01) | yes |  |
| `offset` | Offset | range |  | -10 … 10 (step 0.01) | yes |  |
| `phase` | Start point | number |  | 0 … 1 (step 0.05) | yes |  |
| `rate` | Speed | number | × | 0.05 … 20 (step 0.05) | yes |  |
| `decay` | Fade | number |  | 0 … 30 (step 0.5) | yes | How fast a beat thump dies away. |
| `every` | Every | int |  | 1 … 32 | yes | Beats between thumps. |
| `ease` | Between keys | choice: ease |  |  | yes |  |
| `keys` | Keys | keys |  |  | yes | [time 0..1, value] points. |
| `values` | Steps | list |  |  | yes |  |

## Catalog (`catalog`)

Every kind you could own.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `about` | About | text |  |  |  |  |
| `items` | Items | list |  |  |  |  |

## Materials (`materials`)

What things are made of (S7.1): look, feel and sound. Plain boxes are painted with them.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `about` | About | text |  |  |  |  |
| `materials` | Materials | list |  |  |  |  |

## Parts (`parts`)

The abilities a kind (or a thing) can have; each engine system reads its own.

### Model (`model`)

The 3D model it shows.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `file` | File | model |  |  |  |  |
| `material` | Main material | choice: material |  |  |  |  |

### Boxes (`boxes`)

Plain shapes, drawn and solid: floors, walls, shelves.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `at` | Middle | vec3 | m |  |  |  |
| `size` | Size | vec3 | m |  |  |  |
| `material` | Material | choice: material |  |  |  |  |
| `color` | Colour | color |  |  |  |  |
| `solid` | Solid | bool |  |  |  | Off: you walk through it (a rug). |

### Shapes you bump into (`solids`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `at` | Middle | vec3 | m |  |  |  |
| `half` | Half size | vec3 | m |  |  |  |
| `turn` | Turn | number | ° | -180 … 180 (step 5) |  |  |
| `material` | Sound and feel | choice: material |  |  |  |  |

### Walk blockers (`blocks`)

Floor rectangles you can't walk into (world axes).


### Floors (`floors`)

Raised floor rectangles (world axes): x0 z0 x1 z1 height.


### Seats (`seats`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `name` | Name | text |  |  |  |  |
| `hip` | Where you sit | vec3 | m |  |  |  |
| `facing` | Facing | number | ° | -180 … 180 (step 5) |  |  |
| `low` | How low | number |  | 0 … 1 (step 0.05) |  |  |
| `kind` | Kind of seat | choice: seatKind |  |  |  |  |
| `lights` | Glows when you're near | bool |  |  |  |  |
| `machine` | Machine | int |  | -1 … 9 |  | Which racing rig (-1: none). |

### Tap areas (`uses`)

Where a tap does something (world axes).

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `name` | Name tag | text |  |  |  |  |
| `prompt` | Prompt | text |  |  |  |  |
| `action` | Does | choice: action |  |  |  |  |
| `tag` | Name tag spot | vec3 | m |  |  |  |
| `box` | Tap box | box | m |  |  |  |
| `direct` | Only when looked at directly | bool |  |  |  |  |
| `lights` | Glows when you're near | bool |  |  |  |  |
| `mirror` | Mirror | thingRef |  |  |  |  |
| `value` | Value | int |  | 0 … 99 |  |  |

### Light (`light`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `color` | Colour | color |  |  | yes |  |
| `kelvin` | Colour temperature | number | K | 0 … 12000 (step 50) | yes | How warm the light is: 1800 a candle, 2700 a warm bulb, 6500 daylight (0 = the colour alone). The colour then filters it (white = none). |
| `lumens` | Lumens | number | lm | 0 … 20000 (step 10) | yes | How much light, as on a bulb's box: a candle 12, a 40 W bulb 400, a 100 W bulb 1600 (0 = from Brightness). |
| `intensity` | Brightness (classic) | number |  | 0 … 30 (step 0.1) | yes |  |
| `range` | Reach (classic) | number | m | 0.1 … 30 (step 0.1) | yes |  |
| `radius` | Softness | number | m | 0 … 2 (step 0.01) | yes |  |
| `shape` | Shape | choice: lightShape |  |  | yes | A ball (a bulb), a tube (neon) or a panel (a screen, a lit sign), L1.6. |
| `length` | Length | number | m | 0 … 10 (step 0.01) | yes | A tube's length, a panel's first side. |
| `width` | Width | number | m | 0 … 10 (step 0.01) | yes | A panel's second side. |
| `axis` | Lies along | vec3 |  |  | yes | The direction of a tube or a panel's first side (on the thing). |
| `axis2` | Second side along | vec3 |  |  | yes |  |
| `center` | Middle | vec3 | m |  | yes | Where the tube or panel's middle is, from the thing. |
| `dims` | Dims with the switch | bool |  |  |  |  |
| `wall` | On a wall | bool |  |  |  | A panel on a wall (a sign, a sconce): it shines only from its front, the side its first side x second side points to (J113). |

### Glow (`glowbox`)

A small glowing thing (moths, sparks, smoke).

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `color` | Colour | color |  |  | yes |  |
| `glow` | Glow | number |  | 0 … 10 (step 0.1) | yes |  |
| `size` | Size | range | m | 0.001 … 1 (step 0.001) | yes |  |
| `shy` | Shy | number |  | 0 … 2 (step 0.05) |  | Darts away from you when you come close. |

### Sound (`sound`)

A sound it makes by itself, near it only.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `effect` | Sound | choice: effect |  |  |  |  |
| `every` | How often | range | s | 0.05 … 120 (step 0.05) |  |  |
| `gain` | Loudness | number |  | 0 … 2 (step 0.05) |  |  |
| `near` | Heard from | number | m | 0.5 … 40 (step 0.5) |  |  |
| `while` | Only while | states |  |  |  |  |

### Touch sound (`touchSound`)

The sound when you tap it.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `effect` | Sound | choice: effect |  |  |  |  |
| `gain` | Loudness | number |  | 0 … 2 (step 0.05) |  |  |

### Speaker (`speaker`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `music` | Plays | choice: music |  |  |  |  |
| `at` | Where | vec3 | m |  |  |  |

### Mirror (`mirror`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `center` | Middle | vec3 | m |  |  |  |
| `normal` | Faces | vec3 |  |  |  |  |
| `right` | Right | vec3 |  |  |  |  |
| `up` | Up | vec3 |  |  |  |  |
| `halfWidth` | Half width | number | m | 0.1 … 10 (step 0.01) |  |  |
| `halfHeight` | Half height | number | m | 0.1 … 10 (step 0.01) |  |  |
| `mode` | Shows | choice: mirrorMode |  |  |  |  |
| `buttons` | Its buttons | list |  |  |  |  |

### Screen (`screen`)

A flat screen's four corners.


### Opens (`project`)

The game or project this thing opens.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `id` | Project | id |  |  |  |  |
| `name` | Name | text |  |  |  |  |
| `about` | About | text |  |  |  |  |

### Dartboard (`dartboard`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `face` | Board middle | vec3 | m |  |  |  |
| `oche` | Throwing line | vec3 | m |  |  |  |

### Chessboard (`chessboard`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `center` | Middle | vec3 | m |  |  |  |
| `square` | Square size | number | m | 0.01 … 0.2 (step 0.001) |  |  |

### Chess piece (`chess`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `piece` | Piece | int |  | 0 … 31 |  |  |

### Drink spot (`drink`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `slot` | Spot | int |  | 0 … 7 |  |  |

### Bartender spot (`bartender`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `stand` | Stands at | vec3 | m |  |  |  |

### Racing rig (`rig`)



| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `machine` | Rig number | int |  | 0 … 9 |  |  |

### Room (`room`)

Which streaming room its model belongs to.


### Prop (`prop`)

What a hand holds when this is picked up.


### Piano (`piano`)

This is the piano you can play.


### Credit coin (`coin`)

The coin that pops out when you earn.


## Songs (`songs`)

The built-in songs' tempo, by file.

| Field | Label | Type | Unit | Range | Live | What |
|---|---|---|---|---|---|---|
| `bpm` | Tempo | number | bpm | 30 … 300 (step 0.1) |  |  |
| `firstBeat` | First beat | number | s | 0 … 30 (step 0.01) |  |  |

## Choice lists (`choices`)

| List | Choices |
|---|---|
| `material` | `oak-floor`, `dark-wood`, `varnished-wood`, `door-wood`, `brick`, `plaster`, `leather`, `chrome`, `brass`, `black-steel`, `bottle-glass`, `lacquer`, `glow-glass` |
| `lightShape` | `ball`, `tube`, `panel` |
| `action` | `none`, `jukebox`, `mirror`, `mirror-mode`, `rhythm`, `darts`, `bartender`, `drink`, `bell`, `guitar`, `mic`, `light-switch`, `fireplace`, `door`, `tv`, `books`, `chess`, `tip-jar`, `taps`, `coffee`, `piano-songs`, `arcade` |
| `seatKind` | `stool`, `chair`, `sofa`, `piano-bench`, `sim` |
| `prop` | , `pint`, `rocks` |
| `effect` | `bell`, `thunk`, `clink`, `pour`, `shake`, `click`, `engine`, `whoosh`, `knock`, `cheer`, `crackle`, `buzz`, `bong`, `wood` |
| `channel` | `move`, `move.x`, `move.y`, `move.z`, `turn`, `turn.x`, `turn.y`, `turn.z`, `scale`, `scale.xyz`, `scale.x`, `scale.y`, `scale.z`, `glow`, `light`, `alpha`, `tint`, `tint.r`, `tint.g`, `tint.b` |
| `shape` | `hold`, `wave`, `keys`, `steps`, `noise`, `flicker`, `orbit`, `beat`, `spin` |
| `ease` | `smooth`, `linear`, `step` |
| `mirrorMode` | `full`, `avatars`, `off` |
| `music` | `jukebox`, `arcade` |
| `sendTo` | `all`, `parent`, `children` |
