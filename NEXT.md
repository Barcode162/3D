# NEXT: what "go" does next

The user says "go"; Claude does the whole step below, piece by piece (one
commit each, tagged), deploys once, then rewrites this file for the step after.
How: `CLAUDE.md`. The full engine plan: `docs/ENGINE.md` (E1-E9).

## Where we are (2026-10-21)
- **The engine plan (E1-E9) is all built** (docs/ENGINE.md → Progress). E1-E2
  phone-checked (60 fps); E3-E7 live as `fa64f17`; E8-E9 in the push after.
- Next: the user's phone check of the whole batch (below), then fix what broke.

## The next steps: E3 (rest) to E9, in one batch (user, 2026-10-21)
The user asked for as much of E3-E9 as possible coded in one go, tested
together on the phone at the end. Each piece is its own commit (tag first),
checked before it's committed; one deploy at the end. If the session ends
early, the next "go" carries on from the first unticked piece.

E3, the rest (taps, seats, sharing):
- [x] **E3.3** Seats as states: a stool sinks a little and stops drifting while someone sits on it (from the room's seat claims).
- [x] **E3.4** The jukebox flourishes (a light sweep) when a new song starts, for everyone.
- [x] **E3.5** The dartboard shudders when a dart hits it (yours and other players').
- [x] **E3.6b** A late joiner gets every thing's state (mp_test: a third player joins after the fire went out).
- [x] **E3.7** Mirror modes are the mirror's states (its buttons send it "full" / "avatars" / "off"); the old m<n> key goes.

E4, drawing:
- [x] **E4.1** Precise touches: big things (the counter, the back bar, the piano) are touched through their solid shapes, not their boxes; knock on them and they shine.
- [x] **E4.2** Soft shadows on the floor under every avatar.
- [x] **E4.3** Particles as small things a kind carries: a candle you blow out sends up three wisps of smoke (the fire already has its sparks).
- [x] **E4.4** Reflections: the bar is captured once into a cube picture; brass, lacquer and glass reflect the real neon and bulbs.

E5, avatars:
- [x] **E5.1** Idle life: standing avatars shift their weight and glance around now and then (seeded, so everyone sees the same).
- [x] **E5.2** Smooth changes: sitting down, standing up, an emote starting or ending blend over a quarter second instead of snapping (heads already follow where each player really looks, so no fake look-at).

E6, sound:
- [x] **E6.1** Things make their own sounds: the fire crackles, the neon sign buzzes, near them only (a "sound" part).
- [x] **E6.2** Touch sounds per kind: a knock on wood, a clink on the back bar, a soft bong on a lamp.

E7, places:
- [x] **E7.1** Boxes as parts (floors, walls, shelves made from plain shapes), so a place doesn't need its own model.
- [x] **E7.2** A second place, a small test home (places/home.json), built from boxes and the bar's furniture.
- [x] **E7.3** Travel: Menu → "Visit the test home" / "Back to the bar"; each place has its own room.
- [x] **E7.4** The catalog (assets/world/catalog.json): every kind you could own, with a price and whether it's tradeable; a test keeps it in step with the kinds.

E8, the hub as the control room:
- [x] **E8.1** The hub's World page: every kind and thing of the bar, with its loops and states in plain words.
- [x] **E8.2** Live tuning: a loop's speed and on/off set in the hub; the bar reads them when it loads.

E9, projects plugged in, old code gone:
- [x] **E9.1** Projects as a part (`project`: the arcade machine, the rigs, 4K Mania, the chessboard name what they open).
- [x] **E9.2** Old paths deleted (lt, m<n>, leftovers); docs/ENGINE.md ticked.

Tests: `node tools/check.mjs` (build, native, game, room) per piece; `all` before the push to main.

## The next step: P1, the phone check (after the user has gone through it)
The user walks the check list from the last reply on the phone and says
"broke <piece>" for anything wrong. On "go" after that:
- [ ] **P1.1** Fix every "broke" the user reported (one commit each, tagged with the piece it fixes).
- [ ] **P1.2** A `?perf=1` report in the busy part of the bar (by the fire, the mirror in view): compare with E2's 63H3FJ (16.7 ms avg); if reflections or shadows cost more than 1 ms, make them cheaper.
- [ ] **P1.3** Tune what the user wants slower/faster on the hub's World page, or turn it into the kinds' own numbers.

## Build log (newest first: build code → what → commits; "undo" reverts one row)
- (this push) E8.1, E8.2, E9.1, E9.2: the hub's World page and live tuning, projects as a part, old keys gone → fa64f17..HEAD
- `fa64f17` E3.3–E3.7, E4.1–E4.4, E5.1–E5.2, E6.1–E6.2, E7.1–E7.4 → e807e99..fa64f17
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
