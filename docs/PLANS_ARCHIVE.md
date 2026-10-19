# Plans archive (old 'Next up' notes, moved 2026-10-19)

These were the stacked priority notes at the top of PROGRESS.md before the
master list replaced them. Kept for their details; the open items are mapped
into the master list in PROGRESS.md.

**2026-10-19, NEW TOP PRIORITY (user): `docs/UPGRADE_PLAN.md`**, 15 items
(two-player racing with car choice + real steering feel, an arcade room with
the user's platformer "Test 2", signs/ladders, 4K Mania like osu!mania, mirror
quality, badge slots, clipping props, walk-through fixes, stuck seats, chess,
there. The server-run bot race (below) is replaced by its items 1-2.
Done so far: 12 (stuck seats), 2 (steering, `engine/steering.*`), 1 (two-player
racing with a garage), 3-5 (the arcade, its platformer machine and music).
6 (A-frame signs, barricades, ladder), 7 (4K Mania like osu!mania). 8 (mirror sharp up
close), 9 (badge slots), the Clash Royale style UI, 10 (pots, hanging plants). 11 (walk-through).
13 (chess). 14 (avatar customizer). 15 (walk and idle animations). All 15 done; 4K Mania round 2 (skins, bubbly UI, particles, preview music) done;
next: the UI polish left in UPGRADE_PLAN (HUD and game screens in the Clash style).

**2026-10-18, NEW TOP PRIORITY (user): the server runs the game.** Multiplayer now
goes through the Cloudflare room (build after `173f06d`; `docs/DECISIONS.md` →
"Cloudflare room is the multiplayer server"). The user's order: (1) the connection
and the server side first: move authority for every shared thing into the room
(done 2026-10-18: seats, glasses, darts + 4K Mania, drinks, value checks on every
shared write; see DECISIONS → "The room checks every shared write"), next: the room
*running* the game logic itself (darts, the jukebox and the glasses' physics done 2026-10-18, `cloud/games/`,
`cloud/sim/`; next: races with bots on the room (the same wasm route: engine/vehicle +
app/race in the Worker), 4K Mania score checks, piano/jukebox timing, races with bots, glass physics), and bartender badges checked by the room for the station too; (2) only then client-side work. Work
only on what the user names. Keep joshuavelasco.com (home) + /bar (the game).
Open: joshuavelasco.com doesn't load on the user's phone ("can't be reached",
2026-10-18) while barcode162.github.io/3D does and the domain answers from
everywhere we tested; suspect the domain's IPv6 (AAAA) record at IONOS.

**2026-10-16, NEW TOP PRIORITY: the restructure. The plan is
`docs/RESTRUCTURE.md`; work through its §4 steps in order, one step per
session, starting at 0.1.** Every system was audited (scorecard and
problems checklist in §3). Goals: 60 fps on the iPhone 15 Pro Max with no
start-up lag, zero multiplayer sync issues (the Cloudflare room becomes the
authority), far fewer error paths. Read "How to use this plan" in that file
first. Ask the user the open questions in its §5 when a step needs them
(Q1 is answered, 2026-10-17: the user made the Cloudflare account and added
the GitHub secrets `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID`; don't
ask again). The plan was reviewed against the code on 2026-10-17 (see its
last section). **Done: Phase 0, Phase 1 (0.1-1.8) 2.1a-c, 2.2, 2.3a, 2.3b, 2.4a, 2.4b (2026-10-17) and 2.5, 3.1 and 3.2a (2026-10-18). Next: 3.2b, 3.2c, 3.3 (rest of the room server, then its dark launch)**
(batch them; see Execution order in RESTRUCTURE §4). User's pace (2026-10-17): the full plan stays (no cuts); Claude batches
related steps (about 3 per message), automatic tests on every change, docs
once per batch. Racing: keep the racing steps for multiplayer/sync, but
skip anything that changes car/track models or visuals (the user will
redesign the racing game). Waiting on the user: 3 reports for the G4 baseline (after
"Tap to enter", at the big mirror, after 2 minutes walking), and the tap
stutter (yellow bar on every tap, seen 2026-10-17): read `recentHitches`
(`touches`, `phase`) in their reports.
The racing plan below waits, except small driving-feel tweaks the user asks for.

**2026-10-16, previous top priority: the racing sim. The plan and checklist
is `docs/RACING.md`; work through its "Steps" in order.** Steps 1-3 are
built: loading stutter fixed (`engine/gpu_upload.*`), real car physics
(`engine/vehicle.*`), a 2.45 km circuit, 7 bots, collisions, full-screen
3D race with touch / keyboard / mouse / gamepad / wheel controls (see
"What exists now" in `docs/RACING.md`). **Next:** the user tries it on
the iPhone (sit in a sim rig); tune the driving feel from their
feedback; then step 4 (the race on the rigs' three monitors), step 5 (two
players), step 6 (realtime server on Cloudflare: decided, waiting for the user's
account + GitHub secrets, see `docs/DECISIONS.md` → Hosting split). Everything below waits.

**2026-10-16, "make everything work" pass: built and deployed.** Voice
chat (WebRTC), menu (emotes, avatars, profile, voice, settings, piano,
bartender), seats for everyone, piano with 10 classical MIDI songs + MIDI
keyboards, darts with scoring, racing game on the sims, 4K Mania and races
shown on the machines' screens, bartender badge + drinks, many small
interactions. All crude on purpose; every crude part is on the remaster
list, `docs/REMASTER.md`. **Next:** the user tests it on the iPhone (with
their brother for voice); fix what breaks, then remaster items in the order
the user picks.

**2026-10-16, new top priority: the big bar upgrade.** The user asked for
a completely new bar (new shape, packed with real-bar clutter), 3 rooms
(the other two taped off as "under construction"), one giant 2-in-1
mirror, darts / racing sims / 4K rhythm machine / grand piano with real
strings, and a loading screen. **The full plan and checklist is
`docs/BAR_REDESIGN.md`.** First pass built and deployed the same day:
new room shell, curved bar, tall back bar, piano, big mirror, both
construction doorways, 2 sim rigs, rhythm cabinet, darts zone, lounge
with fireplace, decor pass, loading screen. User checked it: looks
great; rug fixed; the rhythm machine is now playable (circle skin).
Then accounts (sign up, log in, profile picture, bio, person cards,
pictures over heads) were added. Ideas the user mentioned for later: a
racing game for the sims.
**Next:** ask the user how it
looks and runs on the iPhone (fps with Low Power Mode off, load time,
anything that looks wrong), fix that first; then the open items in
`docs/BAR_REDESIGN.md` ("Still open"), then interactions (sit, play the
piano, darts scoring, the rhythm game). Everything below
this paragraph is the older plan and waits until the upgrade is done.

New direction (2026-10-15): the goal is now a **world simulation** where
engines stack (see `docs/VISION.md` → North star). The user lets Claude
decide the details and only reviews/critiques. Work one step at a time,
push after each, tell the user what to look at.
**Asset pass done and verified on iPhone (build `964adfe`):** every piece
looks as it should. The 30 fps the user saw was **iOS Low Power Mode**
(it caps Safari at 30 fps), not the engine. Next:
1. **Multiplayer M2 is built (2026-10-16), check it on two phones.** The
   relay at https://joshuavelasco.com/bar/relay.php works (live test with
   two headless players passed). M2 = shared world: glasses, the jukebox
   song (joins mid-song) and mirrors on/off are the same for everyone (see
   `docs/DECISIONS.md` → Shared world). Ask the user to try it with their
   brother. Then M3 = smoother movement with WebRTC, voice chat.
   **Status at session end (2026-10-16):** build `355fed6` is live on both
   sites; the user hasn't tested it on phones yet and will report failures
   later. Assume it works until they say otherwise. Then ask which comes
   next: the performance pass (2) or glass-clink sounds (W5, item 3).
