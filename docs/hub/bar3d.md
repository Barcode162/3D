# Hub board: bar3d

Copied from the hub by `node tools/hub.mjs snapshot` on 2026-10-20 16:52 UTC. The hub is the live
version; this file is the copy in the repo (read it when the hub is out of reach).

## Arriving

_On top of the world: loading, first screen, home screen, Happening now, menus, HUD. Not here: which lobby you join (Social)._

### Home screen with "Happening now"

`391136d79a` · Idea

```
WHAT: You start at your home; one tap takes you to a busy hub, a live event or a friend.
NOW: Nothing yet: the page opens straight into the bar.
STEPS: 1. A home screen after login: your home (big button), "Happening now" (hubs with player counts, live events, friends online), Settings. 2. "Happening now" reads the lobby list from the room server (who is where, how full). 3. One tap joins that lobby instance; back button returns home. 4. Few hubs on purpose (the bar is the first): repetition builds habit.
NEEDS: Lobbies and matchmaking (few main hubs) (#c344058f47); Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572); Events: date, time, RSVP, invite cards (#1a695dfa09)
UNLOCKS: nothing yet
NOT HERE: which lobby you land in, matchmaking → Lobbies and matchmaking (few main hubs) (#c344058f47)
NOT HERE: your home itself → Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572)
DETAILS: docs/VISION.md → The product (core loop); PROGRESS master list 5.
```

### Loading screen polish

`4de14bc6ea` · Idea

```
WHAT: Faster first load and a loading screen that looks finished.
NOW: Loader stages with progress (loader.*); compressed textures (≈2x faster); rooms stream in as you approach.
STEPS: 1. Measure: 3 phone reports (tap the build code 3 times) right after "Tap to enter". 2. Show the bar's look while loading (a still of the bar, the stage names in plain words). 3. Never walk around in the dark: lights and big shapes first, small props after.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: what loads when (streaming, budgets) → Faster drawing, part 2 (render core, layout from Blender, quality settings) (#410d177f68)
DETAILS: docs/RESTRUCTURE.md 1.8, 2.7; docs/BAR_REDESIGN.md (loading).
```

### Bug: the menu stops answering after some games

`d7d2597bb7` · Idea

```
WHAT: The menu always answers, even after playing games.
NOW: Open bug: after some games the menu stops responding. The quick report (tap the build code 3 times) works even then and lists the last taps.
STEPS: 1. When it happens: tap the build code 3 times and send the code in chat. 2. Read the report (last taps and what they hit), find which game leaves the menu blocked, fix, add it to tools/menu_test.mjs.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: PROGRESS.md master list 1 (open bug).
```

### Menus and HUD in the Clash style

`615ba12c7d` · Idea

```
WHAT: Every screen on top of the world in one look: chunky font, bevelled panels, glossy buttons.
NOW: Menus and profile cards restyled (web/theme.css, 3D badges and icons). The in-game HUD text (pixel font), 4K Mania and race screens are still the old style.
STEPS: 1. In-game HUD text and prompts in the Clash style. 2. 4K Mania and race screens in the same style. 3. Emote menu as a radial wheel with favourites; better emote faces. 4. Settings: graphics options and key rebinding.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: quality settings themselves → Faster drawing, part 2 (render core, layout from Blender, quality settings) (#410d177f68)
DETAILS: docs/UPGRADE_PLAN.md → Clash Royale style UI; docs/REMASTER.md → Menu.
```

## Your character

_Your body and look: avatar, customizer, animation, poses, what you wear. Not here: buying outfits (Shops)._

### Avatar customizer

`8274861e77` · Done

```
WHAT: Make your own look: body, hair with highlights, tops, bottoms, colours, patterns.
NOW: Done: Menu → Avatar → Make my own look. Saved on the phone; others see it through the room.
LATER: Hair and clothes as real models (not code-built). / Separate bodies (not one body with variations).
NEEDS: nothing
UNLOCKS: Your own character builder (face, details) (#92e06ade5e); Outfits and gadgets to show off (shop items) (#12f05869c9)
NOT HERE: face and details → Your own character builder (face, details) (#92e06ade5e)
NOT HERE: clothes you buy → Outfits and gadgets to show off (shop items) (#12f05869c9)
DETAILS: docs/UPGRADE_PLAN.md item 14.
```

**Claude's report:** Body, hair (+ highlights), tops, bottoms, patterns, colours. Menu → Avatar → Make my own look.

### Realistic walk and idle

`bb13039481` · Done

```
WHAT: A walk and idle that look real: planted feet, heel to toe, weight shifts, breathing.
NOW: Done (two-bone IK feet, pelvis twist, glances).
LATER: See the next card for turning on the spot and sitting down.
NEEDS: nothing
UNLOCKS: Stepping when turning on the spot; sit-down / stand-up motions (#75fdc00b98)
DETAILS: docs/UPGRADE_PLAN.md item 15.
```

**Claude's report:** Planted feet, heel-to-toe, weight shifts.

### Stepping when turning on the spot; sit-down / stand-up motions

`75fdc00b98` · Idea

```
WHAT: Feet step when you turn on the spot; you sit down and stand up instead of snapping.
NOW: Turning slides the feet; sitting snaps into place.
STEPS: 1. Turn-in-place steps (feet re-plant past a twist limit). 2. Walk-up, sit-down and stand-up motions for stools, sofas, rigs, the piano bench. 3. Tune each seat's hip point in the mirror (sofa and armchairs are estimates).
NEEDS: Realistic walk and idle (#bb13039481)
UNLOCKS: nothing yet
NOT HERE: hands holding things → Hands that really hold things; a face that moves when you talk (#4c4d68265d)
DETAILS: docs/UPGRADE_PLAN.md item 15 (later); docs/REMASTER.md → Seats.
```

