# Bar3D gallery

Every piece of work, newest first: what it is, its before/after pictures
(left before, right after) and its numbers. Made by `tools/piece.mjs`;
benchmarks by `tools/bench.mjs` (see `bench.jsonl`).

Latest benchmark (2026-10-22 18:44, 922ec64): gi_triangles_full 642040 tris, gi_triangles 99105 tris, gi_load_ms 167 ms, gi_simplify_ms 28 ms, gi_bvh_ms 239 ms, gi_patches 58292, gi_probes_usable 10128, gi_build_ms 6666 ms, gi_rays_per_s 0.59 M/s, gi_solve4_ms 12.6 ms

## J109.1 Books turn their pages with a curl: the page lifts and bends over, its text bending with it (five strips of the page in a chain, each turning more toward the free edge, shaded as it turns); a swipe from the left or right edge drags the page with your finger (past a third or a flick turns it, less falls back; a tap on the edge turns too); taps on the sides and swipes play the turn by themselves; Reduce Motion keeps the plain slide

`3d99d24` · 2026-10-27 05:39 · 5 files, +133 −10 · tests: none

Phone check: Open a book, swipe from the right edge: the page curls over with your finger

## J115.2 The string lights' bulbs glow white (the model's Edison glow was orange: string_lights.glb's bulb material and tools/assets/decor_walls.py, so a rebuild keeps it)

`e2dc252` · 2026-10-27 05:34 · 4 files, +3 −3 · tests: none

Phone check: The bulbs look white

## J28.2 The chess picker: on a fresh game and from New game, Who do you want to play? lists the five bots (a face, a line about how each plays, its rating) and A person (1v1); picking one starts the game; the line under the status names your opponent; the chess test checks the picker (six choices, every rating) and picking a bot, then plays e4 on the 3D board as before

`291d514` · 2026-10-27 05:34 · 5 files, +74 −9 · tests: none

Phone check: Open chess: pick Rosa, the game starts against her

## J28.1 Chess bots with a strength you can see: Pip (about 400 Elo, looks one move ahead and plays a random move 40 % of the time), Rosa (800), Theo (1200), Vera (1500), Odin (1800, four moves deep); the status names who's thinking; a new game takes a bot or waits for a person (1v1); the board opens full screen; plus the batch's tools: the plan's absorb command (a road job joins the job in Now) and the rule One ship for everything

`8e66ce6` · 2026-10-27 05:33 · 9 files, +126 −48 · tests: none

Phone check: Sit at the chessboard: the status says which bot is thinking

## J115.1 The string lights at 5000 K, cool industrial white near daylight (were 2700 K)

`8d081c5` · 2026-10-27 05:18 · 6 files, +46 −37 · tests: none

Phone check: The strands light the hall white, not warm

## J113.3 (fix) The Handbook's generated map rewritten after the merge with J114 (the new light rules folder counted)

`9f6c581` · 2026-10-27 04:20 · 3 files, +5 −5 · tests: none

## J114.1 The Studio parked: the bar plays the world built into the game (the store's live world, and its 15 s watch, only with ?liveworld=1), so a world change is edited in assets/world and goes live with the push; a world change runs the build, native and game checks, not the Studio's; the live world test keeps its 5 checks and adds one (without the option the store is left alone); the Playbook and Handbook say so; the Studio's jobs (J53, J55, J57, J80) parked until the engine works

`0010f40` · 2026-10-27 03:58 · 13 files, +118 −97 · tests: none

Phone check: The bar loads as before; its log says World: built in.

## J113.2 (fix) The ARCADE sign's light keeps its name, light-sign, and its kind holds the panel on the sign's tube (the Studio's test turns that kind's colour red, live)

`f683c9f` · 2026-10-27 03:01 · 3 files, +17 −24 · tests: none

## J113.3 (fix) The record put right: J113.3 changed nothing (the eyes stay at 55 % toward white); 75 % turned the bar grey in side-by-side pictures, and the whiter bulbs of J113.2 already brought it close to the user's screenshot

`329b4cd` · 2026-10-27 02:32 · 1 files, +1 −1 · tests: none

## J113.3 Less orange: the eyes correct the light's colour 75 % of the way to white (was 55 %), so warm bulbs read warm-white, closer to the user's screenshot

`25385e2` · 2026-10-27 02:30 · 2 files, +2 −2 · tests: none

Phone check: The bar reads warm-white, not orange

## J113.2 Only real objects give light: 11 lights where nothing glowed are gone (the arcade's mid-air panels and ceiling light, a 288 lm white light by the stage speaker, a lone sign light); lights now sit on every neon sign, sconce and screen and along the arcade's neon strips (30 wall panels shining forward; four of the strips keep the arcade's wave, the ARCADE sign's light its buzz); lights a little off their source moved onto it (back-bar shelves, the fire, 4K Mania). New preflight rule R-LIGHT-1: every light within 0.6 m of a glowing surface (it flags the old file's 10 strays). String lights 2700 K and ~10 lm a bulb (were 2200 K, 258 lm a strand), pendants and back bar 2700 K; thinner haze (fog 0.012 to 0.008, beams 0.014 to 0.008)

`f3204ef` · 2026-10-27 02:30 · 6 files, +1030 −351 · tests: none

Phone check: Walk the bar and the arcade: every light comes from something glowing; the haze lighter

## J113.1 Wall lights and enough shadow maps: a light can be a panel on a wall that shines only forward (nothing through the wall, and no test against the bounced light's probes, whose 0.5 m steps drew square patches); 40 lamp shadow maps (was 16), so every lamp in the room gets one; the schema's light gains 'On a wall'

`c63aa3a` · 2026-10-27 02:28 · 12 files, +89 −25 · tests: none

Phone check: Look at the darts ceiling and the floor: no square patches

## J112.1 The preflight checklist (node tools/preflight.mjs): plain rules, one file each in tools/preflight/rules/<area>/ from a copy-paste template, run first by the piece and ship tools; a broken rule prints its id, the spot and the fix. First 12: world ids, live loops, numbers and kinds, models present; Handbook pages with page changes, folder lines; model and sound credits; twin numbers in C++ and shaders; no secrets, no test losing checks; the plan only through its tool. It found 28 models without credits lines: added

`7ccec08` · 2026-10-27 01:24 · 26 files, +435 −35 · tests: none

## J111.3 (fix) The quality log keeps its wording (Quality: Low (1.3x), setting), the lamps a pixel after it: the menu test reads it

`67fa65a` · 2026-10-27 00:52 · 1 files, +1 −1 · tests: none

## J111.3 Power in Settings → Graphics: Cooler (30 pictures a second: half the work, a cooler phone), Balanced, EXTREME (never eases: the sharpest every time)

`d2aba95` · 2026-10-27 00:26 · 4 files, +6 −4 · tests: none

Phone check: Settings → Graphics → Power: Cooler, then play a while: the phone stays cooler; EXTREME: always 3x

## J111.2 Ultra and Auto watch the pictures drawn, not just the clock: under 90 % drawn (the GPU still busy) is behind, and Ultra eases its sharpness 3x, 2.5x, 2x, then its lamps a pixel 16, 12, 10, back when 97 % draw again; the engine side of Power too (Cooler expects half the pictures, EXTREME never eases)

`22652e6` · 2026-10-27 00:26 · 5 files, +104 −17 · tests: native

Phone check: On Ultra walk around, send a report: nearly every frame drawn; the log says what eased

## J111.1 A finer light grid: 48 x 48 cells (about 0.5 m, was 16 x 16 at 1.5 m) so a pixel goes through fewer lamps (Ultra: 8.0 to 6.8 on average, the same reach and picture); at High and below the lamps reach further for the same cap (High: down to 2.27 lux, was 3.55); the log names the average

`392e4b0` · 2026-10-27 00:21 · 9 files, +83 −30 · tests: none

Phone check: Ultra looks the same; on High the far corners are a little brighter

## J110.3 (fix) Touch delay measured (the user: touches feel delayed though no frame is late): reports count the frames that drew a picture (while the GPU is busy the clock ticks at 60 but the screen doesn't change) and time each touch to the screen (to its frame, to the hand-over, to the GPU's done); the GPU check adds a step with 1 frame queued instead of 2 and counts drawn frames per step (25 s now)

`205ac39` · 2026-10-26 23:43 · 9 files, +78 −18 · tests: none

Phone check: Walk and look around for 10 s, send a report; then Check the GPU and send another

## J110.3 (fix) Frames drawn under a menu are left out: the reports, the hitch count and Ultra/Auto's watching skip them (report WQUMXC: with Settings open the page's own drawing held frames at 25 ms and eased Ultra to 2x; in the bar itself, standing, no frame was late); the profiler keeps 600 frames so a report still has 300 of play

`5d6c6bb` · 2026-10-26 23:35 · 5 files, +17 −7 · tests: none

Phone check: Open Settings for a while, then close it: Ultra stays at 3x; a report sent from the menu shows the frames before it opened

## J110.3 Ultra and Auto keep it smooth: both step by late frames (over 6 % of frames past 20 ms, or slow, while the CPU is idle), not by the GPU wait; Ultra keeps every feature and eases only its sharpness 3x to 2.5x to 2x while frames run late, back to 3x after 6 steady seconds; Auto can climb back up again

`ee94e60` · 2026-10-26 23:09 · 5 files, +88 −37 · tests: native

Phone check: On Ultra, walk around the bar: smooth; the log says when it eases (Quality: Ultra (2.5x), frames ran late)

## J110.2 Lighter lighting, the same picture: on rough surfaces a lamp adding under half a percent of the light already there is skipped (its shadow and visibility work too), and a lamp under a twentieth takes one shadow look instead of five; before/after pictures at three spots differ only where things move (the swaying pendant, candle flicker)

`0b0cb64` · 2026-10-26 23:05 · 3 files, +20 −5 · tests: none

Phone check: The bar looks the same; fewer stutters

## J110.1 The GPU check measures what you feel: each step's late frames (over 20 ms) and slowest 5 %, beside the wait (a frame's hand-over to the phone saying done, which counts the frames queued ahead: not the chip's own time)

`fb7283c` · 2026-10-26 22:58 · 6 files, +120 −33 · tests: none

Phone check: Stand where it stutters most, Settings → Help Claude fix lag → Check the GPU, then send a report

## J108.3 (fix) the entity id list in its new order: the lounge rework moved three lounge chairs after the sofas (same ids, so nothing changes for players)

`87a2348` · 2026-10-26 22:24 · 1 files, +3 −3 · tests: none

## J108.3 Light rising from the piano's strings: sparks zigzag up from each note and fade, and about every third note shoots one streak far up toward the ceiling

`b479aee` · 2026-10-26 22:03 · 4 files, +68 −2 · tests: none

Phone check: Play or start a song: sparks rise from the strings in zigzags, some streak high

## J108.2 A new, bigger piano keyboard: big shaded keys filling the bottom of the screen, a strip of all 88 keys to slide along, slide across keys to glide, pianissimo to fortissimo by where you press, Songs / Pedal / Stand buttons

`0f05e33` · 2026-10-26 22:03 · 7 files, +198 −81 · tests: none

Phone check: Sit at the piano: big keys at the bottom; drag the thin strip above them to move along; press near the top of a key for soft, near the bottom for loud

## J108.1 The piano's notes back at the piano: its lit keys, glowing strings and sound had lost their place in the live world (shown mid-room); lighter samples that carry across the bar

`61c77a5` · 2026-10-26 22:02 · 126 files, +16 −10 · tests: none

Phone check: Play a song and walk away: the glow stays over the piano's keys and strings, and you still hear it across the room

## J107.2 The GPU check: Settings → Help Claude fix lag → Check the GPU (20 s): the bar draws as it is, then with soft shade, light beams, lamp shadows, bounced light off one at a time, with 6 lamps a pixel and with 1x pixels, measures each, and puts the results in the next phone report, so the biggest costs show by name

`be3b0ed` · 2026-10-26 21:27 · 10 files, +151 −5 · tests: none