2. **Performance pass (no longer urgent).** Re-measure with Low Power Mode
   off first. Goal: 60 fps on the iPhone 15 Pro Max. Room is ~170k triangles, ~200 draw calls, ~80
   textures (all RGBA, uncompressed), and the full mirror draws the whole
   scene a second time. Ideas, cheapest first:
   - Measure: add a debug toggle (`?perf=`) to switch off the mirror pass,
     models by group, or lights, to find what costs the most.
   - Mirror: draw the reflection at half resolution, skip small props in
     it, and only render it when the mirror is on screen.
   - Fewer triangles: bottles (37k in the back bar) and stools (5.6k each)
     can drop a lot (lathe segments, bevel segments); simplify far props.
   - Fewer draw calls: merge static models that share materials into one
     scene mesh at load time (stools, tables, coasters).
   - Textures: 512 for most surfaces, 1k only for floor/brick/counter top.
   - Shading: 8 point lights per pixel is the main per-pixel cost; light
     culling per object (only the nearest 3-4 lights) could halve it.
   - Pixel ratio: try 1.5 instead of 2 as a fallback.
3. **W5 Physical sound** (impact sounds synthesized from collisions and
   materials; room echo), then more sim engines (shadows/bloom,
   heat/flames, liquids, smell field, day/night).

