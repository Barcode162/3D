# The big bar upgrade ("1 star → 6 stars", started 2026-10-16)

The user's request (2026-10-16): rebuild the bar as a completely new,
bigger, differently shaped room, packed with the real clutter a real bar
has; plan for **3 rooms** (the bar first; the other two shown as closed off
"under construction"); one **giant 2-in-1 mirror** (full / avatars-only /
off) from the floor to almost the ceiling so many people can see
themselves; **game areas** (darts, racing sims, a 4-key rhythm machine like
osu!mania / Quaver, a **grand piano with real strings**); everything as
detailed as possible, modeled in Blender; and **loading that doesn't leave
you walking around in the dark** while models stream in.

This file is the master list. Tick items off as they land.

## Reference: what makes a real bar feel real
Studied from cozy pubs, speakeasies and game bars (Barcade-style arcades,
piano bars, sim-racing lounges). The things that sell "real":
- **Layers of light**: warm low pendants over the counter, a glowing back
  bar (bottles lit from below/behind), Edison string lights, a fireplace,
  neon signs in the game corners, screen glow, candles on every table.
  Dark corners stay dark. Never one flat light.
- **Clutter with a purpose**: everything a bartender uses sits within arm's
  reach: speed rail full of bottles, ice well, bar mats, shakers, jiggers,
  strainers, bar spoons in a cup, garnish tray with citrus, straws, napkins,
  bitters, cutting board, POS screen, tip jar, receipt printer. Customers
  leave things too: glasses, coasters, phones, peanuts bowls.
- **Walls tell a story**: framed photos, vintage beer signs, records,
  license plates, a clock, a chalkboard with today's specials, a TV, neon,
  shelves of trinkets, plants. Nothing perfectly aligned.
- **Wear and mix of materials**: worn brass, nicked wood, brick, leather,
  chalk dust, a rubber mat behind the bar, a rug in the lounge.
- **Zones** with their own mood: the bar, a lounge by the fire, a stage,
  game corners, a window seat.

## New floor plan (meters; -z = back wall, +x = right)
Main hall: x -9..9, z -7..7 (18 x 14 m, was 12 x 10), with:
- **Ceiling heights vary**: a lower wood-slat soffit over the bar (3.1 m),
  the open hall at 4.0 m with exposed black beams, ducts and pipes.
- **Chamfered corner** (back-right): a diagonal wall from (6, -7) to
  (9, -4) for the rhythm corner.
- **Bay window** in the front wall (x -2.2..1.8, 1 m deep) with a cushioned
  window seat and a night street outside.
- **Raised stage** (0.3 m, one step) in the front-left corner for the
  piano: x -9..-5.4, z 4..7. You walk up onto it.

Zones:
| Zone | Where | What |
|---|---|---|
| Bar | back wall | Curved L counter (bows into the room, returns to the back wall on the right), walk-behind gap at the left end, floor-to-ceiling back bar |
| Room 2 doorway | back wall x 1.5..3.3 | Brick arch, taped off, "coming soon" |
| Rhythm corner | diagonal wall | 4K rhythm cabinet, neon, speakers, beanbag |
| Racing sims | right wall z -3.2..0.6 | Two sim rigs with triple screens |
| Giant mirror | right wall z 1.6..6.0 | 4.4 m x 3.2 m, Full / Avatars / Off panel |
| Darts | front wall, right | Dartboard cabinet, oche line on the floor, scoreboard, high table |
| Entrance | front wall x 3..4.2 | Double door (closed), coat hooks, umbrella stand |
| Window seat | bay window | Cushioned bench, small table, plants |
| Piano stage | front-left corner | Grand piano with open lid, bench, mic stand, amp, rug |
| Lounge | left wall | Fireplace, Chesterfield sofa, two wingback armchairs, coffee table, rug, bookshelf, floor lamp |
| Room 3 doorway | left wall z -3.2..-1.6 | Plastic sheeting, tape, "under construction" |
| Middle | center | High-top tables with stools, candles |
| Jukebox | front wall, left of the bay window | The existing jukebox |

## The complete item list
### Engine work
- [ ] **Loading screen**: title, progress bar (models loaded / total), the
      room fades in only when everything is ready; "Tap to enter" (also
      unlocks sound). The room shell is requested first.
- [ ] **More lights**: up to 32 lights with a light grid (each floor cell
      lists the ≤ 8 lights that reach it), instead of 8 for the whole room.
- [ ] **More models**: raise the placement limit (128 → 512).
- [ ] **Floor heights**: walkable platforms (the stage, its step).
- [ ] **2-in-1 mirror**: one mirror with three modes, a wall panel with
      three buttons (FULL / AVATARS / OFF), shared in multiplayer.
- [ ] Blockers for curved/diagonal shapes (several rectangles).

### Room shell (`hall.py`)
- [ ] Wide-plank oak floor; dark rubber mats behind the bar; stage boards
- [ ] Red brick walls + dark wainscot with chair rail; plaster upper walls
      in the lounge; the diagonal wall; baseboards everywhere
- [ ] Ceiling: plaster, black steel I-beams, round ducts with hangers,
      sprinkler pipes with heads, junction boxes, conduit
- [ ] Wood-slat soffit over the bar with crown molding
- [ ] Bay window: frames, mullions, glass with a night city glow, sill,
      window seat with cushions and pillows
- [ ] Entrance double door with push bars, transom window, EXIT sign
- [ ] Brick arches for the two doorways + unfinished rooms behind them
      (studs, bare drywall, concrete floor)
- [ ] Stage platform with nosing on the step, stage-front trim