### Your own character builder (face, details)

`92e06ade5e` · Idea

```
WHAT: Your own character from your character builder: face, details, maybe a face scan.
NOW: The user is building a 3D character builder; the bar has one rough body.
STEPS: 1. When the builder is ready: agree on the export (GLB with the same skeleton, or VRM). 2. Load your own avatar model (GLB/VRM) into the bar, seen by everyone. 3. Optional: a face from an iPhone face scan.
NEEDS: Avatar customizer (#8274861e77)
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → Your own avatar; PROGRESS Parked.
```

### Outfits and gadgets to show off (shop items)

`12f05869c9` · Idea

```
WHAT: The craziest, coolest outfits and gadgets to show off, bought and owned.
NOW: Nothing yet (the customizer's clothes are free).
STEPS: 1. Outfits and gadgets as owned items in the ledger. 2. Sold in the mall (credits) and the VIP store (real money, numbered and seasonal). 3. Wear them through the customizer; others see them.
NEEDS: Avatar customizer (#8274861e77); The mall (IKEA-style: pick up, cart, checkout, delivery) (#aa5b3832e5)
UNLOCKS: nothing yet
NOT HERE: the VIP store → VIP marketplace (real money, numbered and seasonal items) (#5b11225f55)
NOT HERE: owning items → The ledger (money and owned things) (#cb4648bc03)
DETAILS: docs/VISION.md → The product.
```

### Hands that really hold things; a face that moves when you talk

`4c4d68265d` · Idea

```
WHAT: Hands grip the wheel, hit the piano keys and hold your drink; your mouth moves when you speak.
NOW: Every activity pose is a few hand-set angles; no finger bones; others' drinks float near the hand; the name tag turns green when talking.
STEPS: 1. Hand and finger bones; IK so hands reach real targets (wheel, keys, dart, glass). 2. Drinks attached to the hand bone for everyone. 3. Mouth and face move with your voice level.
NEEDS: Clean core leftovers: 4.1c, 4.2 entity ids, 4.3b (#b2a21003f9)
UNLOCKS: nothing yet
DETAILS: docs/REMASTER.md → Avatars and animation; RESTRUCTURE 4.3b (pose layer).
```

## The bar

_Shared places and their things: rooms, props, lights, mirrors, jukebox, the street. Not here: games in them (Games)._

### Main bar

`2a2eba0bfc` · Done

```
WHAT: The main hall: curved bar, back bar with bottles, stools, tables, booth, piano stage, bay window, the giant 2-in-1 mirror.
NOW: Done: all real models; walk-through checked (no traps, clipping fixed); mirror full / avatars-only / off, sharp up close.
LATER: Better light: see that card. / Clutter that tells a story (BAR_REDESIGN reference list).
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: interactions done properly → Small interactions done properly (bell, guitar, books, TV, door, taps) (#b87c66a5bd)
NOT HERE: physics props → Physics props: knock bottles off, glasses break and stack, pour drinks (#c99ae6b565)
DETAILS: docs/BAR_REDESIGN.md.
```

**Claude's report:** Curved bar, back bar, stools, tables, mirror, piano stage.

### Arcade room

`271ed02d1e` · Done

```
WHAT: The arcade behind doorway A: carpets, lights, machines, chiptune music that grows as you walk in.
NOW: Done; its inside loads when you're near.
LATER: More machines / Live screens showing the real game.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: new machines → More arcade games (#03f3b523d3)
NOT HERE: live screens and queue → Live low-res machine screens + fair queue (20 s to claim your turn) (#d7d2844851)
DETAILS: docs/UPGRADE_PLAN.md items 3-5.
```

**Claude's report:** Behind doorway A; carpets, lights, platformer machine, chiptune music.

### Room 3 (taped off)

`0a52f35bbc` · Idea

```
WHAT: The third room (taped off, plastic sheeting) becomes something.
NOW: Under construction.
STEPS: 1. The user decides what it becomes (question on this card).
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/BAR_REDESIGN.md (Room 3 doorway).
```

**Open question:** What should room 3 become? Tell me in a line and I will plan it on this card.

### Lounge (fireplace, chessboard)

`3bfddeed8a` · Done

```
WHAT: The lounge: fireplace, Chesterfield sofa, wingback armchairs, bookshelf, chessboard.
NOW: Done.
LATER: Readable books; flames that really go out.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: the chess game → Chess (#23fb9b8423)
DETAILS: docs/BAR_REDESIGN.md (zones).
```

### The street and city outside

`9537ee9062` · Idea

```
WHAT: A street and city outside: walk out the door, buildings, homes there.
NOW: A night street seen through the bay window.
STEPS: 1. A street block outside the door. 2. Buildings that stream in. 3. Homes placed in the city (your "real home").
NEEDS: Faster drawing, part 2 (render core, layout from Blender, quality settings) (#410d177f68); Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572)
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → The product (later).
```

### Small interactions done properly (bell, guitar, books, TV, door, taps)

`b87c66a5bd` · Idea

```
WHAT: Every small thing you tap does its real job, not just a sound or a message.
NOW: Each is a sound and/or a message: bell, guitar (one strum), door knock, books, tip jar, light switch, fireplace, stage mic, beer taps; the TV does nothing.
STEPS: 1. Playable guitar. 2. Readable books. 3. A door that opens. 4. Fireplace flames that go out. 5. The TV shows something. 6. Jukebox buttons.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/REMASTER.md → Other interactions (the other REMASTER items now sit on the card they improve).
```

### World simulation: sound from physics, heat, smell, day/night

`f973d13942` · Idea

