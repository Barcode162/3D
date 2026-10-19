# Upgrade plan (the user's list, 2026-10-19)

The user's request, item by item, in their order of mention. Tick each box
when it's done and deployed. Work top to bottom unless the user reorders.
(The server-run bot race was started and then replaced by item 1-2, as the
user asked: "the race game should actually be fixed".)

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
- [ ] **3. The room next to 4K Mania becomes an arcade** (the taped-off
      "under construction" room). Almost empty: nice carpets, lots of cool
      lights, real 3D modelling (not boxes).
- [ ] **4. A new arcade machine: a platformer**, running the user's game
      "Test 2" as is (`web/arcade/platformer/`, three.js, from their zip).
      They loved how it felt; improvements later.
- [ ] **5. Arcade background music**: only audible around the arcade's door,
      louder inside, normal volume in the room (3D sound zone).

## Polish in the bar
- [ ] **6. Signs and ladders:** the "under construction" signs, the ladders in
      the construction rooms and the "welcome" sign are V-shaped with
      sticks poking out: rework them.
- [ ] **7. 4K Mania:** more songs, a better menu, look and feel like osu!mania.
- [ ] **8. Mirror quality:** high quality for things close to the mirror,
      lower for far away (RESTRUCTURE 6.3; your own avatar always full).
- [ ] **9. Player cards: badge slots.**
- [ ] **10. Pots and plants** have things inside them (clipping); a **lamp
      floats** where a seat's indent is. Look around and fix such things.
- [ ] **11. Walk-through check:** (started 2026-10-19: `walk_test` finds unreachable
      seats and things; the walkway behind the bar is open now) simulate walking through the whole bar, find
      what's in the way, move things a little so it all makes sense; be able
      to walk all around the bar (counter).
- [x] **12. Stuck when sitting:** (2026-10-19: standing up uses `app::standSpot`: behind
      stools/rigs/piano bench, in front of sofas/armchairs, else the nearest floor you
      can walk away from; lounge blockers trimmed; `tools/native/walk_test.cpp` checks
      every seat) you get trapped after sitting at the
      **fireplace** and in the **left racing rig**: standing up must let you walk.
- [ ] **13. The chessboard works** (a real game of chess you can play).

## Avatars
- [ ] **14. Real avatar customization:** choose gender; top and bottom
      (cool-style tops and bottoms); colors; hairstyle; hair color plus a
      secondary color for highlights, gradients, and more.
- [ ] **15. More realistic character animations.**
