# Upgrade plan (the user's list, 2026-10-19)

The user's request, item by item, in their order of mention. Tick each box
when it's done and deployed. Work top to bottom unless the user reorders.
(The server-run bot race was started and then replaced by item 1-2, as the
user asked: "the race game should actually be fixed".)

## Added 2026-10-19 (user): Clash Royale style UI
- [x] **UI like Clash Royale:** chunky outlined font (Lilita One, OFL), blue
      bevelled panels, glossy buttons that press in (`web/theme.css`, over the
      menus and cards). No emojis: 3D-modelled badges (`tools/assets/badges.py`
      → `web/badges/`) and menu/emote icons (`tools/assets/icons.py` →
      `web/icons/`), shared toy look in `tools/assets/toy.py`.
- [ ] Next for the UI: the in-game HUD text (pixel font) and 4K Mania / race
      screens in the same style; better emote faces.

## Added 2026-10-19 (user): 4K Mania, round 2
- [x] Skins for the notes and menus (`kSkins` in `app/rhythm.cpp`): CLASSIC
      bars, CIRCLES, BUBBLE (default, glossy), NEON rings, ARROWS. Tap the
      SKIN button in the song select.
- [x] Bubbly menus: rounded glossy buttons, bobbing picked card, rising bubbles.
- [x] Songs failing to load: the chart's copy of the song is now made at
      ~11 kHz in the page (a quarter of the memory) and a failed load retries once.
- [x] No arcade music while a machine is open; the song select plays the
      picked song's preview (streamed, `engine::audio::previewPlay`).
- [x] Particles: sparkles from the picked card, sparks from hits and held
      notes, a shower every 100 combo.
- [x] Sideways: the whole screen is four tall tap zones (anywhere in a zone counts).
- [x] A tiny ring where each finger lands.
- [x] Combo in the middle, dark blue → blue → green → yellow → orange → red at 1000.
- [x] Hold notes letting go by themselves: Safari cancelling a long touch
      (its own gestures) no longer counts as letting go.

## Racing
- [x] **1. Two-player racing on the two rigs, like Mario Kart:** (done
      2026-10-19: garage with 3 cars, 12 paints, 8 rim colours; the room runs
      the lobby and the start, `cloud/games/race.js`; test
      `tools/race_mp_test.mjs`. Two players race without bots for now; bots in
      a two-player race would need one phone or the room to drive them.) each rig's
      player chooses a car and customizes it (paint, maybe parts), then
      races the other rig's player (bots optional). Synced through the
      Cloudflare room.
- [x] **2. Steering that feels like a real car:** (done 2026-10-19:
      `engine/steering.*`, measured by `tools/native/steering_test.cpp`;
      see DECISIONS → Steering feel.) today it's "so slippery" with
      "no turn radius". Map the steering like a real steering wheel: you turn
      left, it comes back to the middle by itself, but feels like a real car
      would (self-aligning, speed-sensitive, a real turning circle, grip).
      New steering logic, "really smooth but race-like". **This system will be
      reused later, so the feel has to be nailed.**

## Arcade room
- [x] **3. The room next to 4K Mania becomes an arcade** (done 2026-10-19:
      `tools/assets/arcade.py`, an 8 x 8 m room behind doorway A) (the taped-off
      "under construction" room). Almost empty: nice carpets, lots of cool
      lights, real 3D modelling (not boxes).
- [x] **4. A new arcade machine: a platformer** (done 2026-10-19: the
      cabinet `tools/assets/arcade_cabinet.py`, tap → full screen like Meridian), running the user's game
      "Test 2" as is (`web/arcade/platformer/`, three.js, from their zip).
      They loved how it felt; improvements later.
- [x] **5. Arcade background music** (done 2026-10-19: an original
      chiptune made in code, `engine::synth::arcadeLoop`, main.cpp updateArcadeMusic): only audible around the arcade's door,
      louder inside, normal volume in the room (3D sound zone).

## Polish in the bar
- [x] **6. Signs and ladders:** (done 2026-10-19: the A-frame sign by the
      door, the "under construction" sign, the barricades and the step ladder
      were all built pivoting at the floor, so they opened at the top like a
      V and their tops poked out; now real A-frames hinged at the top.) the "under construction" signs, the ladders in
      the construction rooms and the "welcome" sign are V-shaped with
      sticks poking out: rework them.