```
WHAT: World simulation: sounds made from the physics, heat (candles, coffee, ice), smell as a drifting haze, day/night and weather.
NOW: Nothing yet.
STEPS: 1. One material = look, physics and sound. 2. Sound from collisions, room echo. 3. Heat and fire. 4. Smell field, day/night, weather.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: liquids and breaking → Physics props: knock bottles off, glasses break and stack, pour drinks (#c99ae6b565)
DETAILS: docs/VISION.md → North star.
```

### Jukebox (playlist, your own songs, 3D sound)

`2b54cd54a5` · Done

```
WHAT: A real jukebox: pick a song from its menu, heard from its spot; load your own songs from Files.
NOW: Done: smooth jazz (CC BY), song codes, your own songs (local only), shared through the room.
LATER: Audio-reactive lights. / Songs from the music studio (if it is built).
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → Music.
```

### Physics props: knock bottles off, glasses break and stack, pour drinks

`c99ae6b565` · Idea

```
WHAT: Things behave like their material: bottles fall off shelves, glass breaks, glasses stack, beer pours.
NOW: Glasses can be picked up and thrown (the room runs their physics).
STEPS: 1. Bottles as physics bodies. 2. Breaking (a material flag). 3. Stacking glasses. 4. Liquids: beer in the pint, pouring (W7).
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: heat, smell, sound from physics → World simulation: sound from physics, heat, smell, day/night (#f973d13942)
DETAILS: PROGRESS.md → Physics follow-ups.
```

### Better light: shadows, bloom, reflections

`87aba35372` · Idea

```
WHAT: Lamps cast shadows, neon glows, the bar top and floor reflect, light shafts in the haze.
NOW: PBR materials, point lights, haze and tone mapping.
STEPS: 1. Shadows from the main lamps. 2. Bloom for neon. 3. Reflections on the bar top and floor. 4. Haze and light shafts.
NEEDS: Faster drawing, part 2 (render core, layout from Blender, quality settings) (#410d177f68)
UNLOCKS: nothing yet
DETAILS: PROGRESS.md roadmap (open items); RESTRUCTURE 6.4.
```

## Games

_What you play or work at (score, turn, win): darts, 4K Mania, racing, chess, piano, arcade, bartending, music studio._

### Darts

`d1f8233d98` · Done

```
WHAT: Darts: 301 with real scoring, run by the server; a win pays $5-50.
NOW: Done.
LATER: Swipe to throw with power. / Real dart models. / Cricket, turns between players, double-out. / Score written on the chalkboard.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/REMASTER.md → Darts.
```

**Claude's report:** 301 run by the server; a win pays $5-50.

### 4K Mania (rhythm)

`ea89fd246e` · Done

```
WHAT: 4K Mania: osu!mania style, skins, previews, particles.
NOW: Done.
LATER: Hand-made charts. / Spectators see the real chart. / The room checks scores so they can pay.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: score checks → Room checks chess moves and 4K Mania scores (#bcf65dd27a)
NOT HERE: live screen for watchers → Live low-res machine screens + fair queue (20 s to claim your turn) (#d7d2844851)
DETAILS: docs/UPGRADE_PLAN.md (4K Mania); RESTRUCTURE 5.9.
```

**Claude's report:** osu!mania style, skins, previews, particles.

### Driving rework (physics, four cars, drift, free drive)

`5ddbcf678b` · In progress

```
WHAT: The full driving rework: real physics (tires, suspension, aero), four cars (Ridgeback GS first), circuit, drift, free drive.
NOW: MVP built: engine/drive, the Ridgeback GS, the skid pad, live Car setup panel, Old physics switch. Waiting for the phone check.
STEPS: 1. R-0 research pass. 2. Stages 1-8: car data, contact, chassis, tires, driveline, steering, aero, GS calibration. 3. Stages 9-13: cameras and effects, sound, the other three cars, car art, collisions. 4. Stage 14: road tool + the Ring 2.0 + rendering upgrades (shadows, sky): the track visual upgrade lives here. 5. Stages 15-18: drift park, bots and race weekend, tuning garage, free drive map. 6. Stage 19: multiplayer for all modes (replaces RESTRUCTURE Phase 7). 7. Stages 20-22: more circuits, traffic and weather, performance.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: the rigs in the bar and their screens → Racing rigs (two players + garage) (#8fceaf0e4a)
DETAILS: docs/DRIVING_REWORK.md §18 (stage table).
```

**Claude's report:** Started the MVP from docs/DRIVING_REWORK.md: new physics (engine/drive), the Ridgeback GS, a skid pad next to the Ring, the live Car setup panel, Old physics switch.

### Racing rigs (two players + garage)

`8fceaf0e4a` · Review

**What the owner wants:** We need a full visual upgrade of the entire racing game visuals. there’s even some random visual terrain malformation so instead id like a real nice track and some new state of the art visual stuff like realistic ray casted lighting through leaves and nice non blinding reflection of the sun beautiful water and reflections like the side / Answer: The rigs themselves are sick but I wanted to be able to see the racing screen from the rigs themselves for whoever is driving them so anyone ca see it from the outside

```
WHAT: The two racing rigs: race each other (car choice, paint, rims), anyone can watch the race on the rig's screens.
NOW: Two-player races, garage with 3 cars, real steering feel. The rig monitors show text only. The track is built in code (cone trees, terrain glitches).
STEPS: 1. Rig screens: the rig's three monitors show the driver's real race so anyone in the bar can watch. 2. Visual upgrade of the race (real track, light through leaves, sun reflections, water): see the question on this card.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: new physics, cars, track rebuild → Driving rework (physics, four cars, drift, free drive) (#5ddbcf678b)
NOT HERE: live screens on every machine → Live low-res machine screens + fair queue (20 s to claim your turn) (#d7d2844851)
DETAILS: docs/RACING.md; RESTRUCTURE 7.5 (spectator monitors).

Answer: Yes you can do it in stage 14 combine things to not work on the same thing 2 times if we can hep it
```