Phone check: Settings → Check the GPU, wait 20 s for the toast, then Send report and send me the code

## J107.1 Quality in four levels: Ultra (3x pixels, 16 lamps a pixel, beams 24 steps), High (2x, 12, 16), Medium (1.6x, 10, 10, simple mirror), Low (1.3x, 8, no soft shade or beams, simple mirror, lighter bounced light); lamp shadows stay on everywhere (without them the hanging lamps darken the floor: KI-46); Auto moves between Low and High; each feature's own switch can still turn it off

`71d8b24` · 2026-10-26 21:23 · 11 files, +132 −56 · tests: none

Phone check: Settings → Graphics → Quality: Auto, Ultra, High, Medium, Low; Low runs smooth; Ultra is the sharpest

## J74.1 The user's notes from the phone: a little less haze in the light beams (0.02 → 0.014); the highlight on the thing you can tap softer (0.35 → 0.12); the string lights (four strands as twelve tube lights, ~25 lm a bulb), the mirror's vanity bulbs (one 480 lm tube) and the stage and darts spotlights (their own small lamps) give their own light in place of six invisible helper lamps; Settings → Pointer: a crosshair that taps what it points at, with an editor (shape, size, thickness, gap, colour, opacity, outline) whose preview docks at the top while you scroll through it

`b04cdd4` · 2026-10-26 10:43 · 14 files, +446 −57 · tests: none

Phone check: Look up at the string lights and the mirror: they light their surroundings; stand at something: its glow is gentle; Settings → Pointer: turn the crosshair on, tap anywhere: it uses what the centre points at

## J73.2 The stage, the darts corner, the taped-off room and the arcade taken apart: the mic, guitar, amp, light bar and stool; the darts in the board, the surround, the holder, the oche and mat, the spotlights; the tape, plastic sheet, sawhorses, cones, sign, work light, ladder, buckets, drop cloth, lumber, drywall and toolbox; the arcade's signs, neon art and bench: 36 things where they stood (asset scripts mark groups, one model each); the DARTS neon reads the right way (it was backwards), the sawhorse lamps sit on their rails

`5c841ab` · 2026-10-26 10:30 · 54 files, +1182 −303 · tests: none

Phone check: Walk the stage, the darts corner, the taped-off room and the arcade: everything where it was; the DARTS sign reads normally

## J73.1 Fix: the deploy refused J72's place file (the lying wine bottles turned about three axes; a thing's turn is one angle): tilted bottles get models of their own, sizes are one number; the world check now also runs the schema and material tests; incident recorded

`dd40f9c` · 2026-10-26 10:15 · 15 files, +948 −744 · tests: none

Phone check: The bar loads with every bottle in place; the wine rack's bottles lie with their necks out

## J72.2 The glasses on the tables as real things: the drinks on the coffee table, the side table, the bay table and the bar top (14, from 9 drink models: coupe, wine, martini, highball, rocks with ice, snifter, pint) each their own glass thing where it stood, and the coffee table's bottle a bottle thing

`9cb13e3` · 2026-10-26 10:09 · 26 files, +391 −103 · tests: none

Phone check: The drinks on the tables look as before; in the Studio, tap a glass and move it

## J72.1 The back bar's bottles as real things: 275 bottles each its own thing, copies of 23 bottle recipes (shape, glass, own label, drink; 9 MB for all) where they stood, each turned and sized a little differently; the wine rack now shows its bottles (they lay neck-first into the wall) and is full to the top

`f618a04` · 2026-10-26 10:04 · 37 files, +3424 −69 · tests: none

Phone check: Look along the back bar: the bottles look as before, the wine rack is full of bottles; in the Studio, tap a bottle and move it

## J100.2 The Handbook's folder map names assets/piano

`a70619d` · 2026-10-26 09:39 · 4 files, +13 −26 · tests: none

Phone check: Nothing to see in the bar

## J100.1 A real piano: the Salamander Grand Piano's recorded notes (Alexander Holm, CC BY 3.0) replace the synth: 30 notes in 4 loudness layers (7.5 MB), loaded when someone plays, the synth until each arrives; the engine can play sounds from files; Help → Credits names it

`b24a760` · 2026-10-26 09:39 · 135 files, +182 −16 · tests: none

Phone check: Sit at the piano and play soft and hard: it sounds like a real grand; someone else playing sounds the same

## J101.1 Chess full screen: a FULL SCREEN button at the lounge board lays it flat over the game like the chess sites (status, board with the last move, move dots and the check glow, Leave, New game, 3D view), in six calm themes (Walnut, Sage, Dusk, Coral, Slate, Night) that set the squares, the pieces (classic or round, our own drawings) and the background together; the theme is kept

`64e0cd7` · 2026-10-26 09:24 · 10 files, +328 −9 · tests: none

Phone check: Sit at the chess table and play: tap full screen; the board fills the screen; change the theme: board, pieces and background change together

## J99.1 Real books: the lounge bookshelf opens a reader with six public-domain works (Crime and Punishment, The Brothers Karamazov and Notes from Underground in Constance Garnett's English, Einstein's Relativity, Moby-Dick, The Great Gatsby): pages to tap or swipe, contents, text size, light or dark paper, your place kept

`f13aaed` · 2026-10-26 09:07 · 18 files, +433 −16 · tests: none

Phone check: Tap the bookshelf: pick Crime and Punishment, read, turn pages, close it; open it again: you're on the same page

## J102.1 The fire drawn by a shader: a flame volume over the logs (rising noise, tongues of different heights, red to yellow by heat, see-through where thin) instead of four moving pictures; the engine gets a frame clock and a 'flame' material flag; the fire thing stands at the flames' base

`83fb722` · 2026-10-26 08:50 · 17 files, +188 −45 · tests: none

Phone check: Stand by the fire: the flames move and change like a real fire, no flat pictures; tap it: the fire goes out to glowing embers

## J71.2 The walls' props taken apart: string lights, four neon signs, seven gallery photos, tin signs, licence plates, records, the soffit clock and TV, the chalk A-frame, six plants, corner speakers, sconces, extinguishers, smoke detectors, switches, the coat rack, umbrella stand, trash can, beer crates, the keg and the bay table each their own thing where they were (61 things from 43 models); decor_walls.glb gone; the clock face is round (its square corners stuck out), the records lie flat on the wall, the top crate sits on the ones below

`c76ce13` · 2026-10-26 08:23 · 54 files, +2099 −843 · tests: none

Phone check: Walk around the walls: every sign, photo and plant is where it was, nothing floats; the clock over the bar is round; in the Studio, tap a sign and move it

## J71.1 Fix (urgent): the lounge bookshelf's thing renamed lounge_bookshelf (it took the id of the bookshelf's tap spot, so 'read a book' was dropped), and every new lounge kind gets a live loop (embers breathe, candle flames flicker, the floor lamp's bulb breathes, the rest a faint shimmer); incident recorded

`f971219` · 2026-10-26 06:53 · 7 files, +123 −28 · tests: none

Phone check: Tap the bookshelf in the lounge: 'read a book' works; the log shows no 'World:' problems

## J70.2 The floor lamp's shade is held by three brass arms (it hung in the air, 11 cm from its pole: found by the place check)

`3a43f37` · 2026-10-26 06:16 · 2 files, +6 −0 · tests: none

Phone check: By the armchair at the back: the lamp shade sits on its pole

## J70.1 The lounge taken apart: the fireplace, mantel clock, candlesticks, photos, books, vase, painting, fire tools, log basket, rug, coffee table with the chessboard, Chesterfield sofa, two armchairs, bookshelf, floor lamp and side table each their own thing where they stood (sofa, armchairs, clock, painting and vase from Poly Haven); lounge.glb gone; the vase, books and photos that floated now sit on the mantel; the counter's ice scoops lie on the ice

`49bb635` · 2026-10-26 05:56 · 32 files, +936 −544 · tests: none

Phone check: Stand by the fire: everything is where it was, the vase and books sit on the mantel; sit on the sofa and an armchair; chess still plays; in the Studio, tap the vase and move it

## J35.5 The pendant lamps over the counter: Poly Haven's hanging industrial lamp (CC0) at 80% size on its chain, its glass see-through, their light moved down into the bulbs (2.25 → 2.11 m); the kit learnt hanging models and glass

`4bd8356` · 2026-10-26 05:10 · 10 files, +43 −63 · tests: none

Phone check: Under the lamps over the counter: real metal shades, the light comes from the bulbs

## J35.4 The back bar remade: bottles of see-through tinted glass with the drink inside (whiskey, wine, liqueurs; clear spirits show the glass alone), sharp printed labels of 16 made-up brands, the shelf light strips solid from post to post and softer; the counter's fridge and speed-rail bottles match

`b356290` · 2026-10-26 05:03 · 12 files, +183 −20 · tests: none

Phone check: Look at the back bar: bottles look like glass with drinks in them, labels are sharp, the shelf lights are solid lines

## J35.3 The counter remade: a round left end the arm rest and the brass foot rail wrap around (no step, no gap), rounded bar-top edges, moldings along the whole front with boards on the turn, one brushed-steel top per station along the curve with real recessed sinks and ice wells (nothing at the same depth, no seams), fridges you can see into; 75k triangles (was 79k)

`af463ca` · 2026-10-26 04:55 · 7 files, +195 −42 · tests: none

Phone check: Walk along the counter: no step at its end, the rail runs unbroken, the steel behind it has no white spots or flicker

## J35.2 The stools: Poly Haven's carved wooden bar stool (Bar Chair Round 01, CC0) in place of all 8, scaled to a real 0.76 m seat, its low back away from the counter; you sit at the new height; its solids follow the new shape

`2c36d25` · 2026-10-26 04:38 · 9 files, +25 −78 · tests: none

Phone check: Walk to the counter: the stools look real (wood and metal), sit on one: you sit on it, not in it

## J35.1 The kit: Blender 4.5 set up, Poly Haven models fetched and slimmed by script (triangles, 1k textures, origin on the floor), a budget per remade model checked with the quick checks, credits table

`6be0a2a` · 2026-10-26 04:35 · 12 files, +168 −19 · tests: none

Phone check: Nothing changes in the bar yet

## J91.3 Tuned: soft shade 0.5 m at full strength, the bar's haze 0.02, beam steps follow Quality (16/12/8)

`bb6755a` · 2026-10-26 03:57 · 6 files, +11 −5 · tests: none

Phone check: Bar stays smooth with both on; Quality Low still shows soft beams

## J91.2 Light beams: lamp light in a thin haze, cut by what blocks it

`f971261` · 2026-10-26 03:55 · 24 files, +358 −20 · tests: none

Phone check: By the bar counter: soft cones of light under the hanging lamps; Settings → Graphics → Light beams Off: gone

## J91.1 Soft shade: ambient occlusion where things meet

`3455757` · 2026-10-26 03:38 · 15 files, +533 −50 · tests: none

Phone check: Under the stools and along the counter's base the floor darkens softly; Settings → Graphics → Soft shade Off: it's gone

## J97.1 Lamp shadows: the 16 brightest lamps keep cube shadow maps of the still things (drawn once, 12 faces a frame, again when rooms load or the Studio moves things; a lamp's own model left out); their light and highlights blocked by stools, tables, the counter, no more probe squares; Settings → Graphics → Lamp shadows, ?shadows=0

`1cd9231` · 2026-10-26 02:43 · 12 files, +269 −19 · tests: none

Phone check: Stand by the bar stools: under each stool the floor is darker and the lamps' reflections stop where stools and tables are; no squares of light on the walls; Settings → Graphics → Lamp shadows off brings back the old look

## J98.2 Tables softer (roughness 0.32 → 0.42), the floor a little shinier (0.75 → 0.62), brushed steel calmer (0.38 → 0.45), the whole bar a third of a stop darker; the squares' cause confirmed and moved into J97 (lamp shadows)

`1be7df8` · 2026-10-26 01:56 · 9 files, +49 −51 · tests: none

