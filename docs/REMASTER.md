# Remaster list

On 2026-10-16 the user asked for a big batch of features "in very low
format": everything must exist and work, looks come later. This is the list
of every crude first version, so each one can be remastered properly. Tick
an item when its remaster is done.

## Voice chat (`web/voice.js`)
- [ ] TURN server: only Google's public STUN is used. Two phones on strict
      networks (some cellular carriers) may fail to connect ("could not
      connect ... (network)" in the log). Fix: a TURN relay (paid service,
      or coturn on a small VPS; IONOS web hosting can't run one).
- [ ] "Lossless": WebRTC has no lossless codec. We ask Opus for 510 kbps
      (transparent, "sounds lossless"). True lossless would need our own
      PCM/FLAC over the data channel; heavy on phone networks.
- [ ] The noise gate runs on a 20 ms timer, not in the audio thread
      (AudioWorklet would be exact and never cut syllables).
- [ ] Quality toggles (studio, stereo) only fully apply to new connections
      (no renegotiation yet).
- [ ] No per-person volume or mute, no "who is talking" list, no lip sync
      (the name tag turns green and shows (( )) instead).
- [ ] Push to talk is the HUD mic button only (no on-screen hold button
      elsewhere, no keyboard key).

## Menu (`web/menu.js`)
- [ ] Plain HTML panels; restyle to match the bar (or draw in WebGPU).
- [ ] Emote menu is a grid, not a radial wheel; no favorites/hotkeys.
- [ ] Settings: no graphics options yet (pixel ratio, mirror quality),
      no key rebinding.

## Avatars and animation (`app/avatar.*`)
- [ ] 8 looks are color/hair/sleeve variations of one body. Real separate
      avatars (Blender-built, or the user's own GLB/VRM) later.
- [ ] Every pose is a few hand-set joint angles (sit, drive, piano,
      rhythm, darts, bartend, drink, 11 emotes). No hand/finger bones, no
      IK: hands don't really hold the wheel or hit the keys, feet float a
      bit on tall stools, seated legs may clip the seat.
- [ ] Others' drinks float near their hand (not attached to the hand bone).
- [ ] No mouth/face animation while talking.

## Seats (`app/bar_scene.cpp` seats, `app/main.cpp` sitOn/standUp)
- [ ] Sitting snaps into place (no walk-up or sit-down animation).
- [ ] Sofa and armchair hip points are estimates; check each seat in the
      mirror and tune.
- [ ] Two players sitting at the same instant: the lower id keeps it
      (the other is stood up with a message).

## Piano (`app/piano.*`, `engine/synth.cpp`, `engine/midi.*`)
- [ ] The piano sound is additive synthesis (sines + hammer noise). Better:
      physical modeling (string waveguides, soundboard) or sampled.
- [ ] Keys light up but don't physically go down (the model's keys are one
      merged mesh); strings glow and shimmer instead of really vibrating.
      Dampers and hammers don't move.
- [ ] On-screen keyboard shows two octaves; no velocity (touch = 0.75), no
      sliding between keys.
- [ ] Web MIDI only on computers/Android (iPhone Safari has none).
      A Bluetooth MIDI keyboard on iPhone would need a native app.
- [ ] Your own .mid files are local only (not shared with others, gone
      after reload), like your own songs on the jukebox.
- [ ] Only the 10 Mutopia songs; no song search, no tempo control, no
      "learn mode" (falling notes).

## Darts (`app/darts.*`)
- [ ] Tap to throw where you tapped plus random wobble; no swipe power,
      no arm-skill model.
- [ ] Darts are boxes. Only 301; no cricket, no turns between players
      (one player at a time), no double-out rule.
- [ ] The score shows on the HUD; others see it as floating text by the
      chalkboard, not written on the chalkboard.

## Racing (`app/race/*`, plan in `docs/RACING.md`)
- [ ] Car model built in code (boxy loft); replace with a Blender car
      with interior/cockpit.
- [ ] Track scenery built in code (box grandstand, cone trees, flat
      tire walls); replace piece by piece with Blender assets.
- [ ] No sun shadows (soft blobs under cars only), no reflections of
      the world (sky only), no skid marks, smoke, dust or sparks.
- [ ] One engine hum (pitch from rpm); no tire squeal, wind, gear
      whine, crash sounds; other cars are silent.
- [ ] Spectator monitors show text only (step 4).

## 4K Mania for others (`app/rhythm.cpp` drawCabinet)
- [ ] Others see lanes, pressed keys and combo, with a fake note pattern
      (the real chart isn't sent).

## Bartending (`app/bartender.*`, `server/account.php` badges)
- [ ] Making a drink = tapping through its steps (with sounds and a
      shaking animation). No real liquids, bottles or glasses moving (W7).
- [ ] Drinks are colored boxes; served on 8 spots on the counter; anyone
      can take one; 4 sips and it's gone.
- [ ] Badges are given by the owner in the profile ("Give badges"). The
      owner is the first account made, or the names in
      `accounts/admins.json` on the server.
- [ ] The badge is checked on your own phone only (a determined guest could
      fake it). Fine for friends; a server check would need the relay.

## Other interactions (`app/main.cpp` use())
- [ ] Bell, guitar (one E major strum), door knock, chess, books, tip jar,
      light switch (dims the room), fireplace (fire light on/off; the
      flames model stays), stage mic (your voice carries across the bar):
      each is a sound and/or a message. Proper versions: playable guitar,
      a chess game, readable books, flames that go out.
- [ ] Beer taps and coffee machine are shortcuts into bartending.
- [ ] The TV over the bar end has no interaction yet.

## Profiles (`web/account.js`)
- [ ] 13 extra detail fields (pronouns, status, location, birthday, job,
      games, music, instrument, cocktail, languages, socials, looking for,
      fun fact) as plain text boxes. No privacy options per field.
