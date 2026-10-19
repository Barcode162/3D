# Racing sim: the plan (2026-10-16)

The user's request, in short: a **genuine racing sim** on the two sim rigs,
for one or two players (left rig and right rig), with bots you race and
bump into like Need for Speed, physics in the spirit of Assetto Corsa, and
it must end up looking amazing. **Build the best possible shell and
structure first, details later**: the driving feel comes first, art last.
This file is the master list; tick items off as they land. The next feel pass (camera, sound, car, track, tuning) is planned in `docs/RACING_FEEL.md`. Every mini game
should eventually get the same treatment ("make each mini game really
powerful").

What the user asked for, point by point:
- Real physics (tires, suspension, weight transfer, engine, gears), real
  race (grid, countdown, laps, positions), bots that race and collide.
- Touch controls that feel **buttery smooth**: big steering travel, not a
  tiny joystick radius. Plus keyboard, mouse, and real sim controls
  (gamepads and steering wheels through the browser's Gamepad API).
- When you sit in the rig, **the whole screen becomes the game** (you dive
  in). Played sideways (landscape). One view while playing, not three.
- Everyone in the bar sees each race on that rig's **three monitors**
  (left / middle / right views), for both rigs.
- Fix the big stutter while the bar loads.
- A real **realtime sync server** instead of the PHP relay, so movement
  isn't laggy and phones don't connect to each other for game data.

## Architecture (the shell)
Everything is C++ in our own engine, like the rest of Bar3D.

```
engine/vehicle.*      car physics: rigid body, 4 wheels, suspension, tires,
                      engine, gearbox, differential, brakes, aero
app/race/track.*      the circuit: a closed spline -> road, curbs, grass,
                      gravel, walls; fast "where am I on the track" queries
app/race/world.*      the race: cars, bots, car-to-car and wall collisions,
                      grid, countdown, laps, positions, fixed 240 Hz steps
app/race/ai.*         bots: racing line + speed plan, steering, braking,
                      overtaking, avoiding
app/race/controls.*   touch / keyboard / mouse / gamepad / wheel -> steer,
                      throttle, brake, gears, with smoothing and assists
app/race/view.*       3D renderer for the race (own sky, sun, PBR, fog),
                      full screen for the driver, small views for monitors
app/race/game.*       glue: sit -> dive in, HUD, cameras, sounds, leaving,
                      network state
```

### Physics (engine/vehicle)
- Fixed time step (240 Hz, substepped from the frame). *Not yet* the same
  at every frame rate: a frame is capped at 0.1 s (24 steps), so after a
  hitch the race clock falls behind the wall clock, and the lap times use
  the wall clock. Restructure step 2.4a moves the race onto a tick clock
  so results match at 30, 60 or 120 fps.
- Rigid body with mass and inertia, 4 wheels with **raycast suspension**
  (spring, bump and rebound damping, bump stops, anti-roll bars).
- **Tires: Pacejka "Magic Formula"** for lateral and longitudinal force,
  combined slip (braking while turning shares the grip), load sensitivity
  (a heavier-loaded tire grips less per kilo), relaxation length so the car
  sits still without jitter at low speed. Grip per surface: asphalt, curb,
  grass, gravel.
- Wheel spin is simulated (wheelspin, lockups). Engine torque curve, rev
  limiter, engine braking, clutch, gearbox (auto or manual), limited-slip
  differential, brake bias, aero drag and downforce, Ackermann steering.
- Car numbers live in one struct (`CarSpec`) so more cars can be added
  later from data files.
- Collisions: car vs car (oriented boxes, impulses that spin cars like real
  contact), car vs walls/barriers (the track knows where its walls are).

### Track
- One closed circuit (~3 km: long straight, hairpin, esses, fast sweepers,
  elevation changes) defined as a spline with width, banking and surfaces.
- Generated in code first (road, painted lines, red/white curbs, grass,
  gravel traps, tire walls, armco, start line, grandstand blocks, lights).
  Later replaced piece by piece with Blender assets (the asset pipeline in
  `tools/assets/`), same spline underneath so physics doesn't change.

### Bots
- A racing line computed from the track (smoothed inside the edges) and a
  speed plan from the curvature and grip; bots drive the **same physics**
  as you (no cheating cars), steering by "pure pursuit", throttle/brake by
  speed error. They overtake, defend, avoid crashes, and can be bumped.
- Difficulty by how close to the grip limit they drive.

### Controls
- **Touch (landscape):** left part of the screen is a big steering pad:
  put your thumb anywhere and slide; a full lock takes a long slide (about
  a quarter of the screen width), with a curve that's soft in the middle
  for small corrections, smoothing, and speed-sensitive steering (like a
  gamepad assist in sims). Right part: brake and gas pedals; sliding up on
  a pedal presses it harder (analog), a tap ramps smoothly instead of
  slamming. Optional tilt steering (phone as a wheel).
- **Keyboard:** arrows / WASD with smooth ramps (not on/off), space =
  handbrake, Q/E or Shift/Ctrl = gears, C = camera, R = reset, Esc = leave.
- **Mouse steering** option: mouse left/right = wheel position, buttons =
  gas and brake.
- **Gamepad and steering wheels** (Gamepad API): standard controllers out
  of the box (Xbox/PlayStation also work on iPhone); wheels (Logitech,
  Thrustmaster, Fanatec, Moza) through a calibration screen (turn the
  wheel, press each pedal). Force feedback isn't available in browsers.
- Assists: auto gears, ABS, traction control, steering assist; each can
  be turned off for the full sim experience.

### Showing the race
- Driver: full-screen 3D view (chase camera, bumper/hood, cockpit later),
  HUD with speed, gear, revs, lap, position, times, mini map later.
- Spectators: each rig's three monitors show that driver's race, rendered
  by the spectator's own phone from the synced car positions (left /
  middle / right cameras), at low resolution and only when the rig is
  near and on screen, so it stays cheap.

### Multiplayer and the realtime server
- IONOS shared hosting can't run a realtime (WebSocket) server, so the new
  server needs another host. Recommended: **Cloudflare Workers + Durable
  Objects** (free plan, WebSockets, servers close to everyone, deployable
  from GitHub Actions). Needs the user's Cloudflare account and an API
  token stored as a GitHub secret. The PHP relay stays as a fallback.
- Server design: one "room" object per bar; everyone keeps one WebSocket
  to it; the server stamps everything with its own clock, sends state at
  20-30 Hz for the bar and up to 60 Hz for races, and owns the shared
  world (glasses, jukebox, seats/machines so two people can't take one).
- Racing netcode: each phone simulates its own car (instant response,
  like every online racing game), sends inputs + state; others are shown
  slightly in the past with interpolation and short extrapolation. The
  race "host" (later the server) runs the bots, the start and the timing.
  Collisions with the other player's car use their synced car as a solid
  object on your side, and the same happens on theirs. *Caveat:* as built,
  remote cars act with infinite mass (you bounce off, they don't move),
  which feels wrong; the real netcode is redesigned in restructure
  Phase 7 (`docs/RESTRUCTURE.md` 7.3).

## Steps (in order)
- [x] 1. Loading stutter fix: textures upload a little per frame, mipmaps
      made on the GPU, not all at once on the CPU.
- [x] 2. Physics core + test track + full-screen race view + touch /
      keyboard / mouse / gamepad controls + chase camera + HUD (drive
      alone). **The driving feel is tuned here** (on the phone, by the
      user's feedback). Built 2026-10-16.
- [x] 3. Bots and collisions, the real race (grid, countdown, laps,
      positions, results). First version built with step 2: 7 bots on
      a computed racing line, car-car and barrier contact, 5 start
      lights, 3 laps, standings, results, tap to race again.
- [ ] 4. The three monitors on each rig show the race for spectators.
- [x] 5. Two players (left/right rig) racing each other (2026-10-19: garage,
      the room's lobby `cloud/games/race.js`, one start time for both, each
      car shown 100 ms in the past on the other phone; no bots in a duel yet).
- [ ] 6. Realtime server (Cloudflare) for the race and the whole bar.
- [ ] 7. Polish passes: car model and cockpit (Blender), track art,
      shadows, sky, sounds (engine from rpm and load, tires, wind),
      replays, more cars and tracks, settings, wheel calibration.

## What exists now (2026-10-16, first build)
- `engine/vehicle.*`: the car physics described above. Measured with
  `tools/native/vehicle_test.cpp` (runs natively with g++, no browser):
  0-100 km/h 4.2 s, top 276 km/h, 100-0 braking 30.7 m (ABS) / 36.4 m
  (locked), ~1.1-1.2 g cornering, a parked car doesn't move at all.
- `app/race/track.*`: "Bar3D Ring", 2.45 km, 27 control points, curbs,
  gravel traps on slow corners, tire barriers, terrain, ~2000 trees,
  grandstand, pit building, start gantry. 26 chunks, ~128k triangles.
- `app/race/world.*`: 8 cars, 240 Hz steps, bots (racing line by
  smoothing, speed plan from curvature, pure pursuit steering, passing,
  slide catching, stuck/flip reset), collisions, laps, positions.
  `tools/native/race_test.cpp` runs a whole bot race in ~1 s.
- `app/race/controls.*`, `game.*`, `view.*`, `car_model.*`,
  `shaders/race.wgsl`: as in the architecture above. The car is built in
  code (body loft with wheel arches, glass, lights, wheels with spokes).
- Sitting in a sim seat starts a race (left rig grid slot 4, right 5);
  the page's top bar hides while racing (`body.racing`).
- Not yet: spectators see only "<name> IS RACING / speed" text on the
  middle monitor (step 4), no multiplayer race (step 5), one engine hum
  sound, no tire squeal, no shadows from the sun (only soft blobs under
  cars), no cockpit view, no wheel calibration screen, no tilt steering,
  no settings menu for assists (keyboard: manual gears not reachable
  yet, `manualGears_` in game.h).

## Tuning notes
- Steering assist (controls.cpp): full input = wheelbase * 1.15 g / v^2 +
  0.95 * peak slip angle. If touch steering feels too slow/fast, change
  `kSteerSpan` (0.2 screen widths for full lock) or the 1.35 curve.
- Bot pace: `World::buildRacingLine` (grip 1.12 g, braking 9 m/s^2,
  accel 6 m/s^2) and `Racer::skill` (0.9..0.98).
