# The Alive plan (replanned from scratch, 2026-10-22)

The user's vision, from the planning talk on 2026-10-22, turned into tracks
and steps. It replaces `docs/STUDIO.md` as "the plan" (S1-S10 are built; the
Studio stays the place everything is made in). `NEXT.md` holds the step
"go" does next; this file holds the whole road.

## 1. The vision in plain words
- **Enhanced real life, animated, super EXTRA lively.** Real shapes, light
  and physics, pushed livelier than reality. The bar keeps moving on its own;
  there's always something to join. Joining is optional and fun, never a
  chore.
- **The bar is a live movie.** One shared clock, real players inside it. The
  Studio is its editor: a **timeline at the centre** (like a video editor:
  drag a thing on, it's there; stack things and they happen by themselves).
- **A bar script language** under the timeline: the timeline is what you see
  and drag, the script is what's saved. Small, safe, can't crash the bar.
- **Claude works inside the Studio, live.** You watch each change appear on
  the phone, say "move it left", and press **Launch** when it's right.
- **Launch is a check, not a click:** rules, then bot players walk the bar,
  then it goes live (with one-tap undo).
- **Very high quality:** new lighting, textures that never repeat, models
  built like the real thing, avatars that look alive.
- **New axes beyond x/y/z:** memory, mood, chance, time, era, scale, layers.
- **A real manual** for everything, written for a person.

## 2. How "go" works now (more fun to watch)
Same as before (`CLAUDE.md`): one step per "go", one commit per piece, one
phone check per piece, "broke L1.2" finds it. New on top:
- **Wow and foundation take turns,** so every step or two the bar looks or
  behaves visibly different.
- **Every visual piece has an off switch** (`?flag=0`), and every step ends
  with **before/after pictures** (`tools/shots.mjs`) in the hub post, so the
  user sees the difference without hunting for it.
- **Every piece writes its manual page** (`docs/manual/<track>.md`, plain
  words: what it is, where it is in the Studio, the dials). No "docs later".
- **Phone first:** each visual step ends with a `?perf=1` report request;
  a piece that costs more than its budget gets a cheaper version or a
  Quality-tier switch, never shipped as is.

## 3. The tracks
| Tag | Track | What it is |
|---|---|---|
| **L** | Light | real units and sized lights, light that bounces for real (radiosity), shadows from every light, shaped lamps, real eyes and camera, reflections and caustics, the air, light cues, light that is the music, paint the light |
| **M** | Materials | textures that never repeat, rounded edges, wear, dirt and smudges, layers (varnish, velvet, brushed metal), light through things (wax, skin, glass thickness), real depth, real sizes |
| **T** | Timeline & script | the bar script language, the timeline at the centre of the Studio, Launch with checks and bots |
| **R** | Recipes & modelling | models grown from recipes (built like a carpenter would), Claude building live in the Studio, iPhone scanning |
| **A** | Avatars | idle life, one body with sliders, skin, hair, eyes, face from selfies, clothes that move, lips that follow voice |
| **W** | World axes | memory, mood dial, chance, time scrub and ghosts, era dial, scale, layers |

## 4. The road (one "go" each; wow ★ / foundation ◆)
Why light looks fake today (talk, 2026-10-22): light hits a surface once and
stops (no bounce), every light has a hard reach limit, the picture can't
adapt like eyes (tone mapped too early), strengths are guessed not real,
lights are tiny points, materials may be darker than real ones. And the
things games skip "because players won't notice" (they do; they just can't
name it). Every item from that talk is placed below.

1. ★ **L1 Real light**: light only from sources; HDR and real eyes; real
   units (lumens, colour temperature) with no hard reach; lights with size
   (bulbs as balls, windows as panels, neon as tubes); real material
   brightness; **radiosity** (surfaces light each other, many bounces,
   re-spread in a moment after a Studio lamp edit); probes for moving
   things. (Pieces in `NEXT.md`.)
2. ★ **M1 Real surfaces**: textures that never visibly repeat (large
   patterns, blending, per-copy variation), rounded edges, automatic wear
   (rubbed edges, dust in corners), dirt and smudges (fingerprints on glass,
   water rings on the bar, worn handles) changing how light reflects.
3. ★ **L2 Shadows and shaped light**: every light casts shadows (still
   shadows saved, cheap ones for small lights); real lamp shapes (measured
   IES light patterns) and lampshades that glow and colour the light; fire
   light that moves its shadows; contact shade; specular occlusion (no shine
   under tables); energy compensation (rough metal and fabric not too dark).
4. ◆ **T1 Bar script**: the language (built on today's states, rules and
   loops from S8): parser, a tiny safe runner (step limits, no engine access
   beyond what a script is given), errors with the line number in the
   Studio. 5 existing behaviours rewritten as scripts (candle, neon, bell,
   light switch, jukebox flare), looking the same.
5. ★ **L3 Eyes and camera**: smooth edges (temporal anti-aliasing, no
   shimmering bottle necks); white balance that adapts like the eye; seeing
   in the dark (colour fades, blue-grey: the Purkinje effect); glare (a
   bright bulb in view veils the dark) and starbursts; faint moving grain in
   the dark; focus on what you look at.
6. ★ **M2 Layers**: varnish over wood (clear coat), velvet and fabric sheen,
   brushed metal and vinyl stretched highlights (anisotropy), wet surfaces.
7. ★ **L4 Reflections and caustics**: several reflection captures (each
   thing reflects what's around it, not the room centre), screen reflections
   on the bar top and floor, light bounced off shiny things (a polished floor
   lights the ceiling), caustics (amber patterns from a whiskey glass).
8. ★ **T2 The timeline**: the Studio's centre strip: bar time, tracks per
   thing, drag events on, stack them, loop them, preview by scrubbing; it
   plays for everyone on the shared clock.
9. ★ **L5 The air**: haze and beams from every light, dust specks visible
   only inside light, smoke thicker near the ceiling, heat shimmer above the
   fire and candles.
10. ★ **R1 Recipes**: stool, chair, table, bottle, glass grown from recipes;
    every copy slightly different; the Studio shows the recipe's dials.
11. ★ **M3 Light through things and real depth**: wax, marble, milk, skin
    and lampshades glow from inside (subsurface); glass with thickness bends
    what's behind it and colours light by thickness; brick and grain with
    real depth (parallax, tiny self-shadows); every model checked against
    real-world sizes.
12. ◆ **T3 Launch**: the check (rules, overlaps, blocked doors), bot players
    walk the draft, a report, then Launch and one-tap undo.
13. ★ **W1 Memory**: the floor wears where people walk, the dartboard gets
    holes, your usual stool gets your nameplate.
14. ★ **A1 Alive avatars**: breathing, blinking, looking at whoever talks,
    shifting weight, feet planted on stairs; one body with sliders.
15. ★ **L6 Light show**: light cues (theatre style) and light that is the
    music, as scripts; bulbs with real colours (tungsten, neon gas, sodium);
    a dying bulb that flickers.
16. ◆ **R2 Claude in the Studio, live**: Claude's edits stream to an open
    Studio one by one ("Claude is working…"), the user comments live and
    presses Launch. No paid API needed (Claude works through the world store
    like `tools/studio.mjs`; a chat box inside the Studio would need an API
    key: the user's call later).
17. ★ **L7 Living light**: a real sky through the windows (your time and
    weather), players carry light (lighter, phone torch, glowing drinks),
    neon afterglow.
18. ★ **W2 Mood and chance**: one cozy↔wild dial for the whole bar; "maybe"
    events on the timeline (30% rain, a surprise band).
19. ★ **A2 Real people**: skin light passes into, hair strands with their
    shine, wet see-through eyes, clothes that swing; lips follow voice.
20. ★ **R3 Scan it**: bring real things in with the iPhone camera.
21. ★ **W3 Time and era**: scrub time back and see ghosts replay the night,
    branch a "what if" copy; the era dial (1920s ↔ neon future).
22. ★ **L8 Paint the light**: brush warmth and shadow in the Studio; the
    engine places and tunes lamps to match.
23. ★ **A3 Your face**: an avatar face from a few selfies.
24. ★ **W4 Scale and layers**: shrink to mouse size; blend two places.

The order can change at any "start"; new ideas go to Parked first.

## 5. Rules for the whole plan
- Everything still from scratch (CLAUDE.md): no outside engines or
  libraries. Blender stays a tool for making assets, never needed at runtime.
- Anything the Studio can edit must keep working when the user edits it:
  so light is worked out **by the engine at runtime** (it re-does it after an
  edit), not baked once in Blender (that older plan in `docs/LIGHTING.md`
  is replaced by L1).
- **Light only from light sources** (user, 2026-10-22): no fake ambient
  fill anywhere. Everything visible is lit by a real source (bulb, flame,
  neon, screen, sky) or its bounce; darkness is real, eyes adjust to it.
- Each step lists its budget on the phone (ms per frame, MB), checked with
  `?perf=1`. Low quality always stays smooth.
