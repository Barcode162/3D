# NEXT: what "go" does next

The user says "go"; Claude does the whole step below, piece by piece (one
commit each, tagged), deploys once, then rewrites this file for the step after.
How: `CLAUDE.md`. The full engine plan: `docs/ENGINE.md` (E1-E9).

## Where we are (2026-10-21)
- Done: **E1** the core (things, states, rules, loops) · **E2** the bar as
  things (a place file; every object alive; phone check: 60 fps).
- Left: E3 sync · E4 drawing/lighting · E5 avatar · E6 sound/physics ·
  E7 places/travel · E8 hub control room · E9 games re-plugged, old code gone.

## The next step: E3, taps, seats and sharing through the world
Today taps, seats, the jukebox, darts and glasses each have their own sync code
(`bar/shared_world.*`, `bar/game.cpp`, `cloud/games/*`). After E3 they all go
through the world: a tap sends an event, a rule changes a state, the `Change`
goes over the room, and the room (running the same world, `cloud/sim/`) has
the last word. Nothing should look different on the phone, except that what
other players see stays in step.

The pieces (refine them after reading the code, keep each one small and
checkable; one commit each):
- [ ] **E3.1** `Change` in the room protocol: both codecs (`cloud/proto.js`,
  `looped/netproto.*`) + `proto/vectors.json` + docs/PROTOCOL.md; the room
  relays it. Phone: nothing changes (the bar loads, multiplayer works).
- [ ] **E3.2** Taps through rules: tapping a thing sends its event to the
  world; today's simple toggles (fire, mirrors, lights) become states with rules.
  Phone: tap the fireplace and a mirror; they work as before.
- [ ] **E3.3** Seats as states (free / taken by a player). Phone: sit on a
  stool and the sofa, stand up; a second player can't take your seat.
- [ ] **E3.4** The jukebox through the world (song, start time as states).
  Phone: pick a song; it plays and the jukebox thumps on the beat.
- [ ] **E3.5** Darts and glasses through the world (the room's games become
  rules). Phone: throw darts (score shows), pick up and drop a glass.
- [ ] **E3.6** The room runs the world (`cloud/sim/`), its word wins; a late
  joiner gets the states. Test: mp_test with a third player joining late.
- [ ] **E3.7** Delete the old sync paths; `docs/ENGINE.md` E3 ticked.
  Phone: everything in E3.2-E3.5 once more.

Tests that matter here: `node tools/check.mjs` picks them (room + build +
native + game; `all` before the push to main).

## Build log (newest first: build code → what → commits; "undo" reverts one row)
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