**Claude's report:** Real steering feel; 3 cars, paints, rims.

### Chess

`23fb9b8423` · Done

```
WHAT: The lounge chessboard: full rules, 3D pieces, play the computer or a friend.
NOW: Done; the computer thinks a slice per frame.
LATER: The room checks each move.
NEEDS: nothing
UNLOCKS: Chess: Elo bots and a 1v1 / bot menu (#e693f1a3d1)
NOT HERE: move checks → Room checks chess moves and 4K Mania scores (#bcf65dd27a)
DETAILS: docs/UPGRADE_PLAN.md item 13.
```

**Claude's report:** Full rules, 3D pieces, play the computer or a friend.

### Chess: Elo bots and a 1v1 / bot menu

`e693f1a3d1` · Idea

```
WHAT: Pick a bot by Elo, or find a 1v1, from a chess menu.
NOW: One computer strength.
STEPS: 1. Bots at set Elo levels. 2. A chess menu: play a bot, find a 1v1.
NEEDS: Chess (#23fb9b8423)
UNLOCKS: nothing yet
NOT HERE: skill matching across games → Lobbies and matchmaking (few main hubs) (#c344058f47)
DETAILS: PROGRESS.md Parked.
```

### Piano (MIDI songs, real strings)

`90b4e6bb65` · Done

```
WHAT: The grand piano: MIDI songs, keys and strings match what is played.
NOW: Done (additive synth, 10 Mutopia songs).
LATER: Real piano sound (physical model or samples). / Keys go down, hammers and dampers move. / Velocity, sliding between keys, more than two octaves. / Learn mode (falling notes), tempo, song search.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/REMASTER.md → Piano.
```

### Platformer "Test 2" in the arcade

`f650349a4e` · Done

```
WHAT: Your platformer "Test 2" on an arcade machine, full screen.
NOW: Done, as is.
LATER: Improvements when you want them.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: web/arcade/platformer/.
```

### Meridian (your driving game)

`284962fb9a` · Done

```
WHAT: Meridian, your own driving game (three.js), opened full screen from a racing rig.
NOW: Done.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: web/meridian/; CLAUDE.md → Layout.
```

### Live low-res machine screens + fair queue (20 s to claim your turn)

`d7d2844851` · Idea

```
WHAT: Every machine's screen shows the real game live (simplified, pixelated); a fair queue gives you 20 s to reach the machine when it's your turn.
NOW: Others see a fake 4K Mania pattern and text on the rig monitors.
STEPS: 1. Machines send a small live state through the room (combo, notes, car positions). 2. Each machine draws it low-res on its screen. 3. A queue per machine, run by the room: your turn → 20 s to claim it, else the next person.
NEEDS: Room decides every shared thing (Phase 5) (#5bf378ab78)
UNLOCKS: nothing yet
NOT HERE: the rigs' three monitors first → Racing rigs (two players + garage) (#8fceaf0e4a)
DETAILS: docs/VISION.md → The product (lobbies).
```

### More games pay credits (once the server checks their results)

`9b263a4d3b` · Idea

```
WHAT: Games other than darts pay credits (5-50 by length and difficulty, daily caps).
NOW: Only darts pays.
STEPS: 1. 4K Mania and chess pay once the room checks their results. 2. Racing pays once races run on the room.
NEEDS: Room checks chess moves and 4K Mania scores (#bcf65dd27a); The ledger (money and owned things) (#cb4648bc03)
UNLOCKS: nothing yet
DETAILS: PROGRESS master list 3 and 7.
```

### More arcade games

`03f3b523d3` · Idea

```
WHAT: More arcade machines.
NOW: One: the platformer.
STEPS: 1. Pick the games together; one machine at a time.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: PROGRESS.md Parked.
```

### Mini music studio (DAW)

`d9f7171d17` · Idea

```
WHAT: A mini music studio in the bar: synths, drum machine, sequencer, piano roll, mixer; songs play on the jukebox.
NOW: Nothing yet.
STEPS: 1. Synth engine. 2. Drum machine + step sequencer. 3. Piano roll, tempo, loop. 4. Mixer and effects. 5. Songs to the jukebox. 6. Later: the same DSP as a VST plugin.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → Music; PROGRESS roadmap.
```

### Bartending (badge, make drinks)

`2195a3454e` · Done

```
WHAT: With the bartender badge you work the bar: make drinks step by step and serve them on the counter.
NOW: Done (crude): tapping through steps, drinks are coloured boxes, 4 sips.
LATER: Real bottles, glasses and liquids (see Physics props). / Badge checked by the room for the station.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: pouring liquids → Physics props: knock bottles off, glasses break and stack, pour drinks (#c99ae6b565)
DETAILS: docs/REMASTER.md → Bartending; RESTRUCTURE 5.7.
```

### Pool table

`1afd0e935a` · Idea

```
WHAT: A playable pool table.
NOW: Not in the bar.
STEPS: 1. Where it goes, the table model, ball physics, shared through the room.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: PROGRESS.md roadmap Stage 3.
```

## Social

_Other people: voice, profiles, lobbies, events, memory book, messages, safety. Not here: the voice servers (Server)._

### Voice chat

`067e125833` · Done

