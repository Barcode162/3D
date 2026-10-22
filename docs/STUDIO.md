# The Studio, the Dials, and the new look (plan, 2026-10-21)

The user's request (chat, 2026-10-21): "I created an engine, I want a real way
to use it." A **Studio** at joshuavelasco.com/studio where the user builds and
changes the world themselves, by touch, on the iPhone. **No AI inside it** for
now (the user works with Claude through the Claude Pro chat only). The **hub**
stays the barebones place: every dial listed, for tweaking and for Claude. All
of it has to fit together, and **everything gets a new look** (the user
dislikes every current theme: the bar's Clash Royale style, the hub's purple
neon, the news page, the home page).

The user pasted a generic "touch world editor with an AI collaborator" plan
and asked for it to be reworked for this engine. Section 2 says what was
kept, changed, cut and moved to later, and why.

This file is the plan. `NEXT.md` holds the next step's pieces;
`docs/ENGINE.md` is the engine the Studio sits on.

---

## 1. In plain words (for the user)

- **There is one world.** The bar, the test home, every place you'll make are
  files: things, where they sit, what they look like, how they move. Today
  only Claude edits those files, and a change takes a 6-minute deploy.
- **The Studio is where you edit that world yourself.** You open
  joshuavelasco.com/studio (or its icon on your home screen) and you're
  standing in your bar. Tap a stool and it lights up. Drag it, turn it, make it
  taller, copy it, delete it. Build walls and floors out of boxes. Change a
  light's colour. Change how a thing moves (its loop) the way you'd edit a
  beat in FL Studio. Then press **Publish** and the bar changes for
  everyone. No deploy, no Claude needed.
- **The hub is the dials.** The same world as a plain list: every kind,
  every thing, every number, in words, with a slider. It's the fastest way
  to change one number and the clearest way to show Claude exactly what you
  mean. It also stays Claude's board (what needs you, the news).
- **News** shows the world what changed. The **front door** (joshuavelasco.com)
  leads to all of them.
- **Claude still builds the engine.** Whenever the Studio needs a new kind of
  control (a new part, a new loop shape), Claude adds it to the engine, and it
  shows up in the Studio and in the hub by itself, because both read the same
  description of the world (the schema, section 7.3).
- **New look for everything: the faceplate.** Everything looks like the
  hardware loops are made on (a drum machine): flat printed panels, small
  capital labels, keys you press, dials and faders, and four step colours
  (red, orange, yellow, cream) that light up in order wherever something is
  alive. Section 4. A preview page shows it before anything gets built.

---

## 2. Your pasted plan, reworked for this engine

| From the pasted plan | Here | Why |
|---|---|---|
| World fills the screen, controls around the selection | **Kept** | Right for a phone. The 3D view stays full screen; tools live in the bottom third, where your thumb reaches. |
| Undo / Redo / Play in the top bar | **Moved down** | The top of an iPhone is out of thumb reach. The top bar holds only the place's name, the save state and Publish (rare, and safer far from your thumb). Undo/redo go in the bottom bar, plus the gestures every iPad art app uses: **two-finger tap = undo, three-finger tap = redo**. |
| Object / Face / Edge selection | **Thing / Part / Face** | This engine has no editable meshes: models come from Blender, and what you build yourself is **boxes** (`boxes` parts, E7.1). So the ladder is *Thing* (a stool) → *Part* (its seat, its shape you bump into, its tap area, its light, its sound) → *Face* (one side of a box). Edges have nothing to edit on a box, so there's no edge mode. Mesh editing is in "Later". |
| Magnifier lens, candidate list, lock, breadcrumb | **Kept, all four** | Tiny things (candles, glasses, chess pieces) need them. The breadcrumb follows the engine's real parent chain: `Bar › counter › pint_glass-4`. |
| Extend a face, anchor stays fixed | **Kept, made exact** | On a box, "extend one side" is literally "move one face, keep the opposite one". That's **Push/Pull**. |
| Extrude (new geometry from a face) | **Kept, our version** | **Extrude** makes a *new box* glued to that face (a shelf out of a wall, a step out of a floor). Same feel, no mesh code. |
| Axis lock, world/local, numbers, fine scrub, anchor marker | **Kept + better** | The big scrubber gets the iPhone video-scrubbing trick: **slide your finger up off the strip for finer control**. "Local" means relative to the thing it sits on (that's how the engine stores positions). The iPhone number pad has no minus key, so the value field gets a ± key. |
| Arrows show direction, a big control does the precision | **Kept** | The gizmo arrows are for showing and quick drags; the dial strip is for exact values. |
| Context panel changes with what you touch | **Kept, generated** | The panel is built from the schema (7.3): a light shows light controls, a box shows box controls, anything Claude adds later shows its own controls without new Studio code. |
| Pull-out monitor: World / AI activity / Changes | **The Monitor**: World · Inspect · Library · Changes · Problems | No AI tab (no AI inside). **Changes** shows every edit in plain words with *who* made it: you, or Claude (when Claude edits the world during a "go"). That's the AI activity log, honestly labelled, without an AI in the app. |
| Overhead live view in the drawer | **Top camera** instead | Drawing the world twice costs the phone a lot of power. One tap switches the main camera to a top view (a map), and back. |
| Preview → refine → commit, grouped undo | **Kept** as Draft → Publish | Every edit goes into your **draft** (saved by itself, every few seconds). The live bar only changes when you press Publish. Every publish is a version you can go back to. |
| AI permission levels, voice, "Change this…" box | **Later** | No AI inside for now. The Changes/Draft system is built so an AI can later be "just another author" whose changes you preview and apply (section 9). |
| Haptics | **Sound + light** | Safari on iPhone can't vibrate. Snaps make a soft tick (the engine's own synth) and a flash. |
| "Fluency" rules (instant selection, camera vs edit separated, never lose your place) | **Kept, as rules** | Section 5.9. |