### The bar (`bar_counter_l.py`, `back_bar_tall.py`)
- [ ] Curved counter: paneled front with pilasters, padded leather arm
      rest edge, thick mahogany top, brass foot rail on brackets
- [ ] Bartender side: ice well, two sinks with faucets, speed rail with
      bottles, glass racks, drainboard, under-counter fridges with glass
      doors (lit), kegerator, trash bin, rubber mats, cutting board
- [ ] Beer tap tower (8 taps with different handles) + drip tray
- [ ] POS screen (glowing), cash drawer, receipt printer, tip jar, bell
- [ ] Back bar, floor to soffit: lower cabinets with doors and handles,
      counter with mirror panels, 5 lit shelf tiers, ~300 bottles
      (whiskey, gin, vodka, rum, tequila, wine, liqueurs; many shapes and
      label colors), a wine rack section, hanging stemware rack,
      a rolling library ladder on a brass rail
- [ ] Bar tools: shakers, jiggers, strainers, bar spoons in a cup, muddler,
      bitters bottles, citrus garnish tray, straw holder, napkin stacks,
      cocktail picks, a coffee machine, a blender, an ice bucket with
      a champagne bottle
- [ ] Pendant lamps over the counter (reuse), TV over the bar end
- [ ] Bar-top customer clutter: coasters, menus, napkin holders, peanut
      bowls, glasses (some pickable), candle, a phone, keys
- [ ] Bar stools (reuse) along the curve

### Piano stage (`piano.py`), the hero piece
- [ ] Grand piano case: real curved bent side, rim, gloss black lacquer
- [ ] Lid open on its prop stick, front lid fold, music desk
- [ ] Keyboard: all 88 keys (52 white, 36 black), key slip, cheek blocks,
      fallboard
- [ ] Inside: gold cast-iron plate with its holes and struts, ~230
      strings (single/double in the bass with copper winding, triple in
      the treble), tuning pins, agraffes, bridges with bridge pins,
      dampers, hammers row, spruce soundboard with ribs
- [ ] Three legs with casters, lyre with three brass pedals
- [ ] Piano bench (tufted top), sheet music on the desk
- [ ] Stage props: mic on a stand, small amp, guitar on a stand, a stool,
      floor wedge monitor, a rug, stage spot lights

### Racing sims (`sim_rig.py`)
- [ ] Aluminum profile frame (slotted extrusions, corner brackets)
- [ ] Racing bucket seat with harness slots
- [ ] Direct-drive wheel base + wheel (rim, spokes, buttons, paddles)
- [ ] Pedal set (3 pedals), sequential shifter, handbrake
- [ ] Triple monitors on a stand, glowing race scene (road, horizon, HUD)
- [ ] Headphones on a hook, cable runs

### Rhythm corner (`rhythm.py`)
- [ ] 4K rhythm cabinet: tall body, glowing marquee, big portrait screen
      showing 4 lanes with falling notes and a judgement line, 4 big lit
      square keys, start/back buttons, speaker grilles, subwoofer, coin
      door, card reader, LED edge strips, headphone jack
- [ ] Neon wall sign, speaker stacks, a beanbag, a high table

### Darts (`darts_zone.py`)
- [ ] Dartboard cabinet (reuse) with a surround ring, darts stuck in it
- [ ] Brass oche line in the floor at 2.37 m, dart holder on the wall
- [ ] Scoreboard chalkboard, chalk, eraser; spotlight; high table

### Lounge (`lounge.py`)
- [ ] Fireplace: brick surround, mantel with clock, candles, framed
      photos, books; grate with logs and glowing embers/flames; fire tools
- [ ] Chesterfield sofa, two wingback armchairs, coffee table with board
      games, books, glasses; side table; floor lamp
- [ ] Patterned rug, bookshelf full of books, a big plant

### Walls and ceiling decor (`decor_walls.py`)
- [ ] Neon signs: "OPEN", "COCKTAILS", "GAME ON", a music note, "PLAY"
- [ ] Framed photos and posters (a gallery wall), vintage tin beer signs,
      license plates, records on the wall, a wall clock
- [ ] Chalkboard menu, A-frame sign
- [ ] Edison string lights across the hall
- [ ] Shelves with trinkets, hanging and potted plants
- [ ] Speakers in the corners, EXIT signs, fire extinguisher, light
      switches, outlets, smoke detectors, AC vents
- [ ] Coat hooks with coats, umbrella stand, beer crates, a keg, trash can

### Under construction (`construction.py`)
- [ ] Yellow-black caution tape across both doorways (crossed)
- [ ] Striped sawhorse barricades, traffic cones
- [ ] "UNDER CONSTRUCTION / NEW ROOM COMING SOON" signs
- [ ] Plastic sheeting curtain (see-through), a work light on a tripod
      (glowing), a step ladder, paint buckets, drop cloth, lumber stack,
      a toolbox

### Big mirror (`big_mirror.py`)
- [ ] Slim black-and-brass frame, 4.4 x 3.2 m, from the floor to near
      the ceiling
- [ ] Control panel with three lit buttons: FULL, AVATARS, OFF

### Budgets (iPhone 15 Pro Max, 60 fps)
Models total < 25 MB; ~600k triangles in the room; textures 1k only on
big surfaces. Clutter is merged per zone into one model per material, so
hundreds of items cost a handful of draw calls.

## Later rooms (ideas, not decided)
- Room 2 (back): a club / dance room with a DJ booth, or a pool hall.
- Room 3 (left): a music studio (Stage 4b, the mini DAW) or a quiet
  library lounge. Ask the user when we get there.