```
WHAT: Voice chat with everyone in the room, near-lossless.
NOW: Done: WebRTC, noise gate, mic test.
LATER: Per-person volume and mute, a "who is talking" list. / Noise gate in the audio thread. / Push-to-talk button and key.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: TURN / SFU servers → Voice relays: TURN for strict networks, then a server (SFU) for big lobbies (#96ec3676fa)
NOT HERE: mouth moving → Hands that really hold things; a face that moves when you talk (#4c4d68265d)
DETAILS: docs/REMASTER.md → Voice chat.
```

### Profiles, cards, badge slots

`49d23d7d92` · Done

```
WHAT: Profiles and player cards with 6 badge slots.
NOW: Done.
LATER: Earnable badges (180 in darts, SS in 4K Mania, a race win). / Privacy per profile field.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/UPGRADE_PLAN.md item 9; docs/REMASTER.md → Profiles.
```

### Safety: teens and adults kept apart, block / mute / report

`46bf062c89` · Idea

```
WHAT: Teens and adults kept apart; block, mute and report everywhere.
NOW: Age band asked at sign-up (13+).
STEPS: 1. The room knows each player's band (signed by the server, not the phone). 2. Teens and adults never share voice or messages; lobbies split by band. 3. Block, mute, report on every player card; a report keeps the last seconds of voice.
NEEDS: Age at sign-up (13+, teen / adult) (#4ae9e009ad)
UNLOCKS: Messages: phrase builder for teens, filtered text for adults (#e6a53f0655); Memory book (in-game camera, likes, no comments) (#ab645caea2); Lobbies and matchmaking (few main hubs) (#c344058f47); Trading (tradeable items only) (#d311f5a3c1)
NOT HERE: lobby matching → Lobbies and matchmaking (few main hubs) (#c344058f47)
NOT HERE: messages → Messages: phrase builder for teens, filtered text for adults (#e6a53f0655)
DETAILS: PROGRESS master list 4; docs/VISION.md → The product.
```

### Messages: phrase builder for teens, filtered text for adults

`e6a53f0655` · Idea

```
WHAT: Text: teens use quick phrases and a phrase builder (fixed words, no hidden messages); adults type freely, filtered.
NOW: Voice only.
STEPS: 1. Phrase builder for teens. 2. Filtered free text for adults. 3. Block, mute, report apply.
NEEDS: Safety: teens and adults kept apart, block / mute / report (#46bf062c89)
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → The product (talking).
```

### Events: date, time, RSVP, invite cards

`1a695dfa09` · Idea

```
WHAT: Anyone creates an event: date, time, RSVP, a clean invite card.
NOW: Nothing yet.
STEPS: 1. Create and RSVP. 2. Invite card. 3. Shows in "Happening now" when live. 4. Event-only items (with the VIP store).
NEEDS: Lobbies and matchmaking (few main hubs) (#c344058f47)
UNLOCKS: Home screen with "Happening now" (#391136d79a)
DETAILS: PROGRESS master list 9.
```

### Memory book (in-game camera, likes, no comments)

`ab645caea2` · Idea

```
WHAT: An in-game camera and a photo book you're proud of: likes, no comments, per-photo privacy, hide yourself from others' photos.
NOW: Nothing yet.
STEPS: 1. In-game camera (no real photo uploads). 2. Photo book on your profile. 3. Likes, per-photo privacy, hide-me.
NEEDS: Safety: teens and adults kept apart, block / mute / report (#46bf062c89)
UNLOCKS: nothing yet
DETAILS: PROGRESS master list 9.
```

### Lobbies and matchmaking (few main hubs)

`c344058f47` · Idea

```
WHAT: A few main hubs (the bar first) with lobby instances; matchmaking by age band, preferences and skill.
NOW: One shared room.
STEPS: 1. Lobby instances of a hub on the room server. 2. Matching by age band, game preferences, skill rating (so you don't always lose). 3. Lobby list for "Happening now".
NEEDS: Safety: teens and adults kept apart, block / mute / report (#46bf062c89); Room decides every shared thing (Phase 5) (#5bf378ab78)
UNLOCKS: Home screen with "Happening now" (#391136d79a); Events: date, time, RSVP, invite cards (#1a695dfa09); No-friends lobby (left alone) (#83348fa414); Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572)
NOT HERE: the home screen → Home screen with "Happening now" (#391136d79a)
DETAILS: PROGRESS master list 5.
```

### No-friends lobby (left alone)

`83348fa414` · Idea

```
WHAT: A lobby for people who want to be left alone.
NOW: Nothing yet.
STEPS: 1. A lobby type with no voice by default and no invites.
NEEDS: Lobbies and matchmaking (few main hubs) (#c344058f47)
UNLOCKS: nothing yet
DETAILS: docs/VISION.md → The product.
```

## Your home

_Your own place: grid placement, levels, expansion, invites. Not here: buying furniture (Shops)._

### Your home: invite-only, grid placement, expand 3x, levels 1-3

`44d68fd572` · Idea

```
WHAT: Your own home: invite-only, place things on a smooth grid, expand up to 3x, levels 1-3.
NOW: Nothing yet (rooms already stream in, homes will use the same).
STEPS: 1. A home instance per player on the room server. 2. Grid placement of owned items. 3. Invites. 4. Expand up to 3x and levels 1-3 (everything nicer).
NEEDS: The ledger (money and owned things) (#cb4648bc03); Lobbies and matchmaking (few main hubs) (#c344058f47)
UNLOCKS: Home screen with "Happening now" (#391136d79a); The street and city outside (#9537ee9062); The mall (IKEA-style: pick up, cart, checkout, delivery) (#aa5b3832e5)
NOT HERE: buying furniture → The mall (IKEA-style: pick up, cart, checkout, delivery) (#aa5b3832e5)
DETAILS: PROGRESS master list 6.
```

## Shops & money

_Where money and items change hands: credits, the mall, casino, VIP, trading. Not here: the wallet itself (Accounts)._