What the pasted plan didn't have and this engine makes possible:
- **Editing life, not just shape.** Every thing in this engine has states,
  rules and loops. The Studio edits those too: the **Loop Rack** (section
  5.7), your FL Studio idea for things.
- **Publish reaches everyone live.** The room and every phone pick up the new
  world in seconds (step S9).
- **This one, or every one of them.** Change one stool, or every bar stool
  in the bar at once (thing vs kind).
- **The hub and the Studio can't drift apart**, because both are drawn from
  one description of every field (the schema).

---

## 3. Decisions (my pick first; say the word to change one)

- **D1 One world store, on your own server.** The live world (every place,
  kind, the catalog) lives on IONOS in a versioned store next to the hub
  (`private/studio/`). The bar and the room read it when they start, with the
  copy built into the game as a fallback. *Why:* publishing is instant, costs
  no GitHub Actions minutes (the free plan has a monthly cap and a full
  deploy takes ~25 runner minutes), and needs no GitHub password on the
  server. The repo keeps a mirror: Claude pulls the live world into git at the
  start of every "go" (so git history shows your studio work) and publishes
  its own world changes back through the same door.
- **D2 One engine build, two doors.** The Studio runs the bar's own engine
  (`app.wasm`) in an **edit mode**; `/studio/` is a different page around it
  with its own panels. *Why:* what you see in the Studio *is* the bar (same
  renderer, lights, loops), there's one thing to build and test, and players
  only download the small C++ side of the editor (the panels are a separate
  script only the Studio loads). *Not* three.js: the no-outside-engines rule
  stays.
- **D3 One schema for every field.** `assets/world/schema.json` describes
  every part and field in plain words: label, unit, range, step, which control
  to show. The Studio's panels, the hub's dials and the checks all read it. A
  test fails if a world file uses a field the schema doesn't describe, so it
  can never fall behind.
- **D4 Drafts and versions, not live editing.** You edit a draft; Publish
  makes it a new version; History restores any version in one tap. Claude's
  world changes are versions too, labelled "Claude".
- **D5 Thing vs kind is always visible.** Every edit says whether it changes
  *this one* or *every bar_stool (12)*. Default: this one for where it is
  (move/turn), every one for what it is (shape, loop, sound), with a switch.
- **D6 Owner (and admins you choose) only.** Logins are the accounts you
  already have. Guests and players never see the Studio.
- **D7 The look is one system** (`ui/`), the faceplate: shared colours,
  fonts, icons and controls for the bar's menus, the Studio, the hub, news
  and the front door. Studio and bar always on the dark smoke panel (they sit
  on a 3D view); hub, news and the front door follow your phone's light/dark
  setting.
- **D8 Install it like an app.** The Studio (and the bar) get a home-screen
  icon ("Add to Home Screen"): full screen, no Safari bars stealing space or
  gestures.
- **D9 Models still come from Claude/Blender.** The Studio places every model
  the build has, and builds with boxes. Importing your own `.glb` is "Later"
  (it needs the texture-packing step that runs at build time).
- **D10 No AI inside, but ready for it.** Section 9.

---

## 4. The new look: themes

### 4.0 Themes (user, 2026-10-22)
The user liked the faceplate and asked for **themes**: the look is a theme
system, not one fixed style. They asked for more depth (a 3D feel), a
deeper dark, gloss, glass "sometimes", and **more visual feedback lights**
(the step light was the favourite). Blender may render UI parts where CSS
can't get the shine right.

- **How:** `ui/themes/<name>.css` sets the tokens: colours *and* material
  recipes (panel fill and trim, key fill, gloss, press depth, glass blur, the
  shape of the step light and the LEDs, the key click sound, the display
  font). Every control in `ui/controls.css` is drawn only from those tokens,
  so one markup wears every theme. The player picks a theme in Settings (the
  bar, the Studio, the hub share the choice; saved per account and device).
- **Candidates** (preview page, 2026-10-22): **Faceplate** (the drum machine,
  deeper, glossy plastic keys, keycap lights), **Back Bar** (the bar's own
  lacquered wood and brass, Edison bulbs, amber glass), **Obsidian** (deepest
  black, frosted glass panels, piano-black keys, ice light inside glass
  prisms), **Jukebox** (chrome rims, candy jelly keys, bubble tubes, black
  cherry). **Waiting on the user's pick** for which ships first (S1.1); the
  others follow as extra themes.
- **The feedback kit** (every theme, its own style): the step light (alive,
  at the real loop speed), an LED in every switch-like key (on/off), the save
  lamps (edit · saving · saved · live), level meters (a loop's movement
  live), state lamps (which state a thing is in), a dial with a ring of
  lights, a fader whose value pops up while held, a flip switch, numbers that
  roll with what changed popping beside them, and a breathing ring on what's
  selected (in lists and around the thing in the world).