Phone check: The round tables shine softly with no squares in them; the stage isn't washed out; the floor has a soft shine

## J96.1 The square of light on the floor gone (J88.3's paced bounced light undone); the captured room reflected only by glossy things (bar top, glass, chrome), not the floor: no blocky pools that slide with you or show lamps through things; the incident

`a3c9f6f` · 2026-10-25 23:47 · 11 files, +82 −72 · tests: none

Phone check: Walk the bar looking at the floor: no bright square, no pools of light that follow you; the bar top and the taps still shine

## J88.3 Bounced light at its own pace: at most 10 times a second, only while a lamp's light changes by more than 2 %, plus 8 steps to settle (before: two passes over 29,634 patches and 20,655 probes every frame); 3D light cells left out after research

`db6eac2` · 2026-10-25 23:12 · 7 files, +54 −18 · tests: none

Phone check: ?perf=1 standing still in the lounge: GPU time lower than before; the fire still flickers on the walls

## J88.1 Up to 255 lamps (one byte each in a cell's list); bounced light and walls blocking light for the first 127 (four 32-bit words per probe, on the GPU and in the light tool)

`38f092c` · 2026-10-25 22:01 · 9 files, +59 −30 · tests: none

Phone check: The bar looks as before

## J95.1 GitHub's long browser tests only on request (Run workflow, tests ticked); deploys keep their build and quick checks; Dev shows "browser tests on request"; the Handbook says how

`6f66cb1` · 2026-10-25 21:48 · 4 files, +36 −29 · tests: none

Phone check: Dev → Overview: the game's deploy tile says live, browser tests on request; the bar is the new build

## J87.1 Copies drawn together: per-copy data (matrix, tint and glow, highlight) in an instance buffer, every copy of a model part in one draw, in the main view, the mirrors and the reflection capture; each pass at its own place in the buffer; up to 4096 models

`eb42d32` · 2026-10-25 21:34 · 6 files, +160 −93 · tests: none