### Credits (the currency)

`42b352269f` · Done

```
WHAT: Credits, valued like the dollar so prices make sense.
NOW: Done: earned in darts, shown in the Menu with today's cap.
LATER: The new name (with the world's name).
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: the wallet itself → The ledger (money and owned things) (#cb4648bc03)
NOT HERE: more games paying → More games pay credits (once the server checks their results) (#9b263a4d3b)
DETAILS: PROGRESS master list 3.
```

**Claude's report:** Earned in games (darts so far); shown in the Menu. Name comes with the world's new name.

### The mall (IKEA-style: pick up, cart, checkout, delivery)

`aa5b3832e5` · Idea

```
WHAT: An IKEA-style store: walk in, pick items up, cart, checkout, delivered home, install on the grid.
NOW: Nothing yet.
STEPS: 1. The mall as a place. 2. Items you pick up into a cart. 3. Checkout through the ledger. 4. Delivery to your home.
NEEDS: Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572); The ledger (money and owned things) (#cb4648bc03)
UNLOCKS: Outfits and gadgets to show off (shop items) (#12f05869c9)
DETAILS: PROGRESS master list 7.
```

### Casino (18+, earned credits only)

`4e32747cd6` · Idea

```
WHAT: An 18+ casino: high risk, high reward (a rare 1k win), earned credits only.
NOW: Nothing yet.
STEPS: 1. Never bought with real money, never cashed out. 2. Lawyer review before launch.
NEEDS: Age at sign-up (13+, teen / adult) (#4ae9e009ad); The ledger (money and owned things) (#cb4648bc03)
UNLOCKS: nothing yet
DETAILS: PROGRESS master list 7; docs/VISION.md.
```

### VIP marketplace (real money, numbered and seasonal items)

`5b11225f55` · Idea

```
WHAT: A VIP store for real money: its own "big baller" place, unlocked forever by a first purchase; numbered, seasonal and event-only items.
NOW: Nothing yet.
STEPS: 1. The VIP place (closed door and glow for others). 2. Items bought directly, never currency. 3. Payer 18+ (parents for teens).
NEEDS: Payments (Stripe) for the VIP store (#038c4ad046); The ledger (money and owned things) (#cb4648bc03)
UNLOCKS: nothing yet
NOT HERE: trading → Trading (tradeable items only) (#d311f5a3c1)
DETAILS: PROGRESS master list 8.
```

### Trading (tradeable items only)

`d311f5a3c1` · Idea

```
WHAT: Trade only items made to be tradeable: both confirm, the server checks, cooldown.
NOW: The ledger already marks items tradeable.
STEPS: 1. Trade window. 2. Server-checked swap in the ledger. 3. Cooldown.
NEEDS: The ledger (money and owned things) (#cb4648bc03); Safety: teens and adults kept apart, block / mute / report (#46bf062c89)
UNLOCKS: nothing yet
DETAILS: PROGRESS master list 8.
```

## Ideas

_No home yet. Once an idea has a home, the card moves there (stage Idea)._

### Kids edition (under 13, separate game)

`bcf6829f24` · Idea

```
WHAT: A separate, simpler game for under-13s in the same universe.
NOW: Under 13 is refused at sign-up.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: PROGRESS.md Parked.
HOME: not decided (a separate game)
```

### New name for the world and its currency

`d207f08d3f` · Idea

```
WHAT: A real name for the world and its currency (Bar3D is a working name).
NOW: Working name.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: PROGRESS.md Parked.
HOME: not decided (the whole world)
```

## Performance (advanced)

_How fast and light it runs on the phone: frame time, memory, loading, quality settings._

### Performance pass (culling, compressed textures, lights, rooms, streaming)

`3dc594c005` · Done

```
WHAT: The performance pass: compressed textures, culling, cheaper lights, rooms drawn and loaded only when seen.
NOW: Done 2026-10-19/20. Still to do: 3 phone reports to compare with before.
LATER: Send 3 reports (tap the build code 3 times): after entering, at the mirror, after walking incl. the arcade.
NEEDS: nothing
UNLOCKS: Faster drawing, part 2 (render core, layout from Blender, quality settings) (#410d177f68)
DETAILS: PROGRESS master list 1.
```

**Claude's report:** All built 2026-10-19/20. Check: send 3 reports (tap the build code 3 times): after entering, at the mirror, after walking incl. the arcade.

### Site on Cloudflare Pages (threads)

`031aa208a1` · Idea

```
WHAT: The site on Cloudflare Pages: real headers, so threads (faster loading and physics) become possible.
NOW: GitHub Pages (no threads).
STEPS: 1. Cloudflare Pages, hashed asset names. 2. Threads via the headers it allows.
NEEDS: nothing
UNLOCKS: nothing yet
NOT HERE: moving accounts and reports → Move accounts off IONOS (#6c1190c7b4)
DETAILS: docs/RESTRUCTURE.md 8.1.
```

### Faster drawing, part 2 (render core, layout from Blender, quality settings)

`410d177f68` · Idea

```
WHAT: Room to grow for homes, the mall and the street: steady frame rate with more things on screen.
NOW: Part 1 done (see Performance pass).
STEPS: 1. Pick the order from the phone reports. 2. Render core and upload ring. 3. One floor plan exported from Blender (layout.json), geometry packs, instancing. 4. Cheaper mirror view for far things. 5. Shader library and light budget. 6. Lighter avatars, retained HUD. 7. Dynamic resolution and quality settings.
NEEDS: Performance pass (culling, compressed textures, lights, rooms, streaming) (#3dc594c005)
UNLOCKS: The street and city outside (#9537ee9062); Better light: shadows, bloom, reflections (#87aba35372)
DETAILS: docs/RESTRUCTURE.md Phase 6.
```