- **Blender parts** (`tools/assets/ui_*.py`, rendered to images with
  licence-free materials): knob strips (a frame per angle), Edison bulbs
  on/off, chrome and brass bezels (nine-slice), bubble tubes, when the CSS
  version doesn't look rich enough on the phone.

Sections 4.1-4.6 below describe the **Faceplate** theme in detail (the first
one designed); each other theme gets the same sections when it's built.

## 4.F The Faceplate theme

### 4.1 The idea
Your engine is built on **loops** (your FL Studio idea), the hub is **the
dials**, and the new place is a **studio**. So the whole system looks like
the hardware loops are made on: a drum machine's faceplate. Flat printed
panels, small silkscreen labels in capitals, real-looking keys and dials,
and **four step colours** (red, orange, yellow, cream) that light up in
order like a step sequencer. The world (the 3D view) is where the rest of
the colour is.

- **The step light** is the sign that something is alive: four small
  squares lighting one after another, at that thing's real loop speed (a
  slow lamp sway steps slowly, the jukebox steps on the beat). It's also
  the logo (four keys, one lit), the loading sign and the "saved" sign.
- **The four step colours mean "loop"** everywhere: the lanes of the Loop
  Rack, a thing's states (state 1 red, 2 orange…), the steps of a plan. They
  are never decoration.
- **Orange is "do it"** (the main button, the selection in the Studio).
  **A red LED dot is "live"** (published, someone in the room, recording).