- [x] **7. 4K Mania:** (done 2026-10-19: 12 more songs, an osu!-style song
      select, INSANE difficulty, hold notes, osu!mania judgements and HUD,
      results with grades; `tools/rhythm_test.mjs`. Later: song previews in
      the menu, hand-made charts.) more songs, a better menu, look and feel like osu!mania.
- [x] **8. Mirror quality:** (done 2026-10-19: the reflection's resolution
      follows your distance to the glass: full within ~1 m, 40% from ~8 m.
      Later, RESTRUCTURE 6.3: a cheaper shader and culling for far things.) high quality for things close to the mirror,
      lower for far away (RESTRUCTURE 6.3; your own avatar always full).
- [x] **9. Player cards: badge slots.** (done 2026-10-19: 6 slots on every
      card; on your own card tap a slot to put one of your badges there; saved
      by `account.php` (`slots`, only badges you hold). Earnable badges for
      games (180 in darts, SS in 4K Mania, a race win...) would fill them next.)
- [x] **10. Pots and plants** (done 2026-10-19: the plant by the street door
      shared its spot with the umbrella stand (moved to x 5.35); the "floating
      lamps" were the bay window's hanging plants: white pots on 3 mm cords,
      now terracotta pots on visible rope hangers with ceiling hooks.) have things inside them (clipping); a **lamp
      floats** where a seat's indent is. Look around and fix such things.
- [x] **11. Walk-through check:** (done 2026-10-19: `walk_test` finds traps,
      unreachable things and now squeezes (TIGHT=1: floor only reachable by a
      walker narrower than 0.9 m). Fixed: the way behind the bar (counter end
      moved 0.3 m, library ladder parked at the rail's end and steeper,
      doorway B's barricade nudged), stuck seats (item 12), the door plant.)
- [x] **12. Stuck when sitting:** (2026-10-19: standing up uses `app::standSpot`: behind
      stools/rigs/piano bench, in front of sofas/armchairs, else the nearest floor you
      can walk away from; lounge blockers trimmed; `tools/native/walk_test.cpp` checks
      every seat) you get trapped after sitting at the
      **fireplace** and in the **left racing rig**: standing up must let you walk.
- [x] **13. The chessboard works** (done 2026-10-19: full rules,
      `app/chess_rules.*`, perft-tested; 3D pieces `tools/assets/chess.py`
      that hop between squares; tap the board to play the computer, or black
      against whoever plays white; shared through the room (`ch`, claims
      machine.chess.w/.b). Later: the room checking each move's legality.)

## Avatars
- [x] **14. Real avatar customization:** (done 2026-10-19: Menu → Avatar →
      Make my own look; tabs Body / Hair / Top / Bottom; masculine or feminine
      body, 10 skins, 8 eye colours, stubble/beard; 10 hairstyles, 14 hair
      colours, highlights (tips, streaks, roots) in a second colour; T-shirt,
      long sleeve, tank, hoodie, jacket; jeans, shorts, skirt, joggers; 20
      clothing colours with stripes, gradient or two-tone in a second colour;
      shoes. The camera shows you from the front while you edit (drag to turn).
      Saved on the phone; others see it through the room (world key `a<slot>`).
      Test `tools/avatar_test.mjs`.) choose gender; top and bottom
      (cool-style tops and bottoms); colors; hairstyle; hair color plus a
      secondary color for highlights, gradients, and more.
- [x] **15. More realistic character animations.** (done 2026-10-19: the
      walk places each foot on the floor and the leg reaches it (two-bone IK):
      heel strike, toes rolling down, the heel lifting for push-off, the knee
      folding on the swing; feet stay planted (slide 2% forward, 0% back,
      measured by `?walkcheck` in `tools/avatar_test.mjs`); slower walks take
      shorter steps; sideways steps go wide; the pelvis twists and drops, the
      chest counter-twists, the head stays steady; arms swing more forward
      than back. Standing: weight shifts from leg to leg (the free knee
      bends), breathing, glances around. Emotes ease in and out. Later:
      stepping when turning on the spot, sit-down / stand-up motions.)