## Engine & code (advanced)

_Code structure with no visible change: the C++ core, ids, the activity system, cleanup._

### Clean core (main.cpp split, phase table, activities, tap resolver)

`863b8679ec` · Done

```
WHAT: main.cpp split into clear parts, the frame as a phase table, one activity system, one tap resolver.
NOW: Done: 4.1a, 4.1b, 4.3a, 4.4.
NEEDS: nothing
UNLOCKS: Clean core leftovers: 4.1c, 4.2 entity ids, 4.3b (#b2a21003f9)
DETAILS: docs/RESTRUCTURE.md Phase 4.
```

**Claude's report:** 4.1a, 4.1b, 4.3a, 4.4 done.

### Clean core leftovers: 4.1c, 4.2 entity ids, 4.3b

`b2a21003f9` · Idea

```
WHAT: The rest of the clean core: remote visuals in one place, stable entity ids, darts/bartending/race in the activity system + the pose layer.
NOW: Waiting: these need protocol changes.
STEPS: 1. 4.1c remote visuals. 2. 4.2 stable entity ids. 3. 4.3b activities + pose layer.
NEEDS: Clean core (main.cpp split, phase table, activities, tap resolver) (#863b8679ec)
UNLOCKS: Room decides every shared thing (Phase 5) (#5bf378ab78); Hands that really hold things; a face that moves when you talk (#4c4d68265d)
DETAILS: docs/RESTRUCTURE.md 4.1c, 4.2, 4.3b.
```

## Multiplayer server (advanced)

_The live shared room: what it decides and checks, the protocol, voice relays. Not here: data kept forever (Accounts)._

### Room decides every shared thing (Phase 5)

`5bf378ab78` · Idea

```
WHAT: The room decides every shared thing: seats and machines (claims), lights and mirrors, glasses, jukebox, piano, bartender, darts, 4K Mania.
NOW: The room checks every shared write and runs darts, the jukebox and the glasses.
STEPS: 1. 5.2 claims (seats, rigs, machines, bar station, mic). 2. 5.3 room state (lights, fire, mirrors). 3. 5.4-5.9 glasses, jukebox, piano, bartender, darts, 4K Mania sessions. 4. 5.10 voice signalling limits. 5. 5.11 remove the old world store.
NEEDS: Clean core leftovers: 4.1c, 4.2 entity ids, 4.3b (#b2a21003f9)
UNLOCKS: Live low-res machine screens + fair queue (20 s to claim your turn) (#d7d2844851); Lobbies and matchmaking (few main hubs) (#c344058f47)
NOT HERE: logins (5.1) → Move accounts off IONOS (#6c1190c7b4)
NOT HERE: score checks → Room checks chess moves and 4K Mania scores (#bcf65dd27a)
DETAILS: docs/RESTRUCTURE.md Phase 5.
```

### Room checks chess moves and 4K Mania scores

`bcf65dd27a` · Idea

```
WHAT: The room checks every chess move and 4K Mania score, so they can be trusted and pay.
NOW: Checked on the phone only.
STEPS: 1. Chess: the room runs the rules (app/chess_rules on the Worker). 2. 4K Mania: charts baked at build time; the room checks the score against the chart.
NEEDS: nothing
UNLOCKS: More games pay credits (once the server checks their results) (#9b263a4d3b)
DETAILS: docs/RESTRUCTURE.md 5.9a-b.
```

### Voice relays: TURN for strict networks, then a server (SFU) for big lobbies

`96ec3676fa` · Idea

```
WHAT: Voice connects on every network, and big lobbies don't overload phones.
NOW: Phones connect to each other (Google STUN only); some cellular networks fail.
STEPS: 1. A TURN relay (paid service or a small server). 2. An SFU when lobbies get big.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: docs/REMASTER.md → Voice chat; RESTRUCTURE 5.10.
```

## Accounts & ledger (advanced)

_Kept forever per person: logins, ages, money, owned items, payments. Not here: spending it (Shops)._

### The ledger (money and owned things)

`cb4648bc03` · Done

```
WHAT: The ledger: money and owned things on Cloudflare, every change a transaction.
NOW: Done: wallets in cents, unique refs, never below zero, $500 daily cap, numbered items, tradeable flag.
NEEDS: nothing
UNLOCKS: More games pay credits (once the server checks their results) (#9b263a4d3b); Your home: invite-only, grid placement, expand 3x, levels 1-3 (#44d68fd572); The mall (IKEA-style: pick up, cart, checkout, delivery) (#aa5b3832e5); Casino (18+, earned credits only) (#4e32747cd6); VIP marketplace (real money, numbered and seasonal items) (#5b11225f55); Trading (tradeable items only) (#d311f5a3c1)
DETAILS: cloud/ledger.js; DECISIONS.md.
```

**Claude's report:** Cloudflare, every change a transaction, daily cap, numbered items.

### Age at sign-up (13+, teen / adult)

`4ae9e009ad` · Done

```
WHAT: Birth date at sign-up: 13+ only, teen or adult band, kept private.
NOW: Done. Older accounts have no band yet (ask them later).
LATER: Ask older accounts for their birth date.
NEEDS: nothing
UNLOCKS: Safety: teens and adults kept apart, block / mute / report (#46bf062c89); Casino (18+, earned credits only) (#4e32747cd6); Payments (Stripe) for the VIP store (#038c4ad046)
DETAILS: PROGRESS master list 3.
```

### Password change + recovery code

`3ec1e49ff5` · Check on phone