What goes away: the bar's blue bevelled panels, gold glossy buttons and
outlined Lilita font (the Clash Royale style); the hub's purple neon glass
and pink gradients; the emoji tab icons; the 3D-rendered menu icons (badges
stay 3D: they're collectibles, not buttons). Also not chosen: a plain
near-black app with one bright accent (it's what most AI-made apps look like
right now).

### 4.2 Colours (tokens in `ui/tokens.css`)
Two panels, one per mode. Studio and bar are always the smoke panel (they
sit on a 3D view); hub, news and the front door follow the phone's
light/dark setting.

| Token | Faceplate (light) | Smoke (dark) | Use |
|---|---|---|---|
| `--plate` | `#E6E2DA` | `#1A1918` | the panel (page background) |
| `--plate-2` | `#D9D4CB` | `#242220` | recessed areas, fields, rows |
| `--plate-3` | `#CBC5BA` | `#2E2B28` | keys at rest |
| `--rule` | `#BDB6AA` | `#3A3632` | printed lines (1 px) |
| `--print` | `#1C1B19` | `#ECE6DA` | text (silkscreen ink) |
| `--print-2` | `#5C5850` | `#A39C90` | labels, secondary text |
| `--print-3` | `#8C867C` | `#6E685F` | hints, disabled |
| `--step-1` | `#D9402A` | `#EF4B33` | red: step 1, problems' red is separate |
| `--step-2` | `#E57A1F` | `#F28C2C` | orange: step 2, **the main action, selection** |
| `--step-3` | `#D9A91E` | `#F2C744` | yellow: step 3 |
| `--step-4` | `#FBF8F2` | `#EDE7D9` | cream: step 4 (keeps a printed edge in light mode) |
| `--led` | `#FF3B2F` | `#FF4A3D` | the live dot (with a soft glow) |

Meaning colours (separate from the steps): ok `#3FAE6B` / `#56CF86`,
warning = step yellow, problem `#C8322F` / `#FF6A64`, info `#3C7BD6` /
`#7AB0FF`. **Axes** in the Studio: X `#EF4B33`, Y `#56CF86`, Z `#5B9BFF`,
always with their letter beside them (colour-blind safe).

### 4.3 Type
- **Archivo** (free, SIL Open Font License, one variable file with a *width*
  axis; self-hosted in `ui/fonts/` with its licence and a credits entry). Wide
  for titles and place names, normal for text, **condensed capitals with a
  little spacing for labels** (the silkscreen look: `SPEED`, `PERIOD`,
  `MATERIAL`).
- **Martian Mono** (free, OFL; also self-hosted) for every number and id:
  the read-outs on the dial strip, the hub's dials, prices, build codes.
  Tabular, so columns of numbers line up.
- Body text can fall back to the phone's font (SF Pro) where Archivo hasn't
  loaded yet; nothing waits for fonts.
- Sizes (iOS scale): 34 · 28 · 22 · 17 (body) · 15 · 13 · 11 (labels).

### 4.4 Shapes, depth, motion, sound
- Panels are flat and printed: no gradients, no glass, no glow (except the
  LED). Over the 3D view only, a panel is the smoke colour at 88 % with a
  20 px blur, so text stays readable.
- **Keys** (buttons) are square-ish (8 px corners), with a 2 px darker lower
  edge that disappears when pressed (they go down 2 px): the one bit of
  depth, because a key you press should feel like a key. Lit keys glow in
  their step colour.
- **Dials and faders** look like hardware: a round dial with a pointer line
  for angles and loops, a long fader for amounts, a numeric read-out above
  each.
- Corners: 4 (chips), 8 (keys, fields), 16 (sheets). Spacing on a 4-point
  grid. Tap targets at least 44 × 44.
- Motion: 100–200 ms, mechanical (a key goes down and up, a sheet slides
  like a drawer). The only things that move by themselves are alive (step
  lights, LEDs).
- Sound: soft clicks from the engine's synth for key presses, snaps and
  publish (Safari can't vibrate on iPhone). Off in Settings.

### 4.5 Icons
One set, drawn for this project (no third-party set): 24-point grid, 2 px
square-ended strokes (printed, not rounded), about 40 to start (move, turn,
size, push, extrude, duplicate, delete, undo, redo, layers, light, sound,
loop, rule, publish, history, camera modes, lock, search, place, kind, part,
face…). One SVG sprite, `ui/icons.svg`, used by every page.

### 4.6 Per place
- **Bar** (menus, HUD, cards over the 3D view): smoke panels, orange keys
  for the main action, Archivo titles, line icons; the credits read-out in
  Martian Mono. The in-world name tags keep the engine's pixel font for now
  (Later: Archivo drawn by the engine).
- **Studio:** smoke panels, orange selection, axis colours on the gizmo, the
  dial strip as a hardware fader with a read-out.
- **Hub:** the faceplate itself. Rows of dials, mono read-outs, printed
  rules, step lights on everything alive. Less decoration than anywhere else.
- **News and the front door:** the faceplate as a poster: big wide Archivo,
  screenshots in printed frames, lots of space.

---

## 5. The Studio on your phone

### 5.1 The screen
```text
┌──────────────────────────────────────┐
│ ◌ Bar ▾          Draft · saved   Publish │  ← top: place, save state, Publish
│                                          │
│                                          │
│            YOUR BAR, FULL SCREEN         │
│                    ↑Y                    │
│            ┌────────────┐                │
│            │  selected  │──→X            │  ← orange outline + axis arrows
│            └────────────┘                │
│                  ↙Z                      │
│  ╭ Bar › counter › pint_glass-4  🔒 ✕ ╮  │  ← breadcrumb chip
├──────────────────────────────────────────┤
│  Move   Turn   Size   Push   ···         │  ← tools for this selection
│  [X][Y][Z][↗]  world|local   ⌗ 5 cm      │  ← axis, space, snap
│  X  -0.004 m  [±]  ◀━━━━━━●━━━━━━▶       │  ← value + big scrubber
├──────────────────────────────────────────┤
│  ▔▔▔ Monitor    ↶  ↷    Thing|Part|Face  ▶│  ← drawer handle, undo, mode, play
└──────────────────────────────────────────┘
```
Nothing selected: the tool rows fold away and the view gets the space; the
bottom bar shows the camera mode and "Add".

### 5.2 Moving around (camera)
| Gesture | Does |
|---|---|
| One finger drag on empty space | orbit around the point you're looking at |
| Two fingers drag / pinch | pan / zoom |
| Double tap a thing | fly to it and frame it |
| Camera button | Orbit · Fly (two thumbs, like walking in the bar) · Top (map) · Walk (eye height, collisions) |
| Views | save the current view, jump back to one |
| ▶ Play | play the place as a player (walk, tap, sit), then back to editing at the same spot |

### 5.3 Picking exactly what you mean
1. **Tap** a thing: orange outline, breadcrumb shows where it sits.
2. Things overlap (a glass on the counter in front of the back bar)? A
   small list pops up with everything under your finger, nearest first.
3. **Long press**: a magnifier lens appears above your finger; slide to the
   right spot, let go.
4. **Mode Thing / Part / Face:** Part selects one of the thing's parts (its
   shape you bump into, its tap area, its seat, its light, its sound). Face
   selects one side of a box.
5. **🔒 Lock**: the selection stays while you move the camera freely.
6. The **World** list in the Monitor picks anything by name, including things
   you can't see.
7. **X-ray layers** (Monitor → World): show the invisible parts: shapes you
   bump into, walk blockers, tap areas, seats, speakers, how far lights reach,
   where players spawn.

### 5.4 Changing it
- **Move / Turn / Size**: drag an arrow for a quick change, or use the dial
  strip for exact values: pick the axis (X, Y, Z or ↗ along the face), world
  or local, type a number (with a ± key) or drag the big scrubber. Slide your
  finger *up* off the scrubber for 10× finer control, *down* back for coarse.
  Snapping (5 cm, 15°) is on by default; tap ⌗ to change or turn it off.
- **Slide on surfaces**: drag the thing itself and it slides along the floor,
  the counter, a table, staying on top of it. **Drop** puts it down on what's
  under it. **Put on…** makes it sit on another thing (a glass onto a tray:
  move the tray, the glass follows).
- **Push/Pull** (a box face): that side moves, the opposite side stays put
  (the anchor is marked).
- **Extrude** (a box face): a new box grows out of that face.
- **Duplicate**, **Delete**, **Row** (copy it N times, a step apart: a row of
  stools).
- **Inspect** (Monitor): every field of the selection, in words, with the
  right control (numbers with the dial, colours with a wheel, choices,
  switches). Fields that differ from the kind have a dot; "Reset to kind".
- **This one | Every bar_stool (12)**: a switch on every edit.

### 5.5 Adding and building
- **Add** (Library): every kind in the build, with a picture the engine draws
  itself, its catalog name and price. Tap one and it appears in front of you
  on the nearest surface.
- **Box**: draw a rectangle on the floor (or a wall) with your finger; it
  becomes a box you can push, pull, extrude, give a material and make solid
  or walk-through. Floors, walls, shelves, a stage, a whole room.
- **Places**: a new empty place, or a copy of one; set where players appear;
  it shows up in the bar's travel menu.

### 5.6 Light and look (step S7)
Lights: colour wheel, brightness, reach, softness, dims with the switch.
Materials become data (wood, brass, brick… plus your own: colour,
roughness, metal, glow). Moods: named sets of lights (Evening, Party,
Closing). A budget meter keeps the phone fast: draws, triangles, lights,
against the limits the phone can hold at 60 fps.

### 5.7 Life: states, rules, the Loop Rack (step S8)
Your FL Studio idea, for things.
- **States** as chips: `idle` `swinging`. **Rules** in words: "When tapped →
  swinging", "After 2 loops → idle", "When the music starts → playing", "On
  entering: tell everything *fire-off*". A small diagram shows them as a
  loop. **Test** buttons send an event (tap, music, quiet) so you see the rule
  work.
- **The Loop Rack**: the selected state's loop as lanes, one per channel
  (move up/down, turn, size, glow, light, colour…). Each lane has a shape
  (wave, keys, noise, flicker, beat, spin, orbit, steps) and its amounts.
  Period in seconds or beats, locked to the music or not. The thing plays your
  edit live while you change it. **Keys**: draw the curve with your finger.
  Pause and scrub the clock to look at one moment. Copy a loop from another
  kind; presets (sway, breathe, bob, flicker, thump).
- **Sounds**: pick an effect, ▶ to hear it, how often, how loud, how far it
  carries, in which states; the sound a thing makes when you touch it.

### 5.8 Saving, publishing, going back
- Every edit lands in your **draft**, saved to the server every few seconds
  (and on the phone, in case the signal drops). "Draft · saved" at the top.
- **Changes** (Monitor) lists the draft in plain words: "pint_glass-4 moved
  12 cm left", "every bar_stool: idle loop 2.4 s → 3 s". Tap a line to fly
  there. Swipe to undo just that change.
- **Problems** must be clear to publish: things with no life, rules pointing
  at nothing, a model missing, over the phone's budget.
- **Publish**: one tap, a short name if you want ("new stage"). The world
  becomes version N+1. The bar picks it up on load (S3) and live, while people
  are in it (S9).
- **History**: every version, who made it (you or Claude), what changed;
  **Restore** puts any of them back (as a new version, so nothing is lost).

### 5.9 Rules the Studio keeps (fluency)
- A tap answers at once (the outline appears the same frame), even while
  saving.
- Moving the camera never changes the selection; editing never moves the
  camera unless you ask (Frame).
- Opening and closing the Monitor never resets your view or selection.
- Every change can be undone, one gesture at a time (a whole drag is one
  undo), and the draft can always go back to the live version.
- Anything slow shows the step light with words ("Publishing…"); nothing
  freezes.

---

## 6. The hub as the Dials

The hub becomes four things, in the new look:
1. **Needs you**: Claude's questions, things to check on the phone (as now).
2. **Dials**: the whole world as a list: places → things → parts → fields,
   and kinds → states → loops → pieces. Every number with its unit and a
   slider, every choice as chips, search everything. A change goes into the
   same draft the Studio uses; Publish is the same button. Each row has
   "Open in Studio" (jumps to that thing, selected). Today's World page
   (E8.1) and its speed/off tuning fold into this: tuning becomes real
   numbers in the world (loop periods), so there's only one place where
   things are set.
3. **Board**: the cards and places (Explore), for planning with Claude.
4. **News**: the posts, with Publish to the public board.

The hub stays the plain one: no 3D, fast to load, dense. It's where you'd
say "make *this number* bigger" and where Claude points you to "this dial".

---

## 7. How it works underneath (for Claude)

### 7.1 Folders
- `ui/`: the look. `tokens.css` (faceplate/smoke), `base.css` (type, page),
  `controls.css` (keys, chips, segmented, fields, list rows, sheets, dials,
  faders, the dial strip, the step light, the LED), `icons.svg`, `fonts/`
  (Archivo, Martian Mono, OFL + credits), and
  `index.html`, a living style page showing every control (screenshot-tested).
  Copied into each site at build/deploy (`/ui/` on IONOS, `bar/ui/` in the bar
  build) so every page links the same files.
- `looped/world/schema.*`, `edit.*`, `diff.*`, `bundle.*`: engine-generic
  (no bar): the schema reader, edit operations with inverses, the plain-words
  diff, the world bundle (all files of one version). Native tests.
- `bar/studio/`: the edit mode inside the game (C++): picking, outline,
  gizmo drawing, cameras, applying ops to the running world, thumbnails.
- `studio/`: the Studio page (`index.html`, `studio.js`, `studio.css`,
  `manifest.json`, icon) and its server (`api.php`).
- `tools/studio.mjs`: Claude's side (pull, push, log, draft, publish).

### 7.2 The world store (`studio/api.php`, data in `private/studio/`)
- `versions/<n>.json`: a **bundle**: `{"format": 1, "version": n, "author",
  "at", "note", "files": {"places/bar.json": {...}, "kinds/bar.json": {...},
  "catalog.json": {...}, ...}}`. Immutable once written.
- `live.json`: `{"version": n}`. `drafts/<user>.json`: a bundle + `base` (the
  version it started from).
- Actions: `live` (public: version number + bundle URL), `bundle&v=n`
  (public, immutable: served with a long cache), `draft` get/save, `publish`
  (base must equal live, else "update your draft first": the Studio merges
  with `looped/world/diff` + ops and shows what came in), `history`,
  `restore`. Owner/admins by account token, Claude by the hub key. Size cap
  (2 MB a bundle), JSON shape checked, last 100 versions kept plus one a day.
- `format`: the bundle's format number. A bundle needing a newer engine than
  the one running is ignored (the built-in world is used, and the log says
  so), so data and code can't break each other.
- Free-only: PHP files on IONOS, no database, no paid service.

### 7.3 The schema (`assets/world/schema.json`)
For every part and field: `label` (plain words), `about`, `unit` (m, °, s,
beats, ×, colour), `min/max/step`, `control` (dial, slider, colour, choice,
switch, text, vec3, box, list), `choices` (materials, effects, actions,
seat kinds: today those lists are in `bar/scene_names.cpp`; they move into the
schema and the C++ reads them back, one list), `scope` (place, thing, kind),
and `live` (whether a change can apply without rebuilding the scene). The
Studio's Inspect panel, the hub's Dials and the server's checks are all
generated from it. Test: every field in every world file is in the schema;
every schema choice is one the engine knows.

### 7.4 Edits, undo, diff
- An **op**: `set(path, value)`, `add(thing)`, `remove(thing)` (with its
  children), `attach(thing, parent)`, `rename(thing, id)`, each with its
  inverse. Paths name things by id and kinds by id (`thing:pint_glass-4/at`,
  `kind:bar_stool/loops/idle/period`), never by array index, so merges are
  safe.
- Undo/redo: a stack of op groups (one per gesture). The draft is the bundle
  after all ops; ops don't need to be stored on the server.
- **Diff** (`looped/world/diff.*`): two bundles → changes in plain words,
  per thing and kind ("moved 12 cm left", "turned 15°", "idle loop 2.4 s →
  3 s", "added 3 bar_stools"), used by Changes, History, the hub and Claude's
  news posts.
- New things get ids `<kind>-<n>` (the next free number); ids never change
  when things move, so net ids (FNV-1a + probing) and shared states stay put.

### 7.5 Edit mode in the game (`?studio`, `bar/studio/`)
- The studio page loads the bar's `app.js`/`app.wasm` with `<base
  href="/bar/">` (assets shared, nothing duplicated) and turns on edit mode
  through the mailbox (`bridge.*`): no room, no HUD, no player body; the world
  keeps playing its loops; the frame runs edit phases (input → pick → tools →
  world → render + overlays).
- Picking reuses the touch ray (`touchAt`: model boxes, solids, boxes) and
  returns *every* hit along the ray (for the candidate list).
- Overlays: selection outline (a second pass of the selected model in step orange,
  scaled along normals), gizmo arrows/rings (axis colours, drawn last, no
  depth test), X-ray layers (wire boxes, seat markers, light spheres).
- Applying an op: fields marked `live` in the schema change the running world
  directly (positions, loop numbers, light colours); others rebuild the scene
  from the draft bundle (a few hundred ms, models stay cached).
- Thumbnails: the engine renders each kind once into a small image
  (offscreen), cached on the server by kind + model + version.

### 7.6 The bar and the room read the live world
- At load, the game asks `/studio/api.php?live` (tiny), then fetches the
  bundle (cached by version). On failure, or a newer format, it uses the world
  built into the game. Log: "World: v12 (live, by Joshua)".
- The room (Cloudflare) loads the same bundle when it starts (fetch from
  joshuavelasco.com, kept in its storage by version) and hands the files to
  its C++ world (`sim_text`, already file-based). The welcome carries the
  room's world version; a phone on another version switches to the room's
  (versions are immutable, so any phone can fetch any version). Everyone in a
  room always runs the same world.
- S9 adds **hot reload**: after a publish the room loads the new version and
  tells the phones; they rebuild the world in place ("The bar just changed").
  States of things that still exist are kept.
- A **draft room** (S9): the room can run a draft (owner's link), so you can
  try a change with a friend before publishing.

### 7.7 Claude's routine with the store (`tools/studio.mjs`)
- Start of every "go": `node tools/studio.mjs pull`: the live bundle →
  `assets/world/*.json` (formatted with `tools/place_format.py`), committed as
  "World: from the studio v12 (Joshua: 4 changes)" if anything changed.
- Claude edits world files in git as today (tests run on them). After the
  step's deploy is live (so the running engine understands any new fields):
  `node tools/studio.mjs push --note "<piece>"`: publishes the repo's world as
  a new version authored "Claude", refused if the live world moved since the
  pull (then pull, merge, push again).
- CLAUDE.md's "go" routine gets these two lines when S1 lands.

### 7.8 Safety
- World data is data: no code runs from it. The server checks shape and size,
  the game checks it again (`World::check`, the schema), the room checks it
  before running it.
- Only owner/admin tokens (and Claude's key) write; versions are immutable;
  restore is always possible; a draft never touches the live bar.
- A publish with problems is refused; "Undo publish" in the Studio and the hub
  restores the version before in one tap.

### 7.9 Phone limits to respect
- WebGPU on iPhone Safari (already used by the bar). Memory: the Studio runs
  the same scene as the bar plus overlays; no second 3D view.
- `touch-action: none` on the view (no page zoom on double tap), Add to Home
  Screen for full screen (no Safari bottom bar, no edge-swipe back).
- Number fields: `inputmode="decimal"` has no minus key → the ± key.
- No vibration in Safari → sound ticks and flashes.
- Performance judged on the phone: `?perf=1` works in the Studio too; the
  budget meter uses the limits measured on the user's iPhone (E2: 144 draws,
  160k triangles at 60 fps).

### 7.10 Tests
- Native: schema completeness, edit ops and inverses (every op undone gives
  the same bundle), diff wording, bundle load (old/new format), merge.
- `tools/studio_api_test.mjs` (PHP, like hub_test): drafts, publish, base
  check, history, restore, roles, size caps.
- `tools/studio_test.mjs` (headless): open the Studio, tap a stool (outline +
  breadcrumb), move it 1 m with the dial, publish, open the bar, it's moved;
  restore, it's back. Screenshots of each panel for the news post.
- `tools/ui_test.mjs`: the style page renders every control, light and dark,
  no overflow at 375 and 430 px wide.
- `node tools/check.mjs` gets a `studio` system.

---

## 8. The steps (one "go" each; one commit per piece; a phone check per piece)

Order: the foundations and the look first, the first real Studio by S3, the
new look everywhere at S4, then precision, building, light, life, live, and
the bridge to Claude.

### S1 One world, and the new look begins
- **S1.1** The look: `ui/` as a theme system (`ui/themes/`), the theme the
  user picks first (section 4.0), the feedback kit (step light, key LEDs,
  save lamps, meters, state lamps, ring dial, fader, switch, rolling
  numbers, selection ring), the fonts with licences + credits, ~40 icons,
  and the style page joshuavelasco.com/ui with a theme switcher. *Check:* open /ui: every
  control on the smoke panel; switch your phone to light mode, it turns to
  the faceplate.
- **S1.2** The front door in the new look: joshuavelasco.com with three doors,
  Bar · Studio · News. *Check:* the home page, three doors.
- **S1.3** The schema: `assets/world/schema.json` for every part and field in
  the bar and the home; the choices lists move out of `scene_names.cpp`; the
  completeness test. *Check:* nothing changes in the bar (still everything
  alive).
- **S1.4** The world store: `studio/api.php` (live, bundle, draft, publish,
  history, restore) + `tools/studio_api_test.mjs`; the first version (v1) is
  today's repo world. *Check:* joshuavelasco.com/studio/api.php?live shows
  version 1.
- **S1.5** The bar and the room load the live world (fallback: built in;
  format number; the room's version wins). *Check:* the log says "World: v1
  (live)".
- **S1.6** Claude's side: `tools/studio.mjs pull/push/log`, the routine in
  CLAUDE.md, the E8 tuning folded into real numbers (tuning.json retired).
  *Check:* nothing changes in the bar; the hub's World page still lists
  everything.

### S2 The Studio opens: look around, pick things
- **S2.1** joshuavelasco.com/studio: login (owner/admins), home-screen icon,
  the bar's engine in edit mode, the top and bottom bars. *Check:* open it,
  Add to Home Screen, open from the icon: your bar, full screen, nobody in it.
- **S2.2** Cameras: orbit, pan/zoom, double-tap to frame, Fly, Top, Walk,
  saved views. *Check:* fly over the counter, switch to Top, back.
- **S2.3** Picking: tap → orange outline + breadcrumb; the overlap list.
  *Check:* tap a glass on the counter → "Bar › … › pint_glass-4".
- **S2.4** The Monitor: World list (by parent, search, filters), Inspect
  (read-only for now), Problems (checks + budget). *Check:* search "lamp",
  tap it, the camera flies to it.
- **S2.5** X-ray layers. *Check:* turn on "tap areas": boxes show around the
  jukebox, the darts, the mirror.

### S3 Change things and publish
- **S3.1** Edit ops, undo and the plain-words diff (engine, native tests).
  *Check:* nothing to see (tests).
- **S3.2** Move / Turn / Size: arrows + the dial strip (axis, world/local,
  value with ±, scrubber with slide-up fine control, snapping + tick).
  *Check:* move a table exactly 0.25 m with the number field.
- **S3.3** Slide on surfaces, Drop, Put on…. *Check:* slide a glass along the
  counter; put it on a table; move the table, the glass follows.
- **S3.4** Undo/redo (buttons + two/three-finger taps), the draft saving
  itself, Changes in plain words. *Check:* three edits, two-finger tap twice,
  one left; close the Studio, reopen, it's still there.
- **S3.5** Publish, History, Restore. *Check:* move a table, Publish, open the
  bar: it moved. Restore the version before: it's back.

### S4 The new look everywhere
- **S4.1** The hub rebuilt as Needs you · Dials · Board · News, Dials editing
  the same draft, "Open in Studio" on every row. *Check:* change a lamp's
  swing speed in Dials, Publish, the bar's lamps swing faster.
- **S4.2** The bar's menus and HUD in the new look (Clash Royale style and
  Lilita go; line icons; badge frames). *Check:* open Menu in the bar.
- **S4.3** News and the loading screen in the new look. *Check:*
  joshuavelasco.com/news.

### S5 Precision
- **S5.1** The magnifier (long press) and 🔒 lock. *Check:* long-press a
  candle on a table: the lens shows it big; lock; orbit; still selected.
- **S5.2** Part mode: select a thing's shape, tap area, seat, light, sound;
  edit them with the same tools. *Check:* select the jukebox's tap area,
  make it wider, publish, it's easier to tap in the bar.
- **S5.3** This one / every one (thing vs kind), override dots, Reset to kind,
  "New kind from this". *Check:* make every bar stool 5 cm taller in one
  change; make one of them different.
- **S5.4** Inspect becomes editable for every field (from the schema).
  *Check:* change a seat's facing in Inspect.

### S6 Build
- **S6.1** Library with pictures and prices; place on the nearest surface.
  *Check:* add a candle to the bar counter.
- **S6.2** Duplicate, Delete, Row, rename (ids kept safe). *Check:* a row of 4
  stools 70 cm apart.
- **S6.3** Boxes: draw on a surface, Face mode, Push/Pull with the anchor,
  Extrude, material, solid or not. *Check:* build a small stage: a box, pull
  it up 30 cm, extrude a step.
- **S6.4** Places: new / copy, spawn point, the bar's travel menu lists the
  places. *Check:* make a place "Rooftop" from boxes, publish, travel there
  from the bar's menu.

### S7 Light and look
- **S7.1** Materials as data (`assets/world/materials.json`; the 13 built-in
  ones move there) + a material editor. *Check:* make a "blue velvet"
  material, give it to the sofa.
- **S7.2** Lights: colour, brightness, reach, softness, dims; their loops;
  recapture reflections. *Check:* make the neon by the arcade red.
- **S7.3** Moods (named light sets) and the budget meter. *Check:* switch
  Evening → Party in the Studio; the meter stays green.

### S8 Life
- **S8.1** States and rules (list + diagram), add/remove states, Test events.
  *Check:* give the dartboard a state "wobble" when tapped.
- **S8.2** The Loop Rack: lanes, shapes, amounts, period/beats, live preview,
  pause/scrub. *Check:* make the pendant lamps sway twice as wide.
- **S8.3** Draw keys with your finger; copy/paste loops; presets. *Check:*
  draw a bounce for the jukebox.
- **S8.4** Sounds. *Check:* the arcade machine hums when you're near it.

### S9 Live for everyone
- **S9.1** Hot reload: a publish reaches the bar while people are in it.
  *Check:* bar open on one phone (or tab), Studio on the other: Publish, the
  bar changes within seconds.
- **S9.2** Draft rooms: try your draft with a friend before publishing.
  *Check:* send the draft link, both see the draft.
- **S9.3** Undo publish in one tap (Studio and hub). *Check:* publish, undo,
  the bar goes back.

### S10 Claude in the loop (still no AI inside)
- **S10.1** Notes for Claude: on any selection, write a note; it goes to the
  hub inbox with the thing's id, your camera view and a picture. Claude reads
  it at the next "go". *Check:* note on the fireplace, it appears in the hub.
- **S10.2** Claude as an author: Claude's world changes show in Changes and
  History as "Claude: …" with before/after, and can be undone like yours.
  *Check:* after a go, History shows Claude's version.
- **S10.3** Deep links everywhere: hub row → Studio, Studio → hub dial, news
  post → "see it in the Studio". *Check:* tap "Open in Studio" in Dials.
- **S10.4** Proposals: a draft from someone else (Claude) you review: Preview
  in the world (ghosted), Apply, Discard. The same screen an AI inside the
  Studio would use later. *Check:* Claude proposes a change; you preview it
  ghosted, apply it.

---

## 9. Later (explicitly not now)
- **AI inside the Studio** (the pasted plan's "Change this…" box, voice, AI
  activity): plugs into S10.4: an AI is one more author of proposals, with
  three levels (suggest, preview, apply within the selection); grouped undo
  already exists. Needs a paid API; not while the user is on Claude Pro only.
- Importing your own models (`.glb` from Files): needs the texture-packing
  step on the server or a build.
- Mesh editing (vertices, edges, bevels).
- Several people editing in the Studio at once.
- Terrain and outdoor places; the engine's display font in 3D name tags.

## 10. Risks
- **Two copies of the world** (store and repo): only one may move at a time;
  pull at the start of a go, push with a base check, never edit the store by
  hand. The Studio shows Claude's versions as they land.
- **Edit mode inside the game binary**: the editor's C++ must stay small
  (players download it). Panels stay in `studio/studio.js`, which only the
  Studio loads.
- **Touch precision**: solved by the lens, the overlap list, the dial strip and
  numbers; tested with real taps in the headless tests (not code calls).
- **Phone memory**: no second 3D view; thumbnails cached on the server.
- **The schema falling behind**: the completeness test makes it impossible.