Phone check: The bar looks exactly as before (highlights when you stand at a thing, the loops' glow, the glasses); ?perf=1 shows fewer draws

## J94.1 The place viewer draws: the renderer gets a plain surface layer when a program has no textures of its own, and the viewer's scene starts in its own task; the incident

`6a7dd81` · 2026-10-25 21:06 · 6 files, +79 −18 · tests: none

Phone check: joshuavelasco.com/bar/looped/ shows the showroom with its lights and floating sparks; drag to look around

## J93.1 The phone versions of the models carry the engine's texture label again (tools/assets/astc_pack.py writes image/x-looped-astc, the label J86.1 gave the engine), so their textures load; the incident

`28767be` · 2026-10-25 20:49 · 5 files, +67 −20 · tests: none

Phone check: The bar looks as before: wood, brick, the bottles' labels all have their textures (not flat colours)

## J86.2 The engine lays places out and plays them by itself (looped/world/place_layout.*, looped/place_player.*; the bar builds on both), the renderer's limits are given at start, and the place viewer: a second program from the engine alone at /bar/looped/ (?place=showroom), drag to look around, pinch to come closer

`550e9d1` · 2026-10-25 20:40 · 25 files, +776 −182 · tests: none

Phone check: joshuavelasco.com/bar/looped/?place=showroom shows the showroom, alive, with no bar around it; drag to look around, pinch to come closer. The bar itself looks and works as before

## J86.1 Looped built as its own library (the looped target the bar links); every call the engine makes to the page goes through window.loopedHost (looped/host.*), which the bar's page fills with its own parts; tools/lint_engine.mjs in the quick check keeps bar names and bar files out of the engine

`d7b2ce2` · 2026-10-25 20:32 · 21 files, +183 −77 · tests: none

Phone check: The bar works as before (menus, voice, multiplayer, sounds)

## J92.1 The glow (bloom) taken out of the engine: its three shaders, five passes and three textures; the moods' glow settings, ?glow=, ?glowth= and ?post=; J44's glare and starbursts off the road; the rule: the user tests

`f28d037` · 2026-10-25 20:17 · 23 files, +167 −254 · tests: none

Phone check: By the neon signs, the lamps and the fire: no haze of light around them anymore, the bulbs and neon still bright and the room still lit

## J85.2 Timelines in the world model, like a music app's arrangement: tracks over things (#id or kind:id), clips that play a loop from a beat for a length, fades in and out, a spread that runs a wave along a row, layered on each thing's own loop and swapped live; worked out from the shared clock alone (the music's beat, or 120 bpm), so every phone and the room agree without talking. Built in 4c9ea11 (looped/world/timeline.*); checked in J85.1's run (the native world tests: the same pose on two worlds at the same clock, a clip at its start, middle and fade, a wrap past the end, the music's beat)

`2aafaf3` · 2026-10-25 18:23 · 2 files, +2 −2 · tests: none

Phone check: Nothing changes in the bar yet

## J85.1 The loop library: named patterns in assets/world/loops.json (19: swell, spin, hop, sway, pulse and more), used as "@name" wherever a loop goes (a kind, a thing, a clip); a missing pattern or a broken one is named with where it's used; the store, the Studio's draft and the tools carry the file. Built in 4c9ea11; the native world tests cover the library (a pattern applied, a name that isn't there, a broken pattern)

`78c76a4` · 2026-10-25 18:23 · 5 files, +11 −9 · tests: native, game

Phone check: Nothing changes in the bar (the patterns aren't used yet): walk the bar, everything moves as before

## J90.3 The Live panel on Overview: a tile for the bar (its build), who's in it now (a heartbeat that beats faster with more people), each deploy (green, red with the failed tests, stripes while running), Claude's errors this week and Claude's latest job; it checks again every 15 s and a tile whose answer changed glows and counts to it; tap a tile: its details, Open, and Looks good / Broke it / Note, which go to Claude's notes naming the tile. Built in f6099ec; checked in J90.1's run (the Dev test: six tiles, red and green by the deploys, a tile's sheet, Broke it with your words reaching the notes, an empty Note sending nothing; the labels test: the tiles and the sheet)

`c831282` · 2026-10-25 12:10 · 2 files, +2 −2 · tests: none

Phone check: Overview: the Live tiles show the bar's build and the deploys; tap The bar → Looks good: Plan → Tell Claude shows the note waiting

## J90.2 Nested pages: every page has its own address (#plan/engine, #plan/J85, #projects/bar3d), so a reload, the back swipe and a saved link land on it; the trail at the top (each step a way back) and Back; a page per project (its tools by group), per chapter (all its jobs with their verdicts), per job (its words, verdict, place in line, what it waits for, its plan, its pieces and a ring when in Now), Done and Parked; Up next as chips; Find, one field over every page, project, tool, chapter and job. Built in f6099ec; checked in J90.1's run (the Dev test: the job page and its spec, the trail and Back, a chapter's page with all its jobs, what a job waits for, a reload on a nested page, a project's page, Find and Enter; the labels test: the new pages' controls)

`6a303a8` · 2026-10-25 12:10 · 2 files, +2 −2 · tests: none

Phone check: Plan → The engine → a job: its page opens; the trail at the top takes you back; the magnifier, type studio: the Studio is first

## J90.1 Motion and life in Dev: four Ember glows drift behind everything (in steps under a pixel, 8 a second, so the frosted panels over them stay cheap), each page comes in piece by piece and slides (deeper from the right, back from the left; the View Transitions API, a slide dropped after 1.5 s on a slow phone), rows, tiles and keys press and ripple from your finger, cards tilt toward it, lights breathe, numbers count to new values; only transform and opacity move; Calm (the dot at the top right) stops all of it and is remembered, iPhone's Reduce Motion turns it on by itself, and it all pauses while Dev is in the background. Built in f6099ec; the Dev test checks Calm on, kept after a reload, and off again

`98ae472` · 2026-10-25 12:10 · 3 files, +8 −8 · tests: dev, labels, workshop

Phone check: Open Dev: the glow drifts behind the cards; tap a row: it presses in and ripples from your finger; tap the dot at the top right: everything holds still

![J90.1 page-dev-index-demo.jpg](J90.1/page-dev-index-demo.jpg)

## J84.2 The bar says why in words: Voice chat tells a guest that voice is for signed-in players, and an account made before birth dates were asked gets 'Add my birth date' (once: 13 and up, never changed after; account.php's new action, with the same checks as sign-up); a refused call isn't tried again until your login changes; the voice test's players sign in as test adults. Built in d3f9c65 (bar/web, server/account.php, the gate and voice tests, the Handbook's menus, voice and protocol pages); its game, menu-after-games, voice, accounts, Handbook and labels checks passed on it (14 of 15), and the menu test, whose 3x screen starved behind the first page still drawing (KI-5), now lets that page draw one frame every 5 s

`05e186d` · 2026-10-25 11:24 · 3 files, +6 −3 · tests: menu

Phone check: Menu → Voice chat, on an account made before birth dates: Add my birth date; after adding it, voice works with other adults

## J84.1 The room keeps each login's age group (teen or adult, from the accounts server; never sent to anyone) and passes a voice handshake only between two signed-in people of the same group; otherwise the one who called hears why: 'guest' (sign in), 'birth' (add your birth date) or 'apart' (which never says anything about the other person). Built in d3f9c65 (cloud/room.js); the room test covers a guest refused, two adults and two teens connecting, adult with teen refused, to a guest and to or from an account without a birth date refused, and nobody told anyone's group

`41f3e66` · 2026-10-25 10:58 · 2 files, +4 −4 · tests: native, room

Phone check: Two phones, one signed in and one a guest: the guest's Voice chat says voice is for signed-in players, and no one hears the guest

## J54.3 No browser test can hang here or on GitHub: a picture that takes too long is noted and the test goes on (the smoke test's 'not a blank screen' picture still fails it), every browser test has a 30-minute limit and every check a 45-minute one (built in 7d04923; the game and native checks passed on it in J54.2's run; here the workflow checks and the voice test, which now opens its pages through the tests' browser)

`593d6ce` · 2026-10-25 10:13 · 2 files, +2 −2 · tests: workflows, voice

Phone check: Ask for a small world change: it saves in minutes, with no test left waiting

## J54.2 Two game pages on GitHub's 2-core machine no longer starve each other: the game draws nothing while its page is hidden (a phone stops then anyway), a page's frame cap can change while it runs (bar3dBridge.command('fps', N)), and the two-player tests let the player who only waits draw one frame every 5 s while the other loads; the two-player race and the multiplayer test pass on 2 cores (100 s and 10 min). With the earlier part (7d04923: at most 2 frames in flight, ?fps, the tests' light and frame cap for every test page) every browser suite runs to its last check. The 'Back after' line is written at most every 30 s (it pushed 'Alice joined' out of the log)

`e7f8803` · 2026-10-25 10:12 · 9 files, +49 −10 · tests: changed

Phone check: Dev's game row after the next deploy: every browser test reached its end; the two-player race and multiplayer tests green

## J54.1 Found why GitHub's browser tests stop: the game's own drawing keeps the software GPU busy, so a screenshot waits 40 s to minutes behind it, and a second game page never finishes loading while another one draws; the multiplayer and voice tests also used their own browser, without the tests' light. Written into KI-5.

`2ae86dd` · 2026-10-25 03:22 · 4 files, +11 −5 · tests: changed

Phone check: Dev → Plan: J54.1 ticked; the known issue names the cause

## J83.1 Dev → People: Claude's key, owner only. Make a new Claude key asks first, then shows the key once in a field with Copy and says where it goes (the environment's HUB_KEY, never the chat); the hub's button was lost when the hub became Dev. The Dev test makes a key from the page and checks the hub takes it; KI-34 noted (the old key refused)

`19dbbcd` · 2026-10-25 01:37 · 9 files, +125 −29 · tests: changed

Phone check: Dev → People, scroll down: Claude's key with Make a new Claude key; tap it, say yes: the key shows with Copy

## J82.1 The showroom: a new place (Menu → Places) with five stations that show what Looped does: a coin that turns and flips with sparks around it, four lamps from 1800 K to 6500 K, all 13 materials under a tube light, the jukebox with lights on the beat and neon in a wave, a table with a candle, glasses and stools, a chalkboard, a light switch. Build, native and game checks passed on these world files; the Studio test left out on the user's word (2026-10-25 #2)

`3f56ddb` · 2026-10-25 01:08 · 8 files, +378 −28 · tests: plan, workshop

Phone check: Menu → Places → showroom: the coin turns over the pedestal with sparks around it; the four lamps on the left go from orange to blue; the back shelf shows 13 different cubes; play a song and the ceiling lights pulse; tap the candle, the table, a lamp, the coin; the LIGHTS switch behind you dims the room

## J66.2 The room test's grace check made steady (KI-33): from run 214's times, the watching player was closed as away (the test's AFK_MS, 9 s after it joined) before the ghost's leave reached it on GitHub's slow machine; the test's away time is now 20 s, longer than a ghost's close, grace and leave take, so the check waits for the event instead of racing the away timer; the AFK check itself is unchanged (it still waits for the 4004 away); and a changed test now runs only itself, not every check of its system (the room test's change had started the two-player browser test too, which times out here: your rule, test only what the change touches); checked by the room test (passed here) and the workflows test

`6715116` · 2026-10-24 20:16 · 5 files, +14 −9 · tests: changed

Phone check: GitHub's room test passes on the next deploy (Dev's game row lists no failed room test)

## J66.1 The game's tests in their own job after the deploy (KI-32): deploy.yml's deploy job (build, quick checks and the room's tests, the room, the upload, gh-pages) queues alone (pages) and goes live in about 6 minutes; the tests job builds the same commit again on its own machine, writes the same config.json (the room's address passed on from the deploy job) and runs every test, in its own queue (game-tests), so the next deploy waits only for the deploy before it; every step of both jobs now has a time limit; Dev's reports keep working (the deploy job reports started and live, and its end if it stops before the tests; the tests job reports the end with live passed on), and a tests job's end report for a build that's no longer live isn't sent; a build stopped by a quick check no longer runs the browser tests (nothing of it went live); tools/workflows_test.mjs checks the split, the two queues and the time limits (all four checks fail on the old workflow); the CI page and the stuck-deploy procedure say it; checked by the workflows, Playbook, locks and Handbook tests and a dry run of the reporter

`c3980dd` · 2026-10-24 20:08 · 8 files, +151 −19 · tests: changed

Phone check: The next push: Dev's game row says live about 6 minutes after it, even while the previous build is still testing

## J8.3 Unfreeze: your Freeze rule taken out on your word (request 2026-10-24 #7, go finish J8) by the plan tool's new retire command (it takes a rule out only on a recorded request and writes nothing in its place; the plan test covers it); the freeze's lock turns itself off with the rule; the Charter's in-practice lines, CLAUDE.md, the Playbook's intake and the chapters' Now lines say the freeze is lifted; the first real job's spec written (docs/plan/jobs/J66.md: the tests in their own job after the deploy, the room test's grace check made steady); checked by the plan, Playbook, locks and Handbook tests

`e3e310e` · 2026-10-24 19:54 · 11 files, +101 −25 · tests: changed

Phone check: Dev → Plan: no Freeze line in your rules; J66 in Now

## J8.2 Final audit: the docs check green; every flow in the data map checked live (the bar's config.json and version.txt, gh-pages' Site build and Dev's two deploy rows all say 450fb52; the world store v3 and the UI store v0 match the repo; Dev's copy of the plan and the rules equals main's, from build 450fb52, with every job's spec; the room's /health answers, accounts and phone reports answer, the site and Dev load; no unread notes); no instruction outside the Playbook (the root README still called PLAN.md the one doc and told sessions to read it: now it points to CLAUDE.md and the five books); no open drift; the restructure's spec archived to docs/archive/restructure.md with a history banner, every path to it moved (the plan's relink now reaches a path inside a rule too, never the words), CLAUDE.md and the Playbook say it's history; tools/lib/docmoves.mjs mapped to its own checks (it ran the game's whole test set as an unknown file); checked by the plan, workflows, Playbook, locks and Handbook tests

`b2b3f42` · 2026-10-24 19:51 · 19 files, +47 −38 · tests: changed

Phone check: Dev → Plan: J8.2 ticked; Overview: both deploys on one build, nothing waiting on drift

## J8.1 Cold-start drills: 22 requests in your style (a wish, status, broke, undo, VR, a kids mode, a new rule, a Studio publish, a red and a stuck deploy, a copyrighted song, a pasted key, real Ferraris, park, a question, talk, a phone report, free credits, go, deleting branches, the site down, a paid machine), the answer key written first, each given to a fresh read-only AI session with only the repo: all 22 named the right procedure, verdict, files and commands and broke no permission; five gaps they found are fixed and their drills rerun and passed: when a request or park is saved and pushed, a bare go isn't recorded, a secret in your words is written [secret removed] (intake.md step 2), the last job's Done line is sealed with its build at the next piece (sealShipped, plan test), and the commit lock refuses Stripe keys (pre-commit, locks test); the record is docs/records/drills/2026-10-24.md; J7's Done line sealed (450fb52); run 215's results on KI-5 and KI-25; checked by the plan, workflows, Playbook, locks and Handbook tests

`903e814` · 2026-10-24 19:37 · 13 files, +138 −12 · tests: changed

Phone check: Dev → Plan: J8.1 ticked; J7 in Done shows build 450fb52

## J7.4 The order, approved (request 2026-10-24 #7: go): Up next is J8, then quick checks fixes (J66, J54, J68), your phone's speed reports (J37, J39), the world remake and the script language taking turns (J35, J43, J70, J78 … J77), what they unlock (J24, J25, J13, J9), safety before growth (J65, J20); the racing job carries your own plan (made-up makes with close shapes and real specs, the rigs' screens for watchers, the visual upgrade with the track in stage 14; its spec's decision 5 quotes you), and its wrong verdict is fixed; accounts off IONOS (J60) has your yes on record; the paid items parked (J63 voice relays, J64 payments, J23 the VIP marketplace, J69 a GitHub machine with a graphics card; KI-28 parked with J69); the plan tool's park --from moves a road item to Parked and its verdict command names the item's spec; the Playbook's verdict step reads what the user already said first (incident 2026-10-24-verdict-without-spec); checked by the plan, workflows, Playbook, locks and Handbook tests

`fc947b6` · 2026-10-24 19:19 · 10 files, +127 −78 · tests: changed

Phone check: Dev → Plan: Up next lists J8, J66, J54 … in this order; The backbone no longer lists voice relays or payments (they're in Parked)

## J7.3 The script language's realistic scope and its first jobs: small (things, states, events, timers, loops, remembered values, messages), safe (only its own thing, a step budget and a memory limit, never breaks the bar), built on the world model's kind data (a script and a kind's states, rules and loops convert both ways), written for people and AIs, every error with its line, plain words and a fix hint; not a replacement for C++ or JavaScript; its spec in docs/plan/jobs/J43.md with a first sketch; J43 (formerly T1) becomes step 1, the design on paper with 20 scripts for things that exist today, then J78 the parser and checker, J79 the safe runner, J80 scripts in the Studio, J81 five behaviours rewritten as scripts; the light show (J50) and the timeline (J57) wait on them; the Handbook's script language page says it; checked by the plan, Playbook, locks and Handbook tests

`e8b4dbb` · 2026-10-24 19:09 · 6 files, +187 −22 · tests: changed

Phone check: Dev → Plan → The engine: J43 and J78–J81 in order, each with its verdict; tap J43 for the language's scope and a first sketch

## J7.2 The world remake as proposed jobs: J35 (formerly W24) becomes its first step, the kit (every new thing a kind with its own model at real size, grown by script, within a checked budget, copies drawn together), then J70 the lounge taken apart, J71 the walls' props, J72 bottles and glasses as real things, J73 the stage, the darts corner, the construction and the arcade, J74 the room shells remade, J75 placement made strict, J76 light carried by its things, J77 sound carried by its things, each with its verdict; its plan in docs/plan/jobs/J35.md from the models as measured today (seven room-sized models with 175 objects baked in, 33 lights with no thing of their own); the small interactions (J24), the physics props (J25) and the parked street now wait on the steps they need; the world chapter's Now line names the remake; checked by the plan, Playbook, locks and Handbook tests

`102545e` · 2026-10-24 19:08 · 4 files, +220 −21 · tests: changed

Phone check: Dev → Plan → The world: J35 and J70–J77 in order, each with its verdict; tap J35 for the remake's plan

## J7.1 Every road and parked item has a verdict and every road item a job number: the old road renumbered J9–J65 in order (each keeps its old tag as Formerly; J8 keeps its own; the duplicate L1.8 is now J36, KI-20 closed), four new jobs from the known issues (J66 deploys never wait behind or get stopped by the tests, KI-32/KI-33; J67 the account test for the room server, KI-27; J68 the hub's unused tuning out, KI-29; J69 a GitHub machine with a graphics card, a cost, KI-28), each with one of the Playbook's six verdicts and why, tied to the Charter's limits; the plan tool keeps verdicts (add needs one, verdict changes one, park takes one, nextid gives the next number; the plan check refuses a road item without one) and finds a renumbered job's spec through its old tag; Dev → Plan shows each item's verdict word, its reason and its old tag; the freeze is now a lock (tools/lib/freeze.mjs in a commit-msg hook) after a path rename reached frozen code (incident 2026-10-24-freeze-unlocked); checked by the plan, workflows, Playbook, locks, Handbook, Dev and labels tests

`2334b6b` · 2026-10-24 19:07 · 23 files, +947 −244 · tests: changed

Phone check: Dev → Plan: each road item shows its verdict on the right (Doable, After J…, At a cost, Try first) and why under its words; parked ideas too

## J6.7 Fix for J6's red site deploy: the plan test no longer depends on the real plan's Up next line (its copy sets its own); the ship always runs the plan, workflows and workshop checks on the final state, even when the rest is skipped; incident recorded

`88258dc` · 2026-10-24 17:24 · 5 files, +40 −4 · tests: changed

Phone check: Nothing on the phone

## J6.6 The old notes sorted: the room protocol and the manual moved into the Handbook (docs/handbook/protocol.md, manual/); the driving rework plan is its future job's spec (docs/plan/jobs/W20.md); DECISIONS, STUDIO, ENGINE, REFERENCE, AUTOMATION and the audits archived with a banner; their still-true facts in new Handbook pages (README index, build-and-test, phone, traps); the decisions still in force recorded (0002-0010); every doc path in the repo rewritten to where the docs are now (comments, and the plan through its new relink command), and the Handbook check now refuses a path to a missing doc and reads the files as they are on disk (in every quick check); the stores' copies keep their old paths until their next publish (KI-31); duplicate pictures kept on purpose (decision 0010); KI-1, KI-2, KI-13, KI-14, KI-16, KI-19, KI-21, KI-23 closed. Code changes are comments only (checked line by line): checked by the build, the shaders, the native tests, syntax checks of every touched file, and the store, edits, protocol, hub, site, toolbelt and server tests; the game's browser tests aren't run for comments

`820a1f2` · 2026-10-24 17:10 · 193 files, +563 −221 · tests: plan, workflows, workshop, build, native, uikit, hub, server, site, toolbelt, labels

Phone check: Nothing on the phone

## J6.5 How-to recipes: docs/handbook/recipes.md, 27 recipes mapping common requests to their exact files and commands (move, add or remove a thing; a light, a mood, a material, a sound, a loop, a rule; a new kind, a new place; publishing to and taking in from the stores; a site page, a theme, a screen; a game, a test, a tool, a check; a phone report; an urgent fix, a revert; music and outside assets; a news post; a project; a shared room key), each inside the Playbook's job; the Playbook test checks every command they name exists; the piece tool rewrites the Handbook's generated pages before its checks, not after

`0109066` · 2026-10-24 16:49 · 6 files, +128 −10 · tests: changed

Phone check: Nothing on the phone

## J6.4 Every page and control: 21 pages in docs/handbook/ui/ (Dev's tab bar and each tab and section, the UI Editor, the Dials, World notes, the site's frame and pages, the news page, the Studio, the bar's menus), each control by its exact label and what it does; tools/ui_labels_test.mjs crawls the pages a test can open (tools/lib/labels.mjs, on a copy of the webspace) and fails on any control missing from its page or listed but gone (on the site's deploy); the piece tool refuses a change to the Studio or the menus without their page (or --docs-unchanged with the reason); the command list is written from the front door (generated/commands.md, since J6.2); the Playbook says it

`960957d` · 2026-10-24 16:48 · 32 files, +527 −15 · tests: changed

Phone check: Nothing on the phone

## J6.3 The data reference written from the files (docs/handbook/generated/): every field of the world's files with its type, unit, range and whether the Studio changes it live (data-world.md, from the schema); every theme token and screen part, and the screens and themes there are (data-ui.md); the projects, the tools and the game's config fields (data-lists.md); the room protocol's messages read from its codec (data-protocol.md); and the world inventory (inventory.md): every thing in every place, what it sits on, the model that holds it, room-sized models and things with no model flagged, kinds no place uses; the piece tool rewrites these pages in every commit so they follow their files

`f3d86ac` · 2026-10-24 16:39 · 12 files, +928 −8 · tests: changed

Phone check: Nothing on the phone

## J6.2 One page per system: 51 pages under docs/handbook/systems/ (engine 17, world 13, site 5, tools 9, backbone 7), each with the same ten parts (what it is, status, what you can do, how it works, data, commands, limits, checks, known traps, history), from the code and the old notes; the Handbook tool writes the index (generated/systems.md, with each status) and the command list (generated/commands.md, from the front door), and its check refuses a page that breaks the template

`9334698` · 2026-10-24 16:37 · 57 files, +2456 −9 · tests: changed

Phone check: Nothing on the phone

## J6.1 The map of every folder and of every technology: docs/handbook/generated/map.md written by the new Handbook tool (node tools/project.mjs handbook write) from one line per folder (docs/handbook/folders.json), the deploy map and the files themselves (how many, which languages, who uploads them where); docs/handbook/tech.md: every language and outside tool, its version, where, why, runtime or build, ours or not, and the outside assets with their credits; the Handbook check (generated pages current, every folder described, every link leads to a file) runs in every quick check

`694f753` · 2026-10-24 16:06 · 9 files, +312 −5 · tests: changed

Phone check: Nothing on the phone

## J5.8 Fix for the red site deploy, second cause: the Dev test's two pictures (added in J5.6) timed out on GitHub's software-drawing machine and failed the test; they now go through the same helper as the site test (a slow picture prints a note, never fails the deploy); incident recorded

`544f17d` · 2026-10-24 14:13 · 3 files, +36 −2 · tests: changed

Phone check: Nothing on the phone

## J5.7 Fix for J5's red site deploy: Dev → Publish → the front page's words no longer wipes what you've typed when the saved words load slowly (it saved an empty headline on GitHub); the Dev test makes that answer slow on purpose and checks the typed words survive; incident recorded

`c4b3fca` · 2026-10-24 14:03 · 4 files, +41 −1 · tests: changed

Phone check: Dev → Publish → the front page's words: type a headline at once, wait, Save: the headline you typed is the one saved

## J5.6 Dev shows the truth: Overview adds Claude's errors this week (from the incidents), world changes proposed for you to look at, and the questions only you can settle (open known issues that wait for your call), next to both deploys, the news drafts and the job in Now with its next piece; Plan: tap the job in Now, or a job on the road, to read its spec; the plan workflow also sends the jobs' specs (node tools/plan.mjs specs) and your questions (node tools/records.mjs questions); the Dev test covers each and saves two pictures; KI-30 noted (two Studio checks unsettled here)

`5cc1f57` · 2026-10-24 13:52 · 11 files, +101 −21 · tests: changed

Phone check: Dev → Overview: 'Errors this week' under the deploys, 'Your call: KI-28' under Needs you; Dev → Plan: tap J6 on the road (or the job in Now) and its spec opens under it

## J5.5 One list of projects: toolbelt/tools.json → projects is the source (each project with its site card: featured or more, picture, tag, line); Dev → Projects, the site's Projects page (and front page's More projects) and the news page's project names all read it; the Projects page comes out byte for byte the same as before; the toolbelt test checks every site card, the site test that Projects lists the list's projects; the news page is checked by the hub test; KI-11 closed

`b595089` · 2026-10-24 13:48 · 11 files, +59 −28 · tests: changed

Phone check: joshuavelasco.com → Projects: the same three cards as before (Bar3D, Looped, the script language) and Where we're going

## J5.4 The retired board out of the hub: hub.php keeps only news posts, screenshots, the Claude key and the loops' tuning (its board, projects, places and sections actions gone, hub/seed.json and hub/projects/ removed; the server's data left as it was); tools/hub.mjs keeps post and posts; the hub test covers what stays and that the old actions are gone; the Studio's Note for Claude, which still wrote board cards nobody read, now lands in Dev's notes with the thing, the camera and a picture (Studio test S10.1 passes); KI-12 closed, KI-29 noted (the tuning has no reader)

`dad3928` · 2026-10-24 13:45 · 14 files, +88 −1708 · tests: workflows, hub, server

Phone check: Studio: select a thing → ⋯ → Note for Claude → send; then Dev → Plan shows it under Notes

## J5.3 Exactly one deployer per folder: docs/handbook/deploy-map.json (every path → the workflow that uploads it and where, or not uploaded); tools/workflows_test.mjs checks every tracked file has its entry, each workflow uploads only its own folders and starts on changes to them; the game's deploy no longer uploads the site (only hub.yml does); a plan-only change no longer starts a game deploy; KI-9 closed

`8083557` · 2026-10-24 13:20 · 6 files, +70 −8 · tests: changed

Phone check: Nothing on the phone

## J5.2 The plan reaches Dev within about a minute: its own workflow (.github/workflows/plan.yml: on the plan, the Charter and the Records; only the plan's shape check, then the plan's data, the rules, the incidents index and the build it came from into Dev's store); the site deploy no longer carries the plan; Dev → Plan says 'As of build <sha>, N min ago'; node tools/records.mjs index lists the incidents

`fc09e28` · 2026-10-24 13:18 · 9 files, +94 −11 · tests: changed

Phone check: Dev → Plan: under J5's title, 'As of build …, N min ago'

## J5.1 The data map (docs/handbook/data-map.md): every store (the repo, the world store, the UI store, Dev's store, the hub, accounts, phone reports, the room, the ledger, the gallery and gh-pages branches, the phone's saves): who writes and reads it, how it flows, what wins on a conflict, how staleness shows; the world's rule written once (the store's live version is the live world, the repo's copy is the built-in fallback); decision 0001 gives each of the user's channels one role; KI-17 and KI-22 closed

`7258356` · 2026-10-24 13:17 · 5 files, +132 −4 · tests: changed

Phone check: Nothing on the phone

## J4.6 The Playbook names only what exists: tools/playbook_test.mjs (every tool, front-door command, tool command and link it names is real; no 'arrives with J<n>' marker left) and tools/locks_test.mjs (the locks refuse what they should, let the rest through), both in every quick check and in the new 'workshop' system; the Playbook now uses the real commands (status, request, incident, session, ship); ship commits the next job and the session line with the finish; the guard reads only real git commands, not text that mentions git; a changed test runs only its own system

`4a3e765` · 2026-10-24 12:54 · 14 files, +127 −31 · tests: changed

Phone check: Nothing on the phone

## J4.5 Locks: git hooks in tools/githooks (setup.sh turns them on): a commit is refused if it adds a secret or the plan's files disagree; a push to main is refused unless the ship does it, and never as a force push or a delete; Claude Code's lock (tools/lib/guard.mjs, .claude/settings.json) refuses a hand push to main, force pushes, --no-verify, turning the git locks off, and hand edits of PLAN.md, the plan's data, the rules and the theme CSS; every session starts by printing the project's status

`9e6acce` · 2026-10-24 12:42 · 8 files, +94 −3 · tests: changed

Phone check: Nothing on the phone

## J4.4 Records tools (tools/records.mjs, through the front door): request (the user's words exactly, the next id of the day, what was understood), incident (the five parts to fill), decision (numbered, its parts), session (the line with the session's own link and its errors linked, refusing an incident that doesn't exist); docs/records/README.md says what each record is and what writes it

`9669b0f` · 2026-10-24 12:28 · 4 files, +77 −2 · tests: changed

Phone check: Nothing on the phone

## J4.3 The ship command (node tools/project.mjs ship "<what>"): refuses unless on the job's branch, nothing unsaved, every commit belongs to the job, every piece ticked, the plan's files agree; runs the checks for everything the job changed; finishes (merge, Done line, push main once); then waits up to 12 min for Dev's reports to show the build live and says exactly what is and isn't confirmed; commit attribution from the session itself (tools/lib/attribution.mjs); KI-3 and KI-15 closed

`b88a817` · 2026-10-24 12:28 · 8 files, +84 −9 · tests: changed

Phone check: Nothing on the phone

## J4.2 One front door: node tools/project.mjs help lists every command (tools/lib/commands.mjs, one list) and runs each; status shows the whole truth on one screen in about a second (the work here, the job in Now, the freeze, the live build, both deploys, the user's notes and news drafts, the stores, open problems and today's incidents, whether the plan's files agree), each part with its age, every network question cut off after 5 s; the Toolbelt lists it

`136ddae` · 2026-10-24 12:26 · 6 files, +109 −3 · tests: changed

Phone check: Nothing on the phone

## J4.1 The plan as data: docs/plan/plan.json and docs/charter/rules.json (the user's North star and rules) are the source, written only by tools/plan.mjs (same commands, plus chapter and render); every save writes PLAN.md, the Charter's rules block and CLAUDE.md's Never list; converted without losing a word (proved both ways); Dev reads the data instead of parsing PLAN.md (and now shows the North star lines whole); the site deploy uploads the data; the five chapter lines brought up to date; KI-4 and KI-10 closed

`a867cf9` · 2026-10-24 12:24 · 16 files, +1161 −206 · tests: changed

Phone check: Dev → Plan looks the same, nothing missing; the North star lines are now whole sentences

![J4.1 page-dev-index-demo-plan.jpg](J4.1/page-dev-index-demo-plan.jpg)

## J2.4 The tests' own lighting (request 2026-10-24 #6): ?light=test is classic light with a flat light from above instead of every lamp per pixel (scene.wgsl shade() returns early), and no reflections, glow and mood or full mirrors; GitHub's tests use it (TEST_LIGHT=test); the phone and the pictures keep the real light; docs/manual/light.md, KI-5 and KI-28 updated

`959ba86` · 2026-10-24 12:13 · 11 files, +51 −13 · tests: quick

Phone check: Nothing changes on your phone: the bar keeps its real light

## J3.7 One voice: CLAUDE.md rewritten as the short entry (who the user is, the five books in order, the three Playbook steps of every session, the Charter's Never list copied by plan.mjs charter and checked); REFERENCE.md's old how-to-work sections archived; DECISIONS, AUTOMATION, STUDIO and ENGINE marked as not instructions; NEXT.md deleted; pieces and finishes only on job branches; finishes run the checks for what the job changed

`275c7f7` · 2026-10-24 12:06 · 16 files, +246 −238 · tests: changed

Phone check: On GitHub, CLAUDE.md is short and points to the Playbook

## J3.6 Ways of working (docs/playbook/ways.md: normal job, urgent fix, experiments that are never pushed, content from the user, talk and status, phone reports, long jobs, two sessions, audits) and changing the structure (change.md: only by a decision the user approves; where every new procedure, check, tool, page, project, word, issue and decision goes)

`0d920f1` · 2026-10-24 12:04 · 3 files, +95 −1 · tests: changed

Phone check: Nothing on the phone

## J3.5 When something goes wrong: incident.md (what counts as an error, the zero-error loop: stop, contain, record, root cause in the way we work, fix the system, prove it, count it; the unexpected), failures.md (a check fails, a red deploy, live broken, conflicts, the machine, resume, drift, security), revert.md (broke, undo)

`df46b1f` · 2026-10-24 12:03 · 4 files, +162 −1 · tests: changed

Phone check: Nothing on the phone

## J3.4 The life of a job (docs/playbook/job.md: the branch, each piece with only its own checks, ship once, verify live before saying so, the world store, the reply) and how Claude replies (reply.md: understood, done, one question with a recommendation, something went wrong; plain words; never claim what wasn't checked)

`b30db37` · 2026-10-24 12:03 · 3 files, +112 −1 · tests: changed

Phone check: Nothing on the phone

## J3.3 Intake (docs/playbook/intake.md): every kind of message the user sends (12 kinds) and where it goes; the user's words recorded exactly; the six verdicts, the capability check and the permission check before any work; a wish becomes a job with a spec (docs/plan/jobs/TEMPLATE.md); park; a new rule only with its request; unclear messages

`21de803` · 2026-10-24 12:02 · 3 files, +122 −1 · tests: changed

Phone check: Nothing on the phone

## J3.2 The Playbook begins: docs/playbook/README.md (the only book that gives instructions; one shape for every procedure; situation → procedure for every case; the three rules every procedure keeps) and session.md (the start: state, deploys, the user's publishes and notes, known issues, a half-done job; the end: nothing half-saved, the session line with its errors, the reply)

`ac6bb9c` · 2026-10-24 12:02 · 3 files, +118 −1 · tests: changed

Phone check: Nothing on the phone; docs/playbook/README.md is on GitHub

## J3.1 Test only what changed, push only what you want: the user's two new rules in PLAN.md and the Charter ("Speed over tests" taken out with plan.mjs rule --replaces: the user never said it; incident written), pieces now run only the checks covering their files (check.mjs changed, browser tests included) and build only when needed, the engine's self-check idea parked, request 2026-10-24 #5 recorded word for word

`00d6601` · 2026-10-24 12:01 · 11 files, +112 −38 · tests: changed

Phone check: Nothing on the phone

## J2.3 Both deploys report their real state to Dev

`1c9c7ef` · 2026-10-24 12:20 · 9 files, +152 −28 · tests: quick

Phone check: Dev → Overview: two deploy rows (Game deploy and Site deploy), each with its build, live or not, the tests' state and how long ago

![J2.3 page-dev-index-demo-overview.jpg](J2.3/page-dev-index-demo-overview.jpg)

## J2.2 Every test counts: one list of checks (tools/lib/checks.mjs) and tools/workflows_test.mjs in the quick check (every check in exactly one workflow, every test file listed, every test runs after any failure with a time limit, allowed-to-fail only with an open known issue in its name, ids for the reports); deploy.yml and hub.yml rewritten to match; GitHub now also runs the albedo, shader, plan, touch, travel and voice checks; KI-6 and KI-26 closed, KI-24/25/27/28 written down

`2dd5d99` · 2026-10-24 11:43 · 9 files, +326 −91 · tests: quick

Phone check: Nothing to see on the phone; Dev → Overview names any failing test (after J2.3)

## J2.1 The browser tests open the bar with the classic light on GitHub (TEST_LIGHT=classic in deploy.yml; tools/lib/browser.mjs adds ?light=classic to the bar's page, before its scripts run, in every page, context and frame, unless the address names a light); unset locally, so the pictures keep the new light; checked on 17 cases with static pages

`a8e1dd9` · 2026-10-24 11:36 · 4 files, +28 −3 · tests: quick

Phone check: Nothing changes on your phone: the bar keeps the new light. Dev → Overview shows the game deploy's tests passed, or names the one that failed (after J2 ships)

## J1.3 The glossary (docs/charter/glossary.md): the words the user can say, then 82 terms A to Z with one plain meaning each; the clashes settled (a piece is always a piece of work, a loop's channel is a lane, a rule is always the user's or a thing's)

`34ad18c` · 2026-10-24 11:19 · 2 files, +167 −1 · tests: quick

Phone check: Open docs/charter/glossary.md on GitHub: job, piece, build, ship, live, store each have one plain meaning

## J1.2 The Charter (docs/charter/charter.md): the project and the user, the North star and the user's rules word for word (a generated block checked by the quick check: plan.mjs charter), each rule in practice, what's realistic (phone, hosting, CI, the cloud machine, Claude, money, law, licences), the permission list (allowed / ask first / never; anything not listed: ask), how the Charter changes

`711f0d8` · 2026-10-24 11:15 · 4 files, +246 −3 · tests: quick

Phone check: Read docs/charter/charter.md on GitHub and tell me yes or what to change

## J1.1 Close R6, freeze, write the restructure down: R6 sealed with 87bd110 (new plan.mjs seal), the user's new rules, J1-J8 on the road with J1 in Now, the governing spec docs/plan/restructure.md, docs/records/known-issues.md (23 problems, each with its job), the branch clean-up incident, step.mjs --no-news, a restructure pointer at the top of CLAUDE.md

`b8bcf57` · 2026-10-24 11:11 · 8 files, +702 −12 · tests: quick

Phone check: Open Dev → Plan: Now is J1 · Freeze and the Charter with three pieces; Up next lists J2 to J8 (after J1 goes live)

## R6.6 The UI Editor, Compare, Screens and the style page in Ember; the old hub boards gone: the editor's own frame (top bar, buttons, inspector, dialogs) and Compare's are now Ember's dark half, fixed, amber where they were blue, in Archivo (they still never wear the theme being edited); every page's phone top colour is Ember's; hub/board.html and hub/list.html are deleted (and removed from the server at deploy), /hub opens Dev; the hub test now checks hub.php directly (cards, Claude's key, questions, news, places, passwords)

`197d923` · 2026-10-24 09:23 · 11 files, +81 −2153 · tests: quick

Phone check: Dev → Projects → UI Editor: the frame around the preview is warm dark with amber buttons (no blue); /hub/board.html is gone

![R6.6 page-toolbelt-ui-editor.jpg](R6.6/page-toolbelt-ui-editor.jpg)

![R6.6 page-ui-compare.jpg](R6.6/page-ui-compare.jpg)

## R6.5 Dials, World notes and News in Ember: World notes drops its old purple look and joins the house look (warm cards, a moving title, amber filter keys, Alive in amber); News's cards take Ember's round corners and its line reads What's new at J Production; Dials and News get Ember's dark top colour on the phone

`874ebf7` · 2026-10-24 09:17 · 4 files, +29 −31 · tests: quick

Phone check: Dev → Projects → World notes: warm cards, amber keys, no purple; open /news: round warm cards, 'What's new at J Production'

![R6.5 page-hub-dials.jpg](R6.5/page-hub-dials.jpg)

## R6.4 The Studio in Ember: the top bar, the bottom tools, the Monitor, the sheets, the library and every row take the theme's corners (the panels Ember's round 22, the rows a little tighter), the place's name in the top bar is a moving amber title, the last fixed colours (the world's backdrop, the magnifier, the lifecycle map) come from the theme

`304abce` · 2026-10-24 09:10 · 3 files, +34 −31 · tests: quick

Phone check: open the Studio: the place's name at the top shimmers amber, the bars and the Monitor have Ember's round warm corners

![R6.4 page-studio.jpg](R6.4/page-studio.jpg)

## R6.3 The private bar's door in Ember: the sign-in, waiting and People screens take the house look (warm frosted card, amber glow drifting behind, the Bar3D title in amber and still waving, amber Come in key, the guest key in warm glass, amber links and chips); the values come from the theme, with Ember's own as a fallback

`bab7d8a` · 2026-10-24 09:06 · 2 files, +40 −32 · tests: quick

Phone check: open the bar while signed out: the door is warm amber glass, Bar3D waves in amber, Come in is an amber key

## R6.2 The bar's menus and loading screen in Ember: the menu panels are warm near-solid (no blur over the 3D, no extra cost), their titles move and shimmer, keys, tiles, toggles, inputs, the toast, the card and the HUD keys all take the theme's corners, fonts and amber; the loading screen glows like the site, its bar fills amber

`be21a32` · 2026-10-24 09:01 · 2 files, +50 −24 · tests: quick

Phone check: enter the bar: the loading screen glows warm, the bar fills amber; open Menu: warm panels with a moving title, amber keys with round corners

## R6.1 Ember is the default theme: every page on the Looped look (the bar's menus, the Studio, Dials, News, the style page, Screens, the UI Editor) opens in Ember; a theme picked on purpose (the style page) is still remembered, an old automatic save no longer counts; ?look= and ?scheme= in an address don't stick; the bar and the Studio keep Ember's dark half (pinned), other pages follow the phone; the other ten themes stay options; plan keeper: start also clears 'R6,' from Up next

`14936f8` · 2026-10-24 08:55 · 13 files, +42 −34 · tests: quick, toolbelt

Phone check: open the bar and tap Menu: amber keys, words as typed (not capitals); the Studio opens amber too

![R6.1 page-ui.jpg](R6.1/page-ui.jpg)

![R6.1 page-ui-screen-s-bar-menu.jpg](R6.1/page-ui-screen-s-bar-menu.jpg)

## R5.4 Projects: the site's tabs are now Home, Projects, News (+ Dev for staff); /projects shows every J Production project: Bar3D (live, its page is the old Play page), Looped the engine (in progress, used by admins and devs; the old Engine page), the script language (planned), and where we're going (the old Vision page); new project folders join the list; the footer follows

`c2ffbfe` · 2026-10-24 08:00 · 7 files, +66 −18 · tests: none

Phone check: on joshuavelasco.com the tabs read Home, Projects, News; tap Projects: Bar3D, Looped, the script language and Where we're going; Bar3D opens its page

## R5.3 The front page rearranged: J Production's intro (headline and line from Dev → Publish → The site, the last sentence glowing), Enter the bar and See the bar; the highlight you pushed, big, right under it; then Bar3D, our first world (what it is, every bulb, every corner); Made here: Looped and the script language as works in progress; the news, more projects and Open. stay

`e6fd29b` · 2026-10-24 07:43 · 3 files, +41 −46 · tests: quick, site

Phone check: open joshuavelasco.com: 'J Production' over 'We build worlds. From nothing.', Enter the bar; push a highlight in Dev and reload: it's right under the intro; scroll: Bar3D, then Made here

## R5.2 Highlights in Dev → Publish: make one (a short title, one or two friendly lines, where a tap goes, one of the bar's pictures or your own photo, shrunk on the phone), see it as the front page will, Push to the front page (one at a time) or take it down, change, delete; public for the front page (dev/api.php?highlight); the step finisher's news post takes plain friendly words (--news)

`333967a` · 2026-10-24 07:33 · 8 files, +234 −14 · tests: quick, dev

Phone check: Dev → Publish → Highlights: Make a highlight, type a title and a line, tap a picture: the preview shows it; Save, then Push to the front page

![R5.2 page-dev-index-demo-publish.jpg](R5.2/page-dev-index-demo-publish.jpg)

## R5.1 The site in Ember: every page keeps its layout, boxes and reveal animations, now on the house theme's tokens: a three-colour glow drifting behind the page, frosted see-through boxes (cards, tiles, steps, news, sign-in), the amber main buttons, a light passing through the big titles (off when the phone asks for less motion); the plan test no longer depends on what's in Now

`29cf0ac` · 2026-10-24 07:28 · 4 files, +53 −13 · tests: quick, site

Phone check: open joshuavelasco.com: the same site; the boxes are frosted over a slow warm glow, and 'We built a world' has a light sweeping through it every few seconds

## R4.7 The old doors lead to Dev: /hub and /toolbelt open Dev (the old board kept as hub/board.html, unlinked; hub.php stays for news and Claude's key), /portal sends staff to Dev, the site's staff tab and sign-in say Dev, Dials, World notes and the UI Editor go back to Dev, the Studio gets a back key to Dev

`f247b0b` · 2026-10-24 06:39 · 15 files, +836 −896 · tests: quick, dev, toolbelt, hub, site

Phone check: open joshuavelasco.com/hub: Dev opens; joshuavelasco.com shows a Dev tab when you're signed in; in the Studio the back arrow (top left) goes to Dev

![R4.7 page-dev-index-demo-overview.jpg](R4.7/page-dev-index-demo-overview.jpg)

## R4.6 Dev's People tab: everyone with an account (owner and admins first, then who's waiting), Let in / Remove for players, Make admin / Remove admin for the owner only, invite by username (the bar's account.php, moved here from the gate page)

`81477b2` · 2026-10-24 06:31 · 4 files, +59 −1 · tests: quick, dev

Phone check: Dev → People: you at the top as Owner; anyone who signed up and waits has Let in

![R4.6 page-dev-index-demo-people.jpg](R4.6/page-dev-index-demo-people.jpg)

## R4.5 Dev's Publish tab: News (every post Claude writes after a step, its build, pictures and words; Publish puts it on joshuavelasco.com/news in one tap, Unpublish takes it off) and The site (the front page's headline, line, news highlight and its link, saved in Dev's store and public for R5)

`712a5ef` · 2026-10-24 06:27 · 5 files, +100 −1 · tests: quick, dev

Phone check: Dev → Publish: the R3 post is there as Draft; tap Publish, it turns Public and shows on joshuavelasco.com/news; The site: type a headline, Save the words

![R4.5 page-dev-index-demo-publish.jpg](R4.5/page-dev-index-demo-publish.jpg)

## R4.4 Dev's Plan tab: PLAN.md on the phone (Now with its pieces and checks, the road under the five chapters, Parked, Done, the North star, your rules; long lists fold), and Tell Claude: a note or an idea to park, kept in Dev's store; node tools/dev.mjs notes reads them at the start of every go (CLAUDE.md)

`c5be89e` · 2026-10-24 06:22 · 7 files, +130 −2 · tests: quick, dev, toolbelt

Phone check: Dev → Plan: R4 at the top with its pieces; the five chapters below; under Tell Claude pick 'Park an idea', write one, Send: it shows as Waiting

![R4.4 page-dev-index-demo-plan.jpg](R4.4/page-dev-index-demo-plan.jpg)

## R4.3 Dev's Overview, live: the bar's build (its config), the room server (its health: players, messages today), the last deploy (CI now writes its result to Dev's store, even when a test fails), news waiting to be published, the step in Now (how many pieces done, what's next, what comes after); Dev's store (dev/api.php: the plan, the deploy result, notes, the site's words; owner and admins); PLAN.md read in the browser; Projects drops its duplicate Open row

`340ed5a` · 2026-10-24 06:18 · 9 files, +283 −4 · tests: quick, dev

Phone check: Dev → Overview: The bar Up with its build code, Room server Up with people in the bar, Last deploy (after the next game deploy: Green or Red, tap opens the run), Now: R4 with 3 of 7

![R4.3 page-dev-index-demo-overview.jpg](R4.3/page-dev-index-demo-overview.jpg)

## R4.2 Projects in Dev: every project with its own tools from toolbelt/tools.json (Bar3D: open the bar, the Studio, Dials, World notes, perf reports, the gallery, classic, world sync; Looped: UI Editor, Screens, Compare, Style page, engine flags, benchmarks, the light tool, the UI kit; the script language: planned; the workshop: Claude's tools); a new project is one line; ?demo on this computer for pictures

`85fa081` · 2026-10-24 06:09 · 8 files, +101 −29 · tests: quick, dev, toolbelt

Phone check: Dev → Projects: Bar3D with Open Bar3D and the Studio first; tap Studio, it opens; Looped lists the UI Editor

![R4.2 page-dev-index-demo-projects.jpg](R4.2/page-dev-index-demo-projects.jpg)

## R4.1 Dev opens at /dev: the bar's login (owner and admins only; a player is told so), Ember, five tabs (Overview, Projects, Plan, Publish, People), each tab a screen file (ui/system/screens/dev-*.json, editable in the UI Editor) whose slots (parts with an id) dev.js fills with live data; Looped.part() draws single parts; a throwaway-webspace helper for PHP page tests; its test

`03c3fe4` · 2026-10-24 06:04 · 16 files, +661 −128 · tests: quick, dev

Phone check: open joshuavelasco.com/dev and log in with your bar account: five tabs at the bottom (Overview, Projects, Plan, Publish, People); each opens its page (example rows for now)

![R4.1 page-dev.jpg](R4.1/page-dev.jpg)

## R3.3 The plan keeper (tools/plan.mjs): check the shape, now, tick, add (needs its chapter and North star line), park, rule, start (a road item becomes Now with its pieces), done; refuses anything that breaks the eight parts or five chapters; piece.mjs ticks Now's pieces, step.mjs writes Done through it; a test on a copy (quick check); in the Toolbelt

`a75881c` · 2026-10-24 05:32 · 9 files, +204 −11 · tests: quick

Phone check: none on the phone; open PLAN.md: R3.3 is ticked

## R3.2 Carry over: every open thing from NEXT.md, PROGRESS.md, the 68 hub cards, the Alive plan and the rework plan into PLAN.md's road (60 items, each in its chapter with the North star line it serves) and Parked; 14 old plan files moved to docs/archive/ (with a README); NEXT.md points to PLAN.md; CLAUDE.md, REFERENCE.md and README.md read the plan from PLAN.md

`9628809` · 2026-10-24 05:27 · 20 files, +128 −137 · tests: none

Phone check: open PLAN.md: part 5 lists everything still to do under the five chapters; docs/ has no other plan files

## R3.1 The World Plan (PLAN.md): one doc in eight fixed parts (North star N1-N5, your rules, the five chapters with where each stands, Now, the road, Parked, Done with the whole build log, where things are)

`882582d` · 2026-10-24 05:25 · 1 files, +104 −0 · tests: none

Phone check: open PLAN.md on GitHub: the top says what the project is; part 4 shows R3 with R3.1 ticked

## R2.6 Ember, the house theme (the user's mix): Warm's amber and type, Glass's frosted see-through panels, a three-colour background glow that slowly drifts, titles with a light passing through them and a soft glow; light and dark halves; the Toolbelt wears it (pinned), the UI Editor and Compare start on it; the other ten themes stay as options

`b4f2fea` · 2026-10-24 05:03 · 9 files, +249 −9 · tests: quick, toolbelt

Phone check: open the Toolbelt: amber glow behind frosted cards, the title 'Toolbelt' has a light passing through it every few seconds; the UI Editor opens on Ember; Compare themes shows 'House · Ember' first

![R2.6 page-toolbelt.jpg](R2.6/page-toolbelt.jpg)

![R2.6 page-ui-screen-s-front-door-look-ember.jpg](R2.6/page-ui-screen-s-front-door-look-ember.jpg)

![R2.6 page-ui-screen-s-dev-overview-look-ember-scheme-dark.jpg](R2.6/page-ui-screen-s-dev-overview-look-ember-scheme-dark.jpg)

![R2.6 page-ui-screen-s-dev-overview-look-ember-scheme-light.jpg](R2.6/page-ui-screen-s-dev-overview-look-ember-scheme-light.jpg)

## R2.5 Motion as theme tokens: moving titles (a light passing through the letters, a breathing glow, both, or a gentle float; their paint, colour, glow and speed) and a background glow that slowly drifts; a Motion group in the UI Editor; off when the phone asks for less motion; every theme so far keeps still

`393d12e` · 2026-10-24 04:57 · 17 files, +231 −14 · tests: quick, toolbelt

Phone check: nothing moves yet on the bar, Studio or the hub; in the UI Editor, Theme → Motion → title-motion: float, the title starts to bob

![R2.5 page-ui-screen-s-front-door.jpg](R2.5/page-ui-screen-s-front-door.jpg)

## R2.4 Compare and mix: /ui/compare.html shows one screen in every theme side by side (light, dark or both; tap one for full size; in the Toolbelt); the UI Editor takes a whole group (colour, shape, type...) from another theme into this one, both halves, one undo; the editor and store tests use a free port and always stop their server

`8b9d289` · 2026-10-24 04:38 · 5 files, +135 −2 · tests: quick, toolbelt

Phone check: Toolbelt → Compare themes: six phones; switch Light / Dark / Both and the screen menu. In the UI Editor, Theme → Shape → 'Take Shape from…' Bold: the corners go square

![R2.4 page-ui-compare-s-front-door-scheme-dark.jpg](R2.4/page-ui-compare-s-front-door-scheme-dark.jpg)

![R2.4 page-ui-compare-s-dev-overview-scheme-light.jpg](R2.4/page-ui-compare-s-dev-overview-scheme-light.jpg)

## R2.3 Six candidate themes, each with a light and a dark half: Clean (calm, Apple-like), Warm (the bar at night), Bold (black frames, hard shadows, yellow), Glass (frosted panels over violet and cyan), Neon (the bar's signs), Soft (round, pastel); system fonts in the editor's font menu

`7542058` · 2026-10-24 04:30 · 10 files, +1197 −4 · tests: quick, toolbelt

Phone check: in the UI Editor pick each of Clean, Warm, Bold, Glass, Neon, Soft in the preview's theme menu and flip Dark / Light

![R2.3 page-ui-screen-s-dev-overview-look-clean-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-clean-scheme-light.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-warm-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-warm-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-bold-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-bold-scheme-light.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-glass-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-glass-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-neon-scheme-dark.jpg](R2.3/page-ui-screen-s-dev-overview-look-neon-scheme-dark.jpg)

![R2.3 page-ui-screen-s-dev-overview-look-soft-scheme-light.jpg](R2.3/page-ui-screen-s-dev-overview-look-soft-scheme-light.jpg)

## R2.2 Light and dark in one theme: a theme file can carry its other half (only the tokens that differ), the page follows the phone's setting (or ?scheme= / Looped.setScheme), the editor's Dark / Light switch shows and edits either half (Light on a dark-only theme starts a plain light half); new type tokens (word font, key case, size, weight, spacing, width) so a theme can drop the capitals; the engine reads both halves

`3386773` · 2026-10-24 04:24 · 15 files, +244 −20 · tests: quick, toolbelt

Phone check: in the UI Editor tap Light above the preview: the screen turns light; tap Dark: back. The bar and Studio look the same as before.

![R2.2 page-toolbelt-ui-editor-index-screen-dev-overview.jpg](R2.2/page-toolbelt-ui-editor-index-screen-dev-overview.jpg)

## R2.1 Real screens to judge a theme on: the front door (a bar picture behind the words), Dev's Overview (status rows, what needs you, a tab bar) and the bar's jukebox menu; new parts: backdrop, list row, tab bar; pictures in ui/system/img/

`0bdee82` · 2026-10-24 04:19 · 10 files, +922 −45 · tests: quick, toolbelt

Phone check: in the UI Editor's screen menu (top), open Front door, Dev · Overview and Bar · Jukebox; each looks like a real page

![R2.1 page-ui-screen-s-front-door.jpg](R2.1/page-ui-screen-s-front-door.jpg)

![R2.1 page-ui-screen-s-dev-overview.jpg](R2.1/page-ui-screen-s-dev-overview.jpg)

![R2.1 page-ui-screen-s-bar-menu.jpg](R2.1/page-ui-screen-s-bar-menu.jpg)

## R1.7 Save and publish in the UI Editor: undo history, a draft kept on the phone (back after a reload), Publish keeps every version (toolbelt/ui-editor/api.php, the Studio's login, Claude's key), History, the editor starts from the newest published version; node tools/uikit.mjs pull/push/status bring it into the repo (ui/system/live-version.json), pulled at the start of every go; the engine reads theme files (looped/ui_theme.*, with a native test)

`b394ba3` · 2026-10-24 04:09 · 18 files, +951 −81 · tests: quick, toolbelt

Phone check: in the UI Editor change a colour, then tap Publish (top right): log in with your bar account, write a note, Publish; open Publish again: History shows v1 with your note; reload the page: your colour is still there

![R1.7 page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg](R1.7/page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg)

## R1.6 Screen editing in the UI Editor: every option of the selected part (words, numbers, on/off, choices, icons, lists), move up and down, duplicate, delete, add a part after it or inside it, a new screen, the screen's name; deep link to a part (&sel=1.0.1.0); fix: the theme notes no longer overlap, the top bar fits a phone

`0b6722f` · 2026-10-24 04:02 · 4 files, +148 −11 · tests: quick, toolbelt

Phone check: in the UI Editor tap Publish: change its words to 'Go live', the key changes; '+ Add after…' → Chip, then the up arrow moves it before the key; the bin deletes it; undo brings it back

![R1.6 page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg](R1.6/page-toolbelt-ui-editor-index-sel-1-0-1-0.jpg)

## R1.5 Theme editing in the UI Editor: every token in 6 groups (colour, surfaces, shape, depth, type, lights) with colour pickers that keep see-through, sliders, font and choice menus and the exact text; name, about, light or dark, copy as a new theme, back to as loaded; undo and redo (one step per slider drag); deep links (?mode=theme&group=shape)

`f138572` · 2026-10-24 03:57 · 5 files, +511 −51 · tests: quick, toolbelt

Phone check: in the UI Editor tap Theme, then Shape: drag key-r and every key in the preview rounds; Colour: pick a new accent, the preview changes; the undo arrow takes it back

![R1.5 page-toolbelt-ui-editor-index-mode-theme-group-shape.jpg](R1.5/page-toolbelt-ui-editor-index-mode-theme-group-shape.jpg)

![R1.5 page-toolbelt-ui-editor-index-mode-theme-group-colour.jpg](R1.5/page-toolbelt-ui-editor-index-mode-theme-group-colour.jpg)

## R1.4 The UI Editor (toolbelt/ui-editor/): live preview on top (phone, computer, in game; any theme), the parts list and the inspector below, side by side on a computer; a tap in the preview or the list selects the part in both; its own test

`98da435` · 2026-10-24 03:52 · 9 files, +340 −6 · tests: quick, toolbelt

Phone check: open joshuavelasco.com/toolbelt/ui-editor: the sampler in the preview; tap Publish, it gets a dashed outline and 'Key' shows below with its options; tap a row in the list, the preview outlines that part

![R1.4 page-toolbelt-ui-editor.jpg](R1.4/page-toolbelt-ui-editor.jpg)

## R1.3 Screen files and the web renderer: a screen is a tree of parts (ui/system/parts.json lists the 20 kinds and their options), Looped.screen() draws one (ui/screen.js), /ui/screen.html?s=sampler shows every part; uikit checks every screen

`a6d79dd` · 2026-10-24 03:47 · 7 files, +309 −2 · tests: quick

Phone check: open joshuavelasco.com/ui/screen.html?s=sampler: keys, choices, knob, fader, meter, save lamps, icons and a see-through panel, all working; add &look=jukebox to see it in another theme

![R1.3 page-ui-screen-s-sampler.jpg](R1.3/page-ui-screen-s-sampler.jpg)

![R1.3 page-ui-screen-s-sampler-look-backbar.jpg](R1.3/page-ui-screen-s-sampler-look-backbar.jpg)

## R1.2 The theme files: every token described once (ui/system/tokens.json: group, kind, what it does), the 4 themes as files (ui/system/themes/), ui/themes.css and looped.js's theme list written from them by tools/uikit.mjs, a check that they agree (quick check and CI); every token's value is unchanged

`a21442c` · 2026-10-24 03:43 · 14 files, +580 −73 · tests: quick

Phone check: nothing should change: the bar's menus, the Studio and joshuavelasco.com/ui look exactly as before in every theme

![R1.2 page-ui.jpg](R1.2/page-ui.jpg)

![R1.2 page-toolbelt.jpg](R1.2/page-toolbelt.jpg)

## R1.1 The Toolbelt: one list of the engine's tools (toolbelt/tools.json) and one page that opens them (/toolbelt/); the piece runner takes phone pictures of site pages (--pages) and pushes to any branch (--branch)

`a28d85c` · 2026-10-24 03:39 · 7 files, +230 −7 · tests: quick

Phone check: open joshuavelasco.com/toolbelt: three groups (Make, Tune and check, Run by Claude); tap Studio, Dials and Style page, each opens; the search box filters

![R1.1 page-toolbelt.jpg](R1.1/page-toolbelt.jpg)

## L1.7 The new light in the Studio: lumens, colour temperature, shape and size change live; moods in real units (Party brighter, Closing dimmer and warmer)

`73f2d72` · 2026-10-23 03:09 · 6 files, +45 −10 · tests: native

Phone check: In the Studio, select a ceiling lamp, set Colour temperature to 2000 and Lumens to 1500: the room around it turns amber and brighter at once. Look → Closing dims and warms the bar.

## L1.6 Lights with size: tubes and panels (streak and rectangle highlights, soft area light), lamps fitted to the glowing objects beside them (light tool fit + migration), lamp-backed glow not counted twice in the bounced light

`5d99353` · 2026-10-23 01:12 · 17 files, +504 −23 · tests: changed · headless frame 97.23 ms, 288 draws

Phone check: Look at the jukebox: its light comes from its whole front and reflects as a soft rectangle on the floor; in the arcade the neon frames light like big soft squares.

![L1.6 door.jpg](L1.6/door.jpg)

![L1.6 door-switch.jpg](L1.6/door-switch.jpg)

![L1.6 mirror.jpg](L1.6/mirror.jpg)

![L1.6 mirror-switch.jpg](L1.6/mirror-switch.jpg)

![L1.6 bartop.jpg](L1.6/bartop.jpg)

![L1.6 bartop-switch.jpg](L1.6/bartop-switch.jpg)

![L1.6 backbar.jpg](L1.6/backbar.jpg)

![L1.6 backbar-switch.jpg](L1.6/backbar-switch.jpg)

![L1.6 piano.jpg](L1.6/piano.jpg)

![L1.6 piano-switch.jpg](L1.6/piano-switch.jpg)

## L1.5 Bounced light in the game: the GPU casts the build rays (looped/gi.h, shaders/gi.wgsl), one more bounce every frame, every pixel reads its 8 probes; the flat fill replaced; no lamp light through walls; debug views; shader check tool

`e390b36` · 2026-10-22 20:09 · 20 files, +1269 −13 · tests: changed · headless frame 169.82 ms, 288 draws

Phone check: After entering, look around for a few seconds: walls and corners near lamps glow softly with bounced light; the arcade's purple spills onto the floor; nothing glows through walls. ?giview=1 shows the bounced light alone.

![L1.5 door.jpg](L1.5/door.jpg)

![L1.5 door-switch.jpg](L1.5/door-switch.jpg)

![L1.5 mirror.jpg](L1.5/mirror.jpg)

![L1.5 mirror-switch.jpg](L1.5/mirror-switch.jpg)

![L1.5 bartop.jpg](L1.5/bartop.jpg)

![L1.5 bartop-switch.jpg](L1.5/bartop-switch.jpg)

![L1.5 backbar.jpg](L1.5/backbar.jpg)

![L1.5 backbar-switch.jpg](L1.5/backbar-switch.jpg)

![L1.5 piano.jpg](L1.5/piano.jpg)

![L1.5 piano-switch.jpg](L1.5/piano-switch.jpg)

## L1.4 Bounced light, the core: still surfaces gathered and simplified, a ray tree, patches, probes, lamp links, bounces (CPU reference, physics test: a white box ends at 5x its direct light); tools/light.mjs, gi benchmark

`1f6b899` · 2026-10-22 18:51 · 11 files, +1050 −20 · tests: changed

Phone check: nothing to see yet (the engine side of bounced light; it shows in L1.5)

## L1.3 Real material brightness: every material measured (tools/albedo_audit.py → assets/models/albedo.json), the ones darker or brighter than anything real moved into real ranges (new lighting); models keep their shapes for rays

`922ec64` · 2026-10-22 18:40 · 17 files, +5003 −7 · tests: changed · headless frame 69.78 ms, 288 draws

Phone check: Look at the bar top and the stage: wood grain, the mahogany's red and the green wall show clearly instead of near-black.

![L1.3 door.jpg](L1.3/door.jpg)

![L1.3 door-switch.jpg](L1.3/door-switch.jpg)

![L1.3 mirror.jpg](L1.3/mirror.jpg)

![L1.3 mirror-switch.jpg](L1.3/mirror-switch.jpg)

![L1.3 bartop.jpg](L1.3/bartop.jpg)

![L1.3 bartop-switch.jpg](L1.3/bartop-switch.jpg)

![L1.3 backbar.jpg](L1.3/backbar.jpg)

![L1.3 backbar-switch.jpg](L1.3/backbar-switch.jpg)

![L1.3 piano.jpg](L1.3/piano.jpg)

![L1.3 piano-switch.jpg](L1.3/piano-switch.jpg)

## L1.2 Real units: every lamp in lumens and Kelvin, true inverse-square light with no edge, white balance like eyes; world lights migrated (tools/world_migrate.mjs)

`1975523` · 2026-10-22 18:25 · 23 files, +440 −93 · tests: changed · headless frame 76.03 ms, 288 draws

Phone check: Stand at the door: lamp light now spreads across the whole room in soft pools, brightest under each lamp, with no edge where it stops. Bulbs look warm, not orange.

![L1.2 door.jpg](L1.2/door.jpg)

![L1.2 door-switch.jpg](L1.2/door-switch.jpg)

![L1.2 mirror.jpg](L1.2/mirror.jpg)

![L1.2 mirror-switch.jpg](L1.2/mirror-switch.jpg)

![L1.2 bartop.jpg](L1.2/bartop.jpg)

![L1.2 bartop-switch.jpg](L1.2/bartop-switch.jpg)

![L1.2 backbar.jpg](L1.2/backbar.jpg)

![L1.2 backbar-switch.jpg](L1.2/backbar-switch.jpg)

![L1.2 piano.jpg](L1.2/piano.jpg)

![L1.2 piano-switch.jpg](L1.2/piano-switch.jpg)

## L1.1 Eyes: the scene drawn as real light (HDR), metered and adapting like eyes, the AgX film curve; Settings → Lighting New/Classic

`31c8f7b` · 2026-10-22 17:55 · 15 files, +437 −86 · tests: game · headless frame 54.47 ms, 288 draws

Phone check: Walk from the bright bar counter to the dark stage corner: the picture slowly brightens as your eyes adjust. Settings → Graphics → Lighting: flip New and Classic, highlights look softer in New.

![L1.1 door.jpg](L1.1/door.jpg)

![L1.1 door-switch.jpg](L1.1/door-switch.jpg)

![L1.1 mirror.jpg](L1.1/mirror.jpg)

![L1.1 mirror-switch.jpg](L1.1/mirror-switch.jpg)

![L1.1 bartop.jpg](L1.1/bartop.jpg)

![L1.1 bartop-switch.jpg](L1.1/bartop-switch.jpg)

![L1.1 backbar.jpg](L1.1/backbar.jpg)

![L1.1 backbar-switch.jpg](L1.1/backbar-switch.jpg)

![L1.1 piano.jpg](L1.1/piano.jpg)

![L1.1 piano-switch.jpg](L1.1/piano-switch.jpg)

## L0 Automation: piece runner, step finisher, benchmarks, gallery branch, manual pages, feature switches

`69fb05c` · 2026-10-22 17:28 · 18 files, +688 −0 · tests: none

Phone check: nothing to see: tools only

