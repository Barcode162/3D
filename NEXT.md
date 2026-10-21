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
- [x] **E3.1** Thing states over the room: no new codec needed. A state rides
  the room's world keys as `t<net>` = [state, start] (stored, sent to late
  joiners, written only by the room); the `thing` intent asks for an event.
  docs/PROTOCOL.md §4.3, §4.5.
- [x] **E3.2** Taps through rules: tapping a thing sends "tap" to the world.
  The fire and the light switch are states with rules (the old `lt` key is
  gone); lamps swing, tables wobble, candles blow out, the dartboard and the
  chalkboard swing on their nails, all shared through the room.
  (Mirrors: still their own key, folded in with E3.7.)
- [x] **E3.6 (first half)** The room runs the world (`cloud/sim/`: the same
  files and C++): it checks you're within 6 m, runs the rule, shares every
  change (a fire going out = 8: flames, glow, 6 sparks). mp_test: Dave taps a
  lamp on his screen, Bob's game sees it swing.
- [ ] **E3.3** Seats as states (free / taken by a player). Phone: sit on a
  stool and the sofa, stand up; a second player can't take your seat.
- [ ] **E3.4** The jukebox through the world (song, start time as states).
  Phone: pick a song; it plays and the jukebox thumps on the beat.
- [ ] **E3.5** Darts and glasses through the world (the room's games become
  rules). Phone: throw darts (score shows), pick up and drop a glass.
- [ ] **E3.6 (second half)** A late joiner gets every state (mp_test with a
  third player joining after a change).
- [ ] **E3.7** Mirrors to states, delete the old sync paths
  (`bar/shared_world.*` keys, rules.js entries); `docs/ENGINE.md` E3 ticked.
  Phone: everything in E3.2-E3.5 once more.

Tests that matter here: `node tools/check.mjs` picks them (room + build +
native + game; `all` before the push to main).

## Build log (newest first: build code → what → commits; "undo" reverts one row)
- `ff60ac4` E3.1, E3.2, E3.6 (half): touch things, the room runs their rules → a2fe850..HEAD
- `a2fe850` E2 done: the bar as a place file, every object alive (before this file existed).