```
WHAT: Change your password; a recovery code resets it.
NOW: Built; check on the phone: Profile → your card → Password & recovery.
NEEDS: nothing
UNLOCKS: nothing yet
DETAILS: server/account.php.
```

**Claude's report:** Profile → your card → Password & recovery. Make a recovery code and save it.

### Move accounts off IONOS

`6c1190c7b4` · Idea

**What the owner wants:** Let’s get everything out of ionos besides the things that would be security benefits do your thing

```
WHAT: Accounts, reports and everything else off IONOS, except what is safer there.
NOW: Logins, profiles, reports and this hub run on IONOS PHP.
STEPS: 1. Logins to Cloudflare next to the ledger (PBKDF2 hashes, moved on first login). 2. Signed tickets so the room trusts who you are. 3. Phone reports on Cloudflare. 4. Decide what stays on IONOS (home page, maybe this hub).
NEEDS: nothing
UNLOCKS: Payments (Stripe) for the VIP store (#038c4ad046)
NOT HERE: the game site itself → Site on Cloudflare Pages (threads) (#031aa208a1)
DETAILS: docs/RESTRUCTURE.md 5.1a-c, 8.2-8.3.
```

### Payments (Stripe) for the VIP store

`038c4ad046` · Idea

```
WHAT: Real-money payments (Stripe) for the VIP store; payer 18+.
NOW: Nothing yet.
STEPS: 1. Stripe account (the user). 2. Checkout → ledger item, never currency. 3. Refunds and receipts.
NEEDS: Move accounts off IONOS (#6c1190c7b4); Age at sign-up (13+, teen / adult) (#4ae9e009ad)
UNLOCKS: VIP marketplace (real money, numbered and seasonal items) (#5b11225f55)
DETAILS: PROGRESS master list 8.
```

## Tests & tools (advanced)

_How we work: tests, deploys, phone reports, this hub, the AI dock. Bugs go with the broken thing._

### The Hub update.

`bedd869642` · In progress

**What the owner wants:** The hub works but the workflow is not what I want it to be, I’d like to be able to log on or open my site and be able to simply use just words to awnser what the ai wants to know for each box that I chose, also I’d like to be able to have all the different categories easier to access and it should’ve way more user friendly like on of those newer systems the ui is also nice with color but I think I’d like something a bit more sharp not as bubbly and more geared towards production and work efficiency not just something that works with a lot of edits and reading things should be separated by what the current status is, what I want to from that point on and the ai can auto update the card with my new data and it’s on file version of it so we can have multiple sources to check for what goes where what and how and I’d like to make it so I can clearly have my awnser box and a lot of quality of life features since this is supposed to be a visual hub for me human to interact with my entire full project and I’d like it to be so that this hub tool will be used not just for this Bar3d project but for whatever I want to work on so let’s make sure we have an appropriate selection menu for projects with a screenshot of the bar itself at the top half with the name and the info and details as well on there / The goal for this one is I edit everything and work from here and when I’m read for ai Claude to work I say go and you see what the is to finish and work on that I’ve already approved, then you get back to me through here and I repeat that cycle I don’t have enough money to have it work from this site alone so I’ll do it like that

```
WHAT: The hub works the way you work: pick a project, see what needs you, answer Claude in plain words, edit what you want; Claude picks it up on "go" and answers on the card.
NOW: v2 live (build 05cfaf2). Projects screen (Bar3D with a shot of the bar, name, about, counts, link; + New project with your own picture). Sharper, denser page. Views: Needs you (questions, to check, to review), Claude queue, In progress, All, and every section in the nav strip (☰ lists them all). A card opens full screen: Status (stage bar, one-tap next step, report), Your plan (What you want: your words, Claude never overwrites them; Claude's plan below), Talk (the conversation, answer box always at the bottom, "Send + Ready"), Details (title, section, raw plan, order, delete, history). Search, drafts kept on your phone, board download. Claude: hub.mjs inbox (what you wrote) read first on every "go"; the board copied into docs/hub/bar3d.md.
LATER: 1. Drag to reorder cards. 2. A badge or notification when Claude answers. 3. Members and the activity feed stay on This hub (#83d21fa99c).
NEEDS: This hub (#83d21fa99c)
UNLOCKS: nothing yet
NOT HERE: Claude working from the site by itself → AI dock (work on the project from the site) (#0984e150c5)
DETAILS: docs/DECISIONS.md → Hub v2; docs/HUB_MAP.md §5.
```

### AI dock (work on the project from the site)

`0984e150c5` · Idea

```
WHAT: Work on the project from the site: talk to the AI, it changes code, deploys, never leaving the site.
NOW: Partly this hub (cards go to Claude).
NEEDS: This hub (#83d21fa99c)
UNLOCKS: nothing yet
DETAILS: PROGRESS.md Parked.
```

### This hub

`83d21fa99c` · Check on phone · build d752207

```
WHAT: This hub: the board of the world, the Claude queue, reports.
NOW: v1 live. Rules for what goes where: docs/HUB_MAP.md; node tools/hub.mjs check.
STEPS: 1. v2: the user's brothers as members; their cards wait for review. 2. v3: an activity feed.
NEEDS: nothing
UNLOCKS: The Hub update. (#bedd869642); AI dock (work on the project from the site) (#0984e150c5)
DETAILS: hub/; docs/HUB_MAP.md.
```

**Claude's report:** v1 is live: login with your bar account, World / Claude queue / Check on phone / Advanced tabs, cards with stages, plan, Claude's report and questions, up/down order, Account: change password, recovery code, Claude key. / How to check: 1) add a card and set it to Ready for Claude, it shows in Claude queue. 2) Up/Down moves a card in its section. 3) Make a recovery code and save it. Next: v2 members (your brothers, their cards wait for your review), v3 activity feed.

