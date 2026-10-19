# Racing: the "Drive Zone feel" plan (2026-10-19, not started)

The user wants the **bot race on the sim rigs** (our own engine, `app/race/`,
not Meridian) to feel like **Drive Zone Online** (Jet Games, Unity): smooth,
heavy but controlled, comfortable camera, clean visuals, sound that tells
you what the tires are doing. This file is the plan. It is parked in
`PROGRESS.md` until the user promotes it at "start".

Nothing about Drive Zone's internals is public (only: Unity/C#, a custom URP
fork, a road editor, day/night and weather, mobile profiling). So this is
**our own design aiming at the same feel**, not a copy of theirs. The user's
research notes (wassimulator's "Programming vehicles in games", Unity's
interpolation and profiling docs) are the references.

## What we already have (don't rebuild)
| The research says | Bar3D today |
|---|---|
| Fixed-step physics, drawn between steps | Done: 240 Hz tick clock, results the same at 30/60/120 fps, `World::shownCar` blends the last two steps (RESTRUCTURE 2.4a) |
| Force-driven chassis, 4 raycast wheels | Done: `engine/vehicle.*`, spring, bump/rebound damping, bump stops, anti-roll bars |
| Tires: slip ratio, slip angle, load, combined grip, peak then fall-off | Done: one Magic Formula curve over combined slip, load sensitivity, relaxation lengths (a parked car doesn't creep) |
| Engine, clutch, gearbox, differential, brakes, ABS/TC, aero | Done (auto gearbox from wheel speed, LSD, brake bias, drag + downforce) |
| Steering: full lock slow, gentle at speed, rate limit, self-centring, countersteer help | Done: `engine/steering.*` (10.4 m turning circle, ≈ 1 g at speed, `steering_test`) |
| Surfaces with their own grip | Done: asphalt, curb, grass, gravel |
| Measured handling | Partly: `vehicle_test` (0-100, braking, cornering), `steering_test`, `race_test` |

So the physics base is already sound. What separates us from Drive Zone
is everything **around** it: camera, sound, the look of the car and road,
the tuning garage, and proof that it stays smooth on the phone.

## What's missing (the gaps)
1. **Camera:** chase cam only lags the heading (`game.cpp` `updateCamera`);
   position snaps to the car, no spring, no pitch/roll handling, no barrier
   check, no cockpit.
2. **Sound:** one engine hum (pitch from revs). No tires, wind, gear
   shifts, curbs, impacts.
3. **The car's look:** built in code (`car_model.cpp`), one material. No
   clear-coat paint, glass, rubber, chrome, lamp lenses, no interior.
4. **Light and road look:** no sun shadows (blobs under the cars), no sky
   reflections on paint, road texture with no anisotropic filtering
   (shimmers at low angles), no lane/wear detail.
5. **Handling proof:** no test for bump settling, crests, seams, stopping
   on a slope, reversing through zero, landing.
6. **Tuning:** the garage picks car, paint, rims. Drive Zone also tunes
   suspension, camber, engine, gearbox.
7. **Smoothness proof on the phone:** the profiler shows averages; no
   worst-frame (1%) numbers for a race, no quality levels.

## The steps (in order; each one is a deploy the user can feel)

### R1. Handling lab (measure before changing)
- **What:** a native test track in `tools/native/handling_lab.cpp` (runs in
  `run.sh`, no browser) with: straight, 50 m constant-radius turn, slalom,
  braking zone, banked turn, crest and dip, a rough patch, a road seam (a
  1 cm step), a 10 % slope. Each case logs steer asked vs. wheel angle,
  speed, yaw rate, wheel loads, suspension travel, slip, per step.
- **Pass if:** after a 5 cm bump the body settles in ≤ 2 bounces (≤ 0.6 s);
  the seam changes no wheel's load by more than 15 %; parked on the slope
  the car moves < 1 cm in 10 s; braking from 5 km/h through zero into
  reverse has no jerk (no step in speed > 0.3 m/s); landing from the crest
  at 120 km/h doesn't bounce the car off the road; slalom at 60 km/h keeps
  body slip ≤ 4°. All three cars (GT, MUSCLE, ROADSTER).
- **Plus** a `?telemetry=1` overlay in the race (speed, steer, slip per
  tire as 4 small bars, suspension per wheel, frame time) so the user can
  see on the phone what the car is doing.
- **Phone:** nothing changes unless a test fails (then the fix is part of R1).

### R2. Camera that feels like a real chase cam
- **What:** a spring-damper chase camera on the drawn (blended) car:
  position and aim follow separately (aim faster than position), a bit of
  look-into-the-turn, body roll left out (the world stays level), pitch
  from braking/accelerating kept small (≤ 1.5°), FOV widens smoothly with
  speed (already there, keep it), the camera slides in front of barriers
  instead of going through them, no snap on a reset or respawn (it cuts).
  New **cockpit view** (needs R4's interior; until then hood view gets a
  steadier eye).
- **Pass if:** native camera test: no camera jump > 5 cm per frame at a
  steady 200 km/h; after a 90° turn the camera is behind the car again
  within 0.5 s, without overshoot; at 30, 60 and 120 fps the camera path
  is the same within 2 cm.
- **Phone:** turning and sliding look calmer; the world doesn't tilt with
  the car; you can see slightly into corners.

### R3. Sound that tells you what the car is doing
- **What:** made in code like the piano (`engine/synth.*`), no recorded
  files: engine from revs **and** load (two layers: on-throttle and
  off-throttle, a bit of crackle when you lift), the gear shift (a short
  cut and blip), tire **scrub that starts before the slide** (from each
  tire's slip compared to its peak, louder per tire load) turning into a
  squeal past the peak, wind from speed, curb rumble, gravel/grass noise,
  thumps for bumps, landings and hits. The other cars get a simple engine
  each, the nearest 3 only (pooled voices).
- **Pass if:** a native test feeds recorded telemetry from R1 through the
  sound mixer and checks: scrub starts at ≈ 70 % of peak slip, squeal only
  past it; no clicks (no sample-to-sample jump over a set limit); CPU cost
  ≤ 1 ms per 10 ms of audio.
- **Phone (with sound on):** you hear the tires start to complain just
  before they let go; shifts, curbs and wind are audible.

### R4. One beautiful car (the quality benchmark)
- **What:** the GT built in Blender (`tools/assets/`, our pipeline) with
  clean curves, separate wheels, brake discs and calipers, lamp lenses,
  a simple **interior** (dash, wheel that turns, seats) for the cockpit
  view. Materials: clear-coat paint (two highlights: a sharp coat on a
  softer base), tinted glass, rubber, chrome, plastic trim, emissive lamps.
  Paint reflects a small sky image (an environment map made once at load).
  Two levels of detail (near, far) for the bots. MUSCLE and ROADSTER after
  the GT is approved.
- **Pass if:** close-up screenshots in the garage (front, side, rear, a
  moving highlight across the hood without shading bumps); triangle and
  texture budget checked with `glb_report.py` (≤ 60 k triangles near,
  ≤ 6 k far, ≤ 8 MB textures per car).
- **Phone:** the garage car looks like a real car; paint shines with the
  sky; the cockpit view works.

### R5. Track that looks clean in motion
- **What:** sun shadows for cars and nearby objects (one shadow map
  following the car), anisotropic filtering on the road (stops the
  shimmer), asphalt with fine detail + patches + tire marks and crisp
  painted lines, curbs and grass with proper texture, a better sky.
  Road and collision stay on the same spline (physics doesn't change).
  Overlaps RESTRUCTURE 7.6 (renderer polish); do both together.
- **Pass if:** the 5 race screenshot spots (`?sit=30` plus `CAM=`) look
  right; race draws ≤ 100; GPU estimate within R7's budget.
- **Phone:** no sparkly road in the distance, cars sit on the road with
  real shadows.

### R6. Tuning garage + assists menu
- **What:** in the garage, per car: suspension (soft/medium/stiff),
  ride height, camber (more grip in turns, less under braking), brake bias,
  gear ratios (short/long), plus assists on/off (ABS, traction control,
  steering help, auto/manual gears). Settings saved on the phone, later on
  the account (master list item 3). Each setting changes real `CarSpec`
  numbers, so the physics changes, not a fake multiplier.
- **Pass if:** `handling_lab` runs every preset and every one still passes
  R1's rules; the race lobby shares each player's tune so both phones
  simulate the same car.
- **Phone:** stiff suspension feels sharper, soft feels floatier; turning
  traction control off lets the rear step out.

### R7. Smooth on the phone, proven
- **What:** a race budget (GPU ≤ 10 ms, CPU ≤ 4 ms per frame on the iPhone
  15 Pro Max), worst-frame (1 %) and hitch counts in the phone report for a
  race lap, and **quality levels** (Auto / High / Low): shadow size, draw
  distance, trees, reflections, render scale. Auto steps down if the phone
  heats up and frames run late, and back up when it recovers.
- **Pass if:** the user's phone report after 3 laps: 60 fps with 1 % worst
  frame ≤ 20 ms and no hitch over 50 ms.
- **Phone:** stays at 60 fps over several races without getting hot.

### After R7 (already planned elsewhere)
- Two players with proper contact and smoothing: RESTRUCTURE 7.1-7.4.
- Spectator monitors on the rigs: RESTRUCTURE 7.5 (RACING step 4).
- More tracks, replays, wheel calibration: RACING step 7.

## Rules for every step
- Physics changes always pass `vehicle_test`, `steering_test`, `race_test`
  and `handling_lab` before a push; lap times are printed so changes in
  speed are visible.
- Smooth the right thing: your own car's controls are never delayed to
  look smooth. Only the camera, the drawing between steps and the other
  players' cars get smoothing.
- Everything stays our own code (no Unity, no physics library). Blender is
  a tool for models, not part of the game.
- Each step ends with a "How to check it worked" list for the phone.

## Questions for the user (before R1)
1. Order: is R2 (camera) + R3 (sound) first OK? They change the feel the
   most for the least work. Art (R4, R5) is the biggest job.
2. Drive Zone is open-world. Do you want a **free-drive / drift area**
   (no race, just driving) later, or stay with circuit racing?
3. Do you have a car you'd like the GT to look like (a photo or a
   real model name)? It will be our own design, not a copy of a real
   brand.
